#include "storage/ChatHistoryStorage.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

namespace {

QString toIsoUtc(const QDateTime &dateTime)
{
    return dateTime.toUTC().toString(Qt::ISODateWithMs);
}

QDateTime fromIsoUtc(const QString &value)
{
    return QDateTime::fromString(value, Qt::ISODateWithMs);
}

QString nonNullString(const QString &value)
{
    return value.isNull() ? QStringLiteral("") : value;
}

QString escapedLikePattern(QString value)
{
    value.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    value.replace(QLatin1Char('%'), QStringLiteral("\\%"));
    value.replace(QLatin1Char('_'), QStringLiteral("\\_"));
    return QStringLiteral("%") + value + QStringLiteral("%");
}

QString sessionFilterWhereClause(SessionListFilter filter)
{
    switch (filter) {
    case SessionListFilter::Active:
        return QStringLiteral("s.is_archived = 0");
    case SessionListFilter::Favorite:
        return QStringLiteral("s.is_favorite = 1 AND s.is_archived = 0");
    case SessionListFilter::Archived:
        return QStringLiteral("s.is_archived = 1");
    }

    return QStringLiteral("s.is_archived = 0");
}

QJsonObject chatMessageToJson(const ChatMessage &message)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), message.id);
    object.insert(QStringLiteral("sessionId"), message.sessionId);
    object.insert(QStringLiteral("role"), messageRoleToString(message.role));
    object.insert(QStringLiteral("content"), message.content);
    object.insert(QStringLiteral("createdAt"), toIsoUtc(message.createdAt));
    return object;
}

ChatMessage chatMessageFromJson(const QJsonObject &object)
{
    ChatMessage message;
    message.id = object.value(QStringLiteral("id")).toString();
    message.sessionId = object.value(QStringLiteral("sessionId")).toString();
    message.role = messageRoleFromString(object.value(QStringLiteral("role")).toString());
    message.content = object.value(QStringLiteral("content")).toString();
    message.createdAt = fromIsoUtc(object.value(QStringLiteral("createdAt")).toString());
    return message;
}

QString agentStepsToJson(const QVector<AgentStepRecord> &steps)
{
    QJsonArray array;
    for (const AgentStepRecord &step : steps) {
        QJsonObject object;
        object.insert(QStringLiteral("stepNumber"), step.stepNumber);
        object.insert(QStringLiteral("reasoning"), step.reasoning);
        object.insert(QStringLiteral("toolName"), step.toolName);
        object.insert(QStringLiteral("toolArguments"), step.toolArguments);
        object.insert(QStringLiteral("toolResult"), step.toolResult);
        object.insert(QStringLiteral("status"), step.status);
        object.insert(QStringLiteral("timestamp"), toIsoUtc(step.timestamp));
        array.append(object);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QVector<AgentStepRecord> agentStepsFromJson(const QString &json)
{
    QVector<AgentStepRecord> steps;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isArray()) {
        return steps;
    }

    for (const QJsonValue &value : document.array()) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        AgentStepRecord step;
        step.stepNumber = object.value(QStringLiteral("stepNumber")).toInt();
        step.reasoning = object.value(QStringLiteral("reasoning")).toString();
        step.toolName = object.value(QStringLiteral("toolName")).toString();
        step.toolArguments = object.value(QStringLiteral("toolArguments")).toString();
        step.toolResult = object.value(QStringLiteral("toolResult")).toString();
        step.status = object.value(QStringLiteral("status")).toString();
        step.timestamp = fromIsoUtc(object.value(QStringLiteral("timestamp")).toString());
        steps.append(step);
    }
    return steps;
}

