#ifndef PLATFORMTHEME_H
#define PLATFORMTHEME_H

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#include <private/qgenericunixtheme_p.h>
#else
#include <private/qgenericunixthemes_p.h>
#endif
#include "hintsettings.h"
#include "systemtrayicon.h"

#include <QHash>
#include <QKeySequence>

class QIconEngine;
class QWindow;

class PlatformTheme : public QObject, public QGenericUnixTheme
{
    Q_OBJECT

public:
    PlatformTheme();
    ~PlatformTheme() override;

    QVariant themeHint(ThemeHint hint) const override;
    const QFont *font(Font type) const override;

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override
    {
        auto trayIcon = new SystemTrayIcon;
        if (trayIcon->isSystemTrayAvailable())
            return trayIcon;
        else {
            delete trayIcon;
            return nullptr;
        }
    }

private:
    void onFontChanged();
    void onIconThemeChanged();
    void setQtQuickControlsTheme();

private:
    HintsSettings *m_hints;
};

#endif // PLATFORMTHEME_H
