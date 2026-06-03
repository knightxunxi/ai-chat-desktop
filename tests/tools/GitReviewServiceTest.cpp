#include "tools/GitReviewService.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    // Setup: create temp git repo
    QTemporaryDir tempDir;
    assert(tempDir.isValid());

    const QString originalDir = QDir::currentPath();
    QDir::setCurrent(tempDir.path());

    // Init git repo
    QProcess init;
    init.setProgram(QStringLiteral("git"));
    init.setArguments({QStringLiteral("init")});
    init.start();
    assert(init.waitForFinished(10000));
    assert(init.exitCode() == 0);

    // Create initial commit
    QFile readme(tempDir.filePath(QStringLiteral("README.md")));
    assert(readme.open(QFile::WriteOnly | QFile::Text));
    readme.write("# Test\n");
    readme.close();

    {
        QProcess add;
        add.setProgram(QStringLiteral("git"));
        add.setArguments({QStringLiteral("add"), QStringLiteral("README.md")});
        add.start();
        assert(add.waitForFinished(10000));
    }

    {
        QProcess commit;
        commit.setProgram(QStringLiteral("git"));
        commit.setArguments({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial")});
        commit.start();
        assert(commit.waitForFinished(10000));
        assert(commit.exitCode() == 0);
    }

    // Make a change for diff to detect
    assert(readme.open(QFile::Append | QFile::Text));
    readme.write("More content\n");
    readme.close();

    // Test 1: reviewDiff with changes
    {
        const ToolResult result = GitReviewService::reviewDiff(false, 200);
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("Git Diff")));
        assert(result.output.contains(QStringLiteral("More content")));
    }

    // Test 2: reviewDiff staged only (should be empty)
    {
        const ToolResult result = GitReviewService::reviewDiff(true, 200);
        assert(result.ok);
    }

    // Test 3: reviewLog
    {
        const ToolResult result = GitReviewService::reviewLog(5, true);
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("Git Log")));
        assert(result.output.contains(QStringLiteral("Initial")));
    }

    // Test 4: reviewLog full format
    {
        const ToolResult result = GitReviewService::reviewLog(3, false);
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("Initial")));
    }

    QDir::setCurrent(originalDir);
    return 0;
}