QString branchesToJson(const QVector<MessageBranch> &branches)
{
    QJsonArray array;
    for (const MessageBranch &branch : branches) {
        QJsonObject object;
        object.insert(QStringLiteral("branchId"), branch.branchId);
        object.insert(QStringLiteral("parentMessageId"), branch.parentMessageId);
        QJsonArray messages;
        for (const ChatMessage &message : branch.messages) {
            messages.append(chatMessageToJson(message));
        }
        object.insert(QStringLiteral("messages"), messages);
        array.append(object);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QVector<MessageBranch> branchesFromJson(const QString &json)
{
    QVector<MessageBranch> branches;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isArray()) {
        return branches;
    }

    for (const QJsonValue &value : document.array()) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        MessageBranch branch;
        branch.branchId = object.value(QStringLiteral("branchId")).toString();
        branch.parentMessageId = object.value(QStringLiteral("parentMessageId")).toString();
        const QJsonArray messages = object.value(QStringLiteral("messages")).toArray();
        for (const QJsonValue &messageValue : messages) {
            if (messageValue.isObject()) {
                branch.messages.append(chatMessageFromJson(messageValue.toObject()));
            }
        }
        branches.append(branch);
    }
    return branches;
}

} // namespace

ChatHistoryStorage::ChatHistoryStorage(const QString &databasePath)
    : m_databasePath(databasePath)
    , m_connectionName(QStringLiteral("chat_history_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

ChatHistoryStorage::~ChatHistoryStorage()
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        return;
    }

    {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        database.close();
    }

    QSqlDatabase::removeDatabase(m_connectionName);
}

QString ChatHistoryStorage::defaultDatabasePath()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dataDir).filePath(QStringLiteral("chat-history.db"));
}

bool ChatHistoryStorage::initialize(QString *errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS sessions ("
            "id TEXT PRIMARY KEY,"
            "title TEXT NOT NULL,"
            "system_prompt TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL,"
            "is_favorite INTEGER NOT NULL DEFAULT 0,"
            "is_archived INTEGER NOT NULL DEFAULT 0,"
            "agent_steps_json TEXT NOT NULL DEFAULT '[]',"
            "branches_json TEXT NOT NULL DEFAULT '[]',"
            "current_branch_index INTEGER NOT NULL DEFAULT -1"
            ")"))) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    if (!ensureSessionColumn(QStringLiteral("is_favorite"),
                             QStringLiteral("is_favorite INTEGER NOT NULL DEFAULT 0"),
                             errorMessage)) {
        return false;
    }

    if (!ensureSessionColumn(QStringLiteral("is_archived"),
                             QStringLiteral("is_archived INTEGER NOT NULL DEFAULT 0"),
                             errorMessage)) {
        return false;
    }

    if (!ensureSessionColumn(QStringLiteral("agent_steps_json"),
                             QStringLiteral("agent_steps_json TEXT NOT NULL DEFAULT '[]'"),
                             errorMessage)) {
        return false;
    }

    if (!ensureSessionColumn(QStringLiteral("branches_json"),
                             QStringLiteral("branches_json TEXT NOT NULL DEFAULT '[]'"),
                             errorMessage)) {
        return false;
    }

    if (!ensureSessionColumn(QStringLiteral("current_branch_index"),
                             QStringLiteral("current_branch_index INTEGER NOT NULL DEFAULT -1"),
                             errorMessage)) {
        return false;
    }

    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS messages ("
            "id TEXT PRIMARY KEY,"
            "session_id TEXT NOT NULL,"
            "role TEXT NOT NULL,"
            "content TEXT NOT NULL,"
            "created_at TEXT NOT NULL,"
            "FOREIGN KEY(session_id) REFERENCES sessions(id)"
            ")"))) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    return true;
}

