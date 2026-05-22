#pragma once

#include "core/AppConfig.h"

#include <memory>
#include <QString>

class CredentialStorage;

class ConfigStorage
{
public:
    ConfigStorage();
    ConfigStorage(std::shared_ptr<CredentialStorage> credentialStorage,
                  const QString &organizationName,
                  const QString &applicationName);

    AppConfig load() const;
    bool save(const AppConfig &config, QString *error = nullptr) const;

private:
    std::shared_ptr<CredentialStorage> m_credentialStorage;
    QString m_organizationName;
    QString m_applicationName;
};
