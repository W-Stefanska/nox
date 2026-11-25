#pragma once

#include <QPoint>
#include <QRect>
#include <QPolygon>

class QPainter;
class QPainterPath;
class LayerManager;
class Selection;
class SelectionTransformer;

class CanvasPainter {
public:
    struct ActiveSelectionTool {
        bool     rectSelecting = false;
        QRect    rect;
        bool     lassoing = false;
        QPolygon lassoPoints;
    };

    static void paint(QPainter& painter,
        const QPoint& canvasOffset,
        int canvasWidth, int canvasHeight,
        const LayerManager& layerManager,
        const Selection& selection,
        const SelectionTransformer& transformer,
        const ActiveSelectionTool& activeTool,
        int marchOffset);

private:
    static void paintLayers(QPainter& painter, const QPoint& off,
        int canvasWidth, int canvasHeight,
        const LayerManager& layerManager,
        const SelectionTransformer& transformer);

    static void paintActiveSelectionTool(QPainter& painter, const QPoint& off,
        const ActiveSelectionTool& activeTool);

    static void paintSelectionAnts(QPainter& painter, const Selection& selection,
        int marchOffset);

    static void paintTransformOverlay(QPainter& painter,
        const SelectionTransformer& transformer,
        int marchOffset);

    static void paintMarchingAnts(QPainter& painter, const QPainterPath& path,
        int marchOffset);
};
