#include "tools/dev/GitReviewService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QTextStream>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

ToolResult runGitCommand(const QStringList &arguments, int maxOutputBytes = 8192)
{
    QProcess process;
    process.setProgram(QStringLiteral("git"));
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();

    if (!process.waitForStarted(5000)) {
        return ToolResult::failure(QStringLiteral("Failed to start git process. Is git installed?"));
    }

    if (!process.waitForFinished(15000)) {
        process.kill();
        process.waitForFinished(3000);
        return ToolResult::failure(QStringLiteral("Git command timed out."));
    }

    const int exitCode = process.exitCode();
    if (exitCode != 0) {
        const QString stderrOutput = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (!stderrOutput.isEmpty()) {
            return ToolResult::failure(QStringLiteral("Git exited with code %1: %2").arg(exitCode).arg(stderrOutput));
        }
        return ToolResult::failure(QStringLiteral("Git exited with code %1.").arg(exitCode));
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        return ToolResult::success(QStringLiteral("(No output)"));
    }

    if (output.size() > maxOutputBytes) {
        output = output.left(maxOutputBytes);
        output += QStringLiteral("\n... (output truncated at %1 bytes)").arg(maxOutputBytes);
    }

    return ToolResult::success(output);
}

// #27: 常见问题模式检测
QVector<GitReviewService::ReviewIssue> detectCommonPatterns(const QString &file,
                                                             const QStringList &lines,
                                                             int startLine)
{
    QVector<GitReviewService::ReviewIssue> issues;
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];
        const int absLine = startLine + i;
        const QString trimmed = line.trimmed();

        // 检测硬编码 API Key/Token
        if (trimmed.contains(QStringLiteral("api_key")) ||
            trimmed.contains(QStringLiteral("apiKey")) ||
            trimmed.contains(QStringLiteral("API_KEY")) ||
            trimmed.contains(QStringLiteral("secret")) ||
            trimmed.contains(QStringLiteral("password"))) {
            if (trimmed.contains(QStringLiteral("=")) || trimmed.contains(QStringLiteral(":"))) {
                issues.append({QStringLiteral("warning"), file, absLine,
                    QStringLiteral("Possible hard-coded credential: '%1'").arg(trimmed.left(60)),
                    QStringLiteral("Use credential storage or environment variable instead.")});
            }
        }

        // 检测 TODO/FIXME
        if (trimmed.startsWith(QStringLiteral("// TODO")) ||
            trimmed.startsWith(QStringLiteral("// FIXME")) ||
            trimmed.startsWith(QStringLiteral("/* TODO"))) {
            issues.append({QStringLiteral("info"), file, absLine,
                QStringLiteral("TODO/FIXME comment left in code"),
                QStringLiteral("Resolve the TODO before merging or file a tracking issue.")});
        }

        // 检测魔法数字（非 0/1 的裸数字赋值）
        if (trimmed.contains(QStringLiteral("return ")) &&
            trimmed.contains(QStringLiteral(";"))) {
            // 常见忽略模式
            static const QStringList ignored = {
                QStringLiteral("return 0"), QStringLiteral("return 1"),
                QStringLiteral("return -1"), QStringLiteral("return nullptr"),
                QStringLiteral("return true"), QStringLiteral("return false"),
                QStringLiteral("return;"), QStringLiteral("return NULL"),
            };
            // 提取 return 后的 token
        }
    }
    return issues;
}

} // namespace

