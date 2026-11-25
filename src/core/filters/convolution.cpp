#include "convolution.h"
#include <cmath>

static void convolveRows(const QImage& src, QImage& dst,
                         const Kernel& k, int yStart, int yEnd)
{
    const int half = k.size / 2;
    const int w = src.width();
    const int h = src.height();

    for (int y = yStart; y < yEnd; ++y) {
        QRgb* dstLine = reinterpret_cast<QRgb*>(dst.scanLine(y));

        for (int x = 0; x < w; ++x) {
            float r = 0, g = 0, b = 0, a = 0;

            for (int ky = 0; ky < k.size; ++ky) {
                int sy = std::clamp(y + ky - half, 0, h - 1);
                const QRgb* srcLine =
                    reinterpret_cast<const QRgb*>(src.scanLine(sy));

                for (int kx = 0; kx < k.size; ++kx) {
                    int sx = std::clamp(x + kx - half, 0, w - 1);
                    QRgb p = srcLine[sx];
                    float weight = k.data[ky * k.size + kx];

                    const float af = qAlpha(p) / 255.0f;
                    r += qRed(p)   * af * weight;
                    g += qGreen(p) * af * weight;
                    b += qBlue(p)  * af * weight;
                    a += qAlpha(p)      * weight;
                }
            }

            r = r / k.divisor + k.offset;
            g = g / k.divisor + k.offset;
            b = b / k.divisor + k.offset;
            a = a / k.divisor;

            int ai = std::clamp(static_cast<int>(a + 0.5f), 0, 255);

            int ri = 0, gi = 0, bi = 0;
            if (ai > 0) {
                ri = std::clamp(static_cast<int>(r / (ai / 255.0f) + 0.5f), 0, 255);
                gi = std::clamp(static_cast<int>(g / (ai / 255.0f) + 0.5f), 0, 255);
                bi = std::clamp(static_cast<int>(b / (ai / 255.0f) + 0.5f), 0, 255);
            }
            dstLine[x] = qRgba(ri, gi, bi, ai);
        }
    }
}

QImage applyConvolution(const QImage& src, const Kernel& k)
{
    if (src.isNull() || k.data.size() != static_cast<size_t>(k.size * k.size))
        return src;

    QImage work = src.convertToFormat(QImage::Format_ARGB32);
    QImage out(work.size(), work.format());

    convolveRows(work, out, k, 0, work.height());
    return out;
}

QImage applySeparableConvolution(const QImage& src,
                                 const std::vector<float>& kernel1D,
                                 float divisor)
{
    if (src.isNull() || kernel1D.empty())
        return src;

    const int radius = static_cast<int>(kernel1D.size()) / 2;
    const int w = src.width();
    const int h = src.height();

    QImage work = src.convertToFormat(QImage::Format_ARGB32);

    QImage temp(w, h, QImage::Format_ARGB32);

    for (int y = 0; y < h; ++y) {
        const QRgb* srcLine = reinterpret_cast<const QRgb*>(work.scanLine(y));
        QRgb* dstLine = reinterpret_cast<QRgb*>(temp.scanLine(y));

        for (int x = 0; x < w; ++x) {
            float r = 0, g = 0, b = 0, a = 0;

            for (int kx = -radius; kx <= radius; ++kx) {
                int sx = std::clamp(x + kx, 0, w - 1);
                QRgb p = srcLine[sx];
                float weight = kernel1D[kx + radius];

                const float af = qAlpha(p) / 255.0f;
                r += qRed(p)   * af * weight;
                g += qGreen(p) * af * weight;
                b += qBlue(p)  * af * weight;
                a += qAlpha(p)      * weight;
            }

            r /= divisor;
            g /= divisor;
            b /= divisor;
            a /= divisor;

            int ai = std::clamp(static_cast<int>(a + 0.5f), 0, 255);
            int ri = 0, gi = 0, bi = 0;
            if (ai > 0) {
                const float invA = 255.0f / ai;
                ri = std::clamp(static_cast<int>(r * invA + 0.5f), 0, 255);
                gi = std::clamp(static_cast<int>(g * invA + 0.5f), 0, 255);
                bi = std::clamp(static_cast<int>(b * invA + 0.5f), 0, 255);
            }

            dstLine[x] = qRgba(ri, gi, bi, ai);
        }
    }

    QImage out(w, h, QImage::Format_ARGB32);

    for (int y = 0; y < h; ++y) {
        QRgb* dstLine = reinterpret_cast<QRgb*>(out.scanLine(y));

        for (int x = 0; x < w; ++x) {
            float r = 0, g = 0, b = 0, a = 0;

            for (int ky = -radius; ky <= radius; ++ky) {
                int sy = std::clamp(y + ky, 0, h - 1);
                const QRgb* srcLine =
                    reinterpret_cast<const QRgb*>(temp.scanLine(sy));
                QRgb p = srcLine[x];
                float weight = kernel1D[ky + radius];

                const float af = qAlpha(p) / 255.0f;
                r += qRed(p)   * af * weight;
                g += qGreen(p) * af * weight;
                b += qBlue(p)  * af * weight;
                a += qAlpha(p)      * weight;
            }

            r /= divisor;
            g /= divisor;
            b /= divisor;
            a /= divisor;

            int ai = std::clamp(static_cast<int>(a + 0.5f), 0, 255);
            int ri = 0, gi = 0, bi = 0;
            if (ai > 0) {
                const float invA = 255.0f / ai;
                ri = std::clamp(static_cast<int>(r * invA + 0.5f), 0, 255);
                gi = std::clamp(static_cast<int>(g * invA + 0.5f), 0, 255);
                bi = std::clamp(static_cast<int>(b * invA + 0.5f), 0, 255);
            }

            dstLine[x] = qRgba(ri, gi, bi, ai);
        }
    }

    return out;
}

