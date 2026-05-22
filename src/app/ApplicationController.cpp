#include "app/ApplicationController.h"

#include <QDateTime>

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
{
    connect(&m_aiClient, &OpenAICompatibleClient::textDeltaReceived, this, &ApplicationController::handleTextDelta);
    connect(&m_aiClient, &OpenAICompatibleClient::requestFinished, this, &ApplicationController::handleRequestFinished);
    connect(&m_aiClient, &OpenAICompatibleClient::requestFailed, this, &ApplicationController::handleRequestFailed);
}

void ApplicationController::initialize()
{
    m_config = m_configStorage.load();
    emit configChanged();

    QString templateError;
    m_promptTemplates = m_promptTemplateStorage.load(&templateError);
    if (!templateError.isEmpty()) {
        emit startupWarning(
            QStringLiteral("Prompt templates are unavailable. Default templates will be used.\n\n%1").arg(templateError),
            QStringLiteral("角色提示词模板不可用，将使用默认模板。\n\n%1").arg(templateError));
    }
    emit promptTemplatesChanged();

    QString error;
    if (!m_chatHistoryStorage.initialize(&error)) {
        m_session = ChatSession::createDefault();
        m_sessionSummaries.clear();
        m_historyAvailable = false;
        emit startupWarning(
            QStringLiteral("Chat history is unavailable. New messages will not be restored after restart.\n\n%1").arg(error),
            QStringLiteral("聊天记录不可用。新消息在重启后将无法恢复。\n\n%1").arg(error));
        emit sessionListChanged();
        emit currentSessionChanged();
        return;
    }

    m_historyAvailable = true;
    m_sessionSummaries = m_chatHistoryStorage.loadSessionSummaries(&error);
    if (m_sessionSummaries.isEmpty()) {
        m_session = ChatSession::createDefault();
        if (!error.isEmpty()) {
            emit startupWarning(
                QStringLiteral("Failed to load the latest chat history.\n\n%1").arg(error),
                QStringLiteral("加载最近聊天记录失败。\n\n%1").arg(error));
        }
    } else {
        const std::optional<ChatSession> latestSession = m_chatHistoryStorage.loadSession(m_sessionSummaries.first().id, &error);
        m_session = latestSession.value_or(ChatSession::createDefault());
        if (!latestSession.has_value() && !error.isEmpty()) {
            emit startupWarning(
                QStringLiteral("Failed to load the latest chat history.\n\n%1").arg(error),
                QStringLiteral("加载最近聊天记录失败。\n\n%1").arg(error));
        }
    }

    emit sessionListChanged();
    emit currentSessionChanged();
}

const AppConfig &ApplicationController::config() const
{
    return m_config;
}

const ChatSession &ApplicationController::currentSession() const
{
    return m_session;
}

const QVector<ChatSession> &ApplicationController::sessionSummaries() const
{
    return m_sessionSummaries;
}

const QVector<PromptTemplate> &ApplicationController::promptTemplates() const
{
    return m_promptTemplates;
}

bool ApplicationController::isGenerating() const
{
    return m_isGenerating;
}

void ApplicationController::saveConfig(const AppConfig &config)
{
    m_config = config;
    QString error;
    if (!m_configStorage.save(m_config, &error)) {
        emit statusMessage(QStringLiteral("Failed to save API Key securely: %1").arg(error),
                           QStringLiteral("API Key 安全保存失败：%1").arg(error),
                           6000);
    }
    emit configChanged();
}

void ApplicationController::savePromptTemplates(const QVector<PromptTemplate> &templates)
{
    m_promptTemplates = templates;

    QString error;
    if (!m_promptTemplateStorage.save(m_promptTemplates, &error)) {
        emit statusMessage(QStringLiteral("Failed to save prompt templates: %1").arg(error),
                           QStringLiteral("保存角色提示词模板失败：%1").arg(error),
                           6000);
        return;
    }

    emit promptTemplatesChanged();
}

