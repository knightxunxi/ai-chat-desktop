#pragma once

#include "storage/CredentialStorage.h"

class WindowsCredentialStorage final : public CredentialStorage
{
public:
    QString readApiKey(QString *error = nullptr) const override;
    bool writeApiKey(const QString &apiKey, QString *error = nullptr) override;
    bool deleteApiKey(QString *error = nullptr) override;
};
