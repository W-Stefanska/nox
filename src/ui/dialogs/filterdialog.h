#pragma once

#include <QDialog>
#include <QImage>
#include <QMap>
#include <functional>

class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;
class QDialogButtonBox;
class QVBoxLayout;

enum class FilterParamType {
    Int,
    Double,
    Bool
};

struct FilterParam {
    QString         key;
    QString         label;
    FilterParamType type;
    double          min   = 0.0;
    double          max   = 100.0;
    double          value = 1.0;
    double          step  = 0.1;
    int             decimals = 2;
};

class FilterDialog : public QDialog
{
    Q_OBJECT
public:
    using ApplyFn = std::function<QImage(const QImage&, const QMap<QString, double>&)>;

    FilterDialog(const QImage& image,
                 const QString& filterName,
                 const QVector<FilterParam>& params,
                 ApplyFn applyFn,
                 QWidget* parent = nullptr);

    QImage resultImage() const { return m_resultImage; }

signals:
    void parametersChanged();

private slots:
    void onParamChanged();
    void onReset();
    void recompute();

private:
    void computePreview();
    void computeFull();

    QImage m_inputImage;
    QImage m_previewSource;
    QImage m_resultImage;

    QVector<FilterParam> m_params;
    QMap<QString, double> m_values;
    ApplyFn m_applyFn;

    QMap<QString, QSlider*>       m_sliders;
    QMap<QString, QDoubleSpinBox*> m_spins;
    QMap<QString, QLabel*>        m_valueLabels;

    QDialogButtonBox* m_buttons = nullptr;

    QLabel* m_previewLabel = nullptr;

    QTimer* m_debounceTimer = nullptr;

    static constexpr int kPreviewMaxSize = 400;

};
