#include "storage/ChatHistoryStorage.h"

#include <QDir>
#include <QFileInfo>
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
            "updated_at TEXT NOT NULL"
            ")"))) {
        setError(errorMessage, query.lastError().text());
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
        "INSERT INTO sessions (id, title, system_prompt, created_at, updated_at) "
        "VALUES (:id, :title, :system_prompt, :created_at, :updated_at) "
        "ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, "
        "system_prompt = excluded.system_prompt, "
        "updated_at = excluded.updated_at"));
    query.bindValue(QStringLiteral(":id"), session.id);
    query.bindValue(QStringLiteral(":title"), nonNullString(session.title));
    query.bindValue(QStringLiteral(":system_prompt"), nonNullString(session.systemPrompt));
    query.bindValue(QStringLiteral(":created_at"), toIsoUtc(session.createdAt));
    query.bindValue(QStringLiteral(":updated_at"), toIsoUtc(session.updatedAt));

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

QVector<ChatSession> ChatHistoryStorage::loadSessionSummaries(QString *errorMessage) const
{
    QVector<ChatSession> sessions;
    if (!ensureOpen(errorMessage)) {
        return sessions;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral(
            "SELECT id, title, system_prompt, created_at, updated_at "
            "FROM sessions ORDER BY updated_at DESC"))) {
        setError(errorMessage, query.lastError().text());
        return sessions;
    }

    while (query.next()) {
        ChatSession session;
        session.id = query.value(0).toString();
        session.title = query.value(1).toString();
        session.systemPrompt = query.value(2).toString();
        session.createdAt = fromIsoUtc(query.value(3).toString());
        session.updatedAt = fromIsoUtc(query.value(4).toString());
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
        "SELECT id, title, system_prompt, created_at, updated_at "
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
