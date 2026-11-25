#pragma once

#include "transformstate.h"
#include <QPointF>
#include <memory>

class Layer;
class Selection;
class Command;

class SelectionTransformer {
public:
    void begin(Layer* layer, const Selection& selection, int canvasWidth, int canvasHeight);

    std::unique_ptr<Command> commit(Selection& outSelection);

    void cancel();

    bool isActive() const { return m_state.active; }

    void beginDrag(const QPointF& pos);
    void updateDrag(const QPointF& pos);
    void endDrag();
    bool isDragging() const { return m_state.activeHandle != TransformHandle::None || m_moving; }

    QTransform currentPathTransform() const;

    const TransformState& state() const { return m_state; }

private:
    TransformState m_state;
    Layer*         m_layer = nullptr;
    bool           m_moving = false;
};
