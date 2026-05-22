#pragma once

#include <QString>

class CredentialStorage
{
public:
    virtual ~CredentialStorage() = default;

    virtual QString readApiKey(QString *error = nullptr) const = 0;
    virtual bool writeApiKey(const QString &apiKey, QString *error = nullptr) = 0;
    virtual bool deleteApiKey(QString *error = nullptr) = 0;
};
