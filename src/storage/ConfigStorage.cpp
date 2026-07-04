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
constexpr auto BackendTypeKey = "api/backendType";
constexpr auto TemperatureKey = "api/temperature";
constexpr auto MaxTokensKey = "api/maxTokens";
constexpr auto LanguageKey = "ui/language";
constexpr auto AgentWorkspaceDirectoryKey = "agent/workspaceDirectory";
constexpr auto AgentProjectDirectoryKey = "agent/projectDirectory";
constexpr auto PythonExecutableKey = "python/executable";
constexpr auto PythonSidecarDirectoryKey = "python/sidecarDirectory";
constexpr auto DefaultOrganizationName = "AIChatDesktop";
constexpr auto DefaultApplicationName = "AIChatDesktop";

void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

QString backendTypeToString(AIBackendType backendType)
{
    switch (backendType) {
    case AIBackendType::Direct:
        return QStringLiteral("direct");
    case AIBackendType::Sidecar:
        return QStringLiteral("sidecar");
    }
    return QStringLiteral("direct");
}

AIBackendType backendTypeFromString(const QString &value)
{
    if (value.compare(QStringLiteral("sidecar"), Qt::CaseInsensitive) == 0) {
        return AIBackendType::Sidecar;
    }
    return AIBackendType::Direct;
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
    config.backendType = backendTypeFromString(
        settings.value(BackendTypeKey, backendTypeToString(defaults.backendType)).toString());
    config.apiKey = m_credentialStorage->readApiKey();
    if (settings.contains(TemperatureKey)) {
        config.temperature = settings.value(TemperatureKey).toDouble();
    }
    if (settings.contains(MaxTokensKey)) {
        config.maxTokens = settings.value(MaxTokensKey).toInt();
    }
    config.language = appLanguageFromString(settings.value(LanguageKey, appLanguageToString(defaults.language)).toString());
    config.agentWorkspaceDirectory = settings.value(AgentWorkspaceDirectoryKey, defaults.agentWorkspaceDirectory).toString();
    if (config.agentWorkspaceDirectory.trimmed().isEmpty()) {
        config.agentWorkspaceDirectory = defaults.agentWorkspaceDirectory;
    }
    config.agentProjectDirectory = settings.value(AgentProjectDirectoryKey, defaults.agentProjectDirectory).toString();
    if (config.agentProjectDirectory.trimmed().isEmpty()) {
        config.agentProjectDirectory = defaults.agentProjectDirectory;
    }
    config.pythonExecutable = settings.value(PythonExecutableKey, defaults.pythonExecutable).toString();
    if (config.pythonExecutable.trimmed().isEmpty()) {
        config.pythonExecutable = defaults.pythonExecutable;
    }
    config.pythonSidecarDirectory = settings.value(PythonSidecarDirectoryKey, defaults.pythonSidecarDirectory).toString();
    if (config.pythonSidecarDirectory.trimmed().isEmpty()) {
        config.pythonSidecarDirectory = defaults.pythonSidecarDirectory;
    }

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
    settings.setValue(BackendTypeKey, backendTypeToString(config.backendType));
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
    settings.setValue(AgentWorkspaceDirectoryKey,
                      config.agentWorkspaceDirectory.trimmed().isEmpty()
                          ? AppConfig::defaultAgentWorkspaceDirectory()
                          : config.agentWorkspaceDirectory.trimmed());
    settings.setValue(AgentProjectDirectoryKey,
                      config.agentProjectDirectory.trimmed().isEmpty()
                          ? AppConfig::defaultAgentProjectDirectory()
                          : config.agentProjectDirectory.trimmed());
    settings.setValue(PythonExecutableKey,
                      config.pythonExecutable.trimmed().isEmpty()
                          ? AppConfig::defaultPythonExecutable()
                          : config.pythonExecutable.trimmed());
    settings.setValue(PythonSidecarDirectoryKey,
                      config.pythonSidecarDirectory.trimmed().isEmpty()
                          ? AppConfig::defaultPythonSidecarDirectory()
                          : config.pythonSidecarDirectory.trimmed());
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
