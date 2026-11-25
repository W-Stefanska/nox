#include "layerspanel.h"
#include "../../core/layermanager.h"
#include "../../core/layer.h"
#include <QEvent>
#include <QSlider>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QIcon>

LayersPanel::LayersPanel(LayerManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager)
{

    setAcceptDrops(true);
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ===== Nagłówek =====
    auto* titleLabel = new QLabel("WARSTWY", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName("sectionLabel");

    // ===== Lista warstw =====
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setObjectName("layerScroll");

    auto* listContainer = new QWidget(scrollArea);
    listContainer->setAutoFillBackground(false);
    listContainer->setStyleSheet("background: transparent;");
    m_listLayout = new QVBoxLayout(listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(2);
    m_listLayout->addStretch();
    scrollArea->setWidget(listContainer);
    scrollArea->setStyleSheet("QScrollArea { background: #252525; } "
                              "QWidget { background: transparent; }");

    // ===== Separator =====
    auto* line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setObjectName("separator");

    // ===== Opacity =====
    auto* opacityLabel = new QLabel("KRYCIE", this);
    opacityLabel->setAlignment(Qt::AlignCenter);
    opacityLabel->setObjectName("sectionLabel");

    m_opacity = new QSlider(Qt::Horizontal, this);
    m_opacity->setRange(0, 100);
    m_opacity->setValue(100);

    auto* opacityValueLabel = new QLabel("100%", this);
    opacityValueLabel->setAlignment(Qt::AlignCenter);
    opacityValueLabel->setObjectName("valueLabel");

    connect(m_opacity, &QSlider::valueChanged, [this, opacityValueLabel](int v) {
        opacityValueLabel->setText(QString::number(v) + "%");
        if (auto layer = m_manager->activeLayer()) {
            layer->setOpacity(v / 100.0f);
            emit m_manager->layersChanged();
        }
    });

    // ===== Separator =====
    auto* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setObjectName("separator");

    // ===== Przyciski dolne =====
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(4);

    auto makeBtn = [this](const QIcon& icon, const QString& tip) {
        auto* btn = new QToolButton(this);
        btn->setIcon(icon);
        btn->setToolTip(tip);
        btn->setFixedHeight(30);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setObjectName("toolBtn");
        return btn;
    };

    auto* addBtn  = makeBtn(QIcon(":/resources/plus.svg"), "Dodaj warstwę");
    auto* upBtn   = makeBtn(QIcon(":/resources/up.svg"), "Przesuń wyżej");
    auto* downBtn = makeBtn(QIcon(":/resources/down.svg"), "Przesuń niżej");

    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(upBtn);
    btnLayout->addWidget(downBtn);

    connect(addBtn, &QToolButton::clicked, [this]() {
        m_manager->addLayer("Layer " + QString::number(m_manager->layerCount() + 1));

    });

    connect(upBtn, &QToolButton::clicked, [this]() {
        int idx = m_manager->activeIndex();
        if (idx >= m_manager->layerCount() - 1) return;
        m_manager->moveLayerDown(idx);
        m_manager->setActiveLayer(idx + 1);

    });

    connect(downBtn, &QToolButton::clicked, [this]() {
        int idx = m_manager->activeIndex();
        if (idx <= 0) return;
        m_manager->moveLayerUp(idx);
        m_manager->setActiveLayer(idx - 1);

    });

    // ===== Mieszanie =====
    auto* blendLabel = new QLabel("TRYB MIESZANIA", this);
    blendLabel->setAlignment(Qt::AlignCenter);
    blendLabel->setObjectName("sectionLabel");

    m_blendMode = new QComboBox(this);
    m_blendMode->setObjectName("blendCombo");
    for (const auto& name : {
             "Normal", "Multiply", "Screen", "Overlay",
             "Darken", "Lighten", "ColorDodge", "ColorBurn",
             "HardLight", "SoftLight", "Difference", "Exclusion",
             "Add", "Hue", "Saturation", "Color", "Luminosity"
         })
        m_blendMode->addItem(name);

    connect(m_blendMode, &QComboBox::currentTextChanged, [this](const QString& text) {
        if (auto layer = m_manager->activeLayer()) {
            layer->setBlendMode(blendModeFromString(text));
            emit m_manager->layersChanged();
        }
    });

    // ===== Delete & Clear =====
    auto* deleteBtn = makeBtn(QIcon(":/resources/delete.svg"), "Usuń warstwę");
    auto* clearBtn  = makeBtn(QIcon(":/resources/clear.svg"), "Wyczyść warstwę");
    btnLayout->addWidget(clearBtn);
    btnLayout->addWidget(deleteBtn);

    connect(clearBtn, &QToolButton::clicked, [this]() {
        if (auto layer = m_manager->activeLayer()) {
            layer->clear();
        }
        emit m_manager->layersChanged();
    });

    connect(deleteBtn, &QToolButton::clicked, [this]() {
        m_manager->removeLayer(m_manager->activeIndex());

    });

    // ===== Focus =====
    setFocusPolicy(Qt::StrongFocus);

    // ===== Layout =====
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(line1);
    mainLayout->addWidget(opacityLabel);
    mainLayout->addWidget(m_opacity);
    mainLayout->addWidget(opacityValueLabel);
    mainLayout->addWidget(blendLabel);
    mainLayout->addWidget(m_blendMode);
    mainLayout->addWidget(line2);
    mainLayout->addLayout(btnLayout);

    refresh();
    applyStyle();
}

void LayersPanel::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete) {
        m_manager->removeLayer(m_manager->activeIndex());

    }
    QWidget::keyPressEvent(event);
}

