#include "colorpicker.h"

#include <QPainter>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QConicalGradient>

ColorPicker::ColorPicker(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(sizeHint());
    setCursor(Qt::CrossCursor);
    rebuildSvImage();
}

QSize ColorPicker::sizeHint() const
{
    const int w = kMargin * 2 + kSvSize + kGap + kHueWidth;
    const int h = kMargin * 2 + kSvSize + kGap + kSwatchH;
    return QSize(w, h);
}

void ColorPicker::setColor(const QColor& color)
{
    if (!color.isValid() || color == m_color) return;

    m_hue = qMax(0, color.hue());
    m_sat = color.saturationF();
    m_val = color.valueF();
    m_alpha = color.alphaF();

    m_color = color;
    rebuildSvImage();
    update();
}

QRect ColorPicker::svRect() const
{
    return QRect(kMargin, kMargin, kSvSize, kSvSize);
}

QRect ColorPicker::hueRect() const
{
    return QRect(kMargin + kSvSize + kGap, kMargin, kHueWidth, kSvSize);
}

QRect ColorPicker::swatchRect() const
{
    return QRect(kMargin, kMargin + kSvSize + kGap, kSvSize + kGap + kHueWidth, kSwatchH);
}

void ColorPicker::rebuildSvImage()
{
    m_svImage = QImage(kSvSize, kSvSize, QImage::Format_RGB32);
    for (int y = 0; y < kSvSize; ++y) {
        qreal v = 1.0 - qreal(y) / (kSvSize - 1);
        QRgb* line = reinterpret_cast<QRgb*>(m_svImage.scanLine(y));
        for (int x = 0; x < kSvSize; ++x) {
            qreal s = qreal(x) / (kSvSize - 1);
            QColor c = QColor::fromHsvF(m_hue / 360.0, s, v);
            line[x] = c.rgb();
        }
    }
}

void ColorPicker::updateFromHsv()
{
    m_color = QColor::fromHsvF(m_hue / 360.0, m_sat, m_val, m_alpha);
    emit colorChanged(m_color);
    update();
}

void ColorPicker::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect sv = svRect();
    p.drawImage(sv, m_svImage);
    p.setPen(QPen(QColor(60, 60, 60), 1));
    p.drawRect(sv.adjusted(0, 0, -1, -1));

    {
        int cx = sv.left() + int(m_sat * (sv.width() - 1));
        int cy = sv.top()  + int((1.0 - m_val) * (sv.height() - 1));
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPoint(cx, cy), 4, 4);
        p.setPen(QPen(Qt::black, 1));
        p.drawEllipse(QPoint(cx, cy), 5, 5);
    }

    const QRect hr = hueRect();
    QLinearGradient grad(hr.topLeft(), hr.bottomLeft());
    for (int i = 0; i <= 360; i += 30)
        grad.setColorAt(i / 360.0, QColor::fromHsv(i % 360, 255, 255));
    p.fillRect(hr, grad);
    p.setPen(QPen(QColor(60, 60, 60), 1));
    p.drawRect(hr.adjusted(0, 0, -1, -1));

    {
        int hy = hr.top() + int((m_hue / 360.0) * (hr.height() - 1));
        p.setPen(QPen(Qt::white, 2));
        p.drawLine(hr.left() - 2, hy, hr.right() + 2, hy);
    }

    const QRect sw = swatchRect();
    p.setPen(QPen(QColor(60, 60, 60), 1));
    p.setBrush(m_color);
    p.drawRect(sw.adjusted(0, 0, -1, -1));
}

void ColorPicker::handleSvClick(const QPoint& pos)
{
    const QRect sv = svRect();
    qreal s = qreal(pos.x() - sv.left()) / (sv.width() - 1);
    qreal v = 1.0 - qreal(pos.y() - sv.top()) / (sv.height() - 1);
    m_sat = qBound(0.0, s, 1.0);
    m_val = qBound(0.0, v, 1.0);
    updateFromHsv();
}

void ColorPicker::handleHueClick(const QPoint& pos)
{
    const QRect hr = hueRect();
    qreal t = qreal(pos.y() - hr.top()) / (hr.height() - 1);
    m_hue = qBound(0, int(qBound(0.0, t, 1.0) * 360.0), 359);
    rebuildSvImage();
    updateFromHsv();
}

void ColorPicker::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    if (hueRect().contains(event->pos())) {
        m_draggingHue = true;
        m_draggingSv = false;
        handleHueClick(event->pos());
    } else if (svRect().contains(event->pos())) {
        m_draggingSv = true;
        m_draggingHue = false;
        handleSvClick(event->pos());
    }
}

void ColorPicker::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        m_draggingSv = false;
        m_draggingHue = false;
        return;
    }

    if (m_draggingHue) {
        handleHueClick(event->pos());
    } else if (m_draggingSv) {
        handleSvClick(event->pos());
    }
}

void ColorPicker::mouseReleaseEvent(QMouseEvent*)
{
    m_draggingSv = false;
    m_draggingHue = false;
}
