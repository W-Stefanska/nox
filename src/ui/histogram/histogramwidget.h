#pragma once

#include <QWidget>
#include <QImage>

class QComboBox;

class HistogramWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget* parent = nullptr);

public slots:
    void setImage(const QImage& image);
    void refresh();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void computeHistogram();
    void applyStyle();

    QImage m_image;
    QVector<int> m_histR, m_histG, m_histB, m_histGray;
    QComboBox* m_channelCombo = nullptr;
    int m_maxValue = 1;
};
