#pragma once

#include "core/ChatMessage.h"

#include <QDateTime>
#include <QString>
#include <QVector>
#include <QUuid>

struct ChatSession {
    QString id;
    QString title;
    QString systemPrompt;
    QDateTime createdAt;
    QDateTime updatedAt;
    QVector<ChatMessage> messages;

    static ChatSession createDefault()
    {
        ChatSession session;
        session.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        session.title = QStringLiteral("New Chat");
        session.createdAt = QDateTime::currentDateTimeUtc();
        session.updatedAt = session.createdAt;
        return session;
    }

    bool hasSystemPrompt() const
    {
        return !systemPrompt.trimmed().isEmpty();
    }

    ChatMessage addMessage(MessageRole role, const QString &content)
    {
        ChatMessage message = ChatMessage::create(id, role, content);
        messages.append(message);
        updatedAt = message.createdAt;
        return message;
    }
};
