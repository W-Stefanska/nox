#include "selectiontransformer.h"
#include "layer.h"
#include "selection.h"
#include "command.h"

#include <QtMath>
#include <cmath>

void SelectionTransformer::begin(Layer* layer, const Selection& selection,
                                 int canvasWidth, int canvasHeight)
{
    if (!layer) return;

    m_layer = layer;
    m_state = TransformState{};

    m_state.selectionPath = selection.path();
    m_state.active = true;
    m_state.originalImage = layer->image().copy();
    m_state.selectionMask = selection.isEmpty()
                                ? QImage()
                                : selection.mask(canvasWidth, canvasHeight);

    m_state.originalRect = selection.isEmpty()
                               ? QRect(0, 0, canvasWidth, canvasHeight)
                               : selection.boundingRect();

    m_state.center = QRectF(m_state.originalRect).center();
    m_state.width  = m_state.originalRect.width();
    m_state.height = m_state.originalRect.height();
    m_state.angle  = 0.0f;
}

std::unique_ptr<Command> SelectionTransformer::commit(Selection& outSelection)
{
    if (!m_state.active || !m_layer) return nullptr;

    QImage before = m_layer->image().copy();
    QImage result = m_state.apply();
    m_layer->image() = result;

    auto cmd = std::make_unique<DrawCommand>(&m_layer->image(), before, result);

    if (!m_state.selectionPath.isEmpty()) {
        QPainterPath newPath = currentPathTransform().map(m_state.selectionPath);
        outSelection.setPath(newPath);
    }

    m_state.active = false;
    m_layer = nullptr;
    m_moving = false;

    return cmd;
}

void SelectionTransformer::cancel()
{
    if (!m_state.active) return;
    if (m_layer) m_layer->image() = m_state.originalImage;
    m_state.active = false;
    m_layer = nullptr;
    m_moving = false;
}

void SelectionTransformer::beginDrag(const QPointF& pos)
{
    if (!m_state.active) return;

    TransformHandle hit = m_state.hitTest(pos);
    m_state.activeHandle      = hit;
    m_state.dragStart         = pos;
    m_state.centerAtDragStart = m_state.center;
    m_state.widthAtDragStart  = m_state.width;
    m_state.heightAtDragStart = m_state.height;
    m_state.angleAtDragStart  = m_state.angle;

    QTransform toLocal;
    toLocal.rotate(-m_state.angle);
    const QPointF localPos = toLocal.map(pos - m_state.center);
    m_moving = (hit == TransformHandle::None && m_state.localRect().contains(localPos));
}

void SelectionTransformer::updateDrag(const QPointF& pos)
{
    if (!m_state.active) return;
    if (m_state.activeHandle == TransformHandle::None && !m_moving) return;

    if (m_state.activeHandle == TransformHandle::Rotate) {
        const QPointF center = m_state.centerAtDragStart;
        const float a1 = std::atan2(m_state.dragStart.y() - center.y(),
                                    m_state.dragStart.x() - center.x());
        const float a2 = std::atan2(pos.y() - center.y(),
                                    pos.x() - center.x());
        m_state.angle = m_state.angleAtDragStart +
                        qRadiansToDegrees(a2 - a1);
        return;
    }

    if (m_state.activeHandle == TransformHandle::None && m_moving) {
        const QPointF diff = pos - m_state.dragStart;
        m_state.center = m_state.centerAtDragStart + diff;
        return;
    }

    QTransform toLocal;
    toLocal.rotate(-m_state.angleAtDragStart);
    const QPointF localPos = toLocal.map(pos - m_state.centerAtDragStart);

    const qreal halfW = m_state.widthAtDragStart  / 2.0;
    const qreal halfH = m_state.heightAtDragStart / 2.0;

    QPointF anchorLocal;
    QPointF newCornerLocal;

    switch (m_state.activeHandle) {
    case TransformHandle::TopLeft:
        anchorLocal     = QPointF( halfW,  halfH);
        newCornerLocal  = localPos;
        break;
    case TransformHandle::TopRight:
        anchorLocal     = QPointF(-halfW,  halfH);
        newCornerLocal  = localPos;
        break;
    case TransformHandle::BottomLeft:
        anchorLocal     = QPointF( halfW, -halfH);
        newCornerLocal  = localPos;
        break;
    case TransformHandle::BottomRight:
        anchorLocal     = QPointF(-halfW, -halfH);
        newCornerLocal  = localPos;
        break;
    case TransformHandle::Left:
        anchorLocal     = QPointF( halfW, 0);
        newCornerLocal  = QPointF(localPos.x(), 0);
        break;
    case TransformHandle::Right:
        anchorLocal     = QPointF(-halfW, 0);
        newCornerLocal  = QPointF(localPos.x(), 0);
        break;
    case TransformHandle::Top:
        anchorLocal     = QPointF(0,  halfH);
        newCornerLocal  = QPointF(0, localPos.y());
        break;
    case TransformHandle::Bottom:
        anchorLocal     = QPointF(0, -halfH);
        newCornerLocal  = QPointF(0, localPos.y());
        break;
    default:
        return;
    }

    qreal newWidth  = std::abs(newCornerLocal.x() - anchorLocal.x());
    qreal newHeight = std::abs(newCornerLocal.y() - anchorLocal.y());

    newWidth  = qMax<qreal>(newWidth,  2.0);
    newHeight = qMax<qreal>(newHeight, 2.0);

    if (m_state.activeHandle == TransformHandle::Left ||
        m_state.activeHandle == TransformHandle::Right)
        newHeight = m_state.heightAtDragStart;
    if (m_state.activeHandle == TransformHandle::Top ||
        m_state.activeHandle == TransformHandle::Bottom)
        newWidth = m_state.widthAtDragStart;

    QPointF newCenterLocal(0, 0);

    const bool changesX = (m_state.activeHandle != TransformHandle::Top &&
                           m_state.activeHandle != TransformHandle::Bottom);
    const bool changesY = (m_state.activeHandle != TransformHandle::Left &&
                           m_state.activeHandle != TransformHandle::Right);

    if (changesX) {
        newCenterLocal.setX(anchorLocal.x() < newCornerLocal.x()
                            ? anchorLocal.x() + newWidth  / 2.0
                            : anchorLocal.x() - newWidth  / 2.0);
    }
    if (changesY) {
        newCenterLocal.setY(anchorLocal.y() < newCornerLocal.y()
                            ? anchorLocal.y() + newHeight / 2.0
                            : anchorLocal.y() - newHeight / 2.0);
    }

    QTransform toWorld;
    toWorld.rotate(m_state.angleAtDragStart);
    m_state.center = m_state.centerAtDragStart + toWorld.map(newCenterLocal);

    m_state.width  = newWidth;
    m_state.height = newHeight;
}

void SelectionTransformer::endDrag()
{
    m_moving = false;
    m_state.activeHandle = TransformHandle::None;
}

QTransform SelectionTransformer::currentPathTransform() const
{
    return m_state.pathTransform();
}
