#pragma once

#include <QWidget>
#include <QStackedWidget>
#include "../canvasview.h"

class QToolButton;
class FilterActions;
class ColorPicker;

class ToolsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsPanel(CanvasView* canvas,
                        FilterActions* filterActions,
                        QWidget* parent = nullptr);

private slots:
    void onToolChanged(Tool tool);

private:
    QWidget* createBrushOptions();
    QWidget* createEraserOptions();
    QWidget* createEyedropperOptions();
    QWidget* createRectSelectOptions();
    QWidget* createLassoOptions();
    QWidget* createTransformOptions();
    QWidget* createFiltersOptions();
    QWidget* createFillOptions();
    QWidget* createMagicWandOptions();

    void applyStyle();

    CanvasView*     m_canvas = nullptr;
    FilterActions*  m_filterActions = nullptr;
    ColorPicker* m_colorPicker = nullptr;

    QStackedWidget* m_stack = nullptr;
};