bool ChatHistoryStorage::saveSession(const ChatSession &session, QString *errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO sessions (id, title, system_prompt, created_at, updated_at, is_favorite, is_archived, agent_steps_json, branches_json, current_branch_index) "
        "VALUES (:id, :title, :system_prompt, :created_at, :updated_at, :is_favorite, :is_archived, :agent_steps_json, :branches_json, :current_branch_index) "
        "ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, "
        "system_prompt = excluded.system_prompt, "
        "updated_at = excluded.updated_at, "
        "is_favorite = excluded.is_favorite, "
        "is_archived = excluded.is_archived, "
        "agent_steps_json = excluded.agent_steps_json, "
        "branches_json = excluded.branches_json, "
        "current_branch_index = excluded.current_branch_index"));
    query.bindValue(QStringLiteral(":id"), session.id);
    query.bindValue(QStringLiteral(":title"), nonNullString(session.title));
    query.bindValue(QStringLiteral(":system_prompt"), nonNullString(session.systemPrompt));
    query.bindValue(QStringLiteral(":created_at"), toIsoUtc(session.createdAt));
    query.bindValue(QStringLiteral(":updated_at"), toIsoUtc(session.updatedAt));
    query.bindValue(QStringLiteral(":is_favorite"), session.isFavorite ? 1 : 0);
    query.bindValue(QStringLiteral(":is_archived"), session.isArchived ? 1 : 0);
    query.bindValue(QStringLiteral(":agent_steps_json"), agentStepsToJson(session.agentSteps));
    query.bindValue(QStringLiteral(":branches_json"), branchesToJson(session.branches));
    query.bindValue(QStringLiteral(":current_branch_index"), session.currentBranchIndex);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    return true;
}

bool ChatHistoryStorage::saveMessage(const ChatMessage &message, QString *errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO messages (id, session_id, role, content, created_at) "
        "VALUES (:id, :session_id, :role, :content, :created_at) "
        "ON CONFLICT(id) DO UPDATE SET "
        "role = excluded.role, "
        "content = excluded.content"));
    query.bindValue(QStringLiteral(":id"), message.id);
    query.bindValue(QStringLiteral(":session_id"), message.sessionId);
    query.bindValue(QStringLiteral(":role"), messageRoleToString(message.role));
    query.bindValue(QStringLiteral(":content"), nonNullString(message.content));
    query.bindValue(QStringLiteral(":created_at"), toIsoUtc(message.createdAt));

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    return true;
}

bool ChatHistoryStorage::replaceSessionMessages(const ChatSession &session, QString *errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    if (!database.transaction()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }

    QSqlQuery deleteMessages(database);
    deleteMessages.prepare(QStringLiteral("DELETE FROM messages WHERE session_id = :session_id"));
    deleteMessages.bindValue(QStringLiteral(":session_id"), session.id);
    if (!deleteMessages.exec()) {
        database.rollback();
        setError(errorMessage, deleteMessages.lastError().text());
        return false;
    }

    QSqlQuery insertMessage(database);
    insertMessage.prepare(QStringLiteral(
        "INSERT INTO messages (id, session_id, role, content, created_at) "
        "VALUES (:id, :session_id, :role, :content, :created_at)"));

    for (const ChatMessage &message : session.messages) {
        insertMessage.bindValue(QStringLiteral(":id"), message.id);
        insertMessage.bindValue(QStringLiteral(":session_id"), message.sessionId);
        insertMessage.bindValue(QStringLiteral(":role"), messageRoleToString(message.role));
        insertMessage.bindValue(QStringLiteral(":content"), nonNullString(message.content));
        insertMessage.bindValue(QStringLiteral(":created_at"), toIsoUtc(message.createdAt));

        if (!insertMessage.exec()) {
            database.rollback();
            setError(errorMessage, insertMessage.lastError().text());
            return false;
        }
    }

    if (!database.commit()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }

    return true;
}

QVector<ChatSession> ChatHistoryStorage::loadSessionSummaries(QString *errorMessage) const
{
    return loadSessionSummaries(SessionListFilter::Active, errorMessage);
}

