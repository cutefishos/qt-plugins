#ifndef RSVGIMAGEIOHANDLER_H
#define RSVGIMAGEIOHANDLER_H

#include <QByteArray>
#include <QImageIOHandler>
#include <QSize>

class RsvgImageIOHandler final : public QImageIOHandler
{
public:
    bool canRead() const override;
    bool read(QImage *image) override;

    QVariant option(ImageOption option) const override;
    void setOption(ImageOption option, const QVariant &value) override;
    bool supportsOption(ImageOption option) const override;

private:
    bool loadData() const;
    bool loadIntrinsicSize() const;
    bool render(const QSize &size, QImage *image) const;

    mutable QByteArray m_data;
    mutable QSize m_intrinsicSize;
    mutable bool m_dataLoaded = false;
    mutable bool m_metadataLoaded = false;
    mutable bool m_valid = false;
    QSize m_scaledSize;
};

#endif // RSVGIMAGEIOHANDLER_H
