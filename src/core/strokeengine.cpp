#include "strokeengine.h"
#include "layer.h"
#include "selection.h"
#include "command.h"

#include <QPainter>
#include <QLineF>
#include <QRadialGradient>

void StrokeEngine::beginStroke(Layer* layer, const QPoint& /*pos*/, bool isEraser)
{
    m_layer   = layer;
    m_erasing = isEraser;
    m_active  = (layer != nullptr);

    if (!m_active) return;

    m_snapshotBeforeStroke = layer->image().copy();

    if (!isEraser) {
        m_strokeBuffer = QImage(layer->image().size(),
                                QImage::Format_ARGB32_Premultiplied);
        m_strokeBuffer.fill(Qt::transparent);
    } else {
        m_strokeBuffer = QImage();
    }
}

void StrokeEngine::continueStroke(const QPoint& from, const QPoint& to,
                                  const Selection* selection)
{
    if (!m_active || !m_layer) return;

    if (m_erasing)
        strokeEraser(from, to, selection);
    else
        strokeBrush(from, to, selection);
}

std::unique_ptr<Command> StrokeEngine::endStroke()
{
    if (!m_active || !m_layer) {
        m_active = false;
        return nullptr;
    }

    QImage after = m_layer->image().copy();
    auto cmd = std::make_unique<DrawCommand>(&m_layer->image(),
        m_snapshotBeforeStroke, after);

    m_active = false;
    m_layer = nullptr;
    m_strokeBuffer = QImage();

    return cmd;
}

void StrokeEngine::strokeEraser(const QPoint& from, const QPoint& to,
                                const Selection* selection)
{
    QPainter p(&m_layer->image());
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(m_layer->alphaLocked()
                             ? QPainter::CompositionMode_SourceAtop
                             : QPainter::CompositionMode_Clear);

    if (selection && !selection->isEmpty())
        p.setClipRegion(selection->region());

    const int size = m_eraser.size;
    if (m_eraser.hardness >= 0.99f) {
        QPen pen(Qt::transparent, size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.drawLine(from, to);
    } else {
        paintSoftDab(p, from, to, QColor(0, 0, 0, 255), size, m_eraser.hardness);
    }
}

void StrokeEngine::strokeBrush(const QPoint& from, const QPoint& to,
                               const Selection* selection)
{
    if (m_strokeBuffer.isNull()) return;

    QPainter p(&m_strokeBuffer);
    p.setRenderHint(QPainter::Antialiasing);

    QColor solid = m_brush.color;
    solid.setAlphaF(1.0f);

    if (selection && !selection->isEmpty())
        p.setClipRegion(selection->region());

    const int size = m_brush.size;
    if (m_brush.hardness >= 0.99f) {
        QPen pen(solid, size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.drawLine(from, to);
    } else {
        paintSoftDab(p, from, to, solid, size, m_brush.hardness);
    }

    compositeStrokeToLayer();
}

void StrokeEngine::compositeStrokeToLayer()
{
    if (!m_layer || m_strokeBuffer.isNull()) return;

    QImage& dst = m_layer->image();
    dst = m_snapshotBeforeStroke.copy();

    QPainter p(&dst);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(m_layer->alphaLocked()
                             ? QPainter::CompositionMode_SourceAtop
                             : QPainter::CompositionMode_SourceOver);
    p.setOpacity(m_brush.alpha);
    p.drawImage(0, 0, m_strokeBuffer);
}

void StrokeEngine::paintSoftDab(QPainter& p, const QPoint& a, const QPoint& b,
                                const QColor& solidColor, int size, float hardness)
{
    if (size <= 1) {
        p.setPen(QPen(solidColor, 1));
        p.drawLine(a, b);
        return;
    }

    const qreal radius = size / 2.0;
    const qreal h = qBound(0.0f, hardness, 1.0f);;

    const qreal inner = radius * h;

    const qreal maxDabAlpha = 0.15 + 0.85 * h;
    QColor coreColor = solidColor;
    coreColor.setAlphaF(solidColor.alphaF() * maxDabAlpha);

    QColor transparent = solidColor;
    transparent.setAlpha(0);

    QLineF line(a, b);
    const qreal len  = line.length();
    const qreal step = qMax(1.0, radius * 0.25);

    p.setPen(Qt::NoPen);

    for (qreal t = 0; t <= len + step; t += step) {
        const qreal tt = qMin(t, len);
        QPointF pt = (len > 0) ? line.pointAt(tt / len) : a;

        QRadialGradient g(pt, radius);
        g.setColorAt(0.0, solidColor);
        g.setColorAt(inner / radius, solidColor);
        g.setColorAt(1.0, transparent);

        p.setBrush(QBrush(g));
        p.drawEllipse(pt, radius, radius);
    }
}
