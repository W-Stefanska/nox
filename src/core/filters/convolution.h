#pragma once

#include <vector>
#include <QImage>

struct Kernel {
    std::vector<float> data;
    int size;
    float divisor=1;
    float offset=0;
};

QImage applyConvolution(const QImage& src, const Kernel& k);

QImage applySeparableConvolution(const QImage& src,
                                 const std::vector<float>& kernel1D,
                                 float divisor = 1.0f);
namespace Kernels {
    Kernel gaussianBlur3x3();
    Kernel sharpen();
    Kernel edgeDetect();
    Kernel sobelX();
    Kernel sobelY();
    Kernel emboss();
    Kernel gaussianBlur(int radius, float sigma);
        std::vector<float> gaussianKernel1D(int radius, float sigma);
    Kernel boxBlur(int radius);
    Kernel customKernel(const std::vector<float>& data, int size,
                        float divisor = 1.0f, float offset = 0.0f);
}

