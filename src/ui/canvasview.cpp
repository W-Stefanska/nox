#include "canvasview.h"
#include "canvaspainter.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QFileDialog>

CanvasView::CanvasView(int canvasWidth, int canvasHeight, QWidget* parent)
    : QWidget(parent)
    , m_layerManager(canvasWidth, canvasHeight)
{
    setAutoFillBackground(true);
    setPalette(QPalette(Qt::darkGray));
    setMouseTracking(true);
    setAcceptDrops(true);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            this, &CanvasView::showContextMenu);

    m_marchTimer = new QTimer(this);
    m_marchTimer->setInterval(80);
    connect(m_marchTimer, &QTimer::timeout, [this]() {
        m_marchOffset = (m_marchOffset + 1) % 16;
        if (!m_selection.isEmpty()) update();
    });
    m_marchTimer->start();

    setFocusPolicy(Qt::StrongFocus);
}

void CanvasView::resizeCanvas(int width, int height)
{
    if (width == m_layerManager.width() && height == m_layerManager.height())
        return;
    m_layerManager.resize(width, height);
    emit m_layerManager.layersChanged();
    update();
}

void CanvasView::resetCanvas(int width, int height)
{
    m_layerManager.reset(width, height);
    m_selection.clear();
    m_commandManager.clear();
    emit m_layerManager.layersChanged();
    update();
}

QPoint CanvasView::canvasOffset() const
{
    return QPoint((width()  - m_layerManager.width())  / 2,
                  (height() - m_layerManager.height()) / 2);
}

void CanvasView::setPenSize(int size)
{
    BrushSettings s = m_strokeEngine.brushSettings();
    s.size = qMax(1, size);
    m_strokeEngine.setBrushSettings(s);
}

void CanvasView::setPenColor(const QColor& color)
{
    BrushSettings s = m_strokeEngine.brushSettings();
    s.color = color;
    m_strokeEngine.setBrushSettings(s);
}

void CanvasView::setEraserMode(bool enabled)
{
    m_eraserMode = enabled;
}

void CanvasView::setTool(Tool tool)
{
    if (m_tool == Tool::Transform && tool != Tool::Transform && m_transformer.isActive()) {
        if (auto cmd = m_transformer.commit(m_selection))
            m_commandManager.push(std::move(cmd));
        emit m_layerManager.layersChanged();
    }

    m_tool = tool;
    m_eraserMode = (tool == Tool::Eraser);

    if (tool == Tool::Transform && !m_transformer.isActive()) {
        auto layer = m_layerManager.activeLayer();
        if (layer) {
            m_transformer.begin(layer.get(), m_selection,
                                m_layerManager.width(), m_layerManager.height());
        }
    }

    emit toolChanged(tool);
    update();
}

void CanvasView::clearSelection()
{
    m_selection.clear();
    update();
}

void CanvasView::setBrushHardness(float h)
{
    BrushSettings s = m_strokeEngine.brushSettings();
    s.hardness = qBound(0.0f, h, 1.0f);
    m_strokeEngine.setBrushSettings(s);
}

void CanvasView::setBrushAlpha(float a)
{
    BrushSettings s = m_strokeEngine.brushSettings();
    s.alpha = qBound(0.0f, a, 1.0f);
    m_strokeEngine.setBrushSettings(s);
}

void CanvasView::setEraserSize(int size)
{
    EraserSettings s = m_strokeEngine.eraserSettings();
    s.size = qMax(1, size);
    m_strokeEngine.setEraserSettings(s);
}

void CanvasView::setEraserHardness(float h)
{
    EraserSettings s = m_strokeEngine.eraserSettings();
    s.hardness = qBound(0.0f, h, 1.0f);
    m_strokeEngine.setEraserSettings(s);
}