QVector<ChatSession> ChatHistoryStorage::loadSessionSummaries(SessionListFilter filter, QString *errorMessage) const
{
    if (!ensureOpen(errorMessage)) {
        return {};
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    const QString sql = QStringLiteral(
                            "SELECT s.id, s.title, s.system_prompt, s.created_at, s.updated_at, s.is_favorite, s.is_archived "
                            "FROM sessions s WHERE %1 ORDER BY s.updated_at DESC")
                            .arg(sessionFilterWhereClause(filter));
    if (!query.exec(sql)) {
        setError(errorMessage, query.lastError().text());
        return {};
    }

    return readSessionSummaries(&query, errorMessage);
}

QVector<ChatSession> ChatHistoryStorage::searchSessionSummaries(const QString &queryText, QString *errorMessage) const
{
    return searchSessionSummaries(queryText, SessionListFilter::Active, errorMessage);
}

QVector<ChatSession> ChatHistoryStorage::searchSessionSummaries(const QString &queryText, SessionListFilter filter, QString *errorMessage) const
{
    const QString trimmedQuery = queryText.trimmed();
    if (trimmedQuery.isEmpty()) {
        return loadSessionSummaries(filter, errorMessage);
    }

    if (!ensureOpen(errorMessage)) {
        return {};
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
                      "SELECT s.id, s.title, s.system_prompt, s.created_at, s.updated_at, s.is_favorite, s.is_archived "
                      "FROM sessions s "
                      "WHERE %1 "
                      "AND (s.title LIKE :query ESCAPE '\\' "
                      "OR EXISTS ("
                      "    SELECT 1 FROM messages m "
                      "    WHERE m.session_id = s.id "
                      "    AND m.content LIKE :query ESCAPE '\\'"
                      ")) "
                      "ORDER BY s.updated_at DESC")
                      .arg(sessionFilterWhereClause(filter)));
    query.bindValue(QStringLiteral(":query"), escapedLikePattern(trimmedQuery));

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return {};
    }

    return readSessionSummaries(&query, errorMessage);
}

bool ChatHistoryStorage::setSessionFavorite(const QString &sessionId, bool favorite, QString *errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE sessions SET is_favorite = :is_favorite WHERE id = :id"));
    query.bindValue(QStringLiteral(":is_favorite"), favorite ? 1 : 0);
    query.bindValue(QStringLiteral(":id"), sessionId);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    return true;
}

bool ChatHistoryStorage::setSessionArchived(const QString &sessionId, bool archived, QString *errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE sessions SET is_archived = :is_archived WHERE id = :id"));
    query.bindValue(QStringLiteral(":is_archived"), archived ? 1 : 0);
    query.bindValue(QStringLiteral(":id"), sessionId);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    return true;
}

QVector<ChatSession> ChatHistoryStorage::readSessionSummaries(QSqlQuery *query, QString *errorMessage) const
{
    QVector<ChatSession> sessions;
    if (query == nullptr) {
        setError(errorMessage, QStringLiteral("Session query is null."));
        return sessions;
    }

    while (query->next()) {
        ChatSession session;
        session.id = query->value(0).toString();
        session.title = query->value(1).toString();
        session.systemPrompt = query->value(2).toString();
        session.createdAt = fromIsoUtc(query->value(3).toString());
        session.updatedAt = fromIsoUtc(query->value(4).toString());
        session.isFavorite = query->value(5).toInt() != 0;
        session.isArchived = query->value(6).toInt() != 0;
        sessions.append(session);
    }

    return sessions;
}

