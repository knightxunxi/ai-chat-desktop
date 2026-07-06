#include "ui/SettingsDialog.h"

#include "core/ProviderPreset.h"
#include "services/UpdateChecker.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QFrame>
#include <QIntValidator>
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

std::optional<double> optionalDoubleFromText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return std::nullopt;
    }

    bool ok = false;
    const double value = trimmed.toDouble(&ok);
    return ok ? std::optional<double>(value) : std::nullopt;
}

std::optional<int> optionalIntFromText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return std::nullopt;
    }

    bool ok = false;
    const int value = trimmed.toInt(&ok);
    return ok ? std::optional<int>(value) : std::nullopt;
}

} // namespace

SettingsDialog::SettingsDialog(const AppConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_initialConfig(config)
{
    setupUi();
    applyTexts();
}

void SettingsDialog::setDebugMode(bool enabled)
{
    m_debugMode = enabled;
    if (m_debugModeCheckbox != nullptr) {
        m_debugModeCheckbox->setChecked(enabled);
    }
}

AppConfig SettingsDialog::config() const
{
    AppConfig config;
    const QString providerId = m_providerCombo->currentData().toString();
    const std::optional<ProviderPreset> preset = ProviderPresets::findById(providerId);
    config.backendType = static_cast<AIBackendType>(m_backendCombo->currentData().toInt());
    config.providerName = preset.has_value() ? preset->name : m_customProviderEdit->text().trimmed();
    config.baseUrl = m_baseUrlEdit->text().trimmed();
    config.modelName = m_modelNameEdit->text().trimmed();
    config.apiKey = m_apiKeyEdit->text().trimmed();
    config.pythonExecutable = m_pythonExecutableEdit->text().trimmed();
    config.pythonSidecarDirectory = m_pythonSidecarEdit->text().trimmed();
    config.temperature = optionalDoubleFromText(m_temperatureEdit->text());
    config.maxTokens = optionalIntFromText(m_maxTokensEdit->text());
    config.agentWorkspaceDirectory = m_agentWorkspaceEdit->text().trimmed();
    config.agentProjectDirectory = m_agentProjectEdit->text().trimmed();
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

    m_backendCombo = new QComboBox(this);
    m_backendCombo->setObjectName(QStringLiteral("backendCombo"));
    m_backendCombo->addItem(QStringLiteral("Direct C++"), static_cast<int>(AIBackendType::Direct));
    m_backendCombo->addItem(QStringLiteral("Python sidecar"), static_cast<int>(AIBackendType::Sidecar));
    m_backendCombo->setCurrentIndex(m_initialConfig.backendType == AIBackendType::Sidecar ? 1 : 0);

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

    m_pythonExecutableEdit = new QLineEdit(this);
    m_pythonExecutableEdit->setObjectName(QStringLiteral("pythonExecutableEdit"));
    m_pythonExecutableEdit->setText(m_initialConfig.pythonExecutable.trimmed().isEmpty()
                                        ? AppConfig::defaultPythonExecutable()
                                        : m_initialConfig.pythonExecutable);

    m_pythonSidecarEdit = new QLineEdit(this);
    m_pythonSidecarEdit->setObjectName(QStringLiteral("pythonSidecarEdit"));
    m_pythonSidecarEdit->setText(m_initialConfig.pythonSidecarDirectory.trimmed().isEmpty()
                                     ? AppConfig::defaultPythonSidecarDirectory()
                                     : m_initialConfig.pythonSidecarDirectory);

    m_temperatureEdit = new QLineEdit(this);
    m_temperatureEdit->setObjectName(QStringLiteral("temperatureEdit"));
    m_temperatureEdit->setValidator(new QDoubleValidator(0.0, 2.0, 2, m_temperatureEdit));
    if (m_initialConfig.temperature.has_value()) {
        m_temperatureEdit->setText(QString::number(m_initialConfig.temperature.value(), 'f', 2));
    }

    m_maxTokensEdit = new QLineEdit(this);
    m_maxTokensEdit->setObjectName(QStringLiteral("maxTokensEdit"));
    m_maxTokensEdit->setValidator(new QIntValidator(1, 200000, m_maxTokensEdit));
    if (m_initialConfig.maxTokens.has_value()) {
        m_maxTokensEdit->setText(QString::number(m_initialConfig.maxTokens.value()));
    }

    m_agentWorkspaceEdit = new QLineEdit(this);
    m_agentWorkspaceEdit->setObjectName(QStringLiteral("agentWorkspaceEdit"));
    m_agentWorkspaceEdit->setText(m_initialConfig.agentWorkspaceDirectory.trimmed().isEmpty()
                                      ? AppConfig::defaultAgentWorkspaceDirectory()
                                      : m_initialConfig.agentWorkspaceDirectory);

    m_agentProjectEdit = new QLineEdit(this);
    m_agentProjectEdit->setObjectName(QStringLiteral("agentProjectEdit"));
    m_agentProjectEdit->setText(m_initialConfig.agentProjectDirectory.trimmed().isEmpty()
                                    ? AppConfig::defaultAgentProjectDirectory()
                                    : m_initialConfig.agentProjectDirectory);

    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem(QStringLiteral("中文"), static_cast<int>(AppLanguage::Chinese));
    m_languageCombo->addItem(QStringLiteral("English"), static_cast<int>(AppLanguage::English));
    m_languageCombo->setCurrentIndex(m_initialConfig.language == AppLanguage::English ? 1 : 0);

    m_backendLabel = new QLabel(this);
    m_providerLabel = new QLabel(this);
    m_customProviderLabel = new QLabel(this);
    m_baseUrlLabel = new QLabel(this);
    m_modelNameLabel = new QLabel(this);
    m_apiKeyLabel = new QLabel(this);
    m_pythonExecutableLabel = new QLabel(this);
    m_pythonSidecarLabel = new QLabel(this);
    m_temperatureLabel = new QLabel(this);
    m_maxTokensLabel = new QLabel(this);
    m_agentWorkspaceLabel = new QLabel(this);
    m_agentProjectLabel = new QLabel(this);
    m_languageLabel = new QLabel(this);

    m_formLayout->addRow(m_backendLabel, m_backendCombo);
    m_formLayout->addRow(m_providerLabel, m_providerCombo);
    m_formLayout->addRow(m_customProviderLabel, m_customProviderEdit);
    m_formLayout->addRow(m_baseUrlLabel, m_baseUrlEdit);
    m_formLayout->addRow(m_modelNameLabel, m_modelNameEdit);
    m_formLayout->addRow(m_apiKeyLabel, m_apiKeyEdit);
    m_formLayout->addRow(m_pythonExecutableLabel, m_pythonExecutableEdit);
    m_formLayout->addRow(m_pythonSidecarLabel, m_pythonSidecarEdit);
    m_formLayout->addRow(m_temperatureLabel, m_temperatureEdit);
    m_formLayout->addRow(m_maxTokensLabel, m_maxTokensEdit);
    m_formLayout->addRow(m_agentWorkspaceLabel, m_agentWorkspaceEdit);
    m_formLayout->addRow(m_agentProjectLabel, m_agentProjectEdit);
    m_formLayout->addRow(m_languageLabel, m_languageCombo);

    rootLayout->addLayout(m_formLayout);

    // V16.3: Agent 调试模式（移到设置界面）
    {
        auto *debugSeparator = new QFrame(this);
        debugSeparator->setFrameShape(QFrame::HLine);
        rootLayout->addSpacing(8);
        rootLayout->addWidget(debugSeparator);
        rootLayout->addSpacing(4);

        m_debugModeCheckbox = new QCheckBox(this);
        m_debugModeCheckbox->setText(dialogText(m_initialConfig.language,
            QStringLiteral("Developer: Show Agent loop prompts in chat"),
            QStringLiteral("开发者：在聊天中显示 Agent 循环提示词")));
        m_debugModeCheckbox->setChecked(m_debugMode);
        connect(m_debugModeCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
            m_debugMode = checked;
        });
        rootLayout->addWidget(m_debugModeCheckbox);
        rootLayout->addSpacing(4);
    }

    // N3: 检查更新按钮
    {
        auto *updateSeparator = new QFrame(this);
        updateSeparator->setFrameShape(QFrame::HLine);
        rootLayout->addSpacing(8);
        rootLayout->addWidget(updateSeparator);
        rootLayout->addSpacing(4);

        auto *updateLayout = new QHBoxLayout();
        auto *updateBtn = new QPushButton(this);
        updateBtn->setObjectName(QStringLiteral("checkUpdateBtn"));
        updateBtn->setText(dialogText(m_initialConfig.language,
            QStringLiteral("Check for Updates"),
            QStringLiteral("检查更新")));
        updateBtn->setFixedWidth(160);

        auto *versionLabel = new QLabel(this);
        versionLabel->setText(dialogText(m_initialConfig.language,
            QStringLiteral("Current version: 1.0"),
            QStringLiteral("当前版本: 1.0")));
        versionLabel->setStyleSheet(QStringLiteral("color: #888;"));

        updateLayout->addWidget(updateBtn);
        updateLayout->addWidget(versionLabel);
        updateLayout->addStretch();
        rootLayout->addLayout(updateLayout);

        // 连接检查更新
        auto *checker = new UpdateChecker(this);
        connect(updateBtn, &QPushButton::clicked, this, [this, updateBtn, checker]() {
            updateBtn->setEnabled(false);
            updateBtn->setText(dialogText(m_initialConfig.language,
                QStringLiteral("Checking..."),
                QStringLiteral("检查中...")));
            checker->checkForUpdates();
        });
        connect(checker, &UpdateChecker::updateCheckFinished, this,
            [this, updateBtn](const UpdateInfo &info) {
                updateBtn->setEnabled(true);
                updateBtn->setText(dialogText(m_initialConfig.language,
                    QStringLiteral("Check for Updates"),
                    QStringLiteral("检查更新")));

                if (!info.placeholderMessage.isEmpty()) {
                    QMessageBox::information(this,
                        dialogText(m_initialConfig.language,
                            QStringLiteral("Update Placeholder"),
                            QStringLiteral("更新检查占位")),
                        info.placeholderMessage);
                    return;
                }

                if (!info.errorMessage.isEmpty()) {
                    QMessageBox::warning(this,
                        dialogText(m_initialConfig.language,
                            QStringLiteral("Update Check Failed"),
                            QStringLiteral("检查更新失败")),
                        info.errorMessage);
                    return;
                }

                if (!info.hasUpdate) {
                    QMessageBox::information(this,
                        dialogText(m_initialConfig.language,
                            QStringLiteral("Up to Date"),
                            QStringLiteral("已是最新版本")),
                        dialogText(m_initialConfig.language,
                            QStringLiteral("You are running the latest version (%1).")
                                .arg(info.currentVersion),
                            QStringLiteral("当前已是最新版本 (%1)。")
                                .arg(info.currentVersion)));
                    return;
                }

                // 有更新
                QString msg = dialogText(m_initialConfig.language,
                    QStringLiteral("New version %1 is available!\n\nRelease notes:\n%2\n\nCurrent: %3\nLatest: %4")
                        .arg(info.latestVersion, info.releaseNotes, info.currentVersion, info.latestVersion),
                    QStringLiteral("新版本 %1 可用！\n\n更新说明：\n%2\n\n当前版本：%3\n最新版本：%4")
                        .arg(info.latestVersion, info.releaseNotes, info.currentVersion, info.latestVersion));

                QMessageBox box(this);
                box.setWindowTitle(dialogText(m_initialConfig.language,
                    QStringLiteral("Update Available"),
                    QStringLiteral("有可用更新")));
                box.setText(msg);
                box.setStandardButtons(QMessageBox::Ok);
                if (!info.downloadUrl.isEmpty()) {
                    auto *downloadBtn = box.addButton(
                        dialogText(m_initialConfig.language,
                            QStringLiteral("Download"),
                            QStringLiteral("下载")),
                        QMessageBox::ActionRole);
                    connect(downloadBtn, &QPushButton::clicked, this, [url = info.downloadUrl]() {
                        QDesktopServices::openUrl(QUrl(url));
                    });
                }
                box.exec();
            });
    }

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_providerCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::applySelectedProviderPreset);
    connect(m_backendCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateBackendVisibility);
    rootLayout->addWidget(m_buttonBox);

    updateCustomProviderVisibility();
    updateBackendVisibility();
}