void CanvasView::keyPressEvent(QKeyEvent* event)
{
    if (!m_transformer.isActive()) {
        switch (event->key()) {
        case Qt::Key_B: setTool(Tool::Brush);       break;
        case Qt::Key_E: setTool(Tool::Eraser);      break;
        case Qt::Key_M: setTool(Tool::RectSelect);  break;
        case Qt::Key_L: setTool(Tool::LassoSelect); break;
        case Qt::Key_I: setTool(Tool::Eyedropper);  break;
        default: break;
        }
    }

    if (event->key() == Qt::Key_Escape) {
        if (m_transformer.isActive()) cancelTransform();
        else clearSelection();
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        if (m_transformer.isActive()) commitTransform();

    QWidget::keyPressEvent(event);
}

void CanvasView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_transformer.isActive()) {
        commitTransform();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CanvasView::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    CanvasPainter::ActiveSelectionTool activeTool;
    activeTool.rectSelecting = m_selecting;
    activeTool.rect          = m_selectRect;
    activeTool.lassoing      = m_lassoing;
    activeTool.lassoPoints   = m_lassoPoints;

    CanvasPainter::paint(painter, canvasOffset(), m_layerManager.width(), m_layerManager.height(),
                         m_layerManager, m_selection, m_transformer,
                         activeTool, m_marchOffset);
}

void CanvasView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    QPoint pos = event->pos() - canvasOffset();

    if (m_tool == Tool::Eyedropper) {
        if (QRect(0, 0, m_layerManager.width(), m_layerManager.height()).contains(pos)) {
            QColor color = pickColorAt(pos);
            emit colorPicked(color);
        }
        return;
    }

    if (m_tool == Tool::RectSelect) {
        m_selecting   = true;
        m_selectStart = pos;
        m_selectRect  = QRect(pos, pos);
        return;
    }

    if (m_tool == Tool::LassoSelect) {
        m_lassoing = true;
        m_lassoPoints.clear();
        m_lassoPoints << pos;
        return;
    }

    if (m_tool == Tool::Transform && m_transformer.isActive()) {
        QPointF posF = event->pos() - canvasOffset();
        m_transformer.beginDrag(posF);
        return;
    }

    if (m_tool == Tool::Fill) {
        if (QRect(0, 0, m_layerManager.width(),
                  m_layerManager.height()).contains(pos)) {
            applyFill(pos);
            update();
        }
        return;
    }

    if (m_tool == Tool::MagicWand) {
        if (QRect(0, 0, m_layerManager.width(),
                  m_layerManager.height()).contains(pos)) {
            applyMagicWand(pos, m_selectionMode);
            update();
        }
        return;
    }

    m_lastPoint = pos;
    m_strokeEngine.beginStroke(m_layerManager.activeLayer().get(), pos, m_tool == Tool::Eraser);
    m_strokeEngine.continueStroke(pos, pos, &m_selection);
    update();
}

void CanvasView::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pos = event->pos() - canvasOffset();

    if (m_tool == Tool::Eyedropper) {
        QColor c = pickColorAt(pos);
        emit hoverColorChanged(c);
    }

    if (m_tool == Tool::Transform && m_transformer.isActive() &&
        (event->buttons() & Qt::LeftButton))
    {
        QPointF posF = event->pos() - canvasOffset();
        m_transformer.updateDrag(posF);
        update();
        return;
    }

    if (m_selecting && (event->buttons() & Qt::LeftButton)) {
        int x1 = m_selectStart.x();
        int y1 = m_selectStart.y();
        int x2 = pos.x();
        int y2 = pos.y();

        if (event->modifiers() & Qt::ShiftModifier) {
            const int dx = x2 - x1;
            const int dy = y2 - y1;
            const int side = qMax(qAbs(dx), qAbs(dy));
            x2 = x1 + (dx >= 0 ? side : -side);
            y2 = y1 + (dy >= 0 ? side : -side);
        }

        m_selectRect = QRect(QPoint(qMin(x1, x2), qMin(y1, y2)),
                             QPoint(qMax(x1, x2), qMax(y1, y2)));
        update();
        return;
    }

    if (m_lassoing && (event->buttons() & Qt::LeftButton)) {
        m_lassoPoints << pos;
        update();
        return;
    }

    if (!m_strokeEngine.isActive() || !(event->buttons() & Qt::LeftButton)) return;
    m_strokeEngine.continueStroke(m_lastPoint, pos, &m_selection);

    m_lastPoint = pos;
    update();
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_tool == Tool::Transform && m_transformer.isActive()) {
        m_transformer.endDrag();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_selecting) {
        m_selecting = false;
        if (!m_selectRect.isEmpty()) {
            m_selection.setRect(m_selectRect, m_selectionMode);
        }
        update();
        return;
    }

    if (m_lassoing) {
        m_lassoing = false;
        if (m_lassoPoints.size() > 2) {
            m_lassoPoints << m_lassoPoints.first();
            m_selection.setLasso(m_lassoPoints, m_selectionMode);
        }
        update();
        return;
    }

    if (m_strokeEngine.isActive()) {
        if (auto cmd = m_strokeEngine.endStroke())
            m_commandManager.push(std::move(cmd));
        update();
    }
}

