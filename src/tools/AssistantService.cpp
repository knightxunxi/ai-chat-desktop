#include "tools/AssistantService.h"

#include "tools/WorkspacePolicy.h"

#include <QDate>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTextStream>

namespace AssistantService {

ToolResult workJournal(const QString &projectDirectory, const QString &workspaceDirectory)
{
    // 获取今天的 git log
    QProcess gitLog;
    gitLog.setWorkingDirectory(projectDirectory);
    gitLog.setProgram(QStringLiteral("git"));
    gitLog.setArguments({QStringLiteral("log"), QStringLiteral("--oneline"), QStringLiteral("-n"), QStringLiteral("10")});
    gitLog.start();
    QString gitOutput = QStringLiteral("(git not available or no commits today)");
    if (gitLog.waitForFinished(10000) && gitLog.exitCode() == 0) {
        gitOutput = QString::fromUtf8(gitLog.readAllStandardOutput()).trimmed();
        if (gitOutput.isEmpty()) {
            gitOutput = QStringLiteral("(No commits)");
        }
    }

    // 获取 git diff --stat
    QProcess gitDiff;
    gitDiff.setWorkingDirectory(projectDirectory);
    gitDiff.setProgram(QStringLiteral("git"));
    gitDiff.setArguments({QStringLiteral("diff"), QStringLiteral("--stat")});
    gitDiff.start();
    QString diffOutput = QStringLiteral("(No changes)");
    if (gitDiff.waitForFinished(10000) && gitDiff.exitCode() == 0) {
        diffOutput = QString::fromUtf8(gitDiff.readAllStandardOutput()).trimmed();
        if (diffOutput.isEmpty()) {
            diffOutput = QStringLiteral("(No changes)");
        }
    }

    const QString today = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    const QString journalContent = QStringLiteral(
        "# Work Journal — %1\n\n"
        "## Recent Commits\n%2\n\n"
        "## Current Changes\n%3\n")
        .arg(today, gitOutput, diffOutput);

    // 保存到工作目录
    const QString journalPath = QDir(workspaceDirectory).filePath(QStringLiteral("WORK_JOURNAL.md"));
    QFile file(journalPath);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        return ToolResult::failure(QStringLiteral("Cannot write journal file."));
    }
    QTextStream stream(&file);
    stream << journalContent;
    file.close();

    return ToolResult::success(
        QStringLiteral("Work journal saved to WORK_JOURNAL.md\n\n%1").arg(journalContent));
}

ToolResult projectCheck(const QString &projectDirectory)
{
    QDir dir(projectDirectory);
    if (!dir.exists()) {
        return ToolResult::failure(QStringLiteral("Project directory does not exist: %1").arg(projectDirectory));
    }

    // 收集项目信息
    QStringList checks;

    // 检查 git 状态
    QProcess gitStatus;
    gitStatus.setWorkingDirectory(projectDirectory);
    gitStatus.setProgram(QStringLiteral("git"));
    gitStatus.setArguments({QStringLiteral("status"), QStringLiteral("--short")});
    gitStatus.start();
    if (gitStatus.waitForFinished(10000) && gitStatus.exitCode() == 0) {
        const QString status = QString::fromUtf8(gitStatus.readAllStandardOutput()).trimmed();
        checks.append(QStringLiteral("Git status: %1 files changed").arg(
            status.isEmpty() ? 0 : status.count(QLatin1Char('\n')) + 1));
    } else {
        checks.append(QStringLiteral("Git status: not a git repository or git not available"));
    }

    // 检查文件数量
    int fileCount = 0;
    QDirIterator it(projectDirectory, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        ++fileCount;
        if (fileCount >= 1000) break;
    }
    checks.append(QStringLiteral("Project files: %1+").arg(fileCount));

    return ToolResult::success(QStringLiteral("=== Project Check ===\n%1").arg(checks.join(QStringLiteral("\n"))));
}

ToolResult fileOrganize(const QString &workspaceDirectory, const QString &sourcePattern, const QString &targetSubDir)
{
    if (sourcePattern.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Source pattern must not be empty."));
    }
    if (targetSubDir.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Target subdirectory must not be empty."));
    }

    const QString targetDir = QDir(workspaceDirectory).filePath(targetSubDir);
    QDir dir(workspaceDirectory);
    const QStringList matchingFiles = dir.entryList({sourcePattern}, QDir::Files);

    if (matchingFiles.isEmpty()) {
        return ToolResult::success(QStringLiteral("No files matching \"%1\" found.").arg(sourcePattern));
    }

    if (!QDir().mkpath(targetDir)) {
        return ToolResult::failure(QStringLiteral("Cannot create target directory: %1").arg(targetSubDir));
    }

    QStringList movedFiles;
    for (const QString &fileName : matchingFiles) {
        const QString sourcePath = dir.filePath(fileName);
        const QString targetPath = QDir(targetDir).filePath(fileName);

        if (!WorkspacePolicy::isPathInsideWorkspace(workspaceDirectory, sourcePath)
            || !WorkspacePolicy::isPathInsideWorkspace(workspaceDirectory, targetPath)) {
            continue;
        }

        if (QFile::rename(sourcePath, targetPath)) {
            movedFiles.append(fileName);
        }
    }

    return ToolResult::success(
        QStringLiteral("Moved %1 file(s) matching \"%2\" to %3/:\n%4")
            .arg(movedFiles.size())
            .arg(sourcePattern)
            .arg(targetSubDir)
            .arg(movedFiles.join(QStringLiteral(", "))));
}

ToolResult saveReminder(const QString &workspaceDirectory, const QString &title, const QString &content)
{
    if (title.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Reminder title must not be empty."));
    }

    const QString reminderPath = QDir(workspaceDirectory).filePath(QStringLiteral("REMINDERS.md"));
    QFile file(reminderPath);
    const bool isNew = !file.exists();

    if (!file.open(QFile::Append | QFile::Text)) {
        return ToolResult::failure(QStringLiteral("Cannot write reminders file."));
    }

    QTextStream stream(&file);
    if (isNew) {
        stream << QStringLiteral("# Reminders\n\n");
    }
    stream << QStringLiteral("## %1\n%2\n\n").arg(title, content);
    file.close();

    return ToolResult::success(QStringLiteral("Reminder saved: \"%1\"").arg(title));
}

} // namespace AssistantService
