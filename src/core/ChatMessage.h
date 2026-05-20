#pragma once

#include "core/MessageRole.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

struct ChatMessage {
    QString id;
    QString sessionId;
    MessageRole role = MessageRole::User;
    QString content;
    QDateTime createdAt;

    static ChatMessage create(const QString &sessionId, MessageRole role, const QString &content)
    {
        ChatMessage message;
        message.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        message.sessionId = sessionId;
        message.role = role;
        message.content = content;
        message.createdAt = QDateTime::currentDateTimeUtc();
        return message;
    }
};
