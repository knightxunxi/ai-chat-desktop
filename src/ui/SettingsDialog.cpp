#include "ui/SettingsDialog.h"

#include "core/ProviderPreset.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString dialogText(AppLanguage language, const QString &english, const QString &chinese)
{
    return language == AppLanguage::English ? english : chinese;
}

} // namespace

SettingsDialog::SettingsDialog(const AppConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_initialConfig(config)
{
    setupUi();
    applyTexts();
}

AppConfig SettingsDialog::config() const
{
    AppConfig config;
    const QString providerId = m_providerCombo->currentData().toString();
    const std::optional<ProviderPreset> preset = ProviderPresets::findById(providerId);
    config.providerName = preset.has_value() ? preset->name : m_customProviderEdit->text().trimmed();
    config.baseUrl = m_baseUrlEdit->text().trimmed();
    config.modelName = m_modelNameEdit->text().trimmed();
    config.apiKey = m_apiKeyEdit->text().trimmed();
    config.language = static_cast<AppLanguage>(m_languageCombo->currentData().toInt());
    return config;
}

void SettingsDialog::setupUi()
{
    setModal(true);
    setMinimumWidth(460);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(22, 20, 22, 18);
    rootLayout->setSpacing(16);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("settingsTitle"));
    rootLayout->addWidget(m_titleLabel);

    m_formLayout = new QFormLayout();
    m_formLayout->setContentsMargins(0, 0, 0, 0);
    m_formLayout->setSpacing(12);

    m_providerCombo = new QComboBox(this);
    m_providerCombo->setObjectName(QStringLiteral("providerCombo"));
    for (const ProviderPreset &preset : ProviderPresets::defaults()) {
        m_providerCombo->addItem(preset.name, preset.id);
    }
    m_providerCombo->addItem(QStringLiteral("Custom"), ProviderPresets::customId());

    const std::optional<ProviderPreset> initialPreset = ProviderPresets::findByName(m_initialConfig.providerName);
    const QString initialProviderId = initialPreset.has_value() ? initialPreset->id : ProviderPresets::customId();
    const int initialProviderIndex = m_providerCombo->findData(initialProviderId);
    m_providerCombo->setCurrentIndex(initialProviderIndex < 0 ? 0 : initialProviderIndex);

    m_customProviderEdit = new QLineEdit(this);
    m_customProviderEdit->setObjectName(QStringLiteral("customProviderEdit"));
    m_customProviderEdit->setText(initialPreset.has_value() ? QString() : m_initialConfig.providerName);

    m_baseUrlEdit = new QLineEdit(this);
    m_baseUrlEdit->setText(m_initialConfig.baseUrl);

    m_modelNameEdit = new QLineEdit(this);
    m_modelNameEdit->setText(m_initialConfig.modelName);

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setText(m_initialConfig.apiKey);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);

    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem(QStringLiteral("中文"), static_cast<int>(AppLanguage::Chinese));
    m_languageCombo->addItem(QStringLiteral("English"), static_cast<int>(AppLanguage::English));
    m_languageCombo->setCurrentIndex(m_initialConfig.language == AppLanguage::English ? 1 : 0);

    m_providerLabel = new QLabel(this);
    m_customProviderLabel = new QLabel(this);
    m_baseUrlLabel = new QLabel(this);
    m_modelNameLabel = new QLabel(this);
    m_apiKeyLabel = new QLabel(this);
    m_languageLabel = new QLabel(this);

    m_formLayout->addRow(m_providerLabel, m_providerCombo);
    m_formLayout->addRow(m_customProviderLabel, m_customProviderEdit);
    m_formLayout->addRow(m_baseUrlLabel, m_baseUrlEdit);
    m_formLayout->addRow(m_modelNameLabel, m_modelNameEdit);
    m_formLayout->addRow(m_apiKeyLabel, m_apiKeyEdit);
    m_formLayout->addRow(m_languageLabel, m_languageCombo);

    rootLayout->addLayout(m_formLayout);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_providerCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::applySelectedProviderPreset);
    rootLayout->addWidget(m_buttonBox);

    updateCustomProviderVisibility();
}

void SettingsDialog::applyTexts()
{
    const AppLanguage language = m_initialConfig.language;
    setWindowTitle(dialogText(language, QStringLiteral("Settings"), QStringLiteral("设置")));

    m_titleLabel->setText(dialogText(language, QStringLiteral("Settings"), QStringLiteral("设置")));
    m_providerLabel->setText(dialogText(language, QStringLiteral("Provider"), QStringLiteral("服务商")));
    m_customProviderLabel->setText(dialogText(language, QStringLiteral("Custom name"), QStringLiteral("自定义名称")));
    const int customIndex = m_providerCombo->findData(ProviderPresets::customId());
    if (customIndex >= 0) {
        m_providerCombo->setItemText(customIndex, dialogText(language, QStringLiteral("Custom"), QStringLiteral("自定义")));
    }
    m_baseUrlLabel->setText(QStringLiteral("Base URL"));
    m_modelNameLabel->setText(dialogText(language, QStringLiteral("Model"), QStringLiteral("模型名称")));
    m_apiKeyLabel->setText(QStringLiteral("API Key"));
    m_languageLabel->setText(dialogText(language, QStringLiteral("Language"), QStringLiteral("界面语言")));

    m_buttonBox->button(QDialogButtonBox::Save)->setText(dialogText(language, QStringLiteral("Save"), QStringLiteral("保存")));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(dialogText(language, QStringLiteral("Cancel"), QStringLiteral("取消")));
}

void SettingsDialog::applySelectedProviderPreset(int index)
{
    const QString providerId = m_providerCombo->itemData(index).toString();
    const std::optional<ProviderPreset> preset = ProviderPresets::findById(providerId);
    if (preset.has_value()) {
        m_baseUrlEdit->setText(preset->baseUrl);
        m_modelNameEdit->setText(preset->modelName);
        m_customProviderEdit->clear();
    }

    updateCustomProviderVisibility();
}

void SettingsDialog::updateCustomProviderVisibility()
{
    const bool isCustom = m_providerCombo->currentData().toString() == ProviderPresets::customId();
    m_customProviderLabel->setVisible(isCustom);
    m_customProviderEdit->setVisible(isCustom);
}

void SettingsDialog::accept()
{
    const AppConfig nextConfig = config();
    if (nextConfig.providerName.isEmpty() || nextConfig.baseUrl.isEmpty() || nextConfig.modelName.isEmpty()) {
        QMessageBox::warning(
            this,
            dialogText(nextConfig.language, QStringLiteral("Missing settings"), QStringLiteral("配置不完整")),
            dialogText(nextConfig.language,
                       QStringLiteral("Provider, Base URL, and model are required."),
                       QStringLiteral("服务商、Base URL 和模型名称不能为空。")));
        return;
    }

    QDialog::accept();
}
