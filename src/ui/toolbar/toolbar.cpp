#include "toolbar.h"
#include "../canvasview.h"

#include <QAction>
#include <QActionGroup>
#include <QToolButton>

ToolBar::ToolBar(CanvasView* canvas, QWidget* parent)
    : QToolBar(tr("Narzędzia"), parent)
    , m_canvas(canvas)
{
    setOrientation(Qt::Horizontal);
    setMovable(true);
    setFloatable(true);
    setIconSize(QSize(24, 24));
    setToolButtonStyle(Qt::ToolButtonIconOnly);

    m_group = new QActionGroup(this);
    m_group->setExclusive(true);

    auto addTool = [this](const QIcon& icon, const QString& tip,
                          Tool tool, QAction*& out) {
        QAction* a = new QAction(icon, QString(), this);
        a->setToolTip(tip);
        a->setCheckable(true);
        m_group->addAction(a);
        addAction(a);

        connect(a, &QAction::triggered, this, [this, tool]() {
            m_canvas->setTool(tool);
        });
        out = a;
    };

    addTool(QIcon(":resources/pen.svg"), tr("Pędzel"),       Tool::Brush,       m_brushAction);
    addTool(QIcon(":resources/eraser.svg"), tr("Gumka"),        Tool::Eraser,      m_eraserAction);
    addTool(QIcon(":resources/picker.svg"), tr("Pipeta"),       Tool::Eyedropper,  m_eyedropperAction);
    addTool(QIcon(":resources/bucket.svg"), tr("Wypełnienie"),  Tool::Fill,       m_fillAction);

    addSeparator();

    addTool(QIcon(":resources/selection.svg"),  tr("Zaznaczenie"),      Tool::RectSelect,  m_rectSelectAction);
    addTool(QIcon(":resources/lasso.svg"), tr("Lasso"),            Tool::LassoSelect, m_lassoAction);
    addTool(QIcon(":resources/wand.svg"), tr("Magiczna różdżka"), Tool::MagicWand, m_magicWandAction);

    addSeparator();

    m_transformAction = new QAction(QIcon(":resources/transform.svg"), QString(), this);
    m_transformAction->setToolTip(tr("Transformuj"));
    m_transformAction->setCheckable(true);
    m_group->addAction(m_transformAction);
    addAction(m_transformAction);
    connect(m_transformAction, &QAction::triggered, this, [this]() {
        m_canvas->enterTransformMode();
    });

    addSeparator();

    addTool(QIcon(":/resources/filters.svg"), tr("Filtry"), Tool::Filters, m_filtersAction);

    m_brushAction->setChecked(true);

    connect(m_canvas, &CanvasView::toolChanged,
            this, &ToolBar::syncWithCanvas);

}

void ToolBar::syncWithCanvas(Tool tool)
{
    switch (tool) {
    case Tool::Brush:       m_brushAction->setChecked(true);       break;
    case Tool::Eraser:      m_eraserAction->setChecked(true);      break;
    case Tool::Eyedropper:  m_eyedropperAction->setChecked(true);  break;
    case Tool::RectSelect:  m_rectSelectAction->setChecked(true);  break;
    case Tool::LassoSelect: m_lassoAction->setChecked(true);       break;
    case Tool::Transform:   m_transformAction->setChecked(true);   break;
    case Tool::Filters:     m_filtersAction->setChecked(true);     break;
    case Tool::Fill:        m_fillAction->setChecked(true);        break;
    case Tool::MagicWand:   m_magicWandAction->setChecked(true);   break;
    default: break;
    }
}
