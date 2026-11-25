#include "mainwindow.h"
#include "dialogs/newprojectdialog.h"
#include "ui_mainwindow.h"
#include "canvasview.h"
#include "toolbar/toolbarbuilder.h"
#include "menubar/menubuilder.h"
#include "layers/layerspanel.h"
#include "toolbar/toolspanel.h"
#include "toolbar/toolbar.h"
#include "../core/filters/filteractions.h"
#include "histogram/histogramwidget.h"
#include "../core/filters/filteractions.h"
#include "../core/projectfile.h"

#include <QWidget>
#include <QToolBar>
#include <QDockWidget>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QKeySequence>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSettings>
#include <QCloseEvent>
#include <QImageReader>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_canvas = new CanvasView(1280, 720, this);
    setMinimumSize(800, 500);
    setCentralWidget(m_canvas);

    connect(m_canvas->layerManager(), &LayerManager::layersChanged,
            this, &MainWindow::onCanvasModified);
    connect(m_canvas->commandManager(), &CommandManager::stackChanged,
            this, &MainWindow::onCanvasModified);

    createFileActions();
    createEditActions();

    // Narzedzia
    auto* mainToolBar = addToolBar("");
    mainToolBar->setMovable(false);
    mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mainToolBar->addAction(m_newAction);
    mainToolBar->addAction(m_openAction);
    mainToolBar->addAction(m_saveAction);
    mainToolBar->addAction(m_saveAsAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_exportImageAction);
    mainToolBar->addAction(m_exportLayerAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_importImageAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_undoAction);
    mainToolBar->addAction(m_redoAction);

    auto* toolBar = new ToolBar(m_canvas, this);
    toolBar->setMovable(false);
    addToolBar(toolBar);

    auto* toolsDock = new QDockWidget("Ustawienia narzędzi", this);
    auto* m_filterActions = new FilterActions(this, m_canvas, toolBar);
    toolsDock->setWidget(new ToolsPanel(m_canvas, m_filterActions, toolsDock));
    toolsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, toolsDock);

    // Warstwy
    auto* layersDock = new QDockWidget("Layers", this);
    m_layersPanel = new LayersPanel(m_canvas->layerManager(), layersDock);
    layersDock->setWidget(m_layersPanel);
    addDockWidget(Qt::RightDockWidgetArea, layersDock);
    resizeDocks({layersDock}, {200}, Qt::Horizontal);

    // Akcje edycji
    // auto* editToolbar = addToolBar(tr("Edycja"));
    // editToolbar->addAction(m_undoAction);
    // editToolbar->addAction(m_redoAction);
    connect(m_canvas->commandManager(), &CommandManager::stackChanged,
            this, &MainWindow::updateUndoRedoActions);
    connect(m_canvas->commandManager(), &CommandManager::stackChanged,
            m_canvas, qOverload<>(&QWidget::update));

    // FilterActions
    // m_filterActions = new FilterActions(this, m_canvas, this);

    // Histogram
    auto* histDock = new QDockWidget("Histogram", this);
    m_histogramWidget = new HistogramWidget(histDock);
    histDock->setWidget(m_histogramWidget);
    addDockWidget(Qt::RightDockWidgetArea, histDock);

    // Connecty
    connect(m_canvas->layerManager(), &LayerManager::layersChanged,
            this, [this]() {
                m_layersPanel->refresh();
                m_canvas->update();
                m_histogramWidget->refresh();
            });

    connect(m_canvas->commandManager(), &CommandManager::stackChanged,
            this, [this]() {
                m_layersPanel->refreshThumbnails();
                m_histogramWidget->refresh();
            });

    connect(m_canvas, &CanvasView::colorPicked, [this](const QColor& color) {
                m_canvas->setPenColor(color);

            });

    connect(m_canvas->layerManager(), &LayerManager::layersChanged,
            this, [this]() {
                if (m_histogramWidget) {
                    auto layer = m_canvas->layerManager()->activeLayer();
                    if (layer) m_histogramWidget->setImage(layer->image());

                    m_histogramWidget->refresh();
                }
            });
    connect(m_canvas->commandManager(), &CommandManager::stackChanged,
            this, [this]() {
                if (m_histogramWidget) {
                    auto layer = m_canvas->layerManager()->activeLayer();
                    if (layer) m_histogramWidget->setImage(layer->image());

                    m_histogramWidget->refresh();
                }
            });
    connect(m_canvas->layerManager(), &LayerManager::activeLayerChanged,
            this, [this](int) {
                if (m_histogramWidget) {
                    auto layer = m_canvas->layerManager()->activeLayer();
                    if (layer) m_histogramWidget->setImage(layer->image());
                    m_histogramWidget->refresh();
                }
            });
    connect(m_canvas->commandManager(), &CommandManager::stackChanged,
            m_canvas, qOverload<>(&QWidget::update));

    MenuBuilder::createMenu(this);
    updateUndoRedoActions();

    setCurrentProject(QString(), tr("Bez tytułu"));
    markModified(false);
    updateWindowTitle();

    statusBar()->showMessage("Gotowy");

}

