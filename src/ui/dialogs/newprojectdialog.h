#pragma once

#include <QDialog>
#include <QSize>

class QSpinBox;
class QLineEdit;
class QLabel;
class QDialogButtonBox;
class QComboBox;

class NewProjectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewProjectDialog(QWidget* parent = nullptr);

    int      canvasWidth()  const;
    int      canvasHeight() const;
    QString  projectName()  const;
    QSize    canvasSize()   const;

private slots:
    void updatePreview();

private:
    QSpinBox*  m_widthSpin   = nullptr;
    QSpinBox*  m_heightSpin  = nullptr;
    QLineEdit* m_nameEdit    = nullptr;
    QComboBox* m_presetCombo = nullptr;
    QLabel*    m_preview     = nullptr;
    QLabel*    m_sizeInfo    = nullptr;
    QDialogButtonBox* m_buttons = nullptr;

    void applyPreset(int index);
};
