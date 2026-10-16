// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CUTEFISH_FLICKABLESCROLLER_H
#define CUTEFISH_FLICKABLESCROLLER_H

#include "scroller.h"

class QQuickFlickable;

namespace Cutefish {

// Touchpad scrolling of a Flickable, reported as Flickable movement.
class FlickableScroller : public Scroller
{
    Q_OBJECT

public:
    explicit FlickableScroller(QQuickFlickable *flickable);

    QQuickFlickable *flickable() const;

protected:
    Axis horizontalAxis() const override;
    Axis verticalAxis() const override;
    QPointF position() const override;
    void applyPosition(const QPointF &position) override;
    void reportMoving(bool moving, Qt::Orientations axes) override;
    void stopViewMotion() override;
    bool isViewAlive() const override;

private:
    QPointer<QQuickFlickable> m_flickable;
};

} // namespace Cutefish

#endif // CUTEFISH_FLICKABLESCROLLER_H
