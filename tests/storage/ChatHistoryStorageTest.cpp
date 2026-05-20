#include "storage/ChatHistoryStorage.h"

#include <QCoreApplication>
#include <QTemporaryDir>

#include <cassert>

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

    std::optional<ChatSession> loaded = storage.loadLatestSession(&error);
    assert(loaded.has_value());
    assert(loaded->id == session.id);
    assert(loaded->title == QStringLiteral("Storage Test"));
    assert(loaded->systemPrompt == QStringLiteral("You are a test assistant."));
    assert(loaded->messages.size() == 2);
    assert(loaded->messages[0].content == QStringLiteral("Hello"));
    assert(loaded->messages[1].role == MessageRole::Assistant);

    assert(storage.clearSession(session.id, &error));
    loaded = storage.loadLatestSession(&error);
    assert(!loaded.has_value());

    return 0;
}
