#include "toolspanel.h"
#include "../canvasview.h"
#include "../dialogs/filterdialog.h"
#include "../../core/filters/pointops.h"
#include "../../core/filters/convolution.h"
#include "../../core/filters/filteractions.h"
#include "colorpicker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QToolButton>
#include <QColorDialog>
#include <QFrame>
#include <QComboBox>

ToolsPanel::ToolsPanel(CanvasView* canvas,
                       FilterActions* filterActions,
                       QWidget* parent)
    : QWidget(parent)
    , m_canvas(canvas)
    , m_filterActions(filterActions)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Opcje narzędzi
    m_stack = new QStackedWidget(this);
    m_stack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    m_stack->setMinimumHeight(0);
    m_stack->addWidget(createBrushOptions());
    m_stack->addWidget(createEraserOptions());
    m_stack->addWidget(createEyedropperOptions());
    m_stack->addWidget(createRectSelectOptions());
    m_stack->addWidget(createLassoOptions());
    m_stack->addWidget(createTransformOptions());
    m_stack->addWidget(createFiltersOptions());
    m_stack->addWidget(createFillOptions());
    m_stack->addWidget(createMagicWandOptions());

    // Separatoe
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("separator");

    // Kolor
    m_colorPicker = new ColorPicker(this);

    auto* colorLabel = new QLabel("KOLOR", this);
    colorLabel->setAlignment(Qt::AlignCenter);
    colorLabel->setObjectName("sectionLabel");

    connect(m_colorPicker, &ColorPicker::colorChanged,
            m_canvas, &CanvasView::setPenColor);
    connect(m_canvas, &CanvasView::colorPicked,
            m_colorPicker, &ColorPicker::setColor);
    m_colorPicker->setColor(m_canvas->penColor());

    // Layout
    layout->addWidget(m_stack);
    //layout->addStretch();
    layout->addWidget(line);
    layout->addWidget(colorLabel);
    layout->addWidget(m_colorPicker);

    connect(m_canvas, &CanvasView::toolChanged,
            this, &ToolsPanel::onToolChanged);

    applyStyle();
}

void ToolsPanel::onToolChanged(Tool tool)
{
    switch (tool) {
    case Tool::Brush:       m_stack->setCurrentIndex(0); break;
    case Tool::Eraser:      m_stack->setCurrentIndex(1); break;
    case Tool::Eyedropper:  m_stack->setCurrentIndex(2); break;
    case Tool::RectSelect:  m_stack->setCurrentIndex(3); break;
    case Tool::LassoSelect: m_stack->setCurrentIndex(4); break;
    case Tool::Transform:   m_stack->setCurrentIndex(5); break;
    case Tool::Filters:     m_stack->setCurrentIndex(6); break;
    case Tool::Fill:        m_stack->setCurrentIndex(7); break;
    case Tool::MagicWand:   m_stack->setCurrentIndex(8); break;
    default: break;
    }
}

