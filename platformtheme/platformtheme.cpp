#include "platformtheme.h"
#include "filedialoghelper.h"
#include "touchpadscrolling.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QString>
#include <QVariant>
#include <QWindow>

// Qt Private
#include <private/qiconloader_p.h>
#include <qpa/qwindowsysteminterface.h>

static QPalette darkPalette()
{
    const QColor window(45, 45, 45);
    const QColor base(30, 30, 30);
    const QColor text(230, 230, 230);
    const QColor disabledText(127, 127, 127);
    const QColor highlight(42, 130, 218);

    QPalette palette(window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, window);
    palette.setColor(QPalette::ToolTipBase, window);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::PlaceholderText, disabledText);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, window);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, highlight);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Light, window.lighter(150));
    palette.setColor(QPalette::Midlight, window.lighter(125));
    palette.setColor(QPalette::Mid, window.darker(125));
    palette.setColor(QPalette::Dark, window.darker(150));
    palette.setColor(QPalette::Shadow, Qt::black);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, window.lighter(130));
    return palette;
}

PlatformTheme::PlatformTheme()
    : m_hints(new HintsSettings)
{
    connect(m_hints, &HintsSettings::systemFontChanged, this, &PlatformTheme::onFontChanged);
    connect(m_hints, &HintsSettings::systemFontPointSizeChanged, this,
            &PlatformTheme::onFontChanged);
    connect(m_hints, &HintsSettings::iconThemeChanged, this, &PlatformTheme::onIconThemeChanged);
    connect(m_hints, &HintsSettings::cursorThemeChanged, this, [] {
        for (auto *window : qGuiApp->allWindows()) {
            QWindowSystemInterface::handleThemeChange(window);
        }
    });
    connect(m_hints, &HintsSettings::darkModeChanged, this, [] {
        QWindowSystemInterface::handleThemeChange();
    });

    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, false);
    // The application object is still being constructed.
    QMetaObject::invokeMethod(this, &TouchpadScrolling::install, Qt::QueuedConnection);
}

PlatformTheme::~PlatformTheme()
{
    delete m_hints;
}

QVariant PlatformTheme::themeHint(QPlatformTheme::ThemeHint hintType) const
{
    QVariant hint = m_hints->hint(hintType);
    if (hint.isValid()) {
        return hint;
    } else {
        return QGenericUnixTheme::themeHint(hintType);
    }
}

Qt::ColorScheme PlatformTheme::colorScheme() const
{
    return m_hints->darkMode() ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

const QPalette *PlatformTheme::palette(Palette type) const
{
    if (type == SystemPalette && m_hints->darkMode()) {
        static const QPalette dark = darkPalette();
        return &dark;
    }

    return QGenericUnixTheme::palette(type);
}

const QFont *PlatformTheme::font(Font type) const
{
    if (type == FixedFont) {
        const QString &fontName = m_hints->systemFixedFont();
        qreal fontSize = m_hints->systemFontPointSize();
        static QFont font = QFont(QString());
        font.setFamily(fontName);
        font.setPointSizeF(fontSize);
        return &font;
    }

    const QString &fontName = m_hints->systemFont();
    qreal fontSize = m_hints->systemFontPointSize();
    static QFont font = QFont(QString());
    font.setFamily(fontName);
    font.setPointSizeF(fontSize);
    return &font;
}

bool PlatformTheme::usePlatformNativeDialog(DialogType type) const
{
    return type == FileDialog;
}

QPlatformDialogHelper *PlatformTheme::createPlatformDialogHelper(DialogType type) const
{
    if (type == FileDialog)
        return new FileDialogHelper;
    return QGenericUnixTheme::createPlatformDialogHelper(type);
}

void PlatformTheme::onFontChanged()
{
    QFont font;
    font.setFamily(m_hints->systemFont());
    font.setPointSizeF(m_hints->systemFontPointSize());

    // Change font
    if (qobject_cast<QApplication *>(QCoreApplication::instance()))
        QApplication::setFont(font);
    else if (qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
        QGuiApplication::setFont(font);
}

void PlatformTheme::onIconThemeChanged()
{
    QIconLoader::instance()->updateSystemTheme();

    QEvent update(QEvent::UpdateRequest);
    for (QWindow *window : qGuiApp->allWindows()) {
        if (window->type() == Qt::Desktop)
            continue;

        qApp->sendEvent(window, &update);
    }
}
