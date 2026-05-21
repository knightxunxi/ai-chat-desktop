#include "support/AppLogger.h"

#include <QFile>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    QTemporaryDir directory;
    assert(directory.isValid());

    const QString logPath = directory.filePath(QStringLiteral("ai-chat-desktop.log"));
    AppLogger::setLogFilePathForTests(logPath);
    assert(AppLogger::initialize());
    assert(AppLogger::logFilePath() == logPath);

    AppLogger::info(QStringLiteral("Test"), QStringLiteral("Started with apiKey=test-secret"));
    AppLogger::warning(QStringLiteral("Test"), QStringLiteral("Authorization: Bearer test-token"));
    AppLogger::error(QStringLiteral("Test"), QStringLiteral("Failed\nwith two lines"));

    QFile file(logPath);
    assert(file.open(QFile::ReadOnly | QFile::Text));

    const QString content = QString::fromUtf8(file.readAll());
    assert(content.contains(QStringLiteral("[INFO] [Test] Started with apiKey=[REDACTED]")));
    assert(content.contains(QStringLiteral("[WARN] [Test] Authorization: Bearer [REDACTED]")));
    assert(content.contains(QStringLiteral("[ERROR] [Test] Failed with two lines")));
    assert(!content.contains(QStringLiteral("test-secret")));
    assert(!content.contains(QStringLiteral("test-token")));

    return 0;
}
