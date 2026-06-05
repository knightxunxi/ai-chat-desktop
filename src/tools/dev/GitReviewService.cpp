#include "tools/dev/GitReviewService.h"

#include <QProcess>
#include <QStringList>

// 学习注释：GitReviewService 是对 git 只读命令的受控封装。
// 所有命令通过 QProcess 以程序+参数数组形式执行，不使用 shell 字符串。
// 禁止的命令（add/commit/push/reset 等）不在此服务中暴露。

namespace {

// 功能：执行 git 命令并返回 stdout 输出；使用模块：reviewDiff/reviewLog。
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

} // namespace

namespace GitReviewService {

ToolResult reviewDiff(bool stagedOnly, int maxLines)
{
    QStringList arguments;
    arguments << QStringLiteral("diff");

    if (stagedOnly) {
        arguments << QStringLiteral("--cached");
    }

    // 先获取 --stat 概览
    QStringList statArgs = arguments;
    statArgs << QStringLiteral("--stat");
    const ToolResult statResult = runGitCommand(statArgs, 4096);

    // 再获取文本 diff（限制行数）
    QStringList diffArgs = arguments;
    const ToolResult diffResult = runGitCommand(diffArgs, 32768);

    // 构建摘要输出
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
        // diff 失败但 stat 可能成功 — 返回部分信息
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

} // namespace GitReviewService
