#include "support/LogFileReader.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <cassert>

int main()
{
    QTemporaryDir directory;
    assert(directory.isValid());

    const QString logPath = directory.filePath(QStringLiteral("ai-chat-desktop.log"));
    QFile file(logPath);
    assert(file.open(QFile::WriteOnly | QFile::Text));
    QTextStream stream(&file);
    for (int i = 1; i <= 10; ++i) {
        stream << "line-" << i << '\n';
    }
    file.close();

    QString error;
    assert(LogFileReader::readLastLines(logPath, 3, &error) == QStringLiteral("line-8\nline-9\nline-10"));
    assert(error.isEmpty());

    assert(LogFileReader::readLastLines(logPath, 20, &error).startsWith(QStringLiteral("line-1\nline-2")));
    assert(error.isEmpty());

    assert(LogFileReader::readLastLines(logPath, 0, &error).isEmpty());
    assert(error.isEmpty());

    assert(LogFileReader::readLastLines(directory.filePath(QStringLiteral("missing.log")), 5, &error).isEmpty());
    assert(!error.isEmpty());

    return 0;
}
