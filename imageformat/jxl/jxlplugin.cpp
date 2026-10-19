// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "jxlplugin.h"

#include "jxlhandler.h"

QImageIOPlugin::Capabilities JxlPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    const bool jxl = format.compare("jxl", Qt::CaseInsensitive) == 0;
    if (!device)
        return jxl ? Capabilities(CanRead) : Capabilities();
    if (!device->isReadable() || (!format.isEmpty() && !jxl))
        return {};
    return JxlHandler::canRead(device) ? Capabilities(CanRead) : Capabilities();
}

QImageIOHandler *JxlPlugin::create(QIODevice *device, const QByteArray &format) const
{
    auto *handler = new JxlHandler;
    handler->setDevice(device);
    handler->setFormat(format.isEmpty() ? QByteArrayLiteral("jxl") : format);
    return handler;
}
