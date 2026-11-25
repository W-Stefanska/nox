#pragma once

class CanvasView;
class FilterActions;
class HistogramWidget;
class FilterActions;
class ColorPicker;
#include <QMainWindow>
#include "layers/layerspanel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    CanvasView*     canvas()        const;
    FilterActions*  filterActions() const { return m_filterActions; }
    QAction*        undoAction()    const { return m_undoAction; }
    QAction*        redoAction()    const { return m_redoAction; }

    QAction* newAction()        const { return m_newAction; }
    QAction* saveAction()       const { return m_saveAction; }
    QAction* saveAsAction()     const { return m_saveAsAction; }
    QAction* exportLayerAction() const { return m_exportLayerAction; }
    QAction* openAction()       const { return m_openAction; }
    QAction* exportImageAction() const { return m_exportImageAction; }

private slots:
    void onUndoTriggered();
    void onRedoTriggered();
    void updateUndoRedoActions();
    void onNewProject();
    void onOpenProject();
    void onCanvasModified();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createEditActions();
    void createFileActions();

    void setupProjectActions();
    void setCurrentProject(const QString& path, const QString& name);
    void markModified(bool modified);
    bool maybeSave();
    void updateWindowTitle();

    bool saveProjectTo(const QString& path);
    bool onSaveProject();
    bool onSaveProjectAs();
    bool onExportImage();
    bool exportImageTo(const QString& path);

    QAction* m_exportImageAction = nullptr;
    QString m_projectPath;
    QString m_projectName = tr("Bez tytułu");
    bool    m_modified = false;
    bool m_suppressModified = false;

    Ui::MainWindow*  ui;
    CanvasView*      m_canvas = nullptr;
    LayersPanel*     m_layersPanel = nullptr;
    FilterActions*   m_filterActions   = nullptr;
    HistogramWidget* m_histogramWidget = nullptr;

    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;

    QAction* m_newAction        = nullptr;
    QAction* m_saveAction       = nullptr;
    QAction* m_saveAsAction     = nullptr;
    QAction* m_exportLayerAction= nullptr;
    QAction* m_openAction       = nullptr;
    QAction* m_importImageAction= nullptr;
};
