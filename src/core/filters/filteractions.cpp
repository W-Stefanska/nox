#include "filteractions.h"
#include "../../ui/mainwindow.h"
#include "../../ui/canvasview.h"
#include "../layermanager.h"
#include "../layer.h"
#include "../commandmanager.h"
#include "../command.h"
#include "convolution.h"
#include "pointops.h"

#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QInputDialog>

FilterActions::FilterActions(MainWindow* mw, CanvasView* canvas, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mw)
    , m_canvas(canvas)
{}

void FilterActions::applyFilterToActiveLayer(
    const QString& name,
    std::function<QImage(const QImage&)> fn)
{
    auto* mgr   = m_canvas->layerManager();
    auto  layer = mgr->activeLayer();
    if (!layer) return;

    QImage before = layer->image();

    m_canvas->commandManager()->push(
        std::make_unique<FilterCommand>(
            &layer->image(),
            std::move(before),
            std::move(fn),
            name
            )
        );

    emit mgr->layersChanged();
    m_canvas->update();
}

void FilterActions::showFilterDialog(
    const QString& title,
    const QVector<FilterParam>& params,
    std::function<QImage(const QImage&, const QMap<QString, double>&)> applyFn)
{
    auto* mgr = m_canvas->layerManager();
    auto  layer = mgr->activeLayer();
    if (!layer) return;

    FilterDialog dlg(layer->image(), title, params, applyFn, m_mainWindow);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QImage before = layer->image();
    QImage result = dlg.resultImage();

    m_canvas->commandManager()->push(
        std::make_unique<FilterCommand>(
            &layer->image(),
            std::move(before),
            [result](const QImage&) { return result; },
            title
            )
        );

    emit mgr->layersChanged();
    m_canvas->update();
}

void FilterActions::showFilterPreview(
    const QString& title,
    std::function<QImage(const QImage&)> fn)
{
    showFilterDialog(title, {},
                     [fn](const QImage& img, const QMap<QString, double>&) {
                         return fn(img);
                     });
}

QMenu* FilterActions::buildMenu()
{
    auto* filtersMenu = new QMenu(tr("&Filtry"), m_mainWindow);

    auto* blurMenu = filtersMenu->addMenu(tr("Rozmycia"));

    QAction* gaussA = blurMenu->addAction(tr("Gauss…"));
    connect(gaussA, &QAction::triggered, this, [this]() {
        QVector<FilterParam> params = {
            { "radius", tr("Promień"),  FilterParamType::Int,    1,   100,  2,  1.0, 0 },
            { "sigma",  tr("Sigma"),    FilterParamType::Double, 0.1, 10,  1.0, 0.1, 2 }
        };
        showFilterDialog(tr("Rozmycie Gaussa"), params,
                         [](const QImage& img, const QMap<QString, double>& v) {
                             int   radius = int(v["radius"]);
                             float sigma  = float(v["sigma"]);
                             return applyConvolution(img, Kernels::gaussianBlur(radius, sigma));
                         });
    });

    QAction* boxA = blurMenu->addAction(tr("Box Blur…"));
    connect(boxA, &QAction::triggered, this, [this]() {
        QVector<FilterParam> params = {
            { "radius", tr("Promień"), FilterParamType::Int, 1, 20, 2, 1.0, 0 }
        };
        showFilterDialog(tr("Box Blur"), params,
                         [](const QImage& img, const QMap<QString, double>& v) {
                             return applyConvolution(img, Kernels::boxBlur(int(v["radius"])));
                         });
    });

    auto* sharpenMenu = filtersMenu->addMenu(tr("Wyostrzanie"));

    QAction* sharpA = sharpenMenu->addAction(tr("Wyostrzanie"));
    connect(sharpA, &QAction::triggered, this, [this]() {
        showFilterPreview(tr("Wyostrzanie"),
                          [](const QImage& img) {
                              return applyConvolution(img, Kernels::sharpen());
                          });
    });

    auto* edgeMenu = filtersMenu->addMenu(tr("Krawędzie"));

    QAction* lapA = edgeMenu->addAction(tr("Laplacian"));
    connect(lapA, &QAction::triggered, this, [this]() {
        showFilterPreview(tr("Laplacian"),
                          [](const QImage& img) {
                              return applyConvolution(img, Kernels::edgeDetect());
                          });
    });

    QAction* sobelA = edgeMenu->addAction(tr("Sobel"));
    connect(sobelA, &QAction::triggered, this, [this]() {
        showFilterPreview(tr("Sobel"),
                          [](const QImage& img) {
                              return applyConvolution(img, Kernels::sobelX());
                          });
    });

    QAction* embossA = edgeMenu->addAction(tr("Emboss"));
    connect(embossA, &QAction::triggered, this, [this]() {
        showFilterPreview(tr("Emboss"),
                          [](const QImage& img) {
                              return applyConvolution(img, Kernels::emboss());
                          });
    });

    filtersMenu->addSeparator();
    auto* pointMenu = filtersMenu->addMenu(tr("Punktowe"));

    QAction* brightA = pointMenu->addAction(tr("Jasność…"));
    connect(brightA, &QAction::triggered, this, [this]() {
        QVector<FilterParam> params = {
            { "delta", tr("Delta"), FilterParamType::Int, -255, 255, 0, 1.0, 0 }
        };
        showFilterDialog(tr("Jasność"), params,
                         [](const QImage& img, const QMap<QString, double>& v) {
                             return Pointops::adjustBrightness(img, int(v["delta"]));
                         });
    });

    QAction* contrastA = pointMenu->addAction(tr("Kontrast…"));
    connect(contrastA, &QAction::triggered, this, [this]() {
        QVector<FilterParam> params = {
            { "factor", tr("Współczynnik"), FilterParamType::Double, 0.1, 3.0, 1.0, 0.05, 2 }
        };
        showFilterDialog(tr("Kontrast"), params,
                         [](const QImage& img, const QMap<QString, double>& v) {
                             return Pointops::adjustContrast(img, float(v["factor"]));
                         });
    });

    QAction* gammaA = pointMenu->addAction(tr("Gamma…"));
    connect(gammaA, &QAction::triggered, this, [this]() {
        QVector<FilterParam> params = {
            { "gamma", tr("Gamma"), FilterParamType::Double, 0.1, 5.0, 1.0, 0.05, 2 }
        };
        showFilterDialog(tr("Gamma"), params,
                         [](const QImage& img, const QMap<QString, double>& v) {
                             return Pointops::gammaCorrect(img, float(v["gamma"]));
                         });
    });

    QAction* threshA = pointMenu->addAction(tr("Binaryzacja…"));
    connect(threshA, &QAction::triggered, this, [this]() {
        QVector<FilterParam> params = {
            { "level", tr("Próg"), FilterParamType::Int, 0, 255, 128, 1.0, 0 }
        };
        showFilterDialog(tr("Binaryzacja"), params,
                         [](const QImage& img, const QMap<QString, double>& v) {
                             return Pointops::threshold(img, int(v["level"]));
                         });
    });

    QAction* grayA = pointMenu->addAction(tr("Skala szarości"));
    connect(grayA, &QAction::triggered, this, [this]() {
        showFilterPreview(tr("Skala szarości"),
                          [](const QImage& img) {
                              return Pointops::toGrayscale(img);
                          });
    });

    QAction* invA = pointMenu->addAction(tr("Inwersja"));
    connect(invA, &QAction::triggered, this, [this]() {
        showFilterPreview(tr("Inwersja"),
                          [](const QImage& img) {
                              return Pointops::invert(img);
                          });
    });

    return filtersMenu;
}

