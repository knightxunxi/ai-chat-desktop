#pragma once

#include "core/AppConfig.h"
#include "core/ChatSession.h"
#include "services/OpenAICompatibleClient.h"
#include "storage/ChatHistoryStorage.h"
#include "storage/ConfigStorage.h"

#include <QObject>
#include <QVector>

class ApplicationController : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);

    void initialize();

    const AppConfig &config() const;
    const ChatSession &currentSession() const;
    const QVector<ChatSession> &sessionSummaries() const;
    bool isGenerating() const;

public slots:
    void saveConfig(const AppConfig &config);
    void setSystemPrompt(const QString &prompt);
    void startNewChat();
    void switchToSession(const QString &sessionId);
    void deleteCurrentSession();
    void sendMessage(const QString &content);
    void cancelCurrentRequest();

signals:
    void configChanged();
    void sessionListChanged();
    void currentSessionChanged();
    void currentChatCleared();
    void userMessageAdded(const QString &content);
    void assistantMessageStarted();
    void assistantMessageUpdated(const QString &content);
    void generatingChanged(bool generating);
    void configurationMissing();
    void statusMessage(const QString &english, const QString &chinese, int timeoutMs);
    void startupWarning(const QString &english, const QString &chinese);

private:
    void handleTextDelta(const QString &delta);
    void handleRequestFinished();
    void handleRequestFailed(const QString &message);
    void setGenerating(bool generating);
    bool saveCurrentSession();
    void upsertCurrentSessionSummary(bool moveToTop);
    bool hasPersistableCurrentSession() const;
    QString text(const QString &english, const QString &chinese) const;

    AppConfig m_config;
    ChatSession m_session;
    QVector<ChatSession> m_sessionSummaries;
    ConfigStorage m_configStorage;
    ChatHistoryStorage m_chatHistoryStorage;
    OpenAICompatibleClient m_aiClient;
    QString m_currentAssistantContent;
    bool m_historyAvailable = false;
    bool m_isGenerating = false;
};
