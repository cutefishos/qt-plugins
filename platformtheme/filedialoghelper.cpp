#include "filedialoghelper.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeDatabase>
#include <QRandomGenerator>
#include <QWindow>

namespace {

const auto portalService = QStringLiteral("org.freedesktop.portal.Desktop");
const auto portalPath = QStringLiteral("/org/freedesktop/portal/desktop");
const auto fileChooserInterface = QStringLiteral("org.freedesktop.portal.FileChooser");
const auto requestInterface = QStringLiteral("org.freedesktop.portal.Request");

// (us): 0 is a glob pattern, 1 a MIME type.
struct FilterRule {
    uint type = 0;
    QString value;
};

struct Filter {
    QString name;
    QList<FilterRule> rules;
};

} // namespace

Q_DECLARE_METATYPE(FilterRule)
Q_DECLARE_METATYPE(Filter)

namespace {

QDBusArgument &operator<<(QDBusArgument &argument, const FilterRule &rule)
{
    argument.beginStructure();
    argument << rule.type << rule.value;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, FilterRule &rule)
{
    argument.beginStructure();
    argument >> rule.type >> rule.value;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const Filter &filter)
{
    argument.beginStructure();
    argument << filter.name << filter.rules;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Filter &filter)
{
    argument.beginStructure();
    argument >> filter.name >> filter.rules;
    argument.endStructure();
    return argument;
}

void registerFilterTypes()
{
    static bool registered = false;
    if (registered)
        return;
    qDBusRegisterMetaType<FilterRule>();
    qDBusRegisterMetaType<QList<FilterRule>>();
    qDBusRegisterMetaType<Filter>();
    qDBusRegisterMetaType<QList<Filter>>();
    registered = true;
}

// "Images (*.png *.jpg)" -> name "Images", rules for each glob.
Filter filterFromNameFilter(const QString &nameFilter)
{
    Filter filter;
    const int bracket = nameFilter.lastIndexOf('(');
    filter.name = bracket > 0 ? nameFilter.left(bracket).trimmed() : nameFilter;
    const auto globs = QPlatformFileDialogHelper::cleanFilterList(nameFilter);
    for (const QString &glob : globs)
        filter.rules.append({0, glob});
    if (filter.name.isEmpty())
        filter.name = globs.join(QLatin1Char(' '));
    return filter;
}

Filter filterFromMimeType(const QString &mimeType)
{
    Filter filter;
    const auto type = QMimeDatabase().mimeTypeForName(mimeType);
    filter.name = type.isValid() && !type.comment().isEmpty() ? type.comment() : mimeType;
    filter.rules.append({1, mimeType});
    return filter;
}

// The portal wants NUL-terminated paths as byte arrays.
QByteArray portalPathValue(const QUrl &url)
{
    QByteArray path = QFile::encodeName(url.toLocalFile());
    if (!path.isEmpty())
        path.append('\0');
    return path;
}

} // namespace

bool FileDialogHelper::isPortalAvailable()
{
    static const bool available = [] {
        // Escape hatch for a broken or hanging portal.
        if (qEnvironmentVariableIsSet("CUTEFISH_NO_PORTAL_FILE_DIALOG"))
            return false;
        auto *interface = QDBusConnection::sessionBus().interface();
        return interface && interface->isServiceRegistered(portalService).value();
    }();
    return available;
}

FileDialogHelper::FileDialogHelper()
{
    registerFilterTypes();
}

FileDialogHelper::~FileDialogHelper()
{
    closeRequest();
}

void FileDialogHelper::exec()
{
    m_loop.exec();
}

