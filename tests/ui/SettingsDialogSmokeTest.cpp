#include "ui/SettingsDialog.h"

#include <QApplication>

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

    return 0;
}