namespace Kernels {

Kernel gaussianBlur3x3() {
    return Kernel{
        { 1, 2, 1,
         2, 4, 2,
         1, 2, 1 },
        3, 16.0f, 0.0f
    };
}

Kernel sharpen() {
    return Kernel{
        {  0, -1,  0,
         -1,  5, -1,
         0, -1,  0 },
        3, 1.0f, 0.0f
    };
}

// Laplacian
Kernel edgeDetect() {
    return Kernel{
        { -1, -1, -1,
         -1,  8, -1,
         -1, -1, -1 },
        3, 1.0f, 0.0f
    };
}

Kernel sobelX() {
    return Kernel{
        { -1, 0, 1,
         -2, 0, 2,
         -1, 0, 1 },
        3, 1.0f, 128.0f
    };
}

Kernel sobelY() {
    return Kernel{
        { -1, -2, -1,
         0,  0,  0,
         1,  2,  1 },
        3, 1.0f, 128.0f
    };
}

Kernel emboss() {
    return Kernel{
        { -2, -1,  0,
         -1,  1,  1,
         0,  1,  2 },
        3, 1.0f, 128.0f
    };
}

Kernel gaussianBlur(int radius, float sigma)
{
    const int size = 2 * radius + 1;
    std::vector<float> data(size * size);

    if (sigma <= 0.0f)
        sigma = radius * 0.5f;

    const float twoSigmaSq = 2.0f * sigma * sigma;
    float sum = 0.0f;

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            float v = std::exp(-(x*x + y*y) / twoSigmaSq);
            data[(y + radius) * size + (x + radius)] = v;
            sum += v;
        }
    }

    return Kernel{ data, size, sum, 0.0f };
}

Kernel boxBlur(int radius)
{
    const int size = 2 * radius + 1;
    std::vector<float> data(size * size, 1.0f);
    const float divisor = float(size * size);
    return Kernel{ data, size, divisor, 0.0f };
}

Kernel customKernel(const std::vector<float>& data, int size,
                    float divisor, float offset)
{
    return Kernel{ data, size, divisor, offset };
}

std::vector<float> gaussianKernel1D(int radius, float sigma)
{
    const int size = 2 * radius + 1;
    std::vector<float> data(size);

    if (sigma <= 0.0f)
        sigma = radius * 0.5f;

    const float twoSigmaSq = 2.0f * sigma * sigma;
    float sum = 0.0f;

    for (int i = -radius; i <= radius; ++i) {
        float v = std::exp(-(i * i) / twoSigmaSq);
        data[i + radius] = v;
        sum += v;
    }

    return data;
}

} // namespace Kernels
