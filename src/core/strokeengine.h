#pragma once

#include <QColor>
#include <QImage>
#include <QPoint>
#include <memory>

class Layer;
class Selection;
class Command;

struct BrushSettings {
    QColor color     = QColor(0xEE3030);
    int    size       = 20;
    float  hardness   = 100.0f;
    float  alpha      = 1.0f;
};

struct EraserSettings {
    int   size     = 30;
    float hardness = 1.0f;
};

class StrokeEngine {
public:
    void beginStroke(Layer* layer, const QPoint& pos, bool isEraser);

    void continueStroke(const QPoint& from, const QPoint& to,
                        const Selection* selection);

    std::unique_ptr<Command> endStroke();

    bool isActive() const { return m_active; }

    void setBrushSettings(const BrushSettings& settings)   { m_brush = settings; }
    void setEraserSettings(const EraserSettings& settings) { m_eraser = settings; }

    const BrushSettings&  brushSettings()  const { return m_brush; }
    const EraserSettings& eraserSettings() const { return m_eraser; }

private:
    void strokeBrush(const QPoint& from, const QPoint& to, const Selection* selection);
    void strokeEraser(const QPoint& from, const QPoint& to, const Selection* selection);
    void compositeStrokeToLayer();

    static void paintSoftDab(QPainter& p, const QPoint& a, const QPoint& b,
                             const QColor& solidColor, int size, float hardness);

    Layer* m_layer = nullptr;
    bool   m_active = false;
    bool   m_erasing = false;

    QImage m_snapshotBeforeStroke;
    QImage m_strokeBuffer;

    BrushSettings  m_brush;
    EraserSettings m_eraser;
};
