#pragma once

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QColor>
#include "../core/selection.h"
#include "../core/commandmanager.h"
#include "../core/layermanager.h"
#include "../core/strokeengine.h"
#include "../core/selectiontransformer.h"
#include "../core/filltools.h"

enum class Tool {
    Brush,
    Eraser,
    RectSelect,
    LassoSelect,
    Transform,
    Eyedropper,
    Filters,
    Zoom,
    Fill,
    MagicWand
};

class CanvasView : public QWidget
{
    Q_OBJECT
public:
    explicit CanvasView(int canvasWidth = 800, int canvasHeight = 600,
                        QWidget* parent = nullptr);

    void resizeCanvas(int width, int height);
    void resetCanvas(int width, int height);

    void setEraserMode(bool enabled);
    void setPenColor(const QColor& color);
    LayerManager* layerManager() { return &m_layerManager; }
    CommandManager* commandManager() { return &m_commandManager; }

    //tools
    void setTool(Tool tool);
    Tool currentTool() const { return m_tool; }
    Selection& selection() { return m_selection; }
    void clearSelection();

    void setSelectionMode(SelectionMode mode) { m_selectionMode = mode; }
    SelectionMode selectionMode() const { return m_selectionMode; }

    // transform
    void enterTransformMode();
    void commitTransform();
    void cancelTransform();

    // pen opts
    void setPenSize(int size);
    void setBrushHardness(float hardness);
    void setBrushAlpha(float alpha);

    // eraser opts
    void setEraserSize(int size);
    void setEraserHardness(float hardness);

    // zoom opts
    //void setZoom(qreal factor);

    // fill
    void applyFill(const QPoint& pos);
    void applyMagicWand(const QPoint& pos, SelectionMode mode);
    void setFillSettings(const FillSettings& s) { m_fillSettings = s; }
    void setWandSettings(const FillSettings& w) { m_wandSettings = w; }

    // gettery
    int     canvasWidth()  const { return m_layerManager.width();  }
    int     canvasHeight() const { return m_layerManager.height(); }
    int     penSize() const        { return m_strokeEngine.brushSettings().size; }
    float   brushHardness() const  { return m_strokeEngine.brushSettings().hardness; }
    float   brushAlpha() const     { return m_strokeEngine.brushSettings().alpha; }
    int     eraserSize() const     { return m_strokeEngine.eraserSettings().size; }
    float   eraserHardness() const { return m_strokeEngine.eraserSettings().hardness; }
    QColor  penColor() const       { return m_strokeEngine.brushSettings().color; }
    // qreal   zoom() const           { return m_zoom; }

    const FillSettings& fillSettings() const { return m_fillSettings; }
    const FillSettings& wandSettings() const { return m_wandSettings; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;

public slots:
    void clearActiveLayer();
    void showContextMenu(const QPoint& pos);

private:
    LayerManager        m_layerManager;
    QPoint              canvasOffset() const;
    bool                m_eraserMode = false;
    CommandManager      m_commandManager;

    StrokeEngine m_strokeEngine;
    QPoint       m_lastPoint;
    //qreal m_zoom = 1.0;

    // selekcje
    Tool          m_tool = Tool::Brush;
    Selection     m_selection;
    SelectionMode m_selectionMode = SelectionMode::Replace;

    QPoint m_selectStart;
    QRect  m_selectRect;
    bool   m_selecting = false;

    QPolygon m_lassoPoints;
    bool     m_lassoing = false;

    int      m_marchOffset = 0;
    QTimer*  m_marchTimer  = nullptr;

    // transformacje
    SelectionTransformer m_transformer;

    // pipeta
    QColor pickColorAt(const QPoint& pos) const;

    // fill
    FillSettings m_fillSettings;
    FillSettings m_wandSettings;

signals:
    void toolChanged(Tool tool);
    void colorPicked(const QColor& color);
    void hoverColorChanged(const QColor& color);
    void canvasModified();
};
