#include "toolbarbuilder.h"

#include <QAction>
#include <QActionGroup>
#include <QSlider>
#include <QColorDialog>
#include <QFileDialog>

#include "../canvasview.h"
#include "../mainwindow.h"

QToolBar* ToolBarBuilder::build(MainWindow* mainWindow, CanvasView* canvas)
{
    QToolBar* toolBar = new QToolBar(mainWindow);
    toolBar->setOrientation(Qt::Vertical);

    // ===== Narzędzia =====
    QAction* brushAction = new QAction("Pędzel", toolBar);
    QAction* eraserAction = new QAction("Gumka", toolBar);

    brushAction->setCheckable(true);
    eraserAction->setCheckable(true);

    QActionGroup* toolGroup = new QActionGroup(toolBar);
    toolGroup->addAction(brushAction);
    toolGroup->addAction(eraserAction);
    brushAction->setChecked(true);

    toolBar->addAction(brushAction);
    toolBar->addAction(eraserAction);

    QObject::connect(brushAction, &QAction::triggered, [canvas]() {
        canvas->setEraserMode(false);
    });

    QObject::connect(eraserAction, &QAction::triggered, [canvas]() {
        canvas->setEraserMode(true);
    });

    toolBar->addSeparator();

    // ===== Rozmiar pędzla =====
    QSlider* sizeSlider = new QSlider(Qt::Horizontal, toolBar);
    sizeSlider->setRange(1, 200);
    sizeSlider->setValue(canvas->penSize());
    sizeSlider->setFixedWidth(100);

    QObject::connect(sizeSlider, &QSlider::valueChanged,
                     canvas, &CanvasView::setPenSize);

    toolBar->addWidget(sizeSlider);

    // ===== Kolor =====
    QAction* colorAction = new QAction("Kolor", toolBar);
    toolBar->addAction(colorAction);

    QObject::connect(colorAction, &QAction::triggered, [canvas]() {
        QColor color = QColorDialog::getColor(Qt::white);
        if (color.isValid())
            canvas->setPenColor(color);
    });

    return toolBar;
}
