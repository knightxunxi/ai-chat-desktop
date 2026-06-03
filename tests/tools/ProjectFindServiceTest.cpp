#include "tools/ProjectFindService.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    QTemporaryDir tempDir;
    assert(tempDir.isValid());
    const QString projectDir = tempDir.path();

    // Create test file structure
    QDir dir(projectDir);
    assert(dir.mkdir(QStringLiteral("src")));
    assert(dir.mkdir(QStringLiteral("docs")));

    {
        QFile f(dir.filePath(QStringLiteral("src/main.cpp")));
        assert(f.open(QFile::WriteOnly | QFile::Text));
        f.write("int main() {}\n");
        f.close();
    }
    {
        QFile f(dir.filePath(QStringLiteral("src/utils.cpp")));
        assert(f.open(QFile::WriteOnly | QFile::Text));
        f.write("// utils\n");
        f.close();
    }
    {
        QFile f(dir.filePath(QStringLiteral("src/utils.h")));
        assert(f.open(QFile::WriteOnly | QFile::Text));
        f.write("#pragma once\n");
        f.close();
    }
    {
        QFile f(dir.filePath(QStringLiteral("docs/README.md")));
        assert(f.open(QFile::WriteOnly | QFile::Text));
        f.write("# Test\n");
        f.close();
    }

    // Test 1: find .cpp files
    {
        const ToolResult result = ProjectFindService::findFiles(projectDir, QStringLiteral("*.cpp"), 100);
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("main.cpp")));
        assert(result.output.contains(QStringLiteral("utils.cpp")));
        assert(!result.output.contains(QStringLiteral("utils.h")));
        assert(result.output.contains(QStringLiteral("Found 2")));
    }

    // Test 2: find .md files
    {
        const ToolResult result = ProjectFindService::findFiles(projectDir, QStringLiteral("*.md"), 100);
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("README.md")));
    }

    // Test 3: no match
    {
        const ToolResult result = ProjectFindService::findFiles(projectDir, QStringLiteral("*.py"), 100);
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("No files found")));
    }

    // Test 4: max results limit
    {
        const ToolResult result = ProjectFindService::findFiles(projectDir, QStringLiteral("*"), 1);
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("truncated")));
    }

    // Test 5: empty pattern
    {
        const ToolResult result = ProjectFindService::findFiles(projectDir, QString(), 100);
        assert(!result.ok);
    }

    return 0;
}
