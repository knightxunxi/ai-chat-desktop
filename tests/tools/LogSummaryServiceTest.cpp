#include "tools/LogSummaryService.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <cassert>

int main()
{
    QTemporaryDir tempDir;
    assert(tempDir.isValid());

    const QString logFile = tempDir.filePath(QStringLiteral("test.log"));

    // Create test log
    {
        QFile file(logFile);
        assert(file.open(QFile::WriteOnly | QFile::Text));
        QTextStream stream(&file);
        stream << "[INFO] Application started.\n";
        stream << "[WARNING] Something unexpected.\n";
        stream << "[ERROR] Something failed: api_key=sk-abcdefghijklmnop\n";
        stream << "[INFO] Operation completed.\n";
        stream << "[ERROR] Another error with token=ghp_1234567890abcdef\n";
        file.close();
    }

    // Test 1: summarize without filters
    {
        const ToolResult result = LogSummaryService::summarize(logFile, QString(), 100, QStringLiteral("all"));
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("Log Summary")));
        assert(result.output.contains(QStringLiteral("Application started")));
    }

    // Test 2: keyword filter
    {
        const ToolResult result = LogSummaryService::summarize(logFile, QStringLiteral("error"), 100, QStringLiteral("all"));
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("ERROR")));
    }

    // Test 3: level filter
    {
        const ToolResult result = LogSummaryService::summarize(logFile, QString(), 100, QStringLiteral("error"));
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("ERROR")));
        assert(!result.output.contains(QStringLiteral("Operation completed")));
    }

    // Test 4: sanitizes sensitive data
    {
        const ToolResult result = LogSummaryService::summarize(logFile, QStringLiteral("api_key"), 100);
        assert(result.ok);
        assert(!result.output.contains(QStringLiteral("sk-abcdefghijklmnop")));
        assert(result.output.contains(QStringLiteral("[REDACTED]")));
    }

    // Test 5: file not found
    {
        const ToolResult result = LogSummaryService::summarize(QStringLiteral("/nonexistent.log"), QString(), 50);
        assert(!result.ok);
        assert(result.error.contains(QStringLiteral("does not exist")));
    }

    // Test 6: max lines limit
    {
        const ToolResult result = LogSummaryService::summarize(logFile, QString(), 2, QStringLiteral("all"));
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("Total matching lines: 2")));
    }

    return 0;
}
