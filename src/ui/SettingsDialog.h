#pragma once

#include "core/AppConfig.h"

#include <QDialog>

class QComboBox;
class QCheckBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QSpinBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const AppConfig &config, QWidget *parent = nullptr);

    AppConfig config() const;

private:
    void setupUi();
    void applyTexts();
    void applySelectedProviderPreset(int index);
    void updateCustomProviderVisibility();
    void updateModelParameterStates();
    void accept() override;

    AppConfig m_initialConfig;
    QFormLayout *m_formLayout = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_providerLabel = nullptr;
    QLabel *m_customProviderLabel = nullptr;
    QLabel *m_baseUrlLabel = nullptr;
    QLabel *m_modelNameLabel = nullptr;
    QLabel *m_apiKeyLabel = nullptr;
    QLabel *m_languageLabel = nullptr;
    QComboBox *m_providerCombo = nullptr;
    QLineEdit *m_customProviderEdit = nullptr;
    QLineEdit *m_baseUrlEdit = nullptr;
    QLineEdit *m_modelNameEdit = nullptr;
    QLineEdit *m_apiKeyEdit = nullptr;
    QCheckBox *m_temperatureCheckBox = nullptr;
    QDoubleSpinBox *m_temperatureSpinBox = nullptr;
    QCheckBox *m_maxTokensCheckBox = nullptr;
    QSpinBox *m_maxTokensSpinBox = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
};