QWidget* ToolsPanel::createBrushOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    // rozmiar
    auto* sizeLabel = new QLabel("ROZMIAR", w);
    sizeLabel->setAlignment(Qt::AlignCenter);
    sizeLabel->setObjectName("sectionLabel");

    auto* sizeSlider = new QSlider(Qt::Horizontal, w);
    sizeSlider->setRange(1, 200);
    sizeSlider->setValue(m_canvas->penSize());

    auto* sizeVal = new QLabel(QString::number(m_canvas->penSize()) + "px", w);
    sizeVal->setAlignment(Qt::AlignCenter);
    sizeVal->setObjectName("valueLabel");

    connect(sizeSlider, &QSlider::valueChanged, this, [this, sizeVal](int val) {
        m_canvas->setPenSize(val);
        sizeVal->setText(QString::number(val) + "px");
    });

    v->addWidget(sizeLabel);
    v->addWidget(sizeSlider);
    v->addWidget(sizeVal);

    // twardos
    auto* hardLabel = new QLabel("TWARDOŚĆ", w);
    hardLabel->setAlignment(Qt::AlignCenter);
    hardLabel->setObjectName("sectionLabel");

    auto* hardSlider = new QSlider(Qt::Horizontal, w);
    hardSlider->setRange(0, 100);
    hardSlider->setValue(m_canvas->brushHardness());

    auto* hardVal = new QLabel(QString::number(m_canvas->brushHardness()) + "%", w);
    hardVal->setAlignment(Qt::AlignCenter);
    hardVal->setObjectName("valueLabel");

    connect(hardSlider, &QSlider::valueChanged, this, [this, hardVal](int val) {
        m_canvas->setBrushHardness(val / 100.0f);
        hardVal->setText(QString::number(val) + "%");
    });

    v->addWidget(hardLabel);
    v->addWidget(hardSlider);
    v->addWidget(hardVal);

    // krycie
    auto* alphaLabel = new QLabel("KRYCIE", w);
    alphaLabel->setAlignment(Qt::AlignCenter);
    alphaLabel->setObjectName("sectionLabel");

    auto* alphaSlider = new QSlider(Qt::Horizontal, w);
    alphaSlider->setRange(0, 100);
    alphaSlider->setValue(100);

    auto* alphaVal = new QLabel("100%", w);
    alphaVal->setAlignment(Qt::AlignCenter);
    alphaVal->setObjectName("valueLabel");

    connect(alphaSlider, &QSlider::valueChanged, this, [this, alphaVal](int val) {
        m_canvas->setBrushAlpha(val / 100.0f);
        alphaVal->setText(QString::number(val) + "%");
    });

    v->addWidget(alphaLabel);
    v->addWidget(alphaSlider);
    v->addWidget(alphaVal);
    v->addStretch();

    return w;
}

QWidget* ToolsPanel::createEraserOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    auto* lbl = new QLabel("ROZMIAR GUMKI", w);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setObjectName("sectionLabel");

    auto* slider = new QSlider(Qt::Horizontal, w);
    slider->setRange(1, 300);
    slider->setValue(m_canvas->eraserSize());

    auto* vallbl = new QLabel(QString::number(m_canvas->eraserSize()) + "px", w);
    vallbl->setAlignment(Qt::AlignCenter);
    vallbl->setObjectName("valueLabel");

    connect(slider, &QSlider::valueChanged, this, [this, vallbl](int val) {
        m_canvas->setEraserSize(val);
        vallbl->setText(QString::number(val) + "px");
    });

    v->addWidget(lbl);
    v->addWidget(slider);
    v->addWidget(vallbl);
    v->addStretch();

    return w;
}

QWidget* ToolsPanel::createEyedropperOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    auto* lbl = new QLabel("PODGLĄD KOLORU", w);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setObjectName("sectionLabel");

    auto* preview = new QLabel(w);
    preview->setFixedHeight(36);
    preview->setStyleSheet("border: 1px solid #555; background: #000;");

    auto* hex = new QLabel("#000000", w);
    hex->setAlignment(Qt::AlignCenter);
    hex->setObjectName("valueLabel");

    connect(m_canvas, &CanvasView::hoverColorChanged,
            this, [preview, hex](const QColor& c) {
                preview->setStyleSheet(
                    QString("border: 1px solid #555; background: %1;").arg(c.name()));
                hex->setText(c.name());
            });

    v->addWidget(lbl);
    v->addWidget(preview);
    v->addWidget(hex);
    v->addStretch();

    return w;
}

QWidget* ToolsPanel::createRectSelectOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    auto* lbl = new QLabel("TRYB ZAZNACZENIA", w);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setObjectName("sectionLabel");

    auto* mode = new QComboBox(w);
    mode->addItems({"Zastąp", "Dodaj", "Odejmij", "Przecięcie"});

    connect(mode, &QComboBox::currentIndexChanged, this, [this](int idx) {
        static const SelectionMode modes[] = {
            SelectionMode::Replace, SelectionMode::Add,
            SelectionMode::Subtract, SelectionMode::Intersect
        };
        m_canvas->setSelectionMode(modes[idx]);
    });

    v->addWidget(lbl);
    v->addWidget(mode);
    v->addStretch();
    return w;
}

