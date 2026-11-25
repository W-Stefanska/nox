#include "pointops.h"
#include <QVector>
#include <cmath>

static QImage applyLut(const QImage& src, const QVector<uchar>& lut)
{
    Q_ASSERT(lut.size() == 256);

    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    const int w = img.width();
    const int h = img.height();

    for (int y = 0; y < h; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb p = line[x];
            int r = lut[qRed(p)];
            int g = lut[qGreen(p)];
            int b = lut[qBlue(p)];
            int a = qAlpha(p);
            line[x] = qRgba(r, g, b, a);
        }
    }
    return img;
}

QImage Pointops::adjustBrightness(const QImage& src, int delta)
{
    QVector<uchar> lut(256);
    for (int i = 0; i < 256; ++i)
        lut[i] = static_cast<uchar>(qBound(0, i + delta, 255));
    return applyLut(src, lut);
}

QImage Pointops::adjustContrast(const QImage& src, float factor)
{
    QVector<uchar> lut(256);
    for (int i = 0; i < 256; ++i) {
        float v = (i - 128.0f) * factor + 128.0f;
        lut[i] = static_cast<uchar>(qBound(0.0f, v, 255.0f));
    }
    return applyLut(src, lut);
}

QImage Pointops::toGrayscale(const QImage& src)
{

    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    const int w = img.width();
    const int h = img.height();

    for (int y = 0; y < h; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb p = line[x];
            int gray = qBound(0,
                              static_cast<int>(0.299 * qRed(p) +
                                               0.587 * qGreen(p) +
                                               0.114 * qBlue(p)),
                              255);
            line[x] = qRgba(gray, gray, gray, qAlpha(p));
        }
    }
    return img;
}

QImage Pointops::invert(const QImage& src)
{
    QVector<uchar> lut(256);
    for (int i = 0; i < 256; ++i)
        lut[i] = static_cast<uchar>(255 - i);
    return applyLut(src, lut);
}

QImage Pointops::threshold(const QImage& src, int level)
{
    QVector<uchar> lut(256);
    for (int i = 0; i < 256; ++i)
        lut[i] = (i < level) ? 0 : 255;
    return applyLut(src, lut);
}

QImage Pointops::gammaCorrect(const QImage& src, float gamma)
{
    QVector<uchar> lut(256);
    float inv = 1.0f / gamma;
    for (int i = 0; i < 256; ++i) {
        float v = 255.0f * std::pow(i / 255.0f, inv);
        lut[i] = static_cast<uchar>(qBound(0.0f, v, 255.0f));
    }
    return applyLut(src, lut);
}
