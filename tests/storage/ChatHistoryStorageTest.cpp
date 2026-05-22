#include "storage/ChatHistoryStorage.h"

#include <QCoreApplication>
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

    assert(storage.clearSession(session.id, &error));
    assert(storage.clearSession(secondSession.id, &error));
    loaded = storage.loadLatestSession(&error);
    assert(!loaded.has_value());

    return 0;
}
