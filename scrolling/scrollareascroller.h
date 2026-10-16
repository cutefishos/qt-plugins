// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CUTEFISH_SCROLLAREASCROLLER_H
#define CUTEFISH_SCROLLAREASCROLLER_H

#include "scroller.h"

#include <QPoint>

class QAbstractScrollArea;
class QScrollBar;

namespace Cutefish {

// Touchpad scrolling of a scroll area whose scroll bars count pixels. Past an
// end the viewport moves away from its edge.
class ScrollAreaScroller : public Scroller
{
    Q_OBJECT

public:
    explicit ScrollAreaScroller(QAbstractScrollArea *area);

    QAbstractScrollArea *area() const;
    bool scroll(QWheelEvent *event);

protected:
    Axis horizontalAxis() const override;
    Axis verticalAxis() const override;
    QPointF position() const override;
    void applyPosition(const QPointF &position) override;
    bool isViewAlive() const override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPoint values() const;
    // Right to left, the horizontal bar's minimum shows the right end.
    qreal horizontalPosition(int value) const;
    int horizontalValue(qreal position) const;

    QPointer<QAbstractScrollArea> m_area;
    QPointF m_position;
    QPoint m_values;
    bool m_positioned = false;
};

} // namespace Cutefish

#endif // CUTEFISH_SCROLLAREASCROLLER_H
