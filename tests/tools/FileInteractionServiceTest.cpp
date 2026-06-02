#include "support/AppLogger.h"
#include "tools/FileInteractionService.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <cassert>

namespace {

void writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    assert(file.open(QFile::WriteOnly | QFile::Truncate));
    assert(file.write(content) == content.size());
}

QString readFile(const QString &path)
{
    QFile file(path);
    assert(file.open(QFile::ReadOnly | QFile::Text));
    return QString::fromUtf8(file.readAll());
}

} // namespace

int main()
{
    QTemporaryDir directory;
    assert(directory.isValid());

    const QString logPath = directory.filePath(QStringLiteral("file-interaction.log"));
    AppLogger::setLogFilePathForTests(logPath);
    QString loggerError;
    assert(AppLogger::initialize(&loggerError));

    const QString textPath = directory.filePath(QStringLiteral("note.txt"));
    writeFile(textPath, QByteArray("hello\nsuper-secret-body"));

    ToolResult result = FileInteractionService::readTextFile(textPath);
    assert(result.ok);
    assert(result.output == QStringLiteral("hello\nsuper-secret-body"));
    assert(result.error.isEmpty());

    const QString binaryPath = directory.filePath(QStringLiteral("binary.bin"));
    writeFile(binaryPath, QByteArray("a\0b", 3));
    result = FileInteractionService::readTextFile(binaryPath);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("binary"), Qt::CaseInsensitive));

    const QString largePath = directory.filePath(QStringLiteral("large.txt"));
    writeFile(largePath, QByteArray(32, 'x'));
    result = FileInteractionService::readTextFile(largePath, 8);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("too large"), Qt::CaseInsensitive));

    const QString childDirectory = directory.filePath(QStringLiteral("child"));
    assert(QDir().mkpath(childDirectory));
    const QString childFilePath = QDir(childDirectory).filePath(QStringLiteral("child.txt"));
    writeFile(childFilePath, QByteArray("child"));

    result = FileInteractionService::listDirectory(directory.path());
    assert(result.ok);
    assert(result.output.contains(QStringLiteral("[DIR]  child")));
    assert(result.output.contains(QStringLiteral("[FILE] note.txt")));

    const QString savedPath = directory.filePath(QStringLiteral("saved.txt"));
    result = FileInteractionService::saveTextFile(savedPath, QStringLiteral("saved text"), false);
    assert(result.ok);
    assert(readFile(savedPath) == QStringLiteral("saved text"));

    result = FileInteractionService::saveTextFile(savedPath, QStringLiteral("new text"), false);
    assert(!result.ok);
    assert(readFile(savedPath) == QStringLiteral("saved text"));

    result = FileInteractionService::saveTextFile(savedPath, QStringLiteral("new text"), true);
    assert(result.ok);
    assert(readFile(savedPath) == QStringLiteral("new text"));

    result = FileInteractionService::validateOpenPath(savedPath);
    assert(result.ok);

    result = FileInteractionService::validateOpenPath(directory.filePath(QStringLiteral("missing.txt")));
    assert(!result.ok);

    const QString summary = FileInteractionService::pathSummary(savedPath);
    assert(summary.contains(QStringLiteral("saved.txt")));
    assert(!summary.contains(directory.path()));

    const QString logs = readFile(logPath);
    assert(logs.contains(QStringLiteral("read_text_file succeeded")));
    assert(logs.contains(QStringLiteral("dirHash=")));
    assert(!logs.contains(QStringLiteral("super-secret-body")));

    return 0;
}
