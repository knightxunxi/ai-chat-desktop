#pragma once

#include <QString>

enum class AppLanguage {
    Chinese,
    English
};

inline QString appLanguageToString(AppLanguage language)
{
    switch (language) {
    case AppLanguage::Chinese:
        return QStringLiteral("zh_CN");
    case AppLanguage::English:
        return QStringLiteral("en_US");
    }

    return QStringLiteral("zh_CN");
}

inline AppLanguage appLanguageFromString(const QString &value)
{
    if (value.compare(QStringLiteral("en_US"), Qt::CaseInsensitive) == 0 ||
        value.compare(QStringLiteral("english"), Qt::CaseInsensitive) == 0) {
        return AppLanguage::English;
    }

    return AppLanguage::Chinese;
}