void CanvasView::clearActiveLayer()
{
    m_layerManager.clearLayer(m_layerManager.activeIndex());
    update();
}

void CanvasView::enterTransformMode()
{
    setTool(Tool::Transform);
}

void CanvasView::commitTransform()
{
    if (!m_transformer.isActive()) return;

    if (auto cmd = m_transformer.commit(m_selection))
        m_commandManager.push(std::move(cmd));

    setTool(Tool::Brush);
    emit m_layerManager.layersChanged();
    update();
}

void CanvasView::cancelTransform()
{
    if (!m_transformer.isActive()) return;
    m_transformer.cancel();

    setTool(Tool::Brush);
    emit m_layerManager.layersChanged();
    update();
}


QColor CanvasView::pickColorAt(const QPoint& pos) const
{
    if (pos.x() < 0 || pos.x() >= m_layerManager.width() ||
        pos.y() < 0 || pos.y() >= m_layerManager.height())
        return Qt::black;

    QImage composite = m_layerManager.compositeImage();
    if (composite.isNull()) return Qt::black;

    QImage converted = composite.convertToFormat(QImage::Format_ARGB32);
    QColor c = converted.pixelColor(pos);

    if (c.alpha() < 255) {
        QColor white(255, 255, 255);
        int r = (c.red()   * c.alpha() + white.red()   * (255 - c.alpha())) / 255;
        int g = (c.green() * c.alpha() + white.green() * (255 - c.alpha())) / 255;
        int b = (c.blue()  * c.alpha() + white.blue()  * (255 - c.alpha())) / 255;
        return QColor(r, g, b);
    }

    return c;
}

void CanvasView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage())
        event->acceptProposedAction();
}

void CanvasView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    if (event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    }
}

void CanvasView::dropEvent(QDropEvent* event)
{
    const QMimeData* mime = event->mimeData();

    if (mime->hasUrls()) {
        for (const QUrl& url : mime->urls()) {
            if (!url.isLocalFile()) continue;

            QImage img(url.toLocalFile());
            if (img.isNull()) continue;

            QFileInfo fi(url.toLocalFile());
            m_layerManager.addImageLayer(img, fi.completeBaseName());
        }

        emit m_layerManager.layersChanged();
        update();
        event->acceptProposedAction();
        return;
    }

    if (mime->hasImage()) {
        QImage img = qvariant_cast<QImage>(mime->imageData());
        if (!img.isNull()) {
            m_layerManager.addImageLayer(img, "Wklejony obraz");
            emit m_layerManager.layersChanged();
            update();
            event->acceptProposedAction();
        }
    }
}

