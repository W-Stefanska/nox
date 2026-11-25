#include "canvaspainter.h"
#include "../core/layermanager.h"
#include "../core/selection.h"
#include "../core/selectiontransformer.h"
#include "../core/transformstate.h"

#include <QPainter>
#include <QPainterPath>

void CanvasPainter::paint(QPainter& painter,
                          const QPoint& off,
                          int canvasWidth, int canvasHeight,
                          const LayerManager& layerManager,
                          const Selection& selection,
                          const SelectionTransformer& transformer,
                          const ActiveSelectionTool& activeTool,
                          int marchOffset)
{
    painter.fillRect(off.x(), off.y(), canvasWidth, canvasHeight, Qt::white);

    paintLayers(painter, off, canvasWidth, canvasHeight, layerManager, transformer);
    paintActiveSelectionTool(painter, off, activeTool);

    painter.save();
    painter.translate(off);

    if (!transformer.isActive())
        paintSelectionAnts(painter, selection, marchOffset);
    else
        paintTransformOverlay(painter, transformer, marchOffset);

    painter.restore();
}

void CanvasPainter::paintLayers(QPainter& painter, const QPoint& off,
                                int /*canvasWidth*/, int /*canvasHeight*/,
                                const LayerManager& layerManager,
                                const SelectionTransformer& transformer)
{
    if (transformer.isActive()) {
        painter.drawImage(off, layerManager.compositeImageExcluding(
                                   layerManager.activeIndex()));
        painter.save();
        painter.translate(off);
        painter.drawImage(0, 0, transformer.state().apply());
        painter.restore();
    } else {
        painter.drawImage(off, layerManager.compositeImage());
    }
}

void CanvasPainter::paintActiveSelectionTool(QPainter& painter, const QPoint& off,
                                             const ActiveSelectionTool& activeTool)
{
    if (activeTool.rectSelecting) {
        painter.save();
        painter.translate(off);
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.drawRect(activeTool.rect);
        painter.restore();
    }

    if (activeTool.lassoing && activeTool.lassoPoints.size() > 1) {
        painter.save();
        painter.translate(off);
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.drawPolyline(activeTool.lassoPoints);
        painter.restore();
    }
}

void CanvasPainter::paintSelectionAnts(QPainter& painter, const Selection& selection,
                                       int marchOffset)
{
    painter.save();
    paintMarchingAnts(painter, selection.path(), marchOffset);
    painter.restore();
}

void CanvasPainter::paintTransformOverlay(QPainter& painter,
                                          const SelectionTransformer& transformer,
                                          int marchOffset)
{
    const TransformState& ts = transformer.state();
    painter.save();

    QPainterPath transformed = transformer.currentPathTransform().map(ts.selectionPath);
    paintMarchingAnts(painter, transformed, marchOffset);


    painter.save();
    painter.translate(ts.center);
    painter.rotate(ts.angle);

    const QRectF localRect = ts.localRect();

    QVector<qreal> dashes = {4, 4};
    QPen rectPen(QColor(0x0d6efd), 1, Qt::CustomDashLine);
    rectPen.setDashPattern(dashes);
    rectPen.setDashOffset(marchOffset);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setPen(rectPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(localRect);

    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::white);
    auto pts = ts.handlePositions();
    for (int i = 0; i < pts.size() - 1; ++i)
        painter.drawEllipse(pts[i], 5.0, 5.0);

    painter.setBrush(QColor(0x0d6efd));
    painter.drawEllipse(pts.last(), 5.0, 5.0);

    painter.setPen(QPen(Qt::white, 1));
    painter.drawLine(QPointF(0, localRect.top()), pts.last());

    painter.restore();
    painter.restore();
}

void CanvasPainter::paintMarchingAnts(QPainter& painter, const QPainterPath& path,
    int marchOffset)
{
    QVector<qreal> dashes = {4, 4};

    QPen whitePen(Qt::white, 1, Qt::CustomDashLine);
    whitePen.setDashPattern(dashes);
    whitePen.setDashOffset(marchOffset);
    painter.setPen(whitePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    QPen blackPen(Qt::black, 1, Qt::CustomDashLine);
    blackPen.setDashPattern(dashes);
    blackPen.setDashOffset(marchOffset + 4);
    painter.setPen(blackPen);
    painter.drawPath(path);
}
