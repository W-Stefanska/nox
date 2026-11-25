#pragma once
#include <QObject>
#include "layer.h"
#include <vector>
#include <memory>

class LayerManager : public QObject {
    Q_OBJECT
public:
    LayerManager(int width, int height);

    int layerCount() const;
    Layer* layer(int index);
    QString layerName(int index) const;

    std::shared_ptr<Layer> addLayer(const QString& name);
    std::shared_ptr<Layer> activeLayer();
    void setActiveLayer(int index);
    QImage compositeImage() const;
    void removeLayer(int index);
    void clearLayer(int index);
    void clearAllLayers();

    void moveLayerUp(int index);
    void moveLayerDown(int index);
    void moveLayerTo(int from, int to);

    void debugLayers() const;

    int activeIndex() const { return m_activeIndex; }

    void flipHorizontal(int index);
    void flipVertical(int index);
    void rotate(int index, int degrees);

    void resize(int width, int height);
    void reset(int width, int height);

    int width()  const { return m_width;  }
    int height() const { return m_height; }

    QImage compositeImageExcluding(int excludeIndex) const;

    std::shared_ptr<Layer> addImageLayer(const QImage& image, const QString& name);
private:
    std::vector<std::shared_ptr<Layer>> m_layers;
    int m_activeIndex = 0;
    int m_width;
    int m_height;
    QString m_projectPath;

signals:
    void layersChanged();
    void activeLayerChanged(int index);
};

