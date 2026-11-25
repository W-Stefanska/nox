#pragma once

#include <QImage>
#include <QPointF>
#include <QRect>
#include <QTransform>
#include <QVector>
#include <QPainterPath>

enum class TransformHandle {
    None,
    TopLeft,
    Top,
    TopRight,
    Left,
    Right,
    BottomLeft,
    Bottom,
    BottomRight,
    Rotate
};

struct TransformState {
    // Original state
    QImage  originalImage;
    QRect   originalRect;

    // Current state
    QPointF center;
    qreal   width  = 0.0;
    qreal   height = 0.0;
    QImage  selectionMask;
    QPainterPath selectionPath;
    float   angle = 0.0f;
    bool    active = false;

    // Drag handling
    TransformHandle activeHandle = TransformHandle::None;
    QPointF         dragStart;
    QPointF         centerAtDragStart;
    qreal           widthAtDragStart  = 0.0;
    qreal           heightAtDragStart = 0.0;
    float           angleAtDragStart  = 0.0f;

    QRectF localRect() const {
        return QRectF(-width / 2.0, -height / 2.0, width, height);
    }

    // Returns screen positions handles
    QVector<QPointF> handlePositions() const;

    // Returns which handle is under point p
    TransformHandle hitTest(const QPointF& p) const;

    //
    QTransform pathTransform() const;

    // Composites the transformed fragment
    QImage apply() const;
};