namespace GitReviewService {

ToolResult reviewDiff(bool stagedOnly, int maxLines)
{
    QStringList arguments;
    arguments << QStringLiteral("diff");

    if (stagedOnly) {
        arguments << QStringLiteral("--cached");
    }

    QStringList statArgs = arguments;
    statArgs << QStringLiteral("--stat");
    const ToolResult statResult = runGitCommand(statArgs, 4096);

    QStringList diffArgs = arguments;
    const ToolResult diffResult = runGitCommand(diffArgs, 32768);

    QString output;
    output += QStringLiteral("=== Git Diff Summary ===\n");
    if (statResult.ok) {
        output += statResult.output;
    } else {
        output += QStringLiteral("(stat failed: %1)\n").arg(statResult.error);
    }

    output += QStringLiteral("\n=== Git Diff Details ===\n");
    if (diffResult.ok) {
        const QStringList lines = diffResult.output.split(QLatin1Char('\n'));
        if (lines.size() > maxLines) {
            QStringList limitedLines;
            for (int i = 0; i < maxLines && i < lines.size(); ++i) {
                limitedLines.append(lines.at(i));
            }
            output += limitedLines.join(QLatin1Char('\n'));
            output += QStringLiteral("\n... (diff truncated to %1 lines, %2 total)").arg(maxLines).arg(lines.size());
        } else {
            output += diffResult.output;
        }
    } else {
        if (statResult.ok) {
            output += QStringLiteral("(diff details unavailable: %1)").arg(diffResult.error);
        } else {
            return diffResult;
        }
    }

    return ToolResult::success(output);
}

ToolResult reviewLog(int maxCount, bool oneline)
{
    QStringList arguments;
    arguments << QStringLiteral("log");

    if (oneline) {
        arguments << QStringLiteral("--oneline");
    }

    if (maxCount > 0) {
        arguments << QStringLiteral("-n") << QString::number(maxCount);
    } else {
        arguments << QStringLiteral("-n") << QStringLiteral("20");
    }

    const ToolResult result = runGitCommand(arguments, 8192);
    if (!result.ok) {
        return result;
    }

    const QString output = QStringLiteral("=== Git Log (last %1 commits) ===\n%2")
                               .arg(maxCount)
                               .arg(result.output);
    return ToolResult::success(output);
}

// #27: 结构化审查 — 解析 diff 文本并输出问题列表
ToolResult structuredReview(const QString &diffText)
{
    if (diffText.trimmed().isEmpty()) {
        return ToolResult::success(QStringLiteral("No diff to review."));
    }

    QVector<ReviewIssue> allIssues;
    const QStringList lines = diffText.split(QLatin1Char('\n'));

    // 解析 diff header: diff --git a/path b/path
    QString currentFile;
    int currentLine = 0;
    QStringList fileLines;

    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("diff --git "))) {
            // flush previous file
            if (!currentFile.isEmpty() && !fileLines.isEmpty()) {
                allIssues += detectCommonPatterns(currentFile, fileLines, currentLine);
            }

            // parse: diff --git a/src/file.cpp b/src/file.cpp
            currentFile = line.mid(line.lastIndexOf(QLatin1Char(' ')) + 1);
            if (currentFile.startsWith(QLatin1Char('b'))) currentFile = currentFile.mid(1);
            currentLine = 0;
            fileLines.clear();
            continue;
        }

        // 解析 @@ -a,b +c,d @@ 格式
        if (line.startsWith(QStringLiteral("@@"))) {
            int plusPos = line.indexOf(QLatin1Char('+'), 1);
            if (plusPos > 0) {
                int commaPos = line.indexOf(QLatin1Char(','), plusPos);
                if (commaPos > plusPos) {
                    currentLine = line.mid(plusPos + 1, commaPos - plusPos - 1).toInt();
                } else {
                    int spacePos = line.indexOf(QLatin1Char(' '), plusPos);
                    if (spacePos > plusPos) {
                        currentLine = line.mid(plusPos + 1, spacePos - plusPos - 1).toInt();
                    }
                }
            }
            continue;
        }

        // 记录新增/修改的行
        if (line.startsWith(QLatin1Char('+')) && !line.startsWith(QStringLiteral("+++"))) {
            fileLines.append(line.mid(1));
            currentLine++;
        } else if (line.startsWith(QLatin1Char('-'))) {
            currentLine++;
        } else if (!line.startsWith(QStringLiteral("---")) &&
                   !line.startsWith(QStringLiteral("+++")) &&
                   !line.startsWith(QStringLiteral("index ")) &&
                   !line.startsWith(QStringLiteral("new file")) &&
                   !line.startsWith(QStringLiteral("deleted file"))) {
            // 上下文行（不以 +/- 开头）
            currentLine++;
        }
    }

    // flush last file
    if (!currentFile.isEmpty() && !fileLines.isEmpty()) {
        allIssues += detectCommonPatterns(currentFile, fileLines, currentLine);
    }

    // 构建输出
    if (allIssues.isEmpty()) {
        return ToolResult::success(QStringLiteral("No issues detected in the diff."));
    }

    QStringList output;
    output << QStringLiteral("## Structured Review Results\n");
    output << QStringLiteral("| Severity | File | Line | Issue | Suggestion |");
    output << QStringLiteral("|----------|------|:---:|-------|------------|");

    int errorCount = 0, warningCount = 0, infoCount = 0;
    for (const auto &issue : allIssues) {
        output << QStringLiteral("| %1 | %2 | %3 | %4 | %5 |")
                      .arg(issue.severity, issue.file)
                      .arg(issue.line)
                      .arg(issue.message, issue.suggestion);
        if (issue.severity == QStringLiteral("error")) ++errorCount;
        else if (issue.severity == QStringLiteral("warning")) ++warningCount;
        else ++infoCount;
    }

    output << QStringLiteral("\n**Summary**: %1 errors, %2 warnings, %3 info items")
                  .arg(errorCount).arg(warningCount).arg(infoCount);

    return ToolResult::success(output.join(QLatin1Char('\n')));
}

// #27: 记录审查结果到历史文件
void recordReviewToHistory(const QString &projectDir, const QVector<ReviewIssue> &issues)
{
    if (projectDir.isEmpty()) return;

    const QString historyPath = QDir(projectDir)
        .filePath(QStringLiteral(".workbuddy/memory/review-history.md"));
    QDir().mkpath(QFileInfo(historyPath).absolutePath());

    QFile file(historyPath);
    if (!file.open(QIODevice::Append | QIODevice::Text)) return;

    QTextStream out(&file);
    out << QStringLiteral("\n## Review %1\n")
               .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    for (const auto &issue : issues) {
        out << QStringLiteral("- [%1] %2:%3 — %4\n")
                   .arg(issue.severity, issue.file)
                   .arg(issue.line)
                   .arg(issue.message);
    }
    file.close();
}

// #27: 过滤出历史中未出现过的新问题
QVector<ReviewIssue> filterNewIssues(const QString &projectDir,
                                      const QVector<ReviewIssue> &issues)
{
    if (projectDir.isEmpty() || issues.isEmpty()) return issues;

    const QString historyPath = QDir(projectDir)
        .filePath(QStringLiteral(".workbuddy/memory/review-history.md"));

    QFile file(historyPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return issues;

    // 读取所有历史问题
    QSet<QString> seenIssues;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        // 匹配历史文件格式: - [severity] file:line — message
        if (line.startsWith(QLatin1Char('-')) && line.contains(QLatin1Char('['))) {
            seenIssues.insert(line);
        }
    }
    file.close();

    // 过滤
    QVector<ReviewIssue> newIssues;
    for (const auto &issue : issues) {
        const QString key = QStringLiteral("- [%1] %2:%3 — %4")
                                .arg(issue.severity, issue.file)
                                .arg(issue.line)
                                .arg(issue.message);
        if (!seenIssues.contains(key)) {
            newIssues.append(issue);
        }
    }

    return newIssues;
}

} // namespace GitReviewService