QWidget* ToolsPanel::createLassoOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    auto* lbl = new QLabel("TRYB LASSA", w);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setObjectName("sectionLabel");

    auto* mode = new QComboBox(w);
    mode->addItems({"Zastąp", "Dodaj", "Odejmij"});

    connect(mode, &QComboBox::currentIndexChanged, this, [this](int idx) {
        static const SelectionMode modes[] = {
            SelectionMode::Replace, SelectionMode::Add, SelectionMode::Subtract
        };
        m_canvas->setSelectionMode(modes[idx]);
    });

    v->addWidget(lbl);
    v->addWidget(mode);
    v->addStretch();
    return w;
}

QWidget* ToolsPanel::createTransformOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(4);

    auto* lbl = new QLabel("TRANSFORMACJA", w);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setObjectName("sectionLabel");

    auto* angle = new QSlider(Qt::Horizontal, w);
    angle->setRange(-180, 180);
    angle->setValue(0);

    auto* angleVal = new QLabel("0°", w);
    angleVal->setAlignment(Qt::AlignCenter);
    angleVal->setObjectName("valueLabel");

    connect(angle, &QSlider::valueChanged, [angleVal](int x) {
        angleVal->setText(QString::number(x) + "°");
    });

    auto* interp = new QComboBox(w);
    interp->addItems({"Najbliższy sąsiad", "Dwuliniowa"});

    v->addWidget(lbl);
    v->addWidget(angle);
    v->addWidget(angleVal);
    v->addWidget(interp);
    return w;
}

