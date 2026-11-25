#include "filltools.h"

#include <QVector>
#include <vector>
#include <cmath>
#include <cstdint>

namespace {

inline bool colorWithinToleranceSq(QRgb a, QRgb b, int tolSq)
{
    const int dr = qRed(a)   - qRed(b);
    const int dg = qGreen(a) - qGreen(b);
    const int db = qBlue(a)  - qBlue(b);
    const int da = qAlpha(a) - qAlpha(b);
    return (dr*dr + dg*dg + db*db + da*da) <= tolSq;
}

QImage boxBlurSeparable(const QImage& src, int r)
{
    const int w = src.width();
    const int h = src.height();
    QImage tmp(w, h, QImage::Format_ARGB32);
    QImage out(w, h, QImage::Format_ARGB32);

    // przebieg poziomy
    for (int y = 0; y < h; ++y) {
        const QRgb* s = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        QRgb* d = reinterpret_cast<QRgb*>(tmp.scanLine(y));

        int sr=0, sg=0, sb=0, sa=0, cnt=0;
        for (int x = -r; x <= r; ++x) {
            int sx = std::clamp(x, 0, w-1);
            QRgb p = s[sx];
            sr += qRed(p); sg += qGreen(p); sb += qBlue(p); sa += qAlpha(p);
            ++cnt;
        }
        for (int x = 0; x < w; ++x) {
            d[x] = qRgba(sr/cnt, sg/cnt, sb/cnt, sa/cnt);
            int xOut = std::clamp(x - r, 0, w-1);
            int xIn  = std::clamp(x + r + 1, 0, w-1);
            QRgb pOut = s[xOut], pIn = s[xIn];
            sr += qRed(pIn) - qRed(pOut);
            sg += qGreen(pIn) - qGreen(pOut);
            sb += qBlue(pIn) - qBlue(pOut);
            sa += qAlpha(pIn) - qAlpha(pOut);
        }
    }

    // przebieg pionowy
    for (int x = 0; x < w; ++x) {
        int sr=0, sg=0, sb=0, sa=0, cnt=0;
        for (int y = -r; y <= r; ++y) {
            int sy = std::clamp(y, 0, h-1);
            QRgb p = reinterpret_cast<const QRgb*>(tmp.constScanLine(sy))[x];
            sr += qRed(p); sg += qGreen(p); sb += qBlue(p); sa += qAlpha(p);
            ++cnt;
        }
        for (int y = 0; y < h; ++y) {
            reinterpret_cast<QRgb*>(out.scanLine(y))[x] =
                qRgba(sr/cnt, sg/cnt, sb/cnt, sa/cnt);
            int yOut = std::clamp(y - r, 0, h-1);
            int yIn  = std::clamp(y + r + 1, 0, h-1);
            QRgb pOut = reinterpret_cast<const QRgb*>(tmp.constScanLine(yOut))[x];
            QRgb pIn  = reinterpret_cast<const QRgb*>(tmp.constScanLine(yIn))[x];
            sr += qRed(pIn) - qRed(pOut);
            sg += qGreen(pIn) - qGreen(pOut);
            sb += qBlue(pIn) - qBlue(pOut);
            sa += qAlpha(pIn) - qAlpha(pOut);
        }
    }

    return out;
}

QImage morphMask(const QImage& mask, int radius)
{
    if (radius == 0) return mask;

    const bool dilate = radius > 0;
    const int r = std::abs(radius);
    const int w = mask.width();
    const int h = mask.height();

    const uchar fillValue = dilate ? 0 : 255;
    const uchar hitValue  = dilate ? 255 : 0;

    auto pass1D = [&](const QImage& in, QImage& out, bool horizontal) {
        out = QImage(w, h, QImage::Format_Grayscale8);
        out.fill(fillValue);

        const int outer = horizontal ? h : w;
        const int inner = horizontal ? w : h;

        for (int o = 0; o < outer; ++o) {
            const uchar* srcLine = horizontal ? in.constScanLine(o)
                                              : nullptr;
            uchar* dstLine = horizontal ? out.scanLine(o)
                                        : nullptr;

            std::vector<uchar> colIn(inner), colOut(inner);
            if (!horizontal) {
                for (int i = 0; i < inner; ++i)
                    colIn[i] = in.constScanLine(i)[o];
            }

            const uchar* src = horizontal ? srcLine : colIn.data();
            uchar* dst = horizontal ? dstLine : colOut.data();

            for (int i = 0; i < inner; ++i) {
                bool hit = false;
                for (int d = -r; d <= r; ++d) {
                    int ni = i + d;
                    if (ni < 0 || ni >= inner) {
                        if (!dilate) { hit = false; break; }
                        continue;
                    }
                    const uchar v = src[ni];
                    if (dilate) {
                        if (v >= 128) { hit = true; break; }
                    } else {
                        if (v < 128) { hit = false; break; }
                        hit = true;
                    }
                }
                dst[i] = hit ? 255 : 0;
            }

            if (!horizontal) {
                for (int i = 0; i < inner; ++i)
                    out.scanLine(i)[o] = colOut[i];
            }
        }
    };

    QImage tmp;
    pass1D(mask, tmp, true);
    QImage result;
    pass1D(tmp, result, false);
    return result;
}

} // namespace

