#include "storage/ConfigStorage.h"

#include <QSettings>

namespace {

constexpr auto ProviderNameKey = "api/providerName";
constexpr auto BaseUrlKey = "api/baseUrl";
constexpr auto ModelNameKey = "api/modelName";
constexpr auto ApiKeyKey = "api/apiKey";
constexpr auto LanguageKey = "ui/language";

QSettings createSettings()
{
    return QSettings(QStringLiteral("AIChatDesktop"), QStringLiteral("AIChatDesktop"));
}

} // namespace

AppConfig ConfigStorage::load() const
{
    const AppConfig defaults = AppConfig::defaultConfig();
    QSettings settings = createSettings();

    AppConfig config;
    config.providerName = settings.value(ProviderNameKey, defaults.providerName).toString();
    config.baseUrl = settings.value(BaseUrlKey, defaults.baseUrl).toString();
    config.modelName = settings.value(ModelNameKey, defaults.modelName).toString();
    config.apiKey = settings.value(ApiKeyKey, defaults.apiKey).toString();
    config.language = appLanguageFromString(settings.value(LanguageKey, appLanguageToString(defaults.language)).toString());

    return config;
}

void ConfigStorage::save(const AppConfig &config) const
{
    QSettings settings = createSettings();
    settings.setValue(ProviderNameKey, config.providerName);
    settings.setValue(BaseUrlKey, config.baseUrl);
    settings.setValue(ModelNameKey, config.modelName);
    settings.setValue(ApiKeyKey, config.apiKey);
    settings.setValue(LanguageKey, appLanguageToString(config.language));
    settings.sync();
}
