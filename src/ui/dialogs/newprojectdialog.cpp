#include "newprojectdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QPainter>
#include <QPixmap>
#include <QGroupBox>

namespace {
constexpr int kPreviewMaxW = 260;
constexpr int kPreviewMaxH = 180;

struct Preset { const char* name; int w; int h; };
const Preset kPresets[] = {
    { "Niestandardowy",       0,    0    },
    { "HD (1280 × 720)",      1280, 720  },
    { "Full HD (1920 × 1080)",1920, 1080 },
    { "4K UHD (3840 × 2160)", 3840, 2160 },
    { "A4 @ 300 DPI",         2480, 3508 },
    { "Kwadrat 1000 × 1000",  1000, 1000 },
    { "Instagram 1080 × 1080",1080, 1080 },
    { "Ikona 512 × 512",      512,  512  },
    };
} // namespace

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Nowy projekt"));
    setModal(true);
    setMinimumWidth(520);

    auto* mainLayout = new QVBoxLayout(this);

    m_presetCombo = new QComboBox(this);
    for (const auto& p : kPresets)
        m_presetCombo->addItem(tr(p.name));
    m_presetCombo->setCurrentIndex(0);

    auto* presetRow = new QHBoxLayout;
    presetRow->addWidget(new QLabel(tr("Preset:"), this));
    presetRow->addWidget(m_presetCombo, 1);
    mainLayout->addLayout(presetRow);

    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);

    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(1, 4000);
    m_widthSpin->setValue(1280);
    m_widthSpin->setSuffix(" px");

    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(1, 4000);
    m_heightSpin->setValue(720);
    m_heightSpin->setSuffix(" px");

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Bez tytułu"));
    m_nameEdit->setMaxLength(120);

    form->addRow(tr("Szerokość:"), m_widthSpin);
    form->addRow(tr("Wysokość:"),  m_heightSpin);
    form->addRow(tr("Nazwa:"),     m_nameEdit);
    mainLayout->addLayout(form);

    auto* previewBox = new QGroupBox(tr("Podgląd"), this);
    auto* previewLayout = new QVBoxLayout(previewBox);

    m_preview = new QLabel(previewBox);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumHeight(kPreviewMaxH + 20);
    m_preview->setStyleSheet(
        "background: #1e1e1e; border: 1px solid #444; border-radius: 4px;");

    m_sizeInfo = new QLabel(previewBox);
    m_sizeInfo->setAlignment(Qt::AlignCenter);
    m_sizeInfo->setStyleSheet("color: #aaa; font-size: 11px;");

    previewLayout->addWidget(m_preview);
    previewLayout->addWidget(m_sizeInfo);
    mainLayout->addWidget(previewBox);

    m_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Utwórz"));
    m_buttons->button(QDialogButtonBox::Ok)->setDefault(true);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttons);

    connect(m_widthSpin,  QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NewProjectDialog::updatePreview);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NewProjectDialog::updatePreview);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NewProjectDialog::applyPreset);

    updatePreview();
    m_widthSpin->setFocus();
    m_widthSpin->selectAll();
}

int NewProjectDialog::canvasWidth() const  { return m_widthSpin->value();  }
int NewProjectDialog::canvasHeight() const { return m_heightSpin->value(); }

QString NewProjectDialog::projectName() const
{
    QString n = m_nameEdit->text().trimmed();
    return n.isEmpty() ? tr("Bez tytułu") : n;
}

QSize NewProjectDialog::canvasSize() const
{
    return QSize(canvasWidth(), canvasHeight());
}

void NewProjectDialog::applyPreset(int index)
{
    if (index <= 0 || index >= int(std::size(kPresets))) return;
    const auto& p = kPresets[index];
    m_widthSpin->blockSignals(true);
    m_heightSpin->blockSignals(true);
    m_widthSpin->setValue(p.w);
    m_heightSpin->setValue(p.h);
    m_widthSpin->blockSignals(false);
    m_heightSpin->blockSignals(false);
    updatePreview();
}

void NewProjectDialog::updatePreview()
{
    const int w = m_widthSpin->value();
    const int h = m_heightSpin->value();

    int matchingPreset = 0;
    for (int i = 1; i < int(std::size(kPresets)); ++i) {
        if (kPresets[i].w == w && kPresets[i].h == h) {
            matchingPreset = i;
            break;
        }
    }

    if (m_presetCombo->currentIndex() != matchingPreset) {
        m_presetCombo->blockSignals(true);
        m_presetCombo->setCurrentIndex(matchingPreset);
        m_presetCombo->blockSignals(false);
    }

    QSize target = QSize(w, h).scaled(kPreviewMaxW, kPreviewMaxH,
                                      Qt::KeepAspectRatio);

    QPixmap pm(target);
    pm.fill(Qt::white);

    QPainter p(&pm);
    p.setPen(QPen(QColor("#0d6efd"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(0, 0, target.width() - 1, target.height() - 1);

    p.setPen(QPen(QColor(0, 0, 0, 40), 1, Qt::DashLine));
    for (int i = 1; i <= 2; ++i) {
        p.drawLine(target.width()  * i / 3, 0, target.width()  * i / 3, target.height());
        p.drawLine(0, target.height() * i / 3, target.width(), target.height() * i / 3);
    }
    p.end();

    m_preview->setPixmap(pm);

    const double mp = (double(w) * h) / 1'000'000.0;
    const double aspect = double(w) / double(h);
    m_sizeInfo->setText(
        tr("%1 × %2 px  •  %3 MPx  •  proporcje %4:%5")
            .arg(w).arg(h)
            .arg(mp, 0, 'f', 2));
}