void LayersPanel::refresh()
{
    for (auto* item : m_items)
        item->deleteLater();
    m_items.clear();

    int active = m_manager->activeIndex();

    for (int i = m_manager->layerCount() - 1; i >= 0; --i) {
        auto* w = new LayerItemWidget(m_manager->layer(i), this);
        w->refreshThumbnail();
        w->setActive(i == active);

        connect(w, &LayerItemWidget::visibilityToggled, this, [this]() {
            emit m_manager->layersChanged();
        });
        w->installEventFilter(this);
        w->setProperty("layerIndex", i);

        m_listLayout->insertWidget(m_listLayout->count() - 1, w);
        m_items.append(w);
    }

    updateControlsForActiveLayer();

    if (auto layer = m_manager->activeLayer())
        m_opacity->setValue(qRound(layer->opacity() * 100));
}

void LayersPanel::refreshThumbnails()
{
    for (auto* item : m_items)
        item->refreshThumbnail();
}

bool LayersPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto* w = qobject_cast<LayerItemWidget*>(obj);
        if (w) {
            int idx = w->property("layerIndex").toInt();
            m_manager->setActiveLayer(idx);
            for (auto* item : m_items)
                item->setActive(item->property("layerIndex").toInt() == idx);
            if (auto layer = m_manager->activeLayer())
                m_opacity->setValue(qRound(layer->opacity() * 100));
            return false;
        }
    }
    updateControlsForActiveLayer();
    return QWidget::eventFilter(obj, event);
}

void LayersPanel::updateControlsForActiveLayer()
{
    if (auto layer = m_manager->activeLayer()) {
        m_opacity->blockSignals(true);
        m_blendMode->blockSignals(true);
        m_opacity->setValue(qRound(layer->opacity() * 100));
        m_blendMode->setCurrentText(blendModeToString(layer->blendMode()));
        m_opacity->blockSignals(false);
        m_blendMode->blockSignals(false);
    }
}

void LayersPanel::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-layer-index"))
        event->acceptProposedAction();
}

void LayersPanel::dragMoveEvent(QDragMoveEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-layer-index")) return;

    QPoint globalPos = mapToGlobal(event->position().toPoint());
    m_dropIndicatorY = -1;
    m_dropTargetIndex = -1;

    for (int i = 0; i < m_items.size(); ++i) {
        auto* item = m_items[i];
        QRect globalRect(item->mapToGlobal(QPoint(0, 0)), item->size());
        if (globalRect.contains(globalPos)) {
            int layerIndex = item->property("layerIndex").toInt();
            bool dropAbove = globalPos.y() < globalRect.center().y();

            m_dropTargetIndex = dropAbove ? layerIndex + 1 : layerIndex;
            m_dropTargetIndex = std::clamp(m_dropTargetIndex, 0, m_manager->layerCount());

            if (dropAbove && i > 0) {

                auto* above = m_items[i - 1];
                QRect aboveGlobal(above->mapToGlobal(QPoint(0, 0)), above->size());
                int y1 = globalRect.top();
                int y2 = aboveGlobal.bottom();
                m_dropIndicatorY = mapFromGlobal(QPoint(0, (y1 + y2) / 2)).y();
            } else if (!dropAbove && i < m_items.size() - 1) {

                auto* below = m_items[i + 1];
                QRect belowGlobal(below->mapToGlobal(QPoint(0, 0)), below->size());
                int y1 = globalRect.bottom();
                int y2 = belowGlobal.top();
                m_dropIndicatorY = mapFromGlobal(QPoint(0, (y1 + y2) / 2)).y();
            } else {

                QPoint localTopLeft = mapFromGlobal(globalRect.topLeft());
                m_dropIndicatorY = dropAbove
                                       ? localTopLeft.y()
                                       : localTopLeft.y() + item->height();
            }
            break;
        }
    }
    update();
    event->acceptProposedAction();
}

