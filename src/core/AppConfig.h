#pragma once

#include "core/AppLanguage.h"

#include <optional>
#include <QString>

struct AppConfig {
    QString providerName;
    QString baseUrl;
    QString modelName;
    QString apiKey;
    std::optional<double> temperature;
    std::optional<int> maxTokens;
    AppLanguage language = AppLanguage::Chinese;

    static AppConfig defaultConfig()
    {
        AppConfig config;
        config.providerName = QStringLiteral("DeepSeek");
        config.baseUrl = QStringLiteral("https://api.deepseek.com");
        config.modelName = QStringLiteral("deepseek-v4-flash");
        config.language = AppLanguage::Chinese;
        return config;
    }

    bool isComplete() const
    {
        return !baseUrl.trimmed().isEmpty() &&
               !modelName.trimmed().isEmpty() &&
               !apiKey.trimmed().isEmpty();
    }
};
