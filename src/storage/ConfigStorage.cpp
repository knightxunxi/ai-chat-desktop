#include "storage/ConfigStorage.h"

#include "storage/CredentialStorage.h"
#include "storage/WindowsCredentialStorage.h"

#include <QSettings>
#include <utility>

namespace {

constexpr auto ProviderNameKey = "api/providerName";
constexpr auto BaseUrlKey = "api/baseUrl";
constexpr auto ModelNameKey = "api/modelName";
constexpr auto ApiKeyKey = "api/apiKey";
constexpr auto TemperatureKey = "api/temperature";
constexpr auto MaxTokensKey = "api/maxTokens";
constexpr auto LanguageKey = "ui/language";
constexpr auto DefaultOrganizationName = "AIChatDesktop";
constexpr auto DefaultApplicationName = "AIChatDesktop";

void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

} // namespace

ConfigStorage::ConfigStorage()
    : ConfigStorage(std::make_shared<WindowsCredentialStorage>(),
                    QString::fromLatin1(DefaultOrganizationName),
                    QString::fromLatin1(DefaultApplicationName))
{
}

ConfigStorage::ConfigStorage(std::shared_ptr<CredentialStorage> credentialStorage,
                             const QString &organizationName,
                             const QString &applicationName)
    : m_credentialStorage(std::move(credentialStorage))
    , m_organizationName(organizationName)
    , m_applicationName(applicationName)
{
    if (!m_credentialStorage) {
        m_credentialStorage = std::make_shared<WindowsCredentialStorage>();
    }
}

AppConfig ConfigStorage::load() const
{
    const AppConfig defaults = AppConfig::defaultConfig();
    QSettings settings(m_organizationName, m_applicationName);

    AppConfig config;
    config.providerName = settings.value(ProviderNameKey, defaults.providerName).toString();
    config.baseUrl = settings.value(BaseUrlKey, defaults.baseUrl).toString();
    config.modelName = settings.value(ModelNameKey, defaults.modelName).toString();
    config.apiKey = m_credentialStorage->readApiKey();
    if (settings.contains(TemperatureKey)) {
        config.temperature = settings.value(TemperatureKey).toDouble();
    }
    if (settings.contains(MaxTokensKey)) {
        config.maxTokens = settings.value(MaxTokensKey).toInt();
    }
    config.language = appLanguageFromString(settings.value(LanguageKey, appLanguageToString(defaults.language)).toString());

    const QString legacyApiKey = settings.value(ApiKeyKey).toString();
    if (config.apiKey.trimmed().isEmpty() && !legacyApiKey.trimmed().isEmpty()) {
        QString migrationError;
        if (m_credentialStorage->writeApiKey(legacyApiKey, &migrationError)) {
            config.apiKey = legacyApiKey;
            settings.remove(ApiKeyKey);
            settings.sync();
        } else {
            config.apiKey = legacyApiKey;
        }
    } else if (settings.contains(ApiKeyKey)) {
        settings.remove(ApiKeyKey);
        settings.sync();
    }

    return config;
}

bool ConfigStorage::save(const AppConfig &config, QString *error) const
{
    QSettings settings(m_organizationName, m_applicationName);
    settings.setValue(ProviderNameKey, config.providerName);
    settings.setValue(BaseUrlKey, config.baseUrl);
    settings.setValue(ModelNameKey, config.modelName);
    if (config.temperature.has_value()) {
        settings.setValue(TemperatureKey, config.temperature.value());
    } else {
        settings.remove(TemperatureKey);
    }
    if (config.maxTokens.has_value()) {
        settings.setValue(MaxTokensKey, config.maxTokens.value());
    } else {
        settings.remove(MaxTokensKey);
    }
    settings.remove(ApiKeyKey);
    settings.setValue(LanguageKey, appLanguageToString(config.language));
    settings.sync();

    if (settings.status() != QSettings::NoError) {
        setError(error, QStringLiteral("Failed to save local settings."));
        return false;
    }

    if (config.apiKey.trimmed().isEmpty()) {
        return m_credentialStorage->deleteApiKey(error);
    }

    return m_credentialStorage->writeApiKey(config.apiKey, error);
}
