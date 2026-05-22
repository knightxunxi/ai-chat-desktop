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
        config.language = AppLanguage::English;

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
        assert(loaded.language == AppLanguage::English);

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

        QString error;
        assert(!storage.save(config, &error));
        assert(!error.isEmpty());

        QSettings settings(organizationName, applicationName);
        assert(settings.value(QStringLiteral("api/providerName")).toString() == QStringLiteral("StillSaved"));
        assert(!settings.contains(QStringLiteral("api/apiKey")));

        clearSettings(organizationName, applicationName);
    }

    return 0;
}