void SettingsDialog::applyTexts()
{
    const AppLanguage language = m_initialConfig.language;
    setWindowTitle(dialogText(language, QStringLiteral("Settings"), QStringLiteral("设置")));

    m_titleLabel->setText(dialogText(language, QStringLiteral("Settings"), QStringLiteral("设置")));
    m_backendLabel->setText(dialogText(language, QStringLiteral("AI backend"), QStringLiteral("AI 后端")));
    m_backendCombo->setItemText(0, dialogText(language, QStringLiteral("Direct C++ client"), QStringLiteral("C++ 直连")));
    m_backendCombo->setItemText(1, dialogText(language, QStringLiteral("Python sidecar"), QStringLiteral("Python 能力层")));
    m_providerLabel->setText(dialogText(language, QStringLiteral("Provider"), QStringLiteral("服务商")));
    m_customProviderLabel->setText(dialogText(language, QStringLiteral("Custom name"), QStringLiteral("自定义名称")));
    const int customIndex = m_providerCombo->findData(ProviderPresets::customId());
    if (customIndex >= 0) {
        m_providerCombo->setItemText(customIndex, dialogText(language, QStringLiteral("Custom"), QStringLiteral("自定义")));
    }
    m_baseUrlLabel->setText(QStringLiteral("Base URL"));
    m_modelNameLabel->setText(dialogText(language, QStringLiteral("Model"), QStringLiteral("模型名称")));
    m_apiKeyLabel->setText(QStringLiteral("API Key"));
    m_pythonExecutableLabel->setText(dialogText(language, QStringLiteral("Python command"), QStringLiteral("Python 命令")));
    m_pythonExecutableEdit->setPlaceholderText(AppConfig::defaultPythonExecutable());
    m_pythonSidecarLabel->setText(dialogText(language, QStringLiteral("Sidecar directory"), QStringLiteral("Sidecar 目录")));
    m_pythonSidecarEdit->setPlaceholderText(AppConfig::defaultPythonSidecarDirectory());
    m_temperatureLabel->setText(dialogText(language, QStringLiteral("Temperature"), QStringLiteral("随机性")));
    m_temperatureEdit->setPlaceholderText(dialogText(language, QStringLiteral("Optional, 0-2"), QStringLiteral("可选，0-2")));
    m_maxTokensLabel->setText(dialogText(language, QStringLiteral("Max tokens"), QStringLiteral("最大输出")));
    m_maxTokensEdit->setPlaceholderText(dialogText(language, QStringLiteral("Optional integer"), QStringLiteral("可选整数")));
    m_agentWorkspaceLabel->setText(dialogText(language, QStringLiteral("Agent workspace"), QStringLiteral("Agent 工作目录")));
    m_agentWorkspaceEdit->setPlaceholderText(AppConfig::defaultAgentWorkspaceDirectory());
    m_agentProjectLabel->setText(dialogText(language, QStringLiteral("Agent project"), QStringLiteral("Agent 项目目录")));
    m_agentProjectEdit->setPlaceholderText(AppConfig::defaultAgentProjectDirectory());
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

void SettingsDialog::updateBackendVisibility()
{
    const bool isSidecar = static_cast<AIBackendType>(m_backendCombo->currentData().toInt()) == AIBackendType::Sidecar;
    m_pythonExecutableLabel->setVisible(isSidecar);
    m_pythonExecutableEdit->setVisible(isSidecar);
    m_pythonSidecarLabel->setVisible(isSidecar);
    m_pythonSidecarEdit->setVisible(isSidecar);
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

    if (nextConfig.backendType == AIBackendType::Sidecar
        && (nextConfig.pythonExecutable.trimmed().isEmpty()
            || nextConfig.pythonSidecarDirectory.trimmed().isEmpty())) {
        QMessageBox::warning(
            this,
            dialogText(nextConfig.language, QStringLiteral("Missing sidecar settings"), QStringLiteral("Sidecar 配置不完整")),
            dialogText(nextConfig.language,
                       QStringLiteral("Python command and sidecar directory are required for the Python backend."),
                       QStringLiteral("使用 Python 能力层时，Python 命令和 sidecar 目录不能为空。")));
        return;
    }

    if (nextConfig.agentWorkspaceDirectory.trimmed().isEmpty()) {
        m_agentWorkspaceEdit->setText(AppConfig::defaultAgentWorkspaceDirectory());
    }
    if (nextConfig.agentProjectDirectory.trimmed().isEmpty()) {
        m_agentProjectEdit->setText(AppConfig::defaultAgentProjectDirectory());
    }

    QDialog::accept();
}