bool FileDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    Q_UNUSED(flags)

    const auto dialogOptions = options();
    const bool saveMode = dialogOptions->acceptMode() == QFileDialogOptions::AcceptSave;
    const bool directoryMode = dialogOptions->fileMode() == QFileDialogOptions::Directory
            || dialogOptions->fileMode() == QFileDialogOptions::DirectoryOnly;

    QVariantMap portalOptions;
    portalOptions.insert(QStringLiteral("handle_token"),
                         QStringLiteral("cutefish%1").arg(QRandomGenerator::global()->generate()));
    portalOptions.insert(QStringLiteral("modal"), modality != Qt::NonModal);
    portalOptions.insert(QStringLiteral("multiple"),
                         dialogOptions->fileMode() == QFileDialogOptions::ExistingFiles);
    portalOptions.insert(QStringLiteral("directory"), directoryMode);

    const QString acceptLabel = dialogOptions->labelText(QFileDialogOptions::Accept);
    if (!acceptLabel.isEmpty())
        portalOptions.insert(QStringLiteral("accept_label"), acceptLabel);

    QList<Filter> filters;
    for (const QString &mimeType : dialogOptions->mimeTypeFilters())
        filters.append(filterFromMimeType(mimeType));
    for (const QString &nameFilter : dialogOptions->nameFilters())
        filters.append(filterFromNameFilter(nameFilter));
    if (!filters.isEmpty()) {
        portalOptions.insert(QStringLiteral("filters"), QVariant::fromValue(filters));
        const QString wanted = m_selectedNameFilter.isEmpty()
                ? dialogOptions->initiallySelectedNameFilter()
                : m_selectedNameFilter;
        if (!wanted.isEmpty()) {
            const Filter current = wanted.contains('/') ? filterFromMimeType(wanted)
                                                        : filterFromNameFilter(wanted);
            for (const Filter &filter : std::as_const(filters)) {
                if (filter.name != current.name)
                    continue;
                portalOptions.insert(QStringLiteral("current_filter"), QVariant::fromValue(filter));
                m_selectedNameFilter = wanted;
                break;
            }
        }
    }

    const QUrl directory = m_directory.isValid() ? m_directory : dialogOptions->initialDirectory();
    if (directory.isLocalFile())
        portalOptions.insert(QStringLiteral("current_folder"), portalPathValue(directory));

    QUrl selected = m_selectedFile;
    if (!selected.isValid() && !dialogOptions->initiallySelectedFiles().isEmpty())
        selected = dialogOptions->initiallySelectedFiles().constFirst();
    if (selected.isLocalFile() && saveMode) {
        if (QFileInfo::exists(selected.toLocalFile()))
            portalOptions.insert(QStringLiteral("current_file"), portalPathValue(selected));
        portalOptions.insert(QStringLiteral("current_name"), selected.fileName());
    }

    // Wayland needs an exported xdg_foreign handle, which neither side supports
    // yet; without one the dialog opens unparented.
    QString parentWindow;
    if (parent && QGuiApplication::platformName() == QLatin1String("xcb"))
        parentWindow = QStringLiteral("x11:%1").arg(parent->winId(), 0, 16);

    QString title = dialogOptions->windowTitle();
    if (title.isEmpty())
        title = saveMode ? tr("Save File") : directoryMode ? tr("Open Folder") : tr("Open File");

    QDBusMessage message = QDBusMessage::createMethodCall(
            portalService, portalPath, fileChooserInterface,
            saveMode ? QStringLiteral("SaveFile") : QStringLiteral("OpenFile"));
    message << parentWindow << title << portalOptions;

    auto *watcher =
            new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *call) {
                call->deleteLater();
                const QDBusPendingReply<QDBusObjectPath> reply = *call;
                if (reply.isError()) {
                    Q_EMIT reject();
                    m_loop.quit();
                    return;
                }
                m_request = reply.value();
                QDBusConnection::sessionBus().connect(portalService, m_request.path(),
                                                      requestInterface, QStringLiteral("Response"),
                                                      this, SLOT(gotResponse(uint, QVariantMap)));
            });
    return true;
}

void FileDialogHelper::hide()
{
    closeRequest();
    m_loop.quit();
}

void FileDialogHelper::closeRequest()
{
    if (m_request.path().isEmpty())
        return;
    QDBusConnection::sessionBus().disconnect(portalService, m_request.path(), requestInterface,
                                             QStringLiteral("Response"), this,
                                             SLOT(gotResponse(uint, QVariantMap)));
    QDBusConnection::sessionBus().asyncCall(QDBusMessage::createMethodCall(
            portalService, m_request.path(), requestInterface, QStringLiteral("Close")));
    m_request = QDBusObjectPath();
}

void FileDialogHelper::gotResponse(uint response, const QVariantMap &results)
{
    m_request = QDBusObjectPath();
    m_selectedFiles.clear();

    if (response == 0) {
        const auto uris = results.value(QStringLiteral("uris")).toStringList();
        for (const QString &uri : uris)
            m_selectedFiles.append(QUrl(uri));
        if (!m_selectedFiles.isEmpty()) {
            m_selectedFile = m_selectedFiles.constFirst();
            m_directory = m_selectedFile.adjusted(QUrl::RemoveFilename);
            Q_EMIT directoryEntered(m_directory);
            Q_EMIT currentChanged(m_selectedFile);
            Q_EMIT fileSelected(m_selectedFile);
            Q_EMIT filesSelected(m_selectedFiles);
        }
        const auto filter = qdbus_cast<Filter>(results.value(QStringLiteral("current_filter")));
        if (!filter.name.isEmpty()) {
            m_selectedNameFilter = filter.name;
            Q_EMIT filterSelected(m_selectedNameFilter);
        }
        Q_EMIT accept();
    } else {
        Q_EMIT reject();
    }

    m_loop.quit();
}

bool FileDialogHelper::defaultNameFilterDisables() const
{
    return false;
}

void FileDialogHelper::setDirectory(const QUrl &directory)
{
    m_directory = directory;
}

QUrl FileDialogHelper::directory() const
{
    return m_directory;
}

void FileDialogHelper::selectFile(const QUrl &filename)
{
    m_selectedFile = filename;
}

QList<QUrl> FileDialogHelper::selectedFiles() const
{
    return m_selectedFiles;
}

void FileDialogHelper::setFilter()
{
}

void FileDialogHelper::selectNameFilter(const QString &filter)
{
    m_selectedNameFilter = filter;
}

QString FileDialogHelper::selectedNameFilter() const
{
    return m_selectedNameFilter;
}

bool FileDialogHelper::isSupportedUrl(const QUrl &url) const
{
    return url.isLocalFile();
}
