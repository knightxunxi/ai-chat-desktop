#include "storage/ChatHistoryStorage.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>

#include <cassert>

namespace {

void assertStorageResult(bool result, const QString &error)
{
    if (result) {
        return;
    }

    QTextStream(stderr) << error << Qt::endl;
    assert(false);
}

void createLegacyDatabase(const QString &databasePath)
{
    const QString connectionName = QStringLiteral("legacy_chat_history_test");
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(databasePath);
        assert(database.open());

        QSqlQuery query(database);
        assert(query.exec(QStringLiteral(
            "CREATE TABLE sessions ("
            "id TEXT PRIMARY KEY,"
            "title TEXT NOT NULL,"
            "system_prompt TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL"
            ")")));
        assert(query.exec(QStringLiteral(
            "CREATE TABLE messages ("
            "id TEXT PRIMARY KEY,"
            "session_id TEXT NOT NULL,"
            "role TEXT NOT NULL,"
            "content TEXT NOT NULL,"
            "created_at TEXT NOT NULL"
            ")")));

        const QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        query.prepare(QStringLiteral(
            "INSERT INTO sessions (id, title, system_prompt, created_at, updated_at) "
            "VALUES (:id, :title, :system_prompt, :created_at, :updated_at)"));
        query.bindValue(QStringLiteral(":id"), QStringLiteral("legacy-session"));
        query.bindValue(QStringLiteral(":title"), QStringLiteral("Legacy Session"));
        query.bindValue(QStringLiteral(":system_prompt"), QStringLiteral(""));
        query.bindValue(QStringLiteral(":created_at"), timestamp);
        query.bindValue(QStringLiteral(":updated_at"), timestamp);
        assert(query.exec());

        database.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTemporaryDir tempDir;
    assert(tempDir.isValid());

    const QString databasePath = tempDir.filePath(QStringLiteral("history.db"));
    ChatHistoryStorage storage(databasePath);

    QString error;
    assert(storage.initialize(&error));
    assert(error.isEmpty());

    ChatSession session = ChatSession::createDefault();
    session.title = QStringLiteral("Storage Test");
    session.systemPrompt = QStringLiteral("You are a test assistant.");

    ChatMessage userMessage = session.addMessage(MessageRole::User, QStringLiteral("Hello"));
    ChatMessage assistantMessage = session.addMessage(MessageRole::Assistant, QStringLiteral("Hi"));

    assert(storage.saveSession(session, &error));
    assert(storage.saveMessage(userMessage, &error));
    assert(storage.saveMessage(assistantMessage, &error));

    ChatSession secondSession = ChatSession::createDefault();
    secondSession.title = QStringLiteral("Second Session");
    ChatMessage secondMessage = secondSession.addMessage(MessageRole::User, QStringLiteral("Later"));
    secondSession.createdAt = session.createdAt.addSecs(60);
    secondSession.updatedAt = session.updatedAt.addSecs(60);

    assertStorageResult(storage.saveSession(secondSession, &error), error);
    assert(storage.saveMessage(secondMessage, &error));

    const QVector<ChatSession> summaries = storage.loadSessionSummaries(&error);
    assert(summaries.size() == 2);
    assert(summaries[0].id == secondSession.id);
    assert(summaries[0].messages.isEmpty());
    assert(summaries[1].id == session.id);

    session.title = QStringLiteral("Renamed Storage Test");
    assertStorageResult(storage.saveSession(session, &error), error);

    const QVector<ChatSession> renamedSummaries = storage.loadSessionSummaries(&error);
    assert(renamedSummaries.size() == 2);
    assert(renamedSummaries[0].id == secondSession.id);
    assert(renamedSummaries[1].id == session.id);
    assert(renamedSummaries[1].title == QStringLiteral("Renamed Storage Test"));

    const QVector<ChatSession> titleSearchResults = storage.searchSessionSummaries(QStringLiteral("renamed"), &error);
    assert(titleSearchResults.size() == 1);
    assert(titleSearchResults[0].id == session.id);

    const QVector<ChatSession> messageSearchResults = storage.searchSessionSummaries(QStringLiteral("Later"), &error);
    assert(messageSearchResults.size() == 1);
    assert(messageSearchResults[0].id == secondSession.id);

    const QVector<ChatSession> emptySearchResults = storage.searchSessionSummaries(QStringLiteral("not found"), &error);
    assert(emptySearchResults.isEmpty());

    std::optional<ChatSession> loadedById = storage.loadSession(session.id, &error);
    assert(loadedById.has_value());
    assert(loadedById->title == QStringLiteral("Renamed Storage Test"));
    assert(loadedById->messages.size() == 2);
    assert(loadedById->messages[0].content == QStringLiteral("Hello"));

    session.messages.removeLast();
    assertStorageResult(storage.saveSession(session, &error), error);
    assertStorageResult(storage.replaceSessionMessages(session, &error), error);

    loadedById = storage.loadSession(session.id, &error);
    assert(loadedById.has_value());
    assert(loadedById->messages.size() == 1);
    assert(loadedById->messages[0].content == QStringLiteral("Hello"));

    std::optional<ChatSession> loaded = storage.loadLatestSession(&error);
    assert(loaded.has_value());
    assert(loaded->id == secondSession.id);
    assert(loaded->title == QStringLiteral("Second Session"));
    assert(loaded->messages.size() == 1);

    assertStorageResult(storage.setSessionFavorite(session.id, true, &error), error);
    assertStorageResult(storage.setSessionArchived(secondSession.id, true, &error), error);

    loadedById = storage.loadSession(session.id, &error);
    assert(loadedById.has_value());
    assert(loadedById->isFavorite);
    assert(!loadedById->isArchived);
    assert(loadedById->updatedAt == session.updatedAt);

    std::optional<ChatSession> archivedSession = storage.loadSession(secondSession.id, &error);
    assert(archivedSession.has_value());
    assert(archivedSession->isArchived);
    assert(archivedSession->updatedAt == secondSession.updatedAt);

    const QVector<ChatSession> activeSummaries = storage.loadSessionSummaries(SessionListFilter::Active, &error);
    assert(activeSummaries.size() == 1);
    assert(activeSummaries[0].id == session.id);
    assert(activeSummaries[0].isFavorite);
    assert(!activeSummaries[0].isArchived);

    const QVector<ChatSession> favoriteSummaries = storage.loadSessionSummaries(SessionListFilter::Favorite, &error);
    assert(favoriteSummaries.size() == 1);
    assert(favoriteSummaries[0].id == session.id);

    const QVector<ChatSession> archivedSummaries = storage.loadSessionSummaries(SessionListFilter::Archived, &error);
    assert(archivedSummaries.size() == 1);
    assert(archivedSummaries[0].id == secondSession.id);
    assert(archivedSummaries[0].isArchived);

    const QVector<ChatSession> activeSearchAfterArchive =
        storage.searchSessionSummaries(QStringLiteral("Later"), SessionListFilter::Active, &error);
    assert(activeSearchAfterArchive.isEmpty());

    const QVector<ChatSession> archivedSearchResults =
        storage.searchSessionSummaries(QStringLiteral("Later"), SessionListFilter::Archived, &error);
    assert(archivedSearchResults.size() == 1);
    assert(archivedSearchResults[0].id == secondSession.id);

    const QString legacyDatabasePath = tempDir.filePath(QStringLiteral("legacy-history.db"));
    createLegacyDatabase(legacyDatabasePath);
    ChatHistoryStorage legacyStorage(legacyDatabasePath);
    assertStorageResult(legacyStorage.initialize(&error), error);
    const QVector<ChatSession> legacySummaries = legacyStorage.loadSessionSummaries(SessionListFilter::Active, &error);
    assert(legacySummaries.size() == 1);
    assert(legacySummaries[0].id == QStringLiteral("legacy-session"));
    assert(!legacySummaries[0].isFavorite);
    assert(!legacySummaries[0].isArchived);

    assert(storage.clearSession(session.id, &error));
    assert(storage.clearSession(secondSession.id, &error));
    loaded = storage.loadLatestSession(&error);
    assert(!loaded.has_value());

    return 0;
}
