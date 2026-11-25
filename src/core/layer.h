#pragma once

#include <QImage>
#include <QPainter>

// Blendmode - enum contains 17 blending modes
enum class BlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Add,
    Hue,
    Saturation,
    Color,
    Luminosity
};

QString   blendModeToString(BlendMode mode);
BlendMode blendModeFromString(const QString& str);

// Layer - contains a drawing layer
class Layer {
public:
    Layer(int width, int height, const QString& name = "Nowa warstwa");

    // Image access
    QImage&       image();
    const QImage& image() const;

    // Properties
    QString   name()        const;
    bool      isVisible()   const;
    float     opacity()     const;
    bool      alphaLocked() const;
    BlendMode blendMode()   const;

    // Setters
    void setName(const QString& name);
    void setVisible(bool visible);
    void setOpacity(float opacity);
    void setAlphaLocked(bool locked);
    void setBlendMode(BlendMode mode);

    // Utilities
    void clear();
    //void paintTo(Layer& target) const;

private:
    QImage    m_image;
    QString   m_name;
    BlendMode m_blendMode   = BlendMode::Normal;
    float     m_opacity     = 1.0f;
    bool      m_visible     = true;
    bool      m_alphaLocked = false;
};
