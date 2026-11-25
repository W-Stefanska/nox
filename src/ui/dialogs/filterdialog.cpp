#include "filterdialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>

FilterDialog::FilterDialog(const QImage& image,
                           const QString& filterName,
                           const QVector<FilterParam>& params,
                           ApplyFn applyFn,
                           QWidget* parent)
    : QDialog(parent)
    , m_inputImage(image)
    , m_params(params)
    , m_applyFn(std::move(applyFn))
{
    setWindowTitle(filterName);
    setMinimumWidth(450);

    if (m_inputImage.width() > kPreviewMaxSize ||
        m_inputImage.height() > kPreviewMaxSize)
    {
        m_previewSource = m_inputImage.scaled(kPreviewMaxSize, kPreviewMaxSize,
                                              Qt::KeepAspectRatio,
                                              Qt::FastTransformation);
    } else {
        m_previewSource = m_inputImage;
    }

    auto* mainLayout = new QVBoxLayout(this);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(300, 200);
    m_previewLabel->setStyleSheet("border: 1px solid #555; background: #222;");
    mainLayout->addWidget(m_previewLabel);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);

    for (const auto& p : m_params) {
        m_values[p.key] = p.value;

        if (p.type == FilterParamType::Bool) {
            auto* cb = new QCheckBox(this);
            cb->setChecked(p.value > 0.5);
            form->addRow(p.label, cb);
            connect(cb, &QCheckBox::toggled, this, [this, key = p.key](bool on) {
                m_values[key] = on ? 1.0 : 0.0;
                onParamChanged();
            });
            continue;
        }

        auto* row = new QWidget(this);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);

        auto* slider = new QSlider(Qt::Horizontal, row);
        auto* spin   = new QDoubleSpinBox(row);
        auto* valLbl = new QLabel(QString::number(p.value), row);
        valLbl->setFixedWidth(50);

        const double step = (p.type == FilterParamType::Int) ? 1.0 : p.step;
        const int sliderSteps = int((p.max - p.min) / step);

        slider->setRange(0, sliderSteps);
        slider->setValue(int((p.value - p.min) / step));

        spin->setRange(p.min, p.max);
        spin->setSingleStep(step);
        spin->setDecimals((p.type == FilterParamType::Int) ? 0 : p.decimals);
        spin->setValue(p.value);

        h->addWidget(slider);
        h->addWidget(spin);
        h->addWidget(valLbl);

        form->addRow(p.label, row);

        m_sliders[p.key]     = slider;
        m_spins[p.key]       = spin;
        m_valueLabels[p.key] = valLbl;

        connect(slider, &QSlider::valueChanged, this,
                [this, key = p.key, step, min = p.min, spin](int v) {
                    double value = min + v * step;
                    spin->blockSignals(true);
                    spin->setValue(value);
                    spin->blockSignals(false);
                    m_values[key] = value;
                    m_valueLabels[key]->setText(QString::number(value, 'f', spin->decimals()));
                    onParamChanged();
                });

        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this, key = p.key, step, min = p.min, slider](double v) {
                    int s = int((v - min) / step);
                    slider->blockSignals(true);
                    slider->setValue(s);
                    slider->blockSignals(false);
                    m_values[key] = v;
                    m_valueLabels[key]->setText(QString::number(v, 'f', 2));
                    onParamChanged();
                });
    }
    mainLayout->addLayout(form);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* resetBtn = buttons->addButton(tr("Reset"), QDialogButtonBox::ResetRole);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        computeFull();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(resetBtn, &QPushButton::clicked, this, &FilterDialog::onReset);

    mainLayout->addWidget(buttons);

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(120);
    connect(m_debounceTimer, &QTimer::timeout, this, &FilterDialog::recompute);

    computePreview();
    m_resultImage = m_inputImage;
}

void FilterDialog::onParamChanged()
{
    m_debounceTimer->start();
}

void FilterDialog::recompute()
{
    computePreview();
}

void FilterDialog::computePreview()
{
    if (m_previewSource.isNull()) return;

    QImage preview = m_applyFn(m_previewSource, m_values);

    QPixmap pm = QPixmap::fromImage(preview)
                     .scaled(m_previewLabel->size(),
                             Qt::KeepAspectRatio,
                             Qt::FastTransformation);
    m_previewLabel->setPixmap(pm);
}

void FilterDialog::computeFull()
{
    m_resultImage = m_applyFn(m_inputImage, m_values);
}

void FilterDialog::onReset()
{
    for (const auto& p : m_params) {
        m_values[p.key] = p.value;

        if (m_sliders.contains(p.key)) {
            const double step = (p.type == FilterParamType::Int) ? 1.0 : p.step;
            m_sliders[p.key]->blockSignals(true);
            m_sliders[p.key]->setValue(int((p.value - p.min) / step));
            m_sliders[p.key]->blockSignals(false);
        }
        if (m_spins.contains(p.key)) {
            m_spins[p.key]->blockSignals(true);
            m_spins[p.key]->setValue(p.value);
            m_spins[p.key]->blockSignals(false);
        }
        if (m_valueLabels.contains(p.key)) {
            m_valueLabels[p.key]->setText(QString::number(p.value));
        }
    }
    onParamChanged();
}
