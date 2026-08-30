#include "platformtheme.h"
#include "x11integration.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QString>
#include <QVariant>
#include <QDebug>
#include <QLibrary>
#include <QStyleFactory>
#include <QtQuickControls2/QQuickStyle>

// Qt Private
#include <private/qicon_p.h>
#include <private/qiconloader_p.h>
#include <private/qwindow_p.h>
#include <private/qguiapplication_p.h>

#include <KWindowSystem>

extern void updateXdgIconSystemTheme();

void onDarkModeChanged()
{
    if (qApp->applicationName() == "systemsettings"
                || qApp->applicationName().contains("plasma")
                || qApp->applicationName().contains("QtCreator")) {
        return;
    }

    QStyle *style = QStyleFactory::create("cutefish");
    if (style) {
        qApp->setStyle(style);
    }
}

PlatformTheme::PlatformTheme()
    : m_hints(new HintsSettings)
{
    // qApp->setProperty("_hints_settings_object", (quintptr)m_hints);

    if (KWindowSystem::isPlatformX11()) {
        m_x11Integration.reset(new X11Integration());
        m_x11Integration->init();
    }

    connect(m_hints, &HintsSettings::systemFontChanged, this, &PlatformTheme::onFontChanged);
    connect(m_hints, &HintsSettings::systemFontPointSizeChanged, this, &PlatformTheme::onFontChanged);
    connect(m_hints, &HintsSettings::iconThemeChanged, this, &PlatformTheme::onIconThemeChanged);
    connect(m_hints, &HintsSettings::darkModeChanged, &onDarkModeChanged);

    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, false);
    setQtQuickControlsTheme();
}

PlatformTheme::~PlatformTheme()
{
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

const QFont* PlatformTheme::font(Font type) const
{
    switch (type) {
        case SystemFont:
        case MessageBoxFont:
        case LabelFont:
        case TipLabelFont:
        case StatusBarFont:
        case PushButtonFont:
        case ItemViewFont:
        case ListViewFont:
        case HeaderViewFont:
        case ListBoxFont:
        case ComboMenuItemFont:
        case ComboLineEditFont: {
            const QString &fontName = m_hints->systemFont();
            qreal fontSize = m_hints->systemFontPointSize();
            static QFont font = QFont(QString());
            font.setFamily(fontName);
            font.setPointSizeF(fontSize);
            return &font;
        }
        case FixedFont: {
            const QString &fontName = m_hints->systemFixedFont();
            qreal fontSize = m_hints->systemFontPointSize();
            static QFont font = QFont(QString());
            font.setFamily(fontName);
            font.setPointSizeF(fontSize);
            return &font;
        }
        default: {
            const QString &fontName = m_hints->systemFont();
            qreal fontSize = m_hints->systemFontPointSize();
            static QFont font = QFont(QString());
            font.setFamily(fontName);
            font.setPointSizeF(fontSize);
            return &font;
        }
    }

    return QGenericUnixTheme::font(type);
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
    updateXdgIconSystemTheme();

    QEvent update(QEvent::UpdateRequest);
    for (QWindow *window : qGuiApp->allWindows()) {
        if (window->type() == Qt::Desktop)
            continue;

        qApp->sendEvent(window, &update);
    }
}

void PlatformTheme::setQtQuickControlsTheme()
{
    // Qt 6's QQuickStyle::name() resolves to the default style instead of
    // returning an empty string, so it can no longer tell us whether the user
    // picked a style explicitly. Honour the environment variable instead.
    if (!qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        return;
    }

    if (qApp->applicationName() == "systemsettings"
            || qApp->applicationName().contains("plasma")) {
        QQuickStyle::setStyle("Plasma");
        QStyle *style = QStyleFactory::create("Breeze");
        qApp->setStyle(style);
        return;
    }

    QQuickStyle::setStyle(QLatin1String("Fish"));
}
