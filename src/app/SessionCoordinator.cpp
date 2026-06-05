#include "app/SessionCoordinator.h"
#include "app/SessionSummaryList.h"
#include "storage/ChatSessionExporter.h"

#include <QDateTime>
#include <QStringList>

namespace {

bool sessionMatchesFilter(const ChatSession &session, SessionListFilter filter)
{
    switch (filter) {
    case SessionListFilter::Active:
        return !session.isArchived;
    case SessionListFilter::Favorite:
        return session.isFavorite && !session.isArchived;
    case SessionListFilter::Archived:
        return session.isArchived;
    }

    return !session.isArchived;
}

} // namespace

SessionCoordinator::SessionCoordinator(QObject *parent)
    : QObject(parent)
{
}

void SessionCoordinator::initialize()
{
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

const ChatSession &SessionCoordinator::currentSession() const
{
    return m_session;
}

ChatSession &SessionCoordinator::currentSession()
{
    return m_session;
}

const QVector<ChatSession> &SessionCoordinator::sessionSummaries() const
{
    return m_sessionSummaries;
}

SessionListFilter SessionCoordinator::sessionListFilter() const
{
    return m_sessionListFilter;
}

ChatHistoryStorage *SessionCoordinator::chatHistoryStorage()
{
    return &m_chatHistoryStorage;
}

const ChatHistoryStorage *SessionCoordinator::chatHistoryStorage() const
{
    return &m_chatHistoryStorage;
}

bool SessionCoordinator::isHistoryAvailable() const
{
    return m_historyAvailable;
}

bool SessionCoordinator::exportCurrentSessionMarkdown(const QString &filePath, QString *error) const
{
    return ChatSessionExporter::writeMarkdown(m_session, filePath, error);
}

void SessionCoordinator::createMessageBranch(const QString &parentMessageId)
{
    MessageBranch branch;
    branch.branchId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    branch.parentMessageId = parentMessageId;
    m_session.branches.append(branch);
}

bool SessionCoordinator::editCurrentMessage(const QString &messageId, const QString &newContent)
{
    return m_session.editMessage(messageId, newContent);
}

void SessionCoordinator::truncateCurrentSessionFrom(const QString &messageId)
{
    m_session.truncateFrom(messageId);
}

bool SessionCoordinator::setSystemPrompt(const QString &prompt)
{
    m_session.systemPrompt = prompt.trimmed();
    m_session.updatedAt = QDateTime::currentDateTimeUtc();
    if (!saveCurrentSession()) {
        emit statusMessage(QStringLiteral("Failed to save the role prompt."),
                           QStringLiteral("保存角色提示词失败。"),
                           6000);
        return false;
    }

    emit currentSessionChanged();
    return true;
}

bool SessionCoordinator::renameCurrentSession(const QString &title)
{
    const QString trimmedTitle = title.trimmed();
    if (trimmedTitle.isEmpty()) {
        emit statusMessage(QStringLiteral("Chat title cannot be empty."),
                           QStringLiteral("会话标题不能为空。"),
                           3000);
        return false;
    }

    if (m_session.title == trimmedTitle) {
        emit currentSessionChanged();
        return false;
    }

    const QString previousTitle = m_session.title;
    m_session.title = trimmedTitle;
    if (!saveCurrentSession(false)) {
        m_session.title = previousTitle;
        emit statusMessage(QStringLiteral("Failed to rename the chat session."),
                           QStringLiteral("重命名会话失败。"),
                           6000);
        return false;
    }

    emit currentSessionChanged();
    return true;
}

void SessionCoordinator::searchSessions(const QString &query)
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

void SessionCoordinator::setSessionListFilter(SessionListFilter filter)
{
    if (m_sessionListFilter == filter) {
        return;
    }

    m_sessionListFilter = filter;
    QString error;
    if (!reloadSessionSummaries(&error) && !error.isEmpty()) {
        emit statusMessage(QStringLiteral("Failed to load chat sessions: %1").arg(error),
                           QStringLiteral("加载会话列表失败：%1").arg(error),
                           6000);
    }

    emit sessionListFilterChanged();
    emit sessionListChanged();
}

void SessionCoordinator::toggleCurrentSessionFavorite()
{
    const bool previousFavorite = m_session.isFavorite;
    const SessionListFilter previousFilter = m_sessionListFilter;
    m_session.isFavorite = !m_session.isFavorite;
    if (!m_session.isFavorite && m_sessionListFilter == SessionListFilter::Favorite) {
        m_sessionListFilter = SessionListFilter::Active;
    }

    if (!saveCurrentSession(false)) {
        m_session.isFavorite = previousFavorite;
        m_sessionListFilter = previousFilter;
        emit sessionListFilterChanged();
        emit currentSessionChanged();
        return;
    }

    if (m_sessionListFilter != previousFilter) {
        emit sessionListFilterChanged();
    }
    emit currentSessionChanged();
}

void SessionCoordinator::toggleCurrentSessionArchived()
{
    const bool previousArchived = m_session.isArchived;
    const SessionListFilter previousFilter = m_sessionListFilter;
    m_session.isArchived = !m_session.isArchived;
    if (m_session.isArchived) {
        m_sessionListFilter = SessionListFilter::Archived;
    } else if (m_sessionListFilter == SessionListFilter::Archived) {
        m_sessionListFilter = SessionListFilter::Active;
    }

    if (!saveCurrentSession(false)) {
        m_session.isArchived = previousArchived;
        m_sessionListFilter = previousFilter;
        emit sessionListFilterChanged();
        emit currentSessionChanged();
        return;
    }

    if (m_sessionListFilter != previousFilter) {
        emit sessionListFilterChanged();
    }
    emit currentSessionChanged();
}

void SessionCoordinator::createNewChat()
{
    if (m_sessionListFilter != SessionListFilter::Active) {
        m_sessionListFilter = SessionListFilter::Active;
        QString error;
        reloadSessionSummaries(&error);
        emit sessionListFilterChanged();
        emit sessionListChanged();
    }

    if (!hasPersistableCurrentSession()) {
        emit currentSessionChanged();
        return;
    }

    saveCurrentSession(false);

    m_session = ChatSession::createDefault();
    saveCurrentSession();

    emit sessionListChanged();
    emit currentSessionChanged();
}

bool SessionCoordinator::switchToSession(const QString &sessionId)
{
    if (sessionId.isEmpty()) {
        return false;
    }

    if (sessionId == m_session.id) {
        emit currentSessionChanged();
        return true;
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
        return false;
    }

    m_session = loaded.value();
    emit currentSessionChanged();
    return true;
}

void SessionCoordinator::deleteCurrentSession()
{
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

    QString error;
    if (!m_sessionSearchQuery.isEmpty()) {
        reloadSessionSummaries(&error);
    }

    if (m_sessionSummaries.isEmpty()) {
        if (m_sessionListFilter != SessionListFilter::Active) {
            m_sessionListFilter = SessionListFilter::Active;
            emit sessionListFilterChanged();
            reloadSessionSummaries(&error);
        }
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

bool SessionCoordinator::saveCurrentSession(bool moveToTop)
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

bool SessionCoordinator::reloadSessionSummaries(QString *error)
{
    if (!m_historyAvailable) {
        m_sessionSummaries.clear();
        return false;
    }

    m_sessionSummaries = m_sessionSearchQuery.isEmpty()
                             ? m_chatHistoryStorage.loadSessionSummaries(m_sessionListFilter, error)
                             : m_chatHistoryStorage.searchSessionSummaries(m_sessionSearchQuery, m_sessionListFilter, error);
    return error == nullptr || error->isEmpty();
}

void SessionCoordinator::upsertCurrentSessionSummary(bool moveToTop)
{
    if (!sessionMatchesCurrentFilter(m_session)) {
        QString error;
        reloadSessionSummaries(&error);
        return;
    }

    SessionSummaryList::upsert(&m_sessionSummaries, m_session, moveToTop);
}

bool SessionCoordinator::sessionMatchesCurrentFilter(const ChatSession &session) const
{
    return sessionMatchesFilter(session, m_sessionListFilter);
}

bool SessionCoordinator::hasPersistableCurrentSession() const
{
    const QString title = m_session.title.trimmed();
    const bool hasCustomTitle = !title.isEmpty() && title != QStringLiteral("New Chat");
    return !m_session.messages.isEmpty() || m_session.hasSystemPrompt() || hasCustomTitle;
}
