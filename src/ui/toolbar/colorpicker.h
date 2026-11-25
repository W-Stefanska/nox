#pragma once

#include <QWidget>
#include <QColor>
#include <QImage>

class ColorPicker : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPicker(QWidget* parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor& color);

    QSize sizeHint() const override;

signals:
    void colorChanged(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QRect svRect() const;
    QRect hueRect() const;
    QRect swatchRect() const;

    void handleSvClick(const QPoint& pos);
    void handleHueClick(const QPoint& pos);
    void rebuildSvImage();
    void updateFromHsv();

    int    m_hue = 0;
    qreal  m_sat = 1.0;
    qreal  m_val = 1.0;
    qreal  m_alpha = 1.0;
    QColor m_color = Qt::black;

    QImage m_svImage;
    bool   m_draggingSv  = false;
    bool   m_draggingHue = false;

    static constexpr int kSvSize   = 96;
    static constexpr int kHueWidth = 14;
    static constexpr int kGap      = 6;
    static constexpr int kSwatchH  = 18;
    static constexpr int kMargin   = 6;
};
