#pragma once

#include "core/AppConfig.h"

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QFormLayout;
class QLabel;
class QLineEdit;

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
    void accept() override;

    AppConfig m_initialConfig;
    QFormLayout *m_formLayout = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_providerLabel = nullptr;
    QLabel *m_customProviderLabel = nullptr;
    QLabel *m_baseUrlLabel = nullptr;
    QLabel *m_modelNameLabel = nullptr;
    QLabel *m_apiKeyLabel = nullptr;
    QLabel *m_temperatureLabel = nullptr;
    QLabel *m_maxTokensLabel = nullptr;
    QLabel *m_languageLabel = nullptr;
    QComboBox *m_providerCombo = nullptr;
    QLineEdit *m_customProviderEdit = nullptr;
    QLineEdit *m_baseUrlEdit = nullptr;
    QLineEdit *m_modelNameEdit = nullptr;
    QLineEdit *m_apiKeyEdit = nullptr;
    QLineEdit *m_temperatureEdit = nullptr;
    QLineEdit *m_maxTokensEdit = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
};