QWidget* ToolsPanel::createFiltersOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(4);

    auto* lbl = new QLabel("FILTRY", w);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setObjectName("sectionLabel");
    v->addWidget(lbl);

    auto addDialogBtn = [this, v, w](const QString& label,
                                     const QString& title,
                                     const QVector<FilterParam>& params,
                                     std::function<QImage(const QImage&, const QMap<QString, double>&)> fn)
    {
        auto* btn = new QToolButton(w);
        btn->setText(label);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setFixedHeight(32);
        btn->setObjectName("filterBtn");
        v->addWidget(btn);

        connect(btn, &QToolButton::clicked, this, [this, title, params, fn]() {
            m_filterActions->showFilterDialog(title, params, fn);
        });
    };

    auto addPreviewBtn = [this, v, w](const QString& label,
                                      const QString& title,
                                      std::function<QImage(const QImage&)> fn)
    {
        auto* btn = new QToolButton(w);
        btn->setText(label);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setFixedHeight(32);
        btn->setObjectName("filterBtn");
        v->addWidget(btn);

        connect(btn, &QToolButton::clicked, this, [this, title, fn]() {
            m_filterActions->showFilterPreview(title, fn);
        });
    };

    auto* blurLabel = new QLabel("ROZMYCIA", w);
    blurLabel->setAlignment(Qt::AlignCenter);
    blurLabel->setObjectName("sectionLabel");
    v->addWidget(blurLabel);

    addDialogBtn(tr("Gauss…"), tr("Rozmycie Gaussa"),
                 {
                     { "radius", tr("Promień"), FilterParamType::Int,    1,   15,  2,  1.0, 0 },
                     { "sigma",  tr("Sigma"),   FilterParamType::Double, 0.1, 10,  1.0, 0.1, 2 }
                 },
                 [](const QImage& img, const QMap<QString, double>& p) {
                     std::vector<float> k1d = Kernels::gaussianKernel1D(int(p["radius"]),
                                                                        float(p["sigma"]));
                     float sum = 0.0f;
                     for (float x : k1d) sum += x;
                     return applySeparableConvolution(img, k1d, sum);
                 });

    addDialogBtn(tr("Box Blur…"), tr("Box Blur"),
                 {
                     { "radius", tr("Promień"), FilterParamType::Int, 1, 20, 2, 1.0, 0 }
                 },
                 [](const QImage& img, const QMap<QString, double>& p) {
                     return applyConvolution(img, Kernels::boxBlur(int(p["radius"])));
                 });

    auto* sharpLabel = new QLabel("WYOSTRZANIE", w);
    sharpLabel->setAlignment(Qt::AlignCenter);
    sharpLabel->setObjectName("sectionLabel");
    v->addWidget(sharpLabel);

    addPreviewBtn(tr("Wyostrzanie"), tr("Wyostrzanie"),
                  [](const QImage& img) {
                      return applyConvolution(img, Kernels::sharpen());
                  });

    auto* edgeLabel = new QLabel("KRAWĘDZIE", w);
    edgeLabel->setAlignment(Qt::AlignCenter);
    edgeLabel->setObjectName("sectionLabel");
    v->addWidget(edgeLabel);

    addPreviewBtn(tr("Sobel"), tr("Sobel"),
                  [](const QImage& img) {
                      return applyConvolution(img, Kernels::sobelX());
                  });

    addPreviewBtn(tr("Laplacian"), tr("Laplacian"),
                  [](const QImage& img) {
                      return applyConvolution(img, Kernels::edgeDetect());
                  });

    addPreviewBtn(tr("Emboss"), tr("Emboss"),
                  [](const QImage& img) {
                      return applyConvolution(img, Kernels::emboss());
                  });

    auto* pointLabel = new QLabel("PUNKTOWE", w);
    pointLabel->setAlignment(Qt::AlignCenter);
    pointLabel->setObjectName("sectionLabel");
    v->addWidget(pointLabel);

    addDialogBtn(tr("Jasność…"), tr("Jasność"),
                 {
                     { "delta", tr("Delta"), FilterParamType::Int, -255, 255, 0, 1.0, 0 }
                 },
                 [](const QImage& img, const QMap<QString, double>& p) {
                     return Pointops::adjustBrightness(img, int(p["delta"]));
                 });

    addDialogBtn(tr("Kontrast…"), tr("Kontrast"),
                 {
                     { "factor", tr("Współczynnik"), FilterParamType::Double, 0.1, 3.0, 1.0, 0.05, 2 }
                 },
                 [](const QImage& img, const QMap<QString, double>& p) {
                     return Pointops::adjustContrast(img, float(p["factor"]));
                 });

    addDialogBtn(tr("Gamma…"), tr("Gamma"),
                 {
                     { "gamma", tr("Gamma"), FilterParamType::Double, 0.1, 5.0, 1.0, 0.05, 2 }
                 },
                 [](const QImage& img, const QMap<QString, double>& p) {
                     return Pointops::gammaCorrect(img, float(p["gamma"]));
                 });

    addDialogBtn(tr("Binaryzacja…"), tr("Binaryzacja"),
                 {
                     { "level", tr("Próg"), FilterParamType::Int, 0, 255, 128, 1.0, 0 }
                 },
                 [](const QImage& img, const QMap<QString, double>& p) {
                     return Pointops::threshold(img, int(p["level"]));
                 });

    addPreviewBtn(tr("Skala szarości"), tr("Skala szarości"),
                  [](const QImage& img) {
                      return Pointops::toGrayscale(img);
                  });

    addPreviewBtn(tr("Inwersja"), tr("Inwersja"),
                  [](const QImage& img) {
                      return Pointops::invert(img);
                  });

    v->addStretch();
    return w;
}

