#include "storage/ConfigStorage.h"

#include "storage/CredentialStorage.h"

#include <QSettings>
#include <QUuid>
#include <cassert>
#include <memory>

class FakeCredentialStorage final : public CredentialStorage
{
public:
    QString apiKey;
    bool failRead = false;
    bool failWrite = false;
    bool failDelete = false;
    bool deleteCalled = false;

    QString readApiKey(QString *error = nullptr) const override
    {
        if (failRead) {
            if (error != nullptr) {
                *error = QStringLiteral("read failed");
            }
            return {};
        }

        return apiKey;
    }

    bool writeApiKey(const QString &nextApiKey, QString *error = nullptr) override
    {
        if (failWrite) {
            if (error != nullptr) {
                *error = QStringLiteral("write failed");
            }
            return false;
        }

        apiKey = nextApiKey;
        return true;
    }

    bool deleteApiKey(QString *error = nullptr) override
    {
        deleteCalled = true;
        if (failDelete) {
            if (error != nullptr) {
                *error = QStringLiteral("delete failed");
            }
            return false;
        }

        apiKey.clear();
        return true;
    }
};

QString uniqueApplicationName()
{
    return QStringLiteral("ConfigStorageTest_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

void clearSettings(const QString &organizationName, const QString &applicationName)
{
    QSettings settings(organizationName, applicationName);
    settings.clear();
    settings.sync();
}

int main()
{
    const QString organizationName = QStringLiteral("AIChatDesktopTests");

    {
        const QString applicationName = uniqueApplicationName();
        clearSettings(organizationName, applicationName);

        auto credentials = std::make_shared<FakeCredentialStorage>();
        ConfigStorage storage(credentials, organizationName, applicationName);

        AppConfig config = AppConfig::defaultConfig();
        config.providerName = QStringLiteral("Custom");
        config.baseUrl = QStringLiteral("https://example.com/v1");
        config.modelName = QStringLiteral("example-model");
        config.apiKey = QStringLiteral("secret-key");
        config.temperature = 0.4;
        config.maxTokens = 1024;
        config.language = AppLanguage::English;
        config.agentWorkspaceDirectory = QStringLiteral("D:/agent-workspace");
        config.agentProjectDirectory = QStringLiteral("D:/agent-project");
        config.backendType = AIBackendType::Sidecar;
        config.pythonExecutable = QStringLiteral("D:/Python/python.exe");
        config.pythonSidecarDirectory = QStringLiteral("D:/agent-sidecar");

        QString error;
        assert(storage.save(config, &error));
        assert(credentials->apiKey == QStringLiteral("secret-key"));

        QSettings settings(organizationName, applicationName);
        assert(!settings.contains(QStringLiteral("api/apiKey")));

        const AppConfig loaded = storage.load();
        assert(loaded.providerName == config.providerName);
        assert(loaded.baseUrl == config.baseUrl);
        assert(loaded.modelName == config.modelName);
        assert(loaded.apiKey == config.apiKey);
        assert(loaded.temperature.has_value());
        assert(loaded.temperature.value() > 0.39);
        assert(loaded.temperature.value() < 0.41);
        assert(loaded.maxTokens.has_value());
        assert(loaded.maxTokens.value() == 1024);
        assert(loaded.language == AppLanguage::English);
        assert(loaded.agentWorkspaceDirectory == config.agentWorkspaceDirectory);
        assert(loaded.agentProjectDirectory == config.agentProjectDirectory);
        assert(loaded.backendType == AIBackendType::Sidecar);
        assert(loaded.pythonExecutable == config.pythonExecutable);
        assert(loaded.pythonSidecarDirectory == config.pythonSidecarDirectory);

        clearSettings(organizationName, applicationName);
    }

    {
        const QString applicationName = uniqueApplicationName();
        clearSettings(organizationName, applicationName);

        QSettings settings(organizationName, applicationName);
        settings.setValue(QStringLiteral("api/apiKey"), QStringLiteral("legacy-key"));
        settings.sync();

        auto credentials = std::make_shared<FakeCredentialStorage>();
        ConfigStorage storage(credentials, organizationName, applicationName);

        const AppConfig loaded = storage.load();
        assert(loaded.apiKey == QStringLiteral("legacy-key"));
        assert(credentials->apiKey == QStringLiteral("legacy-key"));

        QSettings verifySettings(organizationName, applicationName);
        assert(!verifySettings.contains(QStringLiteral("api/apiKey")));

        clearSettings(organizationName, applicationName);
    }

    {
        const QString applicationName = uniqueApplicationName();
        clearSettings(organizationName, applicationName);

        auto credentials = std::make_shared<FakeCredentialStorage>();
        credentials->apiKey = QStringLiteral("old-key");
        ConfigStorage storage(credentials, organizationName, applicationName);

        AppConfig config = AppConfig::defaultConfig();
        config.apiKey.clear();

        QString error;
        assert(storage.save(config, &error));
        assert(credentials->deleteCalled);
        assert(credentials->apiKey.isEmpty());

        clearSettings(organizationName, applicationName);
    }

    {
        const QString applicationName = uniqueApplicationName();
        clearSettings(organizationName, applicationName);

        auto credentials = std::make_shared<FakeCredentialStorage>();
        credentials->failWrite = true;
        ConfigStorage storage(credentials, organizationName, applicationName);

        AppConfig config = AppConfig::defaultConfig();
        config.providerName = QStringLiteral("StillSaved");
        config.apiKey = QStringLiteral("not-persisted");
        config.temperature = 0.8;
        config.maxTokens = 4096;

        QString error;
        assert(!storage.save(config, &error));
        assert(!error.isEmpty());

        QSettings settings(organizationName, applicationName);
        assert(settings.value(QStringLiteral("api/providerName")).toString() == QStringLiteral("StillSaved"));
        assert(settings.value(QStringLiteral("api/temperature")).toDouble() > 0.79);
        assert(settings.value(QStringLiteral("api/temperature")).toDouble() < 0.81);
        assert(settings.value(QStringLiteral("api/maxTokens")).toInt() == 4096);
        assert(!settings.value(QStringLiteral("agent/workspaceDirectory")).toString().trimmed().isEmpty());
        assert(!settings.value(QStringLiteral("agent/projectDirectory")).toString().trimmed().isEmpty());
        assert(!settings.value(QStringLiteral("python/executable")).toString().trimmed().isEmpty());
        assert(!settings.value(QStringLiteral("python/sidecarDirectory")).toString().trimmed().isEmpty());
        assert(!settings.contains(QStringLiteral("api/apiKey")));

        clearSettings(organizationName, applicationName);
    }

    return 0;
}
