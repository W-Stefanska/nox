#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QComboBox>
#include "layeritemwidget.h"
#include "../../core/commandmanager.h"

class QSlider;
class LayerManager;

class LayersPanel : public QWidget {
    Q_OBJECT
public:
    explicit LayersPanel(LayerManager* manager, QWidget* parent = nullptr);
    void refresh();
    void refreshThumbnails();
    void updateControlsForActiveLayer();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    LayerManager*               m_manager;
    QVBoxLayout*                m_listLayout;
    QSlider*                    m_opacity;
    QList<LayerItemWidget*>     m_items;
    QComboBox*                  m_blendMode;

    int m_dropTargetIndex = -1;
    int m_dropIndicatorY = -1;
    int m_dragSourceIndex = -1;

    void applyStyle();
};
