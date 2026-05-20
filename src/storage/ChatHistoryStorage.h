#pragma once

#include "core/ChatSession.h"

#include <optional>

class ChatHistoryStorage
{
public:
    explicit ChatHistoryStorage(const QString &databasePath = defaultDatabasePath());
    ~ChatHistoryStorage();

    static QString defaultDatabasePath();

    bool initialize(QString *errorMessage = nullptr);
    bool saveSession(const ChatSession &session, QString *errorMessage = nullptr);
    bool saveMessage(const ChatMessage &message, QString *errorMessage = nullptr);
    std::optional<ChatSession> loadLatestSession(QString *errorMessage = nullptr) const;
    bool clearSession(const QString &sessionId, QString *errorMessage = nullptr);

private:
    bool ensureOpen(QString *errorMessage = nullptr) const;
    static void setError(QString *errorMessage, const QString &message);

    QString m_databasePath;
    QString m_connectionName;
};
