#include "selection.h"

#include <QPainter>

void Selection::combine(const QRegion& newRegion, const QPainterPath& newPath, SelectionMode mode)
{
    switch (mode) {
    case SelectionMode::Replace:
        m_region = newRegion;
        m_path   = newPath;
        break;
    case SelectionMode::Add:
        m_region = m_region.united(newRegion);
        m_path   = m_path.united(newPath);
        break;
    case SelectionMode::Subtract:
        m_region = m_region.subtracted(newRegion);
        m_path   = m_path.subtracted(newPath);
        break;
    case SelectionMode::Intersect:
        m_region = m_region.intersected(newRegion);
        m_path   = m_path.intersected(newPath);
        break;
    }
}

void Selection::setRect(const QRect& rect, SelectionMode mode)
{
    QPainterPath path;
    path.addRect(rect);
    combine(QRegion(rect), path, mode);
}

void Selection::setLasso(const QPolygon& polygon, SelectionMode mode)
{
    QPainterPath path;
    path.addPolygon(polygon);
    path.closeSubpath();
    combine(QRegion(polygon), path, mode);
}

void Selection::setPath(const QPainterPath& path, SelectionMode mode)
{
    if (path.isEmpty()) {
        if (mode == SelectionMode::Replace) clear();
        return;
    }

    QPolygon poly = path.toFillPolygon().toPolygon();
    combine(QRegion(poly), path, mode);
}

void Selection::setRegion(const QRegion& region, SelectionMode mode)
{
    QPainterPath path;
    for (const QRect& r : region) {
        path.addRect(r);
    }
    path = path.simplified();
    combine(region, path, mode);
}

void Selection::clear()
{
    m_region = {};
    m_path   = {};
}

bool  Selection::isEmpty()               const { return m_region.isEmpty(); }
bool  Selection::contains(const QPoint& p) const { return m_region.contains(p); }
QRect Selection::boundingRect()          const { return m_region.boundingRect(); }

QImage Selection::mask(int width, int height) const
{
    QImage mask(width, height, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    QPainter p(&mask);
    p.setClipRegion(m_region);
    p.fillRect(0, 0, width, height, Qt::white);
    return mask;
}
