#include "storage/ChatSessionExporter.h"

#include <QFile>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    ChatSession session = ChatSession::createDefault();
    session.id = QStringLiteral("session-1");
    session.title = QStringLiteral("Export Test");
    session.systemPrompt = QStringLiteral("Be concise.");
    session.createdAt = QDateTime::fromString(QStringLiteral("2026-05-22T01:02:03.000Z"), Qt::ISODateWithMs);
    session.updatedAt = session.createdAt.addSecs(120);

    ChatMessage userMessage = ChatMessage::create(session.id, MessageRole::User, QStringLiteral("Hello"));
    userMessage.createdAt = session.createdAt.addSecs(10);
    ChatMessage assistantMessage = ChatMessage::create(session.id, MessageRole::Assistant, QStringLiteral("  Hi\n\n```cpp\nint main() { return 0; }\n```\n"));
    assistantMessage.createdAt = session.createdAt.addSecs(20);
    session.messages.append(userMessage);
    session.messages.append(assistantMessage);

    const QString markdown = ChatSessionExporter::toMarkdown(session);
    assert(markdown.contains(QStringLiteral("# Export Test")));
    assert(markdown.contains(QStringLiteral("- Session ID: session-1")));
    assert(markdown.contains(QStringLiteral("- Created: 2026-05-22T09:02:03.000+08:00")));
    assert(markdown.contains(QStringLiteral("- Time: 2026-05-22T09:02:13.000+08:00")));
    assert(markdown.contains(QStringLiteral("## Role Prompt")));
    assert(markdown.contains(QStringLiteral("Be concise.")));
    assert(markdown.contains(QStringLiteral("- Role: user")));
    assert(markdown.contains(QStringLiteral("- Role: assistant")));
    assert(markdown.contains(QStringLiteral("  Hi\n\n```cpp")));
    assert(markdown.contains(QStringLiteral("```cpp")));
    assert(!markdown.contains(QStringLiteral("API Key")));

    QTemporaryDir tempDir;
    assert(tempDir.isValid());
    const QString exportPath = tempDir.filePath(QStringLiteral("chat.md"));

    QString error;
    assert(ChatSessionExporter::writeMarkdown(session, exportPath, &error));

    QFile file(exportPath);
    assert(file.open(QFile::ReadOnly | QFile::Text));
    const QString writtenContent = QString::fromUtf8(file.readAll());
    assert(writtenContent == markdown);

    return 0;
}
