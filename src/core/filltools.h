// filltools.h
#pragma once

#include <QImage>
#include <QPoint>

struct FillSettings {
    int   tolerance   = 20;
    int   gapClosing  = 0;
    int   expand      = 0;
};

namespace FillTools {
QImage floodFillMask(const QImage& sourceImage, const QPoint& seedPoint,
                     const FillSettings& settings);
QImage maskToAlphaArgb(const QImage& grayscaleMask);

} // namespace FillTools