QImage FillTools::floodFillMask(const QImage& sourceImage, const QPoint& seedPoint,
                                const FillSettings& settings)
{
    const int w = sourceImage.width();
    const int h = sourceImage.height();

    QImage result(w, h, QImage::Format_Grayscale8);
    result.fill(0);

    if (seedPoint.x() < 0 || seedPoint.x() >= w ||
        seedPoint.y() < 0 || seedPoint.y() >= h)
        return result;

    QImage src = (sourceImage.format() == QImage::Format_ARGB32)
                     ? sourceImage
                     : sourceImage.convertToFormat(QImage::Format_ARGB32);

    QImage compareImg = (settings.gapClosing > 0)
                            ? boxBlurSeparable(src, settings.gapClosing)
                            : src;

    const int tol = std::clamp(settings.tolerance, 0, 100);
    const double maxDist = std::sqrt(4.0 * 255.0 * 255.0);
    const double tolDist = (tol / 100.0) * maxDist;
    const int tolSq = int(tolDist * tolDist);

    const QRgb seedColor = compareImg.pixel(seedPoint);

    std::vector<uint8_t> visited(size_t(w) * h, 0);
    std::vector<int> stack;
    stack.reserve(size_t(w) * h / 4 + 16);
    stack.push_back(seedPoint.y() * w + seedPoint.x());
    visited[size_t(seedPoint.y()) * w + seedPoint.x()] = 1;

    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};

    while (!stack.empty()) {
        int idx = stack.back();
        stack.pop_back();

        int px = idx % w;
        int py = idx / w;

        result.scanLine(py)[px] = 255;

        for (int i = 0; i < 4; ++i) {
            int nx = px + dx[i];
            int ny = py + dy[i];
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;

            size_t nidx = size_t(ny) * w + nx;
            if (visited[nidx]) continue;

            const QRgb* line = reinterpret_cast<const QRgb*>(compareImg.constScanLine(ny));
            if (colorWithinToleranceSq(line[nx], seedColor, tolSq)) {
                visited[nidx] = 1;
                stack.push_back(int(nidx));
            }
        }
    }

    if (settings.expand != 0)
        result = morphMask(result, settings.expand);

    return result;
}

QImage FillTools::maskToAlphaArgb(const QImage& grayscaleMask)
{
    if (grayscaleMask.isNull())
        return QImage();

    const int w = grayscaleMask.width();
    const int h = grayscaleMask.height();

    QImage out(w, h, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);

    for (int y = 0; y < h; ++y) {
        const uchar* src = grayscaleMask.constScanLine(y);
        QRgb* dst = reinterpret_cast<QRgb*>(out.scanLine(y));
        for (int x = 0; x < w; ++x) {
            const int a = src[x];
            dst[x] = qRgba(255, 255, 255, a);
        }
    }

    return out;
}
