#include "hintsettings.h"

#include <QApplication>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMainWindow>
#include <QPalette>
#include <QScreen>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryFile>
#include <QToolBar>
#include <QToolButton>
#include <qpa/qplatformdialoghelper.h>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QFontDatabase>

static const QByteArray s_systemFontName = QByteArrayLiteral("Font");
static const QByteArray s_systemFixedFontName = QByteArrayLiteral("FixedFont");
static const QByteArray s_systemPointFontSize = QByteArrayLiteral("FontSize");
static const QByteArray s_darkModeName = QByteArrayLiteral("DarkMode");
static const QString s_defaultIconName = QStringLiteral("Cutefish");

static QString availableFontFamily(const QString &requested, const QString &fallback)
{
    QFontDatabase database;
    const QStringList families = database.families();

    if (families.contains(requested))
        return requested;
    if (families.contains(fallback))
        return fallback;

    return QStringLiteral("Sans Serif");
}

HintsSettings::HintsSettings(QObject *parent)
    : QObject(parent)
    , m_settings(new QSettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                                       + "/cutefish/services.conf",
                               QSettings::IniFormat))
{
    m_settings->beginGroup("Appearance");
    m_iconTheme = m_settings->value("IconTheme", s_defaultIconName).toString();
    updateIconThemeHint();

    m_hints[QPlatformTheme::StyleNames] = QStringList {QStringLiteral("Fusion")};
    m_hints[QPlatformTheme::SystemIconFallbackThemeName] = QStringLiteral("hicolor");
    m_hints[QPlatformTheme::IconThemeSearchPaths] = xdgIconThemePaths();
    m_hints[QPlatformTheme::UseFullScreenForPopupMenu] = false;
    m_hints[QPlatformTheme::DialogButtonBoxLayout] = QPlatformDialogHelper::MacLayout;

    m_settingsFile = m_settings->fileName();
    updateCursorHints();
    for (const auto *signal : {"cursorThemeChanged", "cursorSizeChanged"})
        QDBusConnection::sessionBus().connect(
                "com.cutefish.Services", "/com/cutefish/Services/Appearance",
                "com.cutefish.Services.Appearance", QLatin1String(signal), this,
                SLOT(updateCursorHints()));
    // Delivered in order before any application-side icon reload that listens to the same
    // signal, so QIconLoader is on the new theme first whatever the file watcher does.
    QDBusConnection::sessionBus().connect("com.cutefish.Services",
                                          "/com/cutefish/Services/Appearance",
                                          "com.cutefish.Services.Appearance", "iconThemeChanged",
                                          this, SLOT(updateIconTheme()));

    QMetaObject::invokeMethod(this, "lazyInit", Qt::QueuedConnection);
}

HintsSettings::~HintsSettings()
{
}

void HintsSettings::lazyInit()
{
    m_fileWatcher = new QFileSystemWatcher(this);
    m_fileWatcher->addPath(QFileInfo(m_settingsFile).absolutePath());
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this,
            &HintsSettings::onFileChanged);
    m_fileWatcher->addPath(m_settingsFile);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &HintsSettings::onFileChanged);
}

QStringList HintsSettings::xdgIconThemePaths() const
{
    QStringList paths;

    // make sure we have ~/.local/share/icons in paths if it exists
    paths << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("icons"),
                                       QStandardPaths::LocateDirectory);

    const QFileInfo homeIconDir(QDir::homePath() + QStringLiteral("/.icons"));
    if (homeIconDir.isDir()) {
        paths << homeIconDir.absoluteFilePath();
    }

    return paths;
}

QString HintsSettings::systemFont() const
{
    return availableFontFamily(m_settings->value(s_systemFontName, "Noto Sans").toString(),
                               QStringLiteral("DejaVu Sans"));
}

QString HintsSettings::systemFixedFont() const
{
    return availableFontFamily(m_settings->value(s_systemFixedFontName, "Noto Mono").toString(),
                               QStringLiteral("DejaVu Sans Mono"));
}

qreal HintsSettings::systemFontPointSize() const
{
    return m_settings->value(s_systemPointFontSize, 9).toDouble();
}

void HintsSettings::updateIconThemeHint()
{
    m_hints[QPlatformTheme::SystemIconThemeName] =
            m_iconTheme.isEmpty() ? s_defaultIconName : m_iconTheme;
}

bool HintsSettings::darkMode()
{
    return m_settings->value(s_darkModeName, false).toBool();
}

void HintsSettings::onFileChanged(const QString &path)
{
    Q_UNUSED(path);

    QVariantMap map;
    for (const QString &value : m_settings->allKeys()) {
        map[value] = m_settings->value(value);
    }

    m_settings->sync();

    for (const QString &value : m_settings->allKeys()) {
        const QVariant &oldValue = map.value(value);
        const QVariant &newValue = m_settings->value(value);

        if (oldValue != newValue) {
            if (value == s_systemFontName)
                emit systemFontChanged(newValue.toString());
            else if (value == s_systemFixedFontName)
                emit systemFixedFontChanged(newValue.toString());
            else if (value == s_systemPointFontSize)
                emit systemFontPointSizeChanged(newValue.toDouble());
            else if (value == s_darkModeName)
                emit darkModeChanged(newValue.toBool());
            else if (value == "IconTheme")
                updateIconTheme();
        }
    }

    bool fileDeleted = !m_fileWatcher->files().contains(m_settingsFile);
    if (fileDeleted)
        m_fileWatcher->addPath(m_settingsFile);
    updateCursorHints();
}

void HintsSettings::updateIconTheme()
{
    m_settings->sync();
    const QString iconTheme = m_settings->value("IconTheme", s_defaultIconName).toString();
    if (iconTheme == m_iconTheme)
        return;

    m_iconTheme = iconTheme;
    updateIconThemeHint();
    emit iconThemeChanged();
}

void HintsSettings::updateCursorHints()
{
    m_settings->sync();
    const auto theme = m_settings->value("CursorTheme", "default").toString();
    const int size = qBound(16, m_settings->value("CursorSize", 24).toInt(), 64);
    if (m_hints.value(QPlatformTheme::MouseCursorTheme) == theme
        && m_hints.value(QPlatformTheme::MouseCursorSize) == QSize(size, size))
        return;
    m_hints[QPlatformTheme::MouseCursorTheme] = theme;
    m_hints[QPlatformTheme::MouseCursorSize] = QSize(size, size);
    emit cursorThemeChanged();
}
