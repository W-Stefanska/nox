#include "layer.h"

Layer::Layer(int width, int height, const QString& name)
    : m_image(width, height, QImage::Format_ARGB32_Premultiplied)
    , m_name(name)
{
    m_image.fill(Qt::transparent);
}

void Layer::clear()                         { m_image.fill(Qt::transparent); }
//void Layer::paintTo(Layer& target) const    { QPainter(&target.image()).drawImage(0, 0, m_image); }

QImage&       Layer::image()                { return m_image; }
const QImage& Layer::image() const          { return m_image; }
QString       Layer::name()        const    { return m_name; }
bool          Layer::isVisible()   const    { return m_visible; }
float         Layer::opacity()     const    { return m_opacity; }
bool          Layer::alphaLocked() const    { return m_alphaLocked; }
BlendMode     Layer::blendMode()   const    { return m_blendMode; }

void Layer::setName(const QString& name)    { m_name = name; }
void Layer::setVisible(bool v)              { m_visible = v; }
void Layer::setAlphaLocked(bool v)          { m_alphaLocked = v; }
void Layer::setBlendMode(BlendMode mode)    { m_blendMode = mode; }
void Layer::setOpacity(float opacity)       { m_opacity = std::clamp(opacity, 0.0f, 1.0f); }

QString blendModeToString(BlendMode mode) {
    switch (mode) {
    case BlendMode::Normal:     return "Normal";
    case BlendMode::Multiply:   return "Multiply";
    case BlendMode::Screen:     return "Screen";
    case BlendMode::Overlay:    return "Overlay";
    case BlendMode::Darken:     return "Darken";
    case BlendMode::Lighten:    return "Lighten";
    case BlendMode::ColorDodge: return "ColorDodge";
    case BlendMode::ColorBurn:  return "ColorBurn";
    case BlendMode::HardLight:  return "HardLight";
    case BlendMode::SoftLight:  return "SoftLight";
    case BlendMode::Difference: return "Difference";
    case BlendMode::Exclusion:  return "Exclusion";
    case BlendMode::Add:        return "Add";
    case BlendMode::Hue:        return "Hue";
    case BlendMode::Saturation: return "Saturation";
    case BlendMode::Color:      return "Color";
    case BlendMode::Luminosity: return "Luminosity";
    default:                    return "Normal";
    }
}

BlendMode blendModeFromString(const QString& str) {
    if (str == "Multiply")   return BlendMode::Multiply;
    if (str == "Screen")     return BlendMode::Screen;
    if (str == "Overlay")    return BlendMode::Overlay;
    if (str == "Darken")     return BlendMode::Darken;
    if (str == "Lighten")    return BlendMode::Lighten;
    if (str == "ColorDodge") return BlendMode::ColorDodge;
    if (str == "ColorBurn")  return BlendMode::ColorBurn;
    if (str == "HardLight")  return BlendMode::HardLight;
    if (str == "SoftLight")  return BlendMode::SoftLight;
    if (str == "Difference") return BlendMode::Difference;
    if (str == "Exclusion")  return BlendMode::Exclusion;
    if (str == "Add")        return BlendMode::Add;
    if (str == "Hue")        return BlendMode::Hue;
    if (str == "Saturation") return BlendMode::Saturation;
    if (str == "Color")      return BlendMode::Color;
    if (str == "Luminosity") return BlendMode::Luminosity;
    return BlendMode::Normal;
}