std::optional<ChatSession> ChatHistoryStorage::loadSession(const QString &sessionId, QString *errorMessage) const
{
    if (!ensureOpen(errorMessage)) {
        return std::nullopt;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT id, title, system_prompt, created_at, updated_at, is_favorite, is_archived, agent_steps_json, branches_json, current_branch_index "
        "FROM sessions WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), sessionId);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    ChatSession session;
    session.id = query.value(0).toString();
    session.title = query.value(1).toString();
    session.systemPrompt = query.value(2).toString();
    session.createdAt = fromIsoUtc(query.value(3).toString());
    session.updatedAt = fromIsoUtc(query.value(4).toString());
    session.isFavorite = query.value(5).toInt() != 0;
    session.isArchived = query.value(6).toInt() != 0;
    session.agentSteps = agentStepsFromJson(query.value(7).toString());
    session.branches = branchesFromJson(query.value(8).toString());
    session.currentBranchIndex = query.value(9).isNull() ? -1 : query.value(9).toInt();

    if (!loadMessages(&session, errorMessage)) {
        return std::nullopt;
    }

    return session;
}

std::optional<ChatSession> ChatHistoryStorage::loadLatestSession(QString *errorMessage) const
{
    const QVector<ChatSession> sessions = loadSessionSummaries(errorMessage);
    if (sessions.isEmpty()) {
        return std::nullopt;
    }

    return loadSession(sessions.first().id, errorMessage);
}

bool ChatHistoryStorage::clearSession(const QString &sessionId, QString *errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    if (!database.transaction()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }

    QSqlQuery deleteMessages(database);
    deleteMessages.prepare(QStringLiteral("DELETE FROM messages WHERE session_id = :session_id"));
    deleteMessages.bindValue(QStringLiteral(":session_id"), sessionId);

    if (!deleteMessages.exec()) {
        database.rollback();
        setError(errorMessage, deleteMessages.lastError().text());
        return false;
    }

    QSqlQuery deleteSession(database);
    deleteSession.prepare(QStringLiteral("DELETE FROM sessions WHERE id = :session_id"));
    deleteSession.bindValue(QStringLiteral(":session_id"), sessionId);

    if (!deleteSession.exec()) {
        database.rollback();
        setError(errorMessage, deleteSession.lastError().text());
        return false;
    }

    if (!database.commit()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }

    return true;
}

bool ChatHistoryStorage::ensureOpen(QString *errorMessage) const
{
    QFileInfo databaseFile(m_databasePath);
    QDir databaseDir = databaseFile.absoluteDir();
    if (!databaseDir.exists() && !databaseDir.mkpath(QStringLiteral("."))) {
        setError(errorMessage, QStringLiteral("Failed to create database directory."));
        return false;
    }

    QSqlDatabase database;
    if (QSqlDatabase::contains(m_connectionName)) {
        database = QSqlDatabase::database(m_connectionName);
    } else {
        database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
        database.setDatabaseName(m_databasePath);
    }

    if (!database.isOpen() && !database.open()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }

    return true;
}

bool ChatHistoryStorage::ensureSessionColumn(const QString &columnName, const QString &definition, QString *errorMessage) const
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery tableInfo(database);
    if (!tableInfo.exec(QStringLiteral("PRAGMA table_info(sessions)"))) {
        setError(errorMessage, tableInfo.lastError().text());
        return false;
    }

    while (tableInfo.next()) {
        if (tableInfo.value(1).toString().compare(columnName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    QSqlQuery alterQuery(database);
    if (!alterQuery.exec(QStringLiteral("ALTER TABLE sessions ADD COLUMN %1").arg(definition))) {
        setError(errorMessage, alterQuery.lastError().text());
        return false;
    }

    return true;
}

bool ChatHistoryStorage::loadMessages(ChatSession *session, QString *errorMessage) const
{
    if (session == nullptr) {
        setError(errorMessage, QStringLiteral("Session is null."));
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT id, session_id, role, content, created_at "
        "FROM messages WHERE session_id = :session_id ORDER BY created_at ASC"));
    query.bindValue(QStringLiteral(":session_id"), session->id);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    session->messages.clear();
    while (query.next()) {
        ChatMessage message;
        message.id = query.value(0).toString();
        message.sessionId = query.value(1).toString();
        message.role = messageRoleFromString(query.value(2).toString());
        message.content = query.value(3).toString();
        message.createdAt = fromIsoUtc(query.value(4).toString());
        session->messages.append(message);
    }

    return true;
}

void ChatHistoryStorage::setError(QString *errorMessage, const QString &message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}
