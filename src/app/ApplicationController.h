#pragma once

#include "core/AppConfig.h"
#include "core/ChatSession.h"
#include "core/PromptTemplate.h"
#include "services/RequestErrorCategory.h"
#include "services/OpenAICompatibleClient.h"
#include "storage/ChatHistoryStorage.h"
#include "storage/ConfigStorage.h"
#include "storage/PromptTemplateStorage.h"

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
    const QVector<PromptTemplate> &promptTemplates() const;
    bool isGenerating() const;
    bool exportCurrentSessionMarkdown(const QString &filePath, QString *error = nullptr) const;

public slots:
    void saveConfig(const AppConfig &config);
    void savePromptTemplates(const QVector<PromptTemplate> &templates);
    void setSystemPrompt(const QString &prompt);
    void renameCurrentSession(const QString &title);
    void searchSessions(const QString &query);
    void startNewChat();
    void switchToSession(const QString &sessionId);
    void deleteCurrentSession();
    void sendMessage(const QString &content);
    void cancelCurrentRequest();
    void retryLastRequest();

signals:
    void configChanged();
    void promptTemplatesChanged();
    void sessionListChanged();
    void currentSessionChanged();
    void currentChatCleared();
    void userMessageAdded(const QString &content);
    void assistantMessageStarted();
    void assistantMessageUpdated(const QString &content);
    void generatingChanged(bool generating);
    void retryAvailableChanged(bool available);
    void configurationMissing();
    void statusMessage(const QString &english, const QString &chinese, int timeoutMs);
    void startupWarning(const QString &english, const QString &chinese);

private:
    void handleTextDelta(const QString &delta);
    void handleRequestFinished();
    void handleRequestFailed(const QString &message, RequestErrorCategory category);
    void startAssistantRequest(const QString &userContentForRetry);
    void setGenerating(bool generating);
    void setRetryAvailable(bool available, const QString &userContent = QString());
    QString requestFailureMessage(RequestErrorCategory category, const QString &detail) const;
    bool saveCurrentSession(bool moveToTop = true);
    bool reloadSessionSummaries(QString *error = nullptr);
    void upsertCurrentSessionSummary(bool moveToTop);
    bool hasPersistableCurrentSession() const;
    QString text(const QString &english, const QString &chinese) const;

    AppConfig m_config;
    ChatSession m_session;
    QVector<ChatSession> m_sessionSummaries;
    QVector<PromptTemplate> m_promptTemplates;
    ConfigStorage m_configStorage;
    ChatHistoryStorage m_chatHistoryStorage;
    PromptTemplateStorage m_promptTemplateStorage;
    OpenAICompatibleClient m_aiClient;
    QString m_currentAssistantContent;
    QString m_lastRequestUserContent;
    QString m_retryUserContent;
    QString m_sessionSearchQuery;
    bool m_historyAvailable = false;
    bool m_isGenerating = false;
    bool m_retryAvailable = false;
};