void ApplicationController::setSystemPrompt(const QString &prompt)
{
    if (m_isGenerating) {
        return;
    }

    m_session.systemPrompt = prompt.trimmed();
    m_session.updatedAt = QDateTime::currentDateTimeUtc();
    if (!saveCurrentSession()) {
        emit statusMessage(QStringLiteral("Failed to save the role prompt."),
                           QStringLiteral("保存角色提示词失败。"),
                           6000);
    }

    emit currentSessionChanged();
}

void ApplicationController::startNewChat()
{
    if (m_isGenerating) {
        return;
    }

    if (m_session.messages.isEmpty() && !m_session.hasSystemPrompt()) {
        emit currentSessionChanged();
        return;
    }

    if (hasPersistableCurrentSession()) {
        saveCurrentSession();
    }

    m_session = ChatSession::createDefault();
    m_currentAssistantContent.clear();
    saveCurrentSession();

    emit sessionListChanged();
    emit currentSessionChanged();
}

void ApplicationController::switchToSession(const QString &sessionId)
{
    if (m_isGenerating || sessionId.isEmpty()) {
        return;
    }

    if (sessionId == m_session.id) {
        emit currentSessionChanged();
        return;
    }

    if (hasPersistableCurrentSession()) {
        saveCurrentSession();
    }

    QString error;
    const std::optional<ChatSession> loaded = m_chatHistoryStorage.loadSession(sessionId, &error);
    if (!loaded.has_value()) {
        emit statusMessage(QStringLiteral("Failed to load chat session: %1").arg(error.isEmpty() ? sessionId : error),
                           QStringLiteral("加载会话失败：%1").arg(error.isEmpty() ? sessionId : error),
                           6000);
        emit currentSessionChanged();
        return;
    }

    m_session = loaded.value();
    m_currentAssistantContent.clear();
    emit currentSessionChanged();
}

void ApplicationController::deleteCurrentSession()
{
    if (m_isGenerating) {
        return;
    }

    const QString deletedSessionId = m_session.id;
    if (m_historyAvailable) {
        QString error;
        if (!m_chatHistoryStorage.clearSession(deletedSessionId, &error)) {
            emit statusMessage(QStringLiteral("Failed to delete chat session: %1").arg(error),
                               QStringLiteral("删除会话失败：%1").arg(error),
                               6000);
            return;
        }
    }

    for (int index = 0; index < m_sessionSummaries.size(); ++index) {
        if (m_sessionSummaries[index].id == deletedSessionId) {
            m_sessionSummaries.removeAt(index);
            break;
        }
    }

    m_currentAssistantContent.clear();

    if (m_sessionSummaries.isEmpty()) {
        m_session = ChatSession::createDefault();
        saveCurrentSession();
    } else {
        QString error;
        const std::optional<ChatSession> nextSession = m_chatHistoryStorage.loadSession(m_sessionSummaries.first().id, &error);
        if (nextSession.has_value()) {
            m_session = nextSession.value();
        } else {
            m_session = ChatSession::createDefault();
            emit statusMessage(QStringLiteral("Failed to load the next chat session: %1").arg(error),
                               QStringLiteral("加载下一个会话失败：%1").arg(error),
                               6000);
        }
    }

    emit sessionListChanged();
    emit currentSessionChanged();
}

void ApplicationController::sendMessage(const QString &content)
{
    if (m_isGenerating) {
        return;
    }

    const QString trimmedContent = content.trimmed();
    if (trimmedContent.isEmpty()) {
        return;
    }

    if (!m_config.isComplete()) {
        emit configurationMissing();
        return;
    }

    if (m_session.messages.isEmpty()) {
        m_session.title = trimmedContent.left(36);
        emit currentChatCleared();
        upsertCurrentSessionSummary(true);
        emit sessionListChanged();
    }

    m_session.addMessage(MessageRole::User, trimmedContent);
    emit userMessageAdded(trimmedContent);

    m_session.addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    emit assistantMessageStarted();

    setGenerating(true);
    m_aiClient.sendChat(m_config, m_session);
}

