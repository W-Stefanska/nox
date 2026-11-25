#include "transformstate.h"

#include <QPainter>
#include <QtMath>

static const float HANDLE_RADIUS = 6.0f;
static const float ROTATE_OFFSET = 20.0f;

QVector<QPointF> TransformState::handlePositions() const
{
    const QRectF r = localRect();
    const qreal l = r.left();
    const qreal rr = r.right();
    const qreal t = r.top();
    const qreal b = r.bottom();

    return {
        { l,  t },   // TopLeft
        { 0,  t },   // Top
        { rr, t },   // TopRight
        { l,  0 },   // Left
        { rr, 0 },   // Right
        { l,  b },   // BottomLeft
        { 0,  b },   // Bottom
        { rr, b },   // BottomRight
        { 0,  t - ROTATE_OFFSET }  // Rotate
    };
}

TransformHandle TransformState::hitTest(const QPointF& p) const
{
    static constexpr TransformHandle k_handles[] = {
        TransformHandle::TopLeft,    TransformHandle::Top,    TransformHandle::TopRight,
        TransformHandle::Left,       TransformHandle::Right,
        TransformHandle::BottomLeft, TransformHandle::Bottom, TransformHandle::BottomRight,
        TransformHandle::Rotate
    };

    QTransform toLocal;
    toLocal.rotate(-angle);
    const QPointF localP = toLocal.map(p - center);

    const auto pts = handlePositions();
    for (int i = 0; i < pts.size(); ++i) {
        if (QLineF(localP, pts[i]).length() <= HANDLE_RADIUS * 2)
            return k_handles[i];
    }

    return TransformHandle::None;
}

QTransform TransformState::pathTransform() const
{
    const qreal sx = originalRect.width()  != 0 ? width  / originalRect.width()  : 1.0;
    const qreal sy = originalRect.height() != 0 ? height / originalRect.height() : 1.0;

    const QPointF origCenter = QRectF(originalRect).center();

    QTransform t;
    t.translate(center.x(), center.y());
    t.rotate(angle);
    t.scale(sx, sy);
    t.translate(-origCenter.x(), -origCenter.y());
    return t;
}

QImage TransformState::apply() const
{
    QImage fragment(originalRect.size(), QImage::Format_ARGB32_Premultiplied);
    fragment.fill(Qt::transparent);
    {
        QPainter p(&fragment);
        p.drawImage(QPoint(0, 0), originalImage, originalRect);

        if (!selectionMask.isNull()) {
            p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            p.drawImage(QPoint(0, 0), selectionMask, originalRect);
        }
    }

    QImage result(originalImage.width(), originalImage.height(),
                  QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    {
        QPainter p(&result);
        p.drawImage(0, 0, originalImage);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);

        if (!selectionMask.isNull())
            p.drawImage(0, 0, selectionMask);
        else
            p.fillRect(originalRect, Qt::white);
    }

    {
        QPainter p(&result);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        QTransform t = QTransform::fromTranslate(originalRect.left(), originalRect.top())
                       * pathTransform();

        p.setTransform(t);
        p.drawImage(0, 0, fragment);
    }

    return result;
}
