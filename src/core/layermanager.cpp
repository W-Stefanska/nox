#include "layermanager.h"
#include <QPainter>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

LayerManager::LayerManager(int width, int height)
    : m_width(width), m_height(height)
{
    addLayer("Background");
}

void LayerManager::debugLayers() const
{
    qDebug() << "=== Layers ===";
    for (int i = 0; i < (int)m_layers.size(); ++i)
        qDebug() << i << (i == m_activeIndex ? "[ACTIVE]" : "       ") << m_layers[i]->name();
}

std::shared_ptr<Layer> LayerManager::addLayer(const QString& name)
{
    auto layer = std::make_shared<Layer>(m_width, m_height, name);
    m_layers.push_back(layer);
    m_activeIndex = static_cast<int>(m_layers.size()) - 1;
    emit layersChanged();
    return layer;
}

void LayerManager::resize(int width, int height)
{
    m_width  = width;
    m_height = height;
    for (auto& layer : m_layers) {
        QImage newImg(width, height, QImage::Format_ARGB32_Premultiplied);
        newImg.fill(Qt::transparent);
        QPainter p(&newImg);
        p.drawImage(0, 0, layer->image());
        layer->image() = newImg;
    }
    emit layersChanged();
}

void LayerManager::reset(int width, int height)
{
    m_width  = width;
    m_height = height;
    m_layers.clear();
    m_activeIndex = 0;
    addLayer("Background");
    emit layersChanged();
}

std::shared_ptr<Layer> LayerManager::activeLayer()
{
    if (m_activeIndex >= 0 && m_activeIndex < (int)m_layers.size())
        return m_layers[m_activeIndex];
    return nullptr;
}

void LayerManager::setActiveLayer(int index)
{
    if (index >= 0 && index < (int)m_layers.size()) {
        m_activeIndex = index;
        emit activeLayerChanged(index);
    }
}

static QImage blendManual(const QImage& base, const QImage& blend,
                          BlendMode mode, float opacity)
{
    QImage result = base.convertToFormat(QImage::Format_ARGB32);
    QImage src    = blend.convertToFormat(QImage::Format_ARGB32);

    auto lum = [](float r, float g, float b) {
        return 0.3f * r + 0.59f * g + 0.11f * b;
    };

    auto sat = [](float r, float g, float b) {
        return qMax(r, qMax(g, b)) - qMin(r, qMin(g, b));
    };

    auto clipColor = [&](float& r, float& g, float& b) {
        float l = lum(r, g, b);
        float n = qMin(r, qMin(g, b));
        float x = qMax(r, qMax(g, b));

        if (n < 0.0f) {
            r = l + (r - l) * l / (l - n);
            g = l + (g - l) * l / (l - n);
            b = l + (b - l) * l / (l - n);
        }
        if (x > 1.0f) {
            r = l + (r - l) * (1.0f - l) / (x - l);
            g = l + (g - l) * (1.0f - l) / (x - l);
            b = l + (b - l) * (1.0f - l) / (x - l);
        }
    };

    auto setLum = [&](float& r, float& g, float& b, float l) {
        float d = l - lum(r, g, b);
        r += d;
        g += d;
        b += d;
        clipColor(r, g, b);
    };

    auto setSat = [&](float& r, float& g, float& b, float s) {
        float mn = qMin(r, qMin(g, b));
        float mx = qMax(r, qMax(g, b));

        if (mx > mn) {
            float mid = r + g + b - mn - mx;
            float newMid = (mid - mn) * s / (mx - mn);
            float newMax = s;

            float* comps[3] = { &r, &g, &b };
            for (int i = 0; i < 3; ++i) {
                if (*comps[i] == mn)      *comps[i] = 0.0f;
                else if (*comps[i] == mx) *comps[i] = newMax;
                else                      *comps[i] = newMid;
            }
        } else {
            r = g = b = 0.0f;
        }
    };

    for (int y = 0; y < result.height(); ++y) {
        const QRgb* srcLine = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        QRgb*       dstLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < result.width(); ++x) {
            QColor b(dstLine[x]);
            QColor s(srcLine[x]);

            float sa = qAlpha(srcLine[x]) / 255.0f * opacity;
            if (sa == 0.0f) continue;

            float ba = b.alphaF();
            float br = b.redF(),   bg = b.greenF(),   bb = b.blueF();
            float sr = s.redF(),   sg = s.greenF(),   sb = s.blueF();

            float rr = sr, rg = sg, rb = sb;

            switch (mode) {
            case BlendMode::Hue:
                setSat(sr, sg, sb, sat(br, bg, bb));
                setLum(sr, sg, sb, lum(br, bg, bb));
                rr = sr; rg = sg; rb = sb;
                break;

            case BlendMode::Saturation:
                setSat(br, bg, bb, sat(sr, sg, sb));
                setLum(br, bg, bb, lum(br, bg, bb));
                rr = br; rg = bg; rb = bb;
                break;

            case BlendMode::Color:
                setLum(sr, sg, sb, lum(br, bg, bb));
                rr = sr; rg = sg; rb = sb;
                break;

            case BlendMode::Luminosity:
                setLum(br, bg, bb, lum(sr, sg, sb));
                rr = br; rg = bg; rb = bb;
                break;

            default:
                rr = sr; rg = sg; rb = sb;
                break;
            }

            float a = sa + ba * (1.0f - sa);
            if (a > 0.0f) {
                rr = (rr * sa + br * ba * (1.0f - sa)) / a;
                rg = (rg * sa + bg * ba * (1.0f - sa)) / a;
                rb = (rb * sa + bb * ba * (1.0f - sa)) / a;
            }

            dstLine[x] = qRgba(
                qBound(0, int(rr * 255.0f + 0.5f), 255),
                qBound(0, int(rg * 255.0f + 0.5f), 255),
                qBound(0, int(rb * 255.0f + 0.5f), 255),
                qBound(0, int(a  * 255.0f + 0.5f), 255)
                );
        }
    }

    return result;
}