QWidget* ToolsPanel::createFillOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    auto* tolLabel = new QLabel("TOLERANCJA", w);
    tolLabel->setAlignment(Qt::AlignCenter);
    tolLabel->setObjectName("sectionLabel");

    auto* tolSlider = new QSlider(Qt::Horizontal, w);
    tolSlider->setRange(0, 100);
    tolSlider->setValue(m_canvas->fillSettings().tolerance);

    auto* tolVal = new QLabel(QString::number(tolSlider->value()) + "%", w);
    tolVal->setAlignment(Qt::AlignCenter);
    tolVal->setObjectName("valueLabel");

    connect(tolSlider, &QSlider::valueChanged, this, [this, tolVal](int v) {
        auto s = m_canvas->fillSettings();
        s.tolerance = v;
        m_canvas->setFillSettings(s);
        tolVal->setText(QString::number(v) + "%");
    });

    v->addWidget(tolLabel);
    v->addWidget(tolSlider);
    v->addWidget(tolVal);

    auto* gapLabel = new QLabel("DOMYKANIE PRZERW", w);
    gapLabel->setAlignment(Qt::AlignCenter);
    gapLabel->setObjectName("sectionLabel");

    auto* gapSlider = new QSlider(Qt::Horizontal, w);
    gapSlider->setRange(0, 10);
    gapSlider->setValue(m_canvas->fillSettings().gapClosing);

    auto* gapVal = new QLabel(QString::number(gapSlider->value()) + "px", w);
    gapVal->setAlignment(Qt::AlignCenter);
    gapVal->setObjectName("valueLabel");

    connect(gapSlider, &QSlider::valueChanged, this, [this, gapVal](int v) {
        auto s = m_canvas->fillSettings();
        s.gapClosing = v;
        m_canvas->setFillSettings(s);
        gapVal->setText(QString::number(v) + "px");
    });

    v->addWidget(gapLabel);
    v->addWidget(gapSlider);
    v->addWidget(gapVal);

    auto* expLabel = new QLabel("POSZERZENIE", w);
    expLabel->setAlignment(Qt::AlignCenter);
    expLabel->setObjectName("sectionLabel");

    auto* expSlider = new QSlider(Qt::Horizontal, w);
    expSlider->setRange(-20, 20);
    expSlider->setValue(m_canvas->fillSettings().expand);

    auto* expVal = new QLabel(QString::number(expSlider->value()) + "px", w);
    expVal->setAlignment(Qt::AlignCenter);
    expVal->setObjectName("valueLabel");

    connect(expSlider, &QSlider::valueChanged, this, [this, expVal](int v) {
        auto s = m_canvas->fillSettings();
        s.expand = v;
        m_canvas->setFillSettings(s);
        expVal->setText(QString::number(v) + "px");
    });

    v->addWidget(expLabel);
    v->addWidget(expSlider);
    v->addWidget(expVal);
    v->addStretch();

    return w;
}

