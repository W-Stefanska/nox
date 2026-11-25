#include "histogramwidget.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>

HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto* hLabel = new QLabel("HISTOGRAM", this);
    hLabel->setAlignment(Qt::AlignCenter);
    hLabel->setObjectName("sectionLabel");

    m_channelCombo = new QComboBox(this);
    m_channelCombo->addItems({ "RGB", "R", "G", "B", "Jasność" });

    layout->addWidget(hLabel);
    layout->addStretch(0);
    m_channelCombo->setContentsMargins(0,0,0,0);
    layout->addWidget(m_channelCombo);

    connect(m_channelCombo, &QComboBox::currentTextChanged,
            this, [this]() { update(); });

    setMinimumHeight(90);
    applyStyle();
}

void HistogramWidget::setImage(const QImage& image)
{
    m_image = image.convertToFormat(QImage::Format_ARGB32);
    refresh();
}

void HistogramWidget::refresh()
{
    computeHistogram();
    update();
}

void HistogramWidget::computeHistogram()
{
    m_histR = QVector<int>(256, 0);
    m_histG = QVector<int>(256, 0);
    m_histB = QVector<int>(256, 0);
    m_histGray = QVector<int>(256, 0);

    if (m_image.isNull()) {
        m_maxValue = 0;
        return;
    }

    for (int y = 0; y < m_image.height(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(m_image.constScanLine(y));
        for (int x = 0; x < m_image.width(); ++x) {
            QRgb p = line[x];
            if (qAlpha(p) == 0) continue;

            int r = qRed(p);
            int g = qGreen(p);
            int b = qBlue(p);
            int gray = qBound(0, int(0.299 * r + 0.587 * g + 0.114 * b), 255);

            m_histR[r]++;
            m_histG[g]++;
            m_histB[b]++;
            m_histGray[gray]++;
        }
    }

    m_maxValue = 0;
    for (int i = 0; i < 256; ++i) {
        m_maxValue = qMax(m_maxValue, m_histR[i]);
        m_maxValue = qMax(m_maxValue, m_histG[i]);
        m_maxValue = qMax(m_maxValue, m_histB[i]);
        m_maxValue = qMax(m_maxValue, m_histGray[i]);
    }
}

void HistogramWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(53, 53, 53));

    const int chartTop    = 35;
    const int chartBottom = height() - 35;
    const int chartHeight = chartBottom - chartTop;
    const int chartLeft   = 6;
    const int chartRight  = width() - 6;
    const int chartWidth  = chartRight - chartLeft;

    if (m_image.isNull() || m_maxValue <= 0) {
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, tr("Brak obrazu"));
        return;
    }

    const QString ch = m_channelCombo->currentText();

    auto drawChannel = [&](const QVector<int>& hist, const QColor& color) {
        QPolygon poly;
        poly << QPoint(chartLeft, chartBottom);

        for (int i = 0; i < 256; ++i) {
            int x = chartLeft + (i * chartWidth) / 255;
            int h = qRound(double(hist[i]) * chartHeight / m_maxValue);
            int y = chartBottom - h;
            poly << QPoint(x, y);
        }

        poly << QPoint(chartRight, chartBottom);

        QColor c = color;
        c.setAlpha(140);
        p.setBrush(c);
        p.setPen(color);
        p.drawPolygon(poly);
    };

    if (ch == "RGB") {
        drawChannel(m_histR, QColor(255, 80, 80));
        drawChannel(m_histG, QColor(80, 255, 80));
        drawChannel(m_histB, QColor(80, 80, 255));
    } else if (ch == "R") {
        drawChannel(m_histR, QColor(255, 80, 80));
    } else if (ch == "G") {
        drawChannel(m_histG, QColor(80, 255, 80));
    } else if (ch == "B") {
        drawChannel(m_histB, QColor(80, 80, 255));
    } else {
        drawChannel(m_histGray, QColor(200, 200, 200));
    }
}

void HistogramWidget::applyStyle()
{
    setStyleSheet(R"(
        QLabel#sectionLabel {
            color: #888888;
            font-size: 10px;
            font-weight: bold;
            letter-spacing: 1px;
            padding: 4px 0 2px 0;
        }
    )");
}
