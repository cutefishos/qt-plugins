#include "rsvgimageplugin.h"

#include "rsvgimageiohandler.h"

#include <QIODevice>

namespace {

bool isSvg(const QByteArray &data)
{
    return data.left(64 * 1024).toLower().contains("<svg");
}

} // namespace

QImageIOPlugin::Capabilities RsvgImagePlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (!device || !device->isReadable()) {
        return {};
    }

    if (!format.isEmpty() && format.compare("svg", Qt::CaseInsensitive) != 0) {
        return {};
    }

    if (format.compare("svg", Qt::CaseInsensitive) == 0) {
        return CanRead;
    }

    return isSvg(device->peek(64 * 1024)) ? Capabilities(CanRead) : Capabilities();
}

QImageIOHandler *RsvgImagePlugin::create(QIODevice *device, const QByteArray &format) const
{
    auto *handler = new RsvgImageIOHandler;
    handler->setDevice(device);
    handler->setFormat(format.isEmpty() ? QByteArrayLiteral("svg") : format);
    return handler;
}
