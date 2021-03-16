#include "pstyleplugin.h"
#include "basestyle.h"

#include <QApplication>
#include <QStyleFactory>
#include <QDebug>

QStringList ProxyStylePlugin::keys() const
{
    return {"cutefish"};
}

QStyle *ProxyStylePlugin::create(const QString &key)
{
    if (key != QStringLiteral("cutefish")) {
        return nullptr;
    }

    return new BaseStyle;
}