QWidget* ToolsPanel::createMagicWandOptions()
{
    auto* w = new QWidget(this);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    auto* modeLabel = new QLabel("TRYB", w);
    modeLabel->setAlignment(Qt::AlignCenter);
    modeLabel->setObjectName("sectionLabel");

    auto* modeCombo = new QComboBox(w);
    modeCombo->addItems({"Zastąp", "Dodaj", "Odejmij", "Przecięcie"});

    connect(modeCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        static const SelectionMode modes[] = {
            SelectionMode::Replace, SelectionMode::Add,
            SelectionMode::Subtract, SelectionMode::Intersect
        };
        m_canvas->setSelectionMode(modes[idx]);
    });

    v->addWidget(modeLabel);
    v->addWidget(modeCombo);

    auto* tolLabel = new QLabel("TOLERANCJA", w);
    tolLabel->setAlignment(Qt::AlignCenter);
    tolLabel->setObjectName("sectionLabel");

    auto* tolSlider = new QSlider(Qt::Horizontal, w);
    tolSlider->setRange(0, 100);
    tolSlider->setValue(m_canvas->wandSettings().tolerance);

    auto* tolVal = new QLabel(QString::number(tolSlider->value()) + "%", w);
    tolVal->setAlignment(Qt::AlignCenter);
    tolVal->setObjectName("valueLabel");

    connect(tolSlider, &QSlider::valueChanged, this, [this, tolVal](int v) {
        auto s = m_canvas->wandSettings();
        s.tolerance = v;
        m_canvas->setWandSettings(s);
        tolVal->setText(QString::number(v) + "%");
    });

    v->addWidget(tolLabel);
    v->addWidget(tolSlider);
    v->addWidget(tolVal);

    auto* gapLabel = new QLabel("DOMYKANIE PRZERW", w);
    gapLabel->setAlignment(Qt::AlignCenter);
    gapLabel->setObjectName("sectionLabel");

    auto* gapSlider = new QSlider(Qt::Horizontal, w);
    gapSlider->setRange(0, 10);
    gapSlider->setValue(m_canvas->wandSettings().gapClosing);

    auto* gapVal = new QLabel(QString::number(gapSlider->value()) + "px", w);
    gapVal->setAlignment(Qt::AlignCenter);
    gapVal->setObjectName("valueLabel");

    connect(gapSlider, &QSlider::valueChanged, this, [this, gapVal](int v) {
        auto s = m_canvas->wandSettings();
        s.gapClosing = v;
        m_canvas->setWandSettings(s);
        gapVal->setText(QString::number(v) + "px");
    });

    v->addWidget(gapLabel);
    v->addWidget(gapSlider);
    v->addWidget(gapVal);

    auto* expLabel = new QLabel("POSZERZENIE", w);
    expLabel->setAlignment(Qt::AlignCenter);
    expLabel->setObjectName("sectionLabel");

    auto* expSlider = new QSlider(Qt::Horizontal, w);
    expSlider->setRange(-20, 20);
    expSlider->setValue(m_canvas->wandSettings().expand);

    auto* expVal = new QLabel(QString::number(expSlider->value()) + "px", w);
    expVal->setAlignment(Qt::AlignCenter);
    expVal->setObjectName("valueLabel");

    connect(expSlider, &QSlider::valueChanged, this, [this, expVal](int v) {
        auto s = m_canvas->wandSettings();
        s.expand = v;
        m_canvas->setWandSettings(s);
        expVal->setText(QString::number(v) + "px");
    });

    v->addWidget(expLabel);
    v->addWidget(expSlider);
    v->addWidget(expVal);
    v->addStretch();

    return w;
}

void ToolsPanel::applyStyle()
{
    setStyleSheet(R"(
        ToolsPanel { background-color: #2b2b2b; }
        QLabel#sectionLabel {
            color: #888888; font-size: 10px; font-weight: bold;
            letter-spacing: 1px; padding: 3px 0 1px 0;
        }
        QLabel#valueLabel { color: #cccccc; font-size: 11px; padding: 0; }
        QSlider { min-height: 18px; max-height: 18px; }
        QSlider::groove:horizontal { height: 4px; background: #555555; border-radius: 2px; }
        QSlider::handle:horizontal {
            background: #0d6efd; border-radius: 7px;
            width: 14px; height: 14px; margin: -5px 0;
        }
        QSlider::sub-page:horizontal { background: #0d6efd; border-radius: 2px; }
        QFrame#separator { color: #444444; }
        QComboBox {
            background-color: #3c3c3c; color: #dddddd;
            border: 1px solid #555555; border-radius: 6px;
            padding: 4px 8px; font-size: 12px;
        }
        QToolButton#filterBtn {
            background-color: #3c3c3c;
            color: #dddddd;
            border: 1px solid #555555;
            border-radius: 6px;
            font-size: 12px;
            text-align: left;
            padding: 4px 8px;
        }
        QToolButton#filterBtn:hover {
            background-color: #4a4a4a;
            border-color: #777777;
        }
        QToolButton#filterBtn:pressed {
            background-color: #0d6efd;
        }
    )");
}
