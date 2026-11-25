#pragma once

#include <QToolBar>

class MainWindow;
class CanvasView;

class ToolBarBuilder
{
public:
    static QToolBar* build(MainWindow* mainWindow, CanvasView* canvas);
};