void ApplicationController::cancelCurrentRequest()
{
    if (!m_isGenerating) {
        return;
    }

    m_aiClient.cancel();
    setGenerating(false);
    if (!m_session.messages.isEmpty() && m_session.messages.last().content.isEmpty()) {
        m_session.messages.last().content = text(QStringLiteral("(Stopped)"), QStringLiteral("（已停止）"));
        emit assistantMessageUpdated(m_session.messages.last().content);
    }
    saveCurrentSession();
    emit statusMessage(QStringLiteral("Generation stopped."),
                       QStringLiteral("已停止生成。"),
                       2500);
}

void ApplicationController::handleTextDelta(const QString &delta)
{
    m_currentAssistantContent += delta;
    if (!m_session.messages.isEmpty()) {
        m_session.messages.last().content = m_currentAssistantContent;
    }

    emit assistantMessageUpdated(m_currentAssistantContent);
}

void ApplicationController::handleRequestFinished()
{
    setGenerating(false);
    if (!m_session.messages.isEmpty() && m_session.messages.last().content.isEmpty()) {
        m_session.messages.last().content = text(QStringLiteral("(Empty response)"), QStringLiteral("（空回复）"));
        emit assistantMessageUpdated(m_session.messages.last().content);
    }

    saveCurrentSession();
}

void ApplicationController::handleRequestFailed(const QString &message)
{
    setGenerating(false);
    const QString errorMessage = text(QStringLiteral("Request failed: %1"), QStringLiteral("请求失败：%1")).arg(message);
    const QString displayMessage = m_currentAssistantContent.isEmpty()
                                       ? errorMessage
                                       : QStringLiteral("%1\n\n%2").arg(m_currentAssistantContent, errorMessage);

    if (!m_session.messages.isEmpty()) {
        m_session.messages.last().content = displayMessage;
    }

    emit assistantMessageUpdated(displayMessage);
    saveCurrentSession();
}

void ApplicationController::setGenerating(bool generating)
{
    if (m_isGenerating == generating) {
        return;
    }

    m_isGenerating = generating;
    emit generatingChanged(m_isGenerating);
}

bool ApplicationController::saveCurrentSession()
{
    if (!m_historyAvailable) {
        return false;
    }

    QString error;
    if (!m_chatHistoryStorage.saveSession(m_session, &error)) {
        emit statusMessage(QStringLiteral("Failed to save chat history: %1").arg(error),
                           QStringLiteral("保存聊天记录失败：%1").arg(error),
                           6000);
        return false;
    }

    for (const ChatMessage &message : m_session.messages) {
        if (!m_chatHistoryStorage.saveMessage(message, &error)) {
            emit statusMessage(QStringLiteral("Failed to save chat history: %1").arg(error),
                               QStringLiteral("保存聊天记录失败：%1").arg(error),
                               6000);
            return false;
        }
    }

    upsertCurrentSessionSummary(true);
    emit sessionListChanged();
    return true;
}

void ApplicationController::upsertCurrentSessionSummary(bool moveToTop)
{
    int existingIndex = -1;
    for (int index = 0; index < m_sessionSummaries.size(); ++index) {
        if (m_sessionSummaries[index].id == m_session.id) {
            existingIndex = index;
            break;
        }
    }

    if (existingIndex >= 0) {
        m_sessionSummaries.removeAt(existingIndex);
    }

    if (moveToTop || existingIndex < 0) {
        m_sessionSummaries.prepend(m_session);
    } else {
        m_sessionSummaries.insert(existingIndex, m_session);
    }
}

bool ApplicationController::hasPersistableCurrentSession() const
{
    return !m_session.messages.isEmpty() || m_session.hasSystemPrompt();
}

QString ApplicationController::text(const QString &english, const QString &chinese) const
{
    return m_config.language == AppLanguage::English ? english : chinese;
}
