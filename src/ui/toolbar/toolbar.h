#pragma once

#include <QToolBar>
#include "../canvasview.h"

class QActionGroup;

class ToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit ToolBar(CanvasView* canvas,
                     QWidget* parent = nullptr);

private slots:
    void syncWithCanvas(Tool tool);

private:
    CanvasView* m_canvas = nullptr;

    QAction* m_brushAction       = nullptr;
    QAction* m_eraserAction      = nullptr;
    QAction* m_eyedropperAction  = nullptr;
    QAction* m_rectSelectAction  = nullptr;
    QAction* m_lassoAction       = nullptr;
    QAction* m_transformAction   = nullptr;
    QAction* m_filtersAction     = nullptr;
    QAction* m_fillAction       = nullptr;
    QAction* m_magicWandAction  = nullptr;

    QActionGroup* m_group = nullptr;
};
