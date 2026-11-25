#pragma once

#include <QImage>

class Pointops
{
public:
    static QImage adjustBrightness(const QImage& src, int delta);
    static QImage adjustContrast(const QImage& src, float factor);
    static QImage toGrayscale(const QImage& src);
    static QImage invert(const QImage& src);
    static QImage threshold(const QImage& src, int level);
    static QImage gammaCorrect(const QImage& src, float gamma);
};