QImage LayerManager::compositeImage() const
{
    QImage result(m_width, m_height, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    for (const auto& layer : m_layers) {
        if (!layer->isVisible()) continue;

        BlendMode mode = layer->blendMode();
        float opacity  = layer->opacity();

        if (mode == BlendMode::Hue        ||
            mode == BlendMode::Saturation ||
            mode == BlendMode::Color      ||
            mode == BlendMode::Luminosity)
        {
            QImage unpremul = result.convertToFormat(QImage::Format_ARGB32);
            unpremul = blendManual(unpremul, layer->image(), mode, opacity);
            result = unpremul.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            continue;
        }

        QPainter::CompositionMode cm;
        switch (mode) {
        case BlendMode::Multiply:   cm = QPainter::CompositionMode_Multiply;       break;
        case BlendMode::Screen:     cm = QPainter::CompositionMode_Screen;         break;
        case BlendMode::Overlay:    cm = QPainter::CompositionMode_Overlay;        break;
        case BlendMode::Darken:     cm = QPainter::CompositionMode_Darken;         break;
        case BlendMode::Lighten:    cm = QPainter::CompositionMode_Lighten;        break;
        case BlendMode::ColorDodge: cm = QPainter::CompositionMode_ColorDodge;     break;
        case BlendMode::ColorBurn:  cm = QPainter::CompositionMode_ColorBurn;      break;
        case BlendMode::HardLight:  cm = QPainter::CompositionMode_HardLight;      break;
        case BlendMode::SoftLight:  cm = QPainter::CompositionMode_SoftLight;      break;
        case BlendMode::Difference: cm = QPainter::CompositionMode_Difference;     break;
        case BlendMode::Exclusion:  cm = QPainter::CompositionMode_Exclusion;      break;
        case BlendMode::Add:        cm = QPainter::CompositionMode_Plus;           break;
        default:                    cm = QPainter::CompositionMode_SourceOver;     break;
        }

        QPainter p(&result);
        p.setOpacity(layer->opacity());
        p.setCompositionMode(cm);
        p.drawImage(0, 0, layer->image());
    }

    return result;
}


int LayerManager::layerCount() const
{
    return static_cast<int>(m_layers.size());
}

Layer* LayerManager::layer(int index)
{
    if (index < 0 || index >= layerCount())
        return nullptr;
    return m_layers[index].get();
}

QString LayerManager::layerName(int index) const
{
    if (index < 0 || index >= layerCount())
        return {};
    return m_layers[index]->name();
}

void LayerManager::moveLayerUp(int index)
{
    if (index <= 0 || index >= (int)m_layers.size())
        return;

    std::swap(m_layers[index], m_layers[index - 1]);
    setActiveLayer(index - 1);
    emit layersChanged();
}

void LayerManager::moveLayerDown(int index)
{
    if (index < 0 || index >= (int)m_layers.size() - 1)
        return;

    std::swap(m_layers[index], m_layers[index + 1]);
    setActiveLayer(index + 1);
    emit layersChanged();
}

void LayerManager::moveLayerTo(int from, int to)
{
    if (from == to) return;
    if (from < 0 || from >= (int)m_layers.size()) return;
    if (to   < 0 || to   >= (int)m_layers.size()) return;

    auto layer = m_layers[from];
    m_layers.erase(m_layers.begin() + from);
    m_layers.insert(m_layers.begin() + to, layer);
    emit layersChanged();
}

void LayerManager::removeLayer(int index)
{
    if (index < 0 || index >= (int)m_layers.size()) return;
    if (m_layers.size() == 1) return;
    m_layers.erase(m_layers.begin() + index);
    m_activeIndex = std::clamp(m_activeIndex, 0, (int)m_layers.size() - 1);
    emit layersChanged();
}

void LayerManager::clearLayer(int index)
{
    if (auto* l = layer(index)) {
        l->clear();
        emit layersChanged();
    }
}

// ===== Transformacje =====
void LayerManager::flipHorizontal(int index)
{
    if (auto* l = layer(index)) {
        l->image() = l->image().mirrored(true, false);
        emit layersChanged();
    }
}

void LayerManager::flipVertical(int index)
{
    if (auto* l = layer(index)) {
        l->image() = l->image().mirrored(false, true);
        emit layersChanged();
    }
}

void LayerManager::rotate(int index, int degrees)
{
    if (auto* l = layer(index)) {
        QTransform t;
        t.rotate(degrees);
        l->image() = l->image().transformed(t);
        emit layersChanged();
    }
}

QImage LayerManager::compositeImageExcluding(int excludeIndex) const
{
    QImage result(m_width, m_height, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    for (int i = 0; i < (int)m_layers.size(); ++i) {
        if (i == excludeIndex) continue;

        const auto& layer = m_layers[i];
        if (!layer->isVisible()) continue;

        BlendMode mode = layer->blendMode();

        if (mode == BlendMode::Hue        ||
            mode == BlendMode::Saturation ||
            mode == BlendMode::Color      ||
            mode == BlendMode::Luminosity)
        {
            QImage unpremul = result.convertToFormat(QImage::Format_ARGB32);
            unpremul = blendManual(unpremul, layer->image(), mode, layer->opacity());
            result = unpremul.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            continue;
        }

        QPainter::CompositionMode cm;
        switch (mode) {
        case BlendMode::Multiply:   cm = QPainter::CompositionMode_Multiply;   break;
        case BlendMode::Screen:     cm = QPainter::CompositionMode_Screen;     break;
        case BlendMode::Overlay:    cm = QPainter::CompositionMode_Overlay;    break;
        case BlendMode::Darken:     cm = QPainter::CompositionMode_Darken;     break;
        case BlendMode::Lighten:    cm = QPainter::CompositionMode_Lighten;    break;
        case BlendMode::ColorDodge: cm = QPainter::CompositionMode_ColorDodge; break;
        case BlendMode::ColorBurn:  cm = QPainter::CompositionMode_ColorBurn;  break;
        case BlendMode::HardLight:  cm = QPainter::CompositionMode_HardLight;  break;
        case BlendMode::SoftLight:  cm = QPainter::CompositionMode_SoftLight;  break;
        case BlendMode::Difference: cm = QPainter::CompositionMode_Difference; break;
        case BlendMode::Exclusion:  cm = QPainter::CompositionMode_Exclusion;  break;
        case BlendMode::Add:        cm = QPainter::CompositionMode_Plus;       break;
        default:                    cm = QPainter::CompositionMode_SourceOver; break;
        }

        QPainter p(&result);
        p.setOpacity(layer->opacity());
        p.setCompositionMode(cm);
        p.drawImage(0, 0, layer->image());
    }

    return result;
}

std::shared_ptr<Layer> LayerManager::addImageLayer(const QImage& image, const QString& name)
{
    if (image.isNull()) return nullptr;

    auto layer = std::make_shared<Layer>(m_width, m_height, name);

    QImage scaled = image.scaled(m_width, m_height,
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);

    const int x = (m_width  - scaled.width())  / 2;
    const int y = (m_height - scaled.height()) / 2;

    QPainter p(&layer->image());
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(x, y, scaled);
    p.end();

    m_layers.push_back(layer);
    m_activeIndex = static_cast<int>(m_layers.size()) - 1;
    emit layersChanged();
    return layer;
}

void LayerManager::clearAllLayers()
{
    m_layers.clear();
    m_activeIndex = 0;
    emit layersChanged();
}
