#include "ui/SettingsDialog.h"

#include <QApplication>
#include <QComboBox>
#include <QLineEdit>

#include <cassert>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AppConfig config = AppConfig::defaultConfig();
    config.apiKey = QStringLiteral("test-key");
    config.language = AppLanguage::Chinese;

    SettingsDialog dialog(config);
    const AppConfig readBack = dialog.config();

    assert(readBack.providerName == config.providerName);
    assert(readBack.baseUrl == config.baseUrl);
    assert(readBack.modelName == config.modelName);
    assert(readBack.apiKey == config.apiKey);
    assert(readBack.language == AppLanguage::Chinese);

    QComboBox *providerCombo = dialog.findChild<QComboBox *>(QStringLiteral("providerCombo"));
    assert(providerCombo != nullptr);
    const int openAIIndex = providerCombo->findText(QStringLiteral("OpenAI"));
    assert(openAIIndex >= 0);
    providerCombo->setCurrentIndex(openAIIndex);

    const AppConfig openAIConfig = dialog.config();
    assert(openAIConfig.providerName == QStringLiteral("OpenAI"));
    assert(openAIConfig.baseUrl == QStringLiteral("https://api.openai.com/v1"));
    assert(openAIConfig.modelName == QStringLiteral("gpt-4.1-mini"));

    const int customIndex = providerCombo->findData(QStringLiteral("custom"));
    assert(customIndex >= 0);
    providerCombo->setCurrentIndex(customIndex);

    QLineEdit *customProviderEdit = dialog.findChild<QLineEdit *>(QStringLiteral("customProviderEdit"));
    assert(customProviderEdit != nullptr);
    assert(!customProviderEdit->isHidden());
    customProviderEdit->setText(QStringLiteral("Local Provider"));

    const AppConfig customConfig = dialog.config();
    assert(customConfig.providerName == QStringLiteral("Local Provider"));

    QLineEdit *temperatureEdit = dialog.findChild<QLineEdit *>(QStringLiteral("temperatureEdit"));
    QLineEdit *maxTokensEdit = dialog.findChild<QLineEdit *>(QStringLiteral("maxTokensEdit"));

    assert(temperatureEdit != nullptr);
    assert(maxTokensEdit != nullptr);
    assert(!dialog.config().temperature.has_value());
    assert(!dialog.config().maxTokens.has_value());

    temperatureEdit->setText(QStringLiteral("0.7"));
    maxTokensEdit->setText(QStringLiteral("2048"));

    const AppConfig parameterConfig = dialog.config();
    assert(parameterConfig.temperature.has_value());
    assert(parameterConfig.temperature.value() > 0.69);
    assert(parameterConfig.temperature.value() < 0.71);
    assert(parameterConfig.maxTokens.has_value());
    assert(parameterConfig.maxTokens.value() == 2048);

    return 0;
}
