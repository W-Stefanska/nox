#pragma once

#include <QObject>
#include <QImage>
#include <QMap>
#include <functional>
#include "../../ui/dialogs/filterdialog.h"

class MainWindow;
class CanvasView;
class QMenu;
class QToolBar;

class FilterActions : public QObject
{
    Q_OBJECT
public:
    explicit FilterActions(MainWindow* mw, CanvasView* canvas, QObject* parent = nullptr);

    QMenu*    buildMenu();

    void applyFilterToActiveLayer(const QString& name,
                                  std::function<QImage(const QImage&)> fn);
    void showFilterDialog(const QString& title,
                          const QVector<struct FilterParam>& params,
                          std::function<QImage(const QImage&, const QMap<QString, double>&)> applyFn);
    void showFilterPreview(const QString& title,
                           std::function<QImage(const QImage&)> fn);

private:
    MainWindow* m_mainWindow = nullptr;
    CanvasView* m_canvas     = nullptr;

};
