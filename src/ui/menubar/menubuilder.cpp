#include "menubuilder.h"
#include "../canvasview.h"
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QUrl>

void MenuBuilder::createMenu(MainWindow* mainWindow) {
    QMenuBar* menuBar = mainWindow->menuBar();

    QMenu* fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction(mainWindow->newAction());
    fileMenu->addAction(mainWindow->openAction());
    fileMenu->addSeparator();
    fileMenu->addAction(mainWindow->saveAction());
    fileMenu->addAction(mainWindow->saveAsAction());
    fileMenu->addSeparator();
    fileMenu->addAction(mainWindow->exportImageAction());
    fileMenu->addAction(mainWindow->exportLayerAction());
    fileMenu->addSeparator();
    fileMenu->addSeparator();

    QAction* exitAction = new QAction("&Exit", mainWindow);
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    QObject::connect(exitAction, &QAction::triggered, []() { qDebug() << "Exit clicked"; });
    fileMenu->addAction(exitAction);

    // -------------------- Edit --------------------
    QMenu* editMenu = menuBar->addMenu("&Edit");

    editMenu->addAction(mainWindow->undoAction());
    editMenu->addAction(mainWindow->redoAction());

    editMenu->addSeparator();

    QAction* copyAction = new QAction("&Copy", mainWindow);
    copyAction->setShortcut(QKeySequence("Ctrl+C"));
    QObject::connect(copyAction, &QAction::triggered, []() {
        qDebug() << "Copy clicked";
    });
    editMenu->addAction(copyAction);

    QAction* pasteAction = new QAction("&Paste", mainWindow);
    pasteAction->setShortcut(QKeySequence("Ctrl+V"));
    QObject::connect(pasteAction, &QAction::triggered, mainWindow, [mainWindow]() {
        QClipboard* clipboard = QApplication::clipboard();
        const QMimeData* mime = clipboard->mimeData();

        QImage img;

        if (mime->hasImage()) {
            img = qvariant_cast<QImage>(mime->imageData());
        }
        else if (mime->hasUrls()) {
            for (const QUrl& url : mime->urls()) {
                if (url.isLocalFile()) {
                    QImage loaded(url.toLocalFile());
                    if (!loaded.isNull()) {
                        img = loaded;
                        break;
                    }
                }
            }
        }

        if (img.isNull()) {
            QMessageBox::information(mainWindow, "Wklej",
                                     "Schowek nie zawiera obrazu.");
            return;
        }

        auto* mgr = mainWindow->canvas()->layerManager();
        mgr->addImageLayer(img, "Wklejony obraz");

        emit mgr->layersChanged();
        mainWindow->canvas()->update();
    });
    editMenu->addAction(pasteAction);

    editMenu->addSeparator();
    QMenu* transformMenu = editMenu->addMenu("Transformacje");

    auto addTransform = [&](const QString& name, auto fn) {
        QAction* a = new QAction(name, mainWindow);
        QObject::connect(a, &QAction::triggered, mainWindow, [mainWindow, fn]() {
            auto* mgr = mainWindow->canvas()->layerManager();
            fn(mgr, mgr->activeIndex());
            emit mgr->layersChanged();
            mainWindow->canvas()->update();
        });
        transformMenu->addAction(a);
    };

    addTransform("Przerzuć poziomo",  [](LayerManager* m, int i) { m->flipHorizontal(i); });
    addTransform("Przerzuć pionowo",  [](LayerManager* m, int i) { m->flipVertical(i);   });
    addTransform("Obróć 90° w prawo",[](LayerManager* m, int i) { m->rotate(i,  90);    });
    addTransform("Obróć 90° w lewo", [](LayerManager* m, int i) { m->rotate(i, -90);    });
    addTransform("Obróć 180°",       [](LayerManager* m, int i) { m->rotate(i, 180);    });

    QAction* transformAction = new QAction("&Transformuj", mainWindow);
    transformAction->setShortcut(QKeySequence("Ctrl+T"));
    QObject::connect(transformAction, &QAction::triggered, mainWindow, [mainWindow]() {
        mainWindow->canvas()->enterTransformMode();
    });
    editMenu->addAction(transformAction);

    // -------------------- View --------------------
    QMenu* viewMenu = menuBar->addMenu("&View");

    viewMenu->addAction("Show guidelines");

    // -------------------- Help --------------------
    QMenu* helpMenu = menuBar->addMenu("&Help");

    helpMenu->addAction("About");
}
