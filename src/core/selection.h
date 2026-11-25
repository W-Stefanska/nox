#pragma once

#include <QImage>
#include <QPainterPath>
#include <QPolygon>
#include <QRect>
#include <QRegion>

enum class SelectionMode {
    Replace,   // Zastąp
    Add,       // Dodaj
    Subtract,  // Odejmij
    Intersect  // Przecięcie
};

// Selection
class Selection {
public:
    Selection() = default;

    void setRect(const QRect& rect, SelectionMode mode = SelectionMode::Replace);
    void setLasso(const QPolygon& polygon, SelectionMode mode = SelectionMode::Replace);
    void setPath(const QPainterPath& path, SelectionMode mode = SelectionMode::Replace);
    void setRegion(const QRegion& region, SelectionMode mode = SelectionMode::Replace);
    void combine(const QRegion& newRegion, const QPainterPath& newPath, SelectionMode mode);
    void clear();

    bool  isEmpty()                     const;
    bool  contains(const QPoint& p)     const;
    QRect boundingRect()                const;
    QImage mask(int width, int height)  const;

    const QRegion&     region() const { return m_region; }
    const QPainterPath path()   const { return m_path;   }

private:

    QRegion      m_region;
    QPainterPath m_path;
};
