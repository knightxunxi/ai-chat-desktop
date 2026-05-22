#include "storage/ChatSessionExporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringConverter>
#include <QTextStream>

namespace {

QString displayTitle(const ChatSession &session)
{
    const QString title = session.title.trimmed();
    return title.isEmpty() ? QStringLiteral("Untitled Chat") : title;
}

QString isoBeijingTime(const QDateTime &dateTime)
{
    if (!dateTime.isValid()) {
        return QStringLiteral("unknown");
    }

    return dateTime.toUTC().addSecs(8 * 60 * 60).toString(QStringLiteral("yyyy-MM-dd'T'HH:mm:ss.zzz'+08:00'"));
}

QString roleLabel(MessageRole role)
{
    switch (role) {
    case MessageRole::System:
        return QStringLiteral("system");
    case MessageRole::User:
        return QStringLiteral("user");
    case MessageRole::Assistant:
        return QStringLiteral("assistant");
    }

    return QStringLiteral("user");
}

void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

} // namespace

namespace ChatSessionExporter {

QString toMarkdown(const ChatSession &session)
{
    QString output;
    QTextStream stream(&output);
    stream.setEncoding(QStringConverter::Utf8);

    stream << "# " << displayTitle(session) << "\n\n";
    stream << "- Session ID: " << session.id << '\n';
    stream << "- Created: " << isoBeijingTime(session.createdAt) << '\n';
    stream << "- Updated: " << isoBeijingTime(session.updatedAt) << "\n\n";

    if (session.hasSystemPrompt()) {
        stream << "## Role Prompt\n\n";
        stream << session.systemPrompt.trimmed() << "\n\n";
    }

    stream << "## Messages\n\n";
    if (session.messages.isEmpty()) {
        stream << "_No messages._\n";
        return output;
    }

    for (int index = 0; index < session.messages.size(); ++index) {
        const ChatMessage &message = session.messages.at(index);
        stream << "### Message " << (index + 1) << "\n\n";
        stream << "- Role: " << roleLabel(message.role) << '\n';
        stream << "- Time: " << isoBeijingTime(message.createdAt) << "\n\n";
        stream << message.content;
        if (!message.content.endsWith(QLatin1Char('\n'))) {
            stream << '\n';
        }
        stream << '\n';
    }

    return output;
}

bool writeMarkdown(const ChatSession &session, const QString &filePath, QString *error)
{
    const QString trimmedPath = filePath.trimmed();
    if (trimmedPath.isEmpty()) {
        setError(error, QStringLiteral("Export file path is empty."));
        return false;
    }

    const QFileInfo fileInfo(trimmedPath);
    QDir directory(fileInfo.absolutePath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        setError(error, QStringLiteral("Failed to create export directory: %1").arg(directory.absolutePath()));
        return false;
    }

    QFile file(trimmedPath);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        setError(error, QStringLiteral("Failed to write export file: %1").arg(trimmedPath));
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << toMarkdown(session);
    return true;
}

} // namespace ChatSessionExporter
