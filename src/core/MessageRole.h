#pragma once

#include <QString>

enum class MessageRole {
    System,
    User,
    Assistant
};

inline QString messageRoleToString(MessageRole role)
{
    switch (role) {
    case MessageRole::System:
        return QStringLiteral("system");
    case MessageRole::User:
        return QStringLiteral("user");
    case MessageRole::Assistant:
        return QStringLiteral("assistant");
    }

    return QStringLiteral("user");
}

inline MessageRole messageRoleFromString(const QString &value)
{
    if (value.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0) {
        return MessageRole::System;
    }

    if (value.compare(QStringLiteral("assistant"), Qt::CaseInsensitive) == 0) {
        return MessageRole::Assistant;
    }

    return MessageRole::User;
}
