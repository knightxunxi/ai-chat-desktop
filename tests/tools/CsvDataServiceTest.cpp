#include "tools/CsvDataService.h"

#include <QDir>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    QTemporaryDir tempDir;
    assert(tempDir.isValid());
    const QString workspaceDir = tempDir.path();

    // Test 1: write and read simple CSV with header
    {
        const QString path = QStringLiteral("test.csv");
        QVector<QStringList> rows;
        rows.append({QStringLiteral("Alice"), QStringLiteral("30"), QStringLiteral("Engineer")});
        rows.append({QStringLiteral("Bob"), QStringLiteral("25"), QStringLiteral("Designer")});

        const ToolResult writeResult = CsvDataService::writeCsv(
            workspaceDir, path, rows, {QStringLiteral("Name"), QStringLiteral("Age"), QStringLiteral("Role")});
        assert(writeResult.ok);

        const ToolResult readResult = CsvDataService::readCsv(workspaceDir, path, 100, true);
        assert(readResult.ok);
        assert(readResult.output.contains(QStringLiteral("Alice")));
        assert(readResult.output.contains(QStringLiteral("Bob")));
    }

    // Test 2: CSV without header
    {
        const QString path = QStringLiteral("noheader.csv");
        QVector<QStringList> rows;
        rows.append({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")});

        const ToolResult writeResult = CsvDataService::writeCsv(workspaceDir, path, rows);
        assert(writeResult.ok);

        const ToolResult readResult = CsvDataService::readCsv(workspaceDir, path, 100, false);
        assert(readResult.ok);
        assert(readResult.output.contains(QStringLiteral("1, 2, 3")));
    }

    // Test 3: file not found
    {
        const ToolResult result = CsvDataService::readCsv(workspaceDir, QStringLiteral("nonexistent.csv"), 100, true);
        assert(!result.ok);
    }

    // Test 4: column count mismatch
    {
        const QString path = QStringLiteral("bad.csv");
        QVector<QStringList> rows;
        rows.append({QStringLiteral("A"), QStringLiteral("B")});
        rows.append({QStringLiteral("C")});

        const ToolResult result = CsvDataService::writeCsv(
            workspaceDir, path, rows, {QStringLiteral("Col1"), QStringLiteral("Col2")});
        assert(!result.ok);
        assert(result.error.contains(QStringLiteral("mismatch")));
    }

    // Test 5: empty path
    {
        QVector<QStringList> rows;
        rows.append({QStringLiteral("test")});
        const ToolResult result = CsvDataService::writeCsv(workspaceDir, QString(), rows);
        assert(!result.ok);
    }

    // Test 6: path outside workspace
    {
        const ToolResult result = CsvDataService::readCsv(workspaceDir, QStringLiteral("../outside.csv"), 100, true);
        assert(!result.ok);
    }

    return 0;
}
