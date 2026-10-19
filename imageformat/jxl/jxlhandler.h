// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <QImageIOHandler>
#include <QSize>

// The first frame, oriented as the file asks.
class JxlHandler final : public QImageIOHandler
{
public:
    static bool canRead(QIODevice *device);

    bool canRead() const override;
    bool read(QImage *image) override;

    QVariant option(ImageOption option) const override;
    bool supportsOption(ImageOption option) const override;

private:
    bool readInfo() const;

    mutable QByteArray m_data;
    mutable QSize m_size;
    mutable bool m_alpha = false;
    mutable bool m_infoRead = false;
    bool m_read = false;
};
