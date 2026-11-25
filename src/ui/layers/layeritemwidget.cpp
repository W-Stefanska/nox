#include "layeritemwidget.h"
#include "../../core/layer.h"
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QEvent>
#include <QStyle>
#include <QFocusEvent>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QApplication>
#include <QPaintEvent>
#include <QStyleOption>

LayerItemWidget::LayerItemWidget(Layer* layer, QWidget* parent)
    : QWidget(parent), m_layer(layer)
{
    setFixedHeight(48);

    m_thumbnail = new QLabel(this);
    m_thumbnail->setFixedSize(32, 32);
    m_thumbnail->setObjectName("thumbnail");

    m_nameLabel = new QLabel(layer->name(), this);
    m_nameLabel->setObjectName("layerName");
    m_nameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    //m_nameLabel->setMinimumWidth(0);

    m_nameEdit = new QLineEdit(layer->name(), this);
    m_nameEdit->setObjectName("layerNameEdit");
    m_nameEdit->hide();

    m_visBtn = new QToolButton(this);
    m_visBtn->setFixedSize(24, 24);
    m_visBtn->setCheckable(true);
    m_visBtn->setChecked(layer->isVisible());
    m_visBtn->setText(layer->isVisible() ? "O️" : "X");
    m_visBtn->setIcon(layer->isVisible() ? QIcon(":resources/eye.svg") : QIcon(":resources/neye.svg"));
    m_visBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_visBtn->setObjectName("iconBtn");
    m_visBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_alphaBtn = new QToolButton(this);
    m_alphaBtn->setFixedSize(24, 24);
    m_alphaBtn->setCheckable(true);
    m_alphaBtn->setChecked(layer->alphaLocked());
    m_alphaBtn->setText(layer->alphaLocked() ? "X" : "O");
    m_alphaBtn->setIcon(layer->alphaLocked() ? QIcon(":resources/lock.svg") : QIcon(":resources/unlock.svg"));
    m_alphaBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_alphaBtn->setObjectName("iconBtn");
    m_alphaBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);


    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(6);
    layout->addWidget(m_thumbnail);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_nameEdit);
    layout->addWidget(m_visBtn);
    layout->addWidget(m_alphaBtn);

    refreshThumbnail();

    // dwuklik = rename

    qApp->installEventFilter(this);
    m_nameLabel->installEventFilter(this);

    connect(m_visBtn, &QToolButton::clicked, [this](bool checked) {
        m_layer->setVisible(checked);
        m_visBtn->setText(checked ? "O️" : "X");
        m_visBtn->setIcon(checked ? QIcon(":resources/eye.svg") : QIcon(":resources/neye.svg"));
        emit visibilityToggled();
    });

    connect(m_alphaBtn, &QToolButton::clicked, [this](bool checked) {
        m_layer->setAlphaLocked(checked);
        m_alphaBtn->setText(checked ? "X" : "O");
        m_alphaBtn->setIcon(checked ? QIcon(":resources/lock.svg") : QIcon(":resources/unlock.svg"));
        emit alphaLockToggled();
    });
}

void LayerItemWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QDrag* drag = new QDrag(this);
        QMimeData* mime = new QMimeData;
        mime->setData("application/x-layer-index",
                      QByteArray::number(property("layerIndex").toInt()));
        drag->setMimeData(mime);

        QPixmap pix = m_thumbnail->pixmap().isNull()
                          ? QPixmap(32, 32)
                          : m_thumbnail->pixmap();
        drag->setPixmap(pix);
        drag->exec(Qt::MoveAction);
    }
    QWidget::mousePressEvent(event);
}

bool LayerItemWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_nameLabel && event->type() == QEvent::MouseButtonDblClick) {
        enterRenameMode();
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress && m_nameEdit->isVisible()) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalPos = mouseEvent->globalPosition().toPoint();
        QRect editGlobalRect(
            m_nameEdit->mapToGlobal(QPoint(0, 0)),
            m_nameEdit->size()
            );
        if (!editGlobalRect.contains(globalPos)) {
            commitRename();
        }
    }

    return QWidget::eventFilter(obj, event);
}

void LayerItemWidget::enterRenameMode()
{
    m_nameEdit->setText(m_nameLabel->text());
    m_nameLabel->hide();
    m_nameEdit->show();
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

void LayerItemWidget::commitRename()
{
    QString newName = m_nameEdit->text().trimmed();
    if (newName.isEmpty()) newName = m_nameLabel->text();
    m_layer->setName(newName);
    m_nameLabel->setText(newName);
    m_nameEdit->hide();
    m_nameLabel->show();
    emit renameRequested(newName);
}

void LayerItemWidget::refreshThumbnail()
{
    QPixmap checker(32, 32);
    QPainter cp(&checker);
    QColor c1(80, 80, 80), c2(120, 120, 120);
    int s = 4;
    for (int y = 0; y < 32; y += s)
        for (int x = 0; x < 32; x += s)
            cp.fillRect(x, y, s, s, ((x/s + y/s) % 2 == 0) ? c1 : c2);

    cp.drawImage(QRect(0, 0, 32, 32), m_layer->image());
    cp.end();

    m_thumbnail->setPixmap(checker);
}

void LayerItemWidget::setActive(bool active)
{
    m_active = active;
    update();
}

void LayerItemWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(m_active ? QColor("#0d6efd") : QColor("#262626"));
    p.drawRoundedRect(rect(), 4, 4);
}

LayerItemWidget::~LayerItemWidget()
{
    qApp->removeEventFilter(this);
}
