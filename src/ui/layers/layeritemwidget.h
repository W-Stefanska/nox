#pragma once
#include <QWidget>
#include <qabstractspinbox.h>

class QLabel;
class QToolButton;
class Layer;

class LayerItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit LayerItemWidget(Layer* layer, QWidget* parent = nullptr);
    void refreshThumbnail();
    void setActive(bool active);

    ~LayerItemWidget() override;
signals:
    void visibilityToggled();
    void alphaLockToggled();
    void renameRequested(const QString& newName);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    Layer*       m_layer;
    QLabel*      m_thumbnail;
    QLabel*      m_nameLabel;
    QLineEdit*   m_nameEdit;
    QToolButton* m_visBtn;
    QToolButton* m_alphaBtn;

    bool m_active = false;
    void enterRenameMode();
    void commitRename();
};
