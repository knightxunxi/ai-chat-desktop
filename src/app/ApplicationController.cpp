#include "app/ApplicationController.h"

#include "app/SessionSummaryList.h"

#include "storage/ChatSessionExporter.h"

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
    reloadSessionSummaries(&error);
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

bool ApplicationController::exportCurrentSessionMarkdown(const QString &filePath, QString *error) const
{
    return ChatSessionExporter::writeMarkdown(m_session, filePath, error);
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

void ApplicationController::renameCurrentSession(const QString &title)
{
    if (m_isGenerating) {
        return;
    }

    const QString trimmedTitle = title.trimmed();
    if (trimmedTitle.isEmpty()) {
        emit statusMessage(QStringLiteral("Chat title cannot be empty."),
                           QStringLiteral("会话标题不能为空。"),
                           3000);
        return;
    }

    if (m_session.title == trimmedTitle) {
        emit currentSessionChanged();
        return;
    }

    const QString previousTitle = m_session.title;
    m_session.title = trimmedTitle;
    if (!saveCurrentSession(false)) {
        m_session.title = previousTitle;
        emit statusMessage(QStringLiteral("Failed to rename the chat session."),
                           QStringLiteral("重命名会话失败。"),
                           6000);
        return;
    }

    emit currentSessionChanged();
}

void ApplicationController::searchSessions(const QString &query)
{
    m_sessionSearchQuery = query.trimmed();
    if (!m_historyAvailable) {
        m_sessionSummaries.clear();
        emit sessionListChanged();
        return;
    }

    QString error;
    if (!reloadSessionSummaries(&error) && !error.isEmpty()) {
        emit statusMessage(QStringLiteral("Failed to search chat sessions: %1").arg(error),
                           QStringLiteral("搜索会话失败：%1").arg(error),
                           6000);
    }

    emit sessionListChanged();
}

void ApplicationController::startNewChat()
{
    if (m_isGenerating) {
        return;
    }

    setRetryAvailable(false);
    if (!hasPersistableCurrentSession()) {
        emit currentSessionChanged();
        return;
    }

    saveCurrentSession(false);

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

    setRetryAvailable(false);
    if (sessionId == m_session.id) {
        emit currentSessionChanged();
        return;
    }

    if (hasPersistableCurrentSession()) {
        saveCurrentSession(false);
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

    setRetryAvailable(false);
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

    QString error;
    if (!m_sessionSearchQuery.isEmpty()) {
        reloadSessionSummaries(&error);
    }

    if (m_sessionSummaries.isEmpty()) {
        m_session = ChatSession::createDefault();
        saveCurrentSession();
    } else {
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

    setRetryAvailable(false);
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

    startAssistantRequest(trimmedContent);
}

void ApplicationController::startAssistantRequest(const QString &userContentForRetry)
{
    m_lastRequestUserContent = userContentForRetry;
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
    setRetryAvailable(false);
    saveCurrentSession();
    emit statusMessage(QStringLiteral("Generation stopped."),
                       QStringLiteral("已停止生成。"),
                       2500);
}

void ApplicationController::retryLastRequest()
{
    if (m_isGenerating || !m_retryAvailable || m_retryUserContent.trimmed().isEmpty()) {
        return;
    }

    if (!m_config.isComplete()) {
        emit configurationMissing();
        return;
    }

    const QString retryContent = m_retryUserContent;
    setRetryAvailable(false);

    if (!m_session.messages.isEmpty() && m_session.messages.last().role == MessageRole::Assistant) {
        m_session.messages.removeLast();
    }

    m_currentAssistantContent.clear();
    emit currentSessionChanged();
    startAssistantRequest(retryContent);
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
    setRetryAvailable(false);
    if (!m_session.messages.isEmpty() && m_session.messages.last().content.isEmpty()) {
        m_session.messages.last().content = text(QStringLiteral("(Empty response)"), QStringLiteral("（空回复）"));
        emit assistantMessageUpdated(m_session.messages.last().content);
    }

    saveCurrentSession();
}

void ApplicationController::handleRequestFailed(const QString &message, RequestErrorCategory category)
{
    setGenerating(false);
    const QString errorMessage = requestFailureMessage(category, message);
    const QString displayMessage = m_currentAssistantContent.isEmpty()
                                       ? errorMessage
                                       : QStringLiteral("%1\n\n%2").arg(m_currentAssistantContent, errorMessage);

    if (!m_session.messages.isEmpty()) {
        m_session.messages.last().content = displayMessage;
    }

    emit assistantMessageUpdated(displayMessage);
    setRetryAvailable(true, m_lastRequestUserContent);
    emit statusMessage(QStringLiteral("Request failed. You can retry the last message."),
                       QStringLiteral("请求失败，可以重试上一条消息。"),
                       5000);
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

void ApplicationController::setRetryAvailable(bool available, const QString &userContent)
{
    const bool nextAvailable = available && !userContent.trimmed().isEmpty();
    const QString nextUserContent = nextAvailable ? userContent : QString();
    if (m_retryAvailable == nextAvailable && m_retryUserContent == nextUserContent) {
        return;
    }

    m_retryAvailable = nextAvailable;
    m_retryUserContent = nextUserContent;
    emit retryAvailableChanged(m_retryAvailable);
}

QString ApplicationController::requestFailureMessage(RequestErrorCategory category, const QString &detail) const
{
    QString summaryEnglish;
    QString summaryChinese;

    switch (category) {
    case RequestErrorCategory::Network:
        summaryEnglish = QStringLiteral("Network error. Check your connection or Base URL, then retry.");
        summaryChinese = QStringLiteral("网络错误。请检查网络连接或 Base URL，然后重试。");
        break;
    case RequestErrorCategory::Authentication:
        summaryEnglish = QStringLiteral("Authentication failed. Check whether the API Key is valid.");
        summaryChinese = QStringLiteral("认证失败。请检查 API Key 是否有效。");
        break;
    case RequestErrorCategory::Quota:
        summaryEnglish = QStringLiteral("Quota or rate limit reached. Check account quota or try again later.");
        summaryChinese = QStringLiteral("额度不足或请求过于频繁。请检查账户额度，或稍后再试。");
        break;
    case RequestErrorCategory::Model:
        summaryEnglish = QStringLiteral("Model or request configuration error. Check the model name and optional parameters.");
        summaryChinese = QStringLiteral("模型或请求配置错误。请检查模型名称和可选参数。");
        break;
    case RequestErrorCategory::Server:
        summaryEnglish = QStringLiteral("Service provider error. The server response was incomplete or unavailable.");
        summaryChinese = QStringLiteral("服务商错误。服务器响应不完整或暂时不可用。");
        break;
    case RequestErrorCategory::Unknown:
        summaryEnglish = QStringLiteral("Request failed.");
        summaryChinese = QStringLiteral("请求失败。");
        break;
    }

    const QString summary = text(summaryEnglish, summaryChinese);
    const QString trimmedDetail = detail.trimmed();
    if (trimmedDetail.isEmpty()) {
        return summary;
    }

    return text(QStringLiteral("%1\n\nDetails: %2"), QStringLiteral("%1\n\n详细信息：%2")).arg(summary, trimmedDetail);
}

bool ApplicationController::saveCurrentSession(bool moveToTop)
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

    if (!m_chatHistoryStorage.replaceSessionMessages(m_session, &error)) {
        emit statusMessage(QStringLiteral("Failed to save chat history: %1").arg(error),
                           QStringLiteral("保存聊天记录失败：%1").arg(error),
                           6000);
        return false;
    }

    if (m_sessionSearchQuery.isEmpty()) {
        upsertCurrentSessionSummary(moveToTop);
    } else {
        reloadSessionSummaries(&error);
    }
    emit sessionListChanged();
    return true;
}

bool ApplicationController::reloadSessionSummaries(QString *error)
{
    if (!m_historyAvailable) {
        m_sessionSummaries.clear();
        return false;
    }

    m_sessionSummaries = m_sessionSearchQuery.isEmpty()
                             ? m_chatHistoryStorage.loadSessionSummaries(error)
                             : m_chatHistoryStorage.searchSessionSummaries(m_sessionSearchQuery, error);
    return error == nullptr || error->isEmpty();
}

void ApplicationController::upsertCurrentSessionSummary(bool moveToTop)
{
    SessionSummaryList::upsert(&m_sessionSummaries, m_session, moveToTop);
}

bool ApplicationController::hasPersistableCurrentSession() const
{
    const QString title = m_session.title.trimmed();
    const bool hasCustomTitle = !title.isEmpty() && title != QStringLiteral("New Chat");
    return !m_session.messages.isEmpty() || m_session.hasSystemPrompt() || hasCustomTitle;
}

QString ApplicationController::text(const QString &english, const QString &chinese) const
{
    return m_config.language == AppLanguage::English ? english : chinese;
}