MainWindow::~MainWindow()
{
    delete ui;
}

CanvasView* MainWindow::canvas() const
{
    return m_canvas;
}

void MainWindow::createEditActions()
{
    m_undoAction = new QAction(tr("&Cofnij"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setIcon(QIcon(":resources/undo.svg"));
    m_undoAction->setStatusTip(tr("Cofa ostatnią operację"));
    connect(m_undoAction, &QAction::triggered,
            this, &MainWindow::onUndoTriggered);

    m_redoAction = new QAction(tr("&Ponów"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setIcon(QIcon(":resources/redo.svg"));
    m_redoAction->setStatusTip(tr("Ponawia ostatnią operację"));
    connect(m_redoAction, &QAction::triggered,
            this, &MainWindow::onRedoTriggered);

    addAction(m_undoAction);
    addAction(m_redoAction);
}

void MainWindow::createFileActions()
{
    m_newAction = new QAction(QIcon(), "&Nowy projekt", this);
    m_newAction->setShortcut(QKeySequence("Ctrl+N"));
    m_newAction->setStatusTip(tr("Utwórz nowy projekt"));
    connect(m_newAction, &QAction::triggered, this, &MainWindow::onNewProject);

    m_openAction = new QAction(QIcon(), "&Otwórz projekt...", this);
    m_openAction->setShortcut(QKeySequence("Ctrl+O"));
    m_openAction->setStatusTip(tr("Otwórz istniejący projekt"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenProject);

    m_saveAction = new QAction(QIcon(), "&Zapisz", this);
    m_saveAction->setShortcut(QKeySequence("Ctrl+S"));
    m_saveAction->setStatusTip(tr("Zapisz projekt"));
    connect(m_saveAction, &QAction::triggered, this, [this]() {
        onSaveProject();
    });

    m_saveAsAction = new QAction(QIcon(), "Zapisz &jako...", this);
    m_saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    m_saveAsAction->setStatusTip(tr("Zapisz projekt pod nową nazwą"));
    connect(m_saveAsAction, &QAction::triggered, this, [this]() {
        onSaveProjectAs();
    });

    m_exportImageAction = new QAction(QIcon(), "&Eksportuj obraz...", this);
    m_exportImageAction->setShortcut(QKeySequence("Ctrl+E"));
    m_exportImageAction->setStatusTip(tr("Eksportuj złożony obraz do PNG/JPG/BMP/TIFF"));
    connect(m_exportImageAction, &QAction::triggered, this, [this]() {
        onExportImage();
    });

    m_exportLayerAction = new QAction(QIcon(), "&Eksportuj warstwę...", this);
    m_exportLayerAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    m_exportLayerAction->setStatusTip(tr("Eksportuj aktywną warstwę do pliku"));
    connect(m_exportLayerAction, &QAction::triggered, this, [this]() {
        auto* mgr = canvas()->layerManager();
        auto layer = mgr->activeLayer();
        if (!layer) {
            QMessageBox::warning(this, "Błąd", "Brak aktywnej warstwy.");
            return;
        }
        QString filter = "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)";
        QString path = QFileDialog::getSaveFileName(
            this, "Eksportuj warstwę", layer->name() + ".png", filter);
        if (path.isEmpty()) return;

        QImage img = layer->image();
        if (path.endsWith(".jpg", Qt::CaseInsensitive) ||
            path.endsWith(".jpeg", Qt::CaseInsensitive)) {
            QImage rgb(img.size(), QImage::Format_RGB32);
            rgb.fill(Qt::white);
            QPainter p(&rgb);
            p.drawImage(0, 0, img);
            p.end();
            img = rgb;
        }
        if (!img.save(path))
            QMessageBox::warning(this, "Błąd",
                                 "Nie udało się zapisać pliku:\n" + path);
        else
            statusBar()->showMessage("Zapisano warstwę: " + path, 3000);
    });

    m_importImageAction = new QAction(QIcon(), "&Importuj obraz...", this);
    m_importImageAction->setStatusTip(tr("Wczytaj obraz jako nową warstwę"));
    connect(m_importImageAction, &QAction::triggered, this, [this]() {
        QString filter = "Obrazy (*.png *.jpg *.jpeg *.bmp *.gif *.tiff);;"
                         "Wszystkie pliki (*)";
        QString path = QFileDialog::getOpenFileName(
            this, "Importuj obraz", QString(), filter);
        if (path.isEmpty()) return;

        QImage img(path);
        if (img.isNull()) {
            QMessageBox::warning(this, "Błąd",
                                 "Nie udało się wczytać pliku:\n" + path);
            return;
        }
        auto* mgr = canvas()->layerManager();
        QFileInfo fi(path);
        mgr->addImageLayer(img, fi.completeBaseName());
    });
    addAction(m_importImageAction);
}

void MainWindow::updateUndoRedoActions()
{
    auto* cm = m_canvas->commandManager();

    const bool canUndo = cm->canUndo();
    const bool canRedo = cm->canRedo();

    m_undoAction->setEnabled(canUndo);
    m_redoAction->setEnabled(canRedo);

    const QString uName = cm->undoName();
    const QString rName = cm->redoName();

    m_undoAction->setText(uName.isEmpty()
                              ? tr("&Cofnij")
                              : tr("&Cofnij: %1").arg(uName));

    m_redoAction->setText(rName.isEmpty()
                              ? tr("&Ponów")
                              : tr("&Ponów: %1").arg(rName));
}

void MainWindow::onUndoTriggered()
{
    m_canvas->commandManager()->undo();
}

void MainWindow::onRedoTriggered()
{
    m_canvas->commandManager()->redo();
}

void MainWindow::onNewProject()
{
    if (!maybeSave()) return;

    NewProjectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_suppressModified = true;
    m_canvas->resetCanvas(dlg.canvasWidth(), dlg.canvasHeight());
    m_suppressModified = false;

    setCurrentProject(QString(), dlg.projectName());
    markModified(false);
    updateWindowTitle();
}

void MainWindow::onOpenProject()
{
    if (!maybeSave()) return;

    QSettings settings;
    const QString startDir = settings.value("lastProjectDir",
                                            QDir::homePath()).toString();

    QString path = QFileDialog::getOpenFileName(
        this, tr("Otwórz projekt"),
        startDir,
        ProjectFile::kFileFilter);
    if (path.isEmpty()) return;

    if (!ProjectFile::load(path, *m_canvas->layerManager())) {
        QMessageBox::warning(this, tr("Błąd"),
                             tr("Nie udało się wczytać projektu:\n") + path);
        return;
    }

    settings.setValue("lastProjectDir", QFileInfo(path).absolutePath());

    setCurrentProject(path, QFileInfo(path).completeBaseName());
    markModified(false);
    statusBar()->showMessage(tr("Wczytano: ") + path, 3000);
}

void MainWindow::onCanvasModified()
{
    if (m_suppressModified) return;
    if (m_modified) return;
    markModified(true);
}

void MainWindow::setCurrentProject(const QString& path, const QString& name)
{
    m_projectPath = path;
    m_projectName = name.isEmpty() ? tr("Bez tytułu") : name;
    updateWindowTitle();
}

void MainWindow::markModified(bool modified)
{
    if (m_modified == modified) return;
    m_modified = modified;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr("%1[*] — %2")
                       .arg(m_projectName,
                            QCoreApplication::applicationName()));
    setWindowModified(m_modified);
}

bool MainWindow::onSaveProject()
{
    if (m_projectPath.isEmpty())
        return onSaveProjectAs();

    if (!saveProjectTo(m_projectPath))
        return false;

    markModified(false);
    statusBar()->showMessage(tr("Zapisano: ") + m_projectPath, 4000);
    return true;
}

bool MainWindow::onSaveProjectAs()
{
    QSettings settings;
    const QString startDir = m_projectPath.isEmpty()
                                 ? settings.value("lastProjectDir", QDir::homePath()).toString()
                                 : QFileInfo(m_projectPath).absolutePath();

    const QString proposed = QDir(startDir).filePath(m_projectName + ".nox");

    QString path = QFileDialog::getSaveFileName(
        this,
        tr("Zapisz projekt jako"),
        proposed,
        ProjectFile::kFileFilter
        );

    if (path.isEmpty()) return false;

    if (!path.endsWith(".nox", Qt::CaseInsensitive))
        path += ".nox";

    if (!saveProjectTo(path))
        return false;

    settings.setValue("lastProjectDir", QFileInfo(path).absolutePath());
    markModified(false);
    statusBar()->showMessage(tr("Zapisano: ") + path, 3000);
    return true;
}

bool MainWindow::onExportImage()
{
    QSettings settings;
    const QString startDir = m_projectPath.isEmpty()
                                 ? settings.value("lastProjectDir", QDir::homePath()).toString()
                                 : QFileInfo(m_projectPath).absolutePath();

    const QString proposed = QDir(startDir).filePath(m_projectName + ".png");

    const QString filter =
        "PNG (*.png);;"
        "JPEG (*.jpg *.jpeg);;"
        "BMP (*.bmp);;"
        "TIFF (*.tif *.tiff)";

    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(
        this,
        tr("Eksportuj obraz"),
        proposed,
        filter,
        &selectedFilter);

    if (path.isEmpty()) return false;

    QFileInfo fi(path);
    QString suffix = fi.suffix().toLower();

    if (suffix.isEmpty()) {
        if      (selectedFilter.contains("*.png"))  suffix = "png";
        else if (selectedFilter.contains("*.jpg"))  suffix = "jpg";
        else if (selectedFilter.contains("*.jpeg")) suffix = "jpeg";
        else if (selectedFilter.contains("*.bmp"))  suffix = "bmp";
        else if (selectedFilter.contains("*.tif"))  suffix = "tif";
        else                                        suffix = "png";

        path += "." + suffix;
    }

    if (!exportImageTo(path))
        return false;

    settings.setValue("lastProjectDir", QFileInfo(path).absolutePath());
    statusBar()->showMessage(tr("Wyeksportowano: ") + path, 3000);
    return true;
}

bool MainWindow::exportImageTo(const QString& path)
{
    QImage composite = m_canvas->layerManager()->compositeImage();

    if (composite.isNull()) {
        QMessageBox::warning(this, tr("Błąd"),
                             tr("Brak obrazu do wyeksportowania."));
        return false;
    }

    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == "jpg" || suffix == "jpeg" || suffix == "bmp") {
        QImage rgb(composite.size(), QImage::Format_RGB32);
        rgb.fill(Qt::white);
        QPainter p(&rgb);
        p.drawImage(0, 0, composite);
        p.end();
        composite = rgb;
    }

    if (!composite.save(path, nullptr, -1)) {
        QMessageBox::warning(this, tr("Błąd"),
                             tr("Nie udało się zapisać obrazu:\n") + path);
        return false;
    }
    return true;
}

bool MainWindow::saveProjectTo(const QString& path)
{
    if (!ProjectFile::save(path, *m_canvas->layerManager())) {
        QMessageBox::warning(this, tr("Błąd"),
                             tr("Nie udało się zapisać projektu:\n") + path);
        return false;
    }
    setCurrentProject(path, QFileInfo(path).completeBaseName());
    return true;
}

bool MainWindow::maybeSave()
{
    if (!m_modified) return true;

    auto ret = QMessageBox::warning(
        this, tr("Niezapisane zmiany"),
        tr("Projekt \"%1\" ma niezapisane zmiany.\nZapisać przed kontynuacją?")
            .arg(m_projectName),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)   return onSaveProject();
    if (ret == QMessageBox::Cancel) return false;
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (maybeSave()) event->accept();
    else             event->ignore();
}