void LayersPanel::dropEvent(QDropEvent* event)
{
    m_dropIndicatorY = -1;
    update();

    int from = event->mimeData()->data("application/x-layer-index").toInt();
    int target = m_dropTargetIndex;
    m_dropTargetIndex = -1;

    if (target == -1 || target == from) return;

    if (from < target) target--;

    target = std::clamp(target, 0, m_manager->layerCount() - 1);
    if (target == from) return;

    m_manager->moveLayerTo(from, target);
    m_manager->setActiveLayer(target);
    emit m_manager->layersChanged();
    event->acceptProposedAction();
}

void LayersPanel::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (m_dropIndicatorY < 0) return;

    QPainter p(this);
    p.setPen(QPen(QColor("#0d6efd"), 2));
    p.drawLine(8, m_dropIndicatorY, width() - 8, m_dropIndicatorY);

    p.setBrush(QColor("#0d6efd"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(4, m_dropIndicatorY - 3, 6, 6);
}

void LayersPanel::applyStyle()
{
    setStyleSheet(R"(
        LayersPanel {
            background-color: #2b2b2b;
        }
        QLabel#sectionLabel {
            color: #888888;
            font-size: 10px;
            font-weight: bold;
            letter-spacing: 1px;
            padding: 4px 0 2px 0;
        }
        QLabel#valueLabel {
            color: #cccccc;
            font-size: 11px;
        }
        QToolButton#toolBtn {
            background-color: #3c3c3c;
            color: #dddddd;
            border: 1px solid #555555;
            border-radius: 6px;
            font-size: 13px;
        }
        QToolButton#toolBtn:hover {
            background-color: #4a4a4a;
            border-color: #777777;
        }
        QSlider::groove:horizontal {
            height: 4px;
            background: #555555;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #0d6efd;
            border-radius: 7px;
            width: 14px;
            height: 14px;
            margin: -5px 0;
        }
        QSlider::sub-page:horizontal {
            background: #0d6efd;
            border-radius: 2px;
        }
        QFrame#separator {
            color: #444444;
        }
        QScrollArea#layerScroll {
            background-color: #252525;
            border-radius: 6px;
        }
        LayerItemWidget {
            background-color: #333333;
            border-radius: 4px;
        }
        LayerItemWidget[active=true] {
            background-color: #0d6efd;
        }
        QLabel#layerName {
            color: #dddddd;
            font-size: 12px;
        }
        QLabel#thumbnail {
            border: 1px solid #555555;
            border-radius: 3px;
        }
        QToolButton#iconBtn {
            background: transparent;
            border: none;
            font-size: 14px;
        }
        QToolButton#iconBtn:hover {
            background-color: rgba(255,255,255,0.1);
            border-radius: 4px;
        }
        QLineEdit#layerNameEdit {
            background-color: #1e1e1e;
            color: #ffffff;
            border: 1px solid #0d6efd;
            border-radius: 3px;
            padding: 1px 4px;
            font-size: 12px;
        }
        QComboBox#blendCombo {
            background-color: #3c3c3c;
            color: #dddddd;
            border: 1px solid #555555;
            border-radius: 6px;
            padding: 4px 8px;
            font-size: 12px;
        }
        QComboBox#blendCombo:hover {
            border-color: #777777;
        }
        QComboBox#blendCombo QAbstractItemView {
            background-color: #3c3c3c;
            color: #dddddd;
            selection-background-color: #0d6efd;
        }
    )");
}