void CanvasView::showContextMenu(const QPoint& pos)
{
    QMenu menu(this);

    QPoint canvasPos = pos - canvasOffset();
    const bool overCanvas = QRect(0, 0,
                                  m_layerManager.width(),
                                  m_layerManager.height()).contains(canvasPos);

    QAction* pasteA = menu.addAction(tr("Wklej"));
    QAction* copyA  = menu.addAction(tr("Kopiuj warstwę"));

    menu.addSeparator();

    QAction* clearA     = menu.addAction(tr("Wyczyść warstwę"));
    QAction* transformA = menu.addAction(tr("Transformuj"));
    QAction* deselectA  = menu.addAction(tr("Odznacz"));

    menu.addSeparator();

    QAction* saveA = menu.addAction(tr("Zapisz jako..."));

    copyA->setEnabled(overCanvas);
    clearA->setEnabled(overCanvas);
    transformA->setEnabled(overCanvas);
    deselectA->setEnabled(!m_selection.isEmpty());

    QAction* chosen = menu.exec(mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == pasteA) {
        qDebug() << "Paste chosen";
        QClipboard* clipboard = QApplication::clipboard();
        const QMimeData* mime = clipboard->mimeData();
        qDebug() << "hasImage:" << mime->hasImage()
                 << "hasUrls:" << mime->hasUrls();

        QImage img;
        if (mime->hasImage()) {
            img = qvariant_cast<QImage>(mime->imageData());
        } else if (mime->hasUrls()) {
            for (const QUrl& url : mime->urls()) {
                if (url.isLocalFile()) {
                    QImage loaded(url.toLocalFile());
                    if (!loaded.isNull()) { img = loaded; break; }
                }
            }
        }

        if (img.isNull()) {
            QMessageBox::information(this, tr("Wklej"),
                                     tr("Schowek nie zawiera obrazu."));
            return;
        }

        m_layerManager.addImageLayer(img, tr("Wklejony obraz"));
        emit m_layerManager.layersChanged();
        update();
    }
    else if (chosen == copyA) {
        auto layer = m_layerManager.activeLayer();
        if (layer) {
            QApplication::clipboard()->setImage(layer->image());
        }
    }
    else if (chosen == clearA) {
        qDebug() << "Clear chosen";
        clearActiveLayer();
    }
    else if (chosen == transformA) {
        enterTransformMode();
    }
    else if (chosen == deselectA) {
        clearSelection();
    }
    else if (chosen == saveA) {
        QString filter = "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)";
        QString path = QFileDialog::getSaveFileName(this,
                                                    tr("Zapisz obraz"),
                                                    "obraz.png",
                                                    filter);
        if (path.isEmpty()) return;

        QImage composite = m_layerManager.compositeImage();

        if (path.endsWith(".jpg", Qt::CaseInsensitive) ||
            path.endsWith(".jpeg", Qt::CaseInsensitive))
        {
            QImage rgb(composite.size(), QImage::Format_RGB32);
            rgb.fill(Qt::white);
            QPainter p(&rgb);
            p.drawImage(0, 0, composite);
            p.end();
            composite = rgb;
        }

        if (!composite.save(path)) {
            QMessageBox::warning(this, tr("Błąd"),
                                 tr("Nie udało się zapisać pliku:\n") + path);
        }
    }
}

void CanvasView::applyFill(const QPoint& pos)
{
    auto layer = m_layerManager.activeLayer();
    if (!layer) return;

    QImage composite = m_layerManager.compositeImage();
    QImage before = layer->image().copy();

    QImage mask = FillTools::floodFillMask(composite, pos, m_fillSettings);
    if (mask.isNull()) return;

    QImage maskAlpha = FillTools::maskToAlphaArgb(mask);

    QImage colorLayer(before.size(), QImage::Format_ARGB32_Premultiplied);
    colorLayer.fill(Qt::transparent);
    {
        QPainter cp(&colorLayer);
        cp.setCompositionMode(QPainter::CompositionMode_Source);
        cp.fillRect(colorLayer.rect(), penColor());
        cp.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        cp.drawImage(0, 0, maskAlpha);
    }

    QImage filled = before;
    {
        QPainter p(&filled);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        if (!m_selection.isEmpty())
            p.setClipRegion(m_selection.region());

        p.drawImage(0, 0, colorLayer);
    }

    if (layer->alphaLocked()) {
        QImage result = before;
        QPainter rp(&result);
        rp.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        rp.drawImage(0, 0, filled);
        filled = result;
    }

    auto cmd = std::make_unique<DrawCommand>(
        &layer->image(), before, filled, tr("Wypełnienie"));
    m_commandManager.push(std::move(cmd));
}

void CanvasView::applyMagicWand(const QPoint& pos, SelectionMode mode)
{
    auto layer = m_layerManager.activeLayer();
    if (!layer) return;

    QImage mask = FillTools::floodFillMask(layer->image(), pos, m_wandSettings);
    if (mask.isNull()) return;

    QRegion region;
    for (int y = 0; y < mask.height(); ++y) {
        const uchar* line = mask.constScanLine(y);
        int x = 0;
        while (x < mask.width()) {
            if (line[x] >= 128) {
                int start = x;
                while (x < mask.width() && line[x] >= 128) ++x;
                region += QRect(start, y, x - start, 1);
            } else {
                ++x;
            }
        }
    }
    m_selection.setRegion(region, mode);
    update();
}
