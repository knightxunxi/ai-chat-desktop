#include "tools/CommandRunner.h"

#include "support/AppLogger.h"

#include <QProcess>
#include <QRegularExpression>
#include <QVector>

namespace {

QString yesNo(bool value)
{
    return value ? QStringLiteral("yes") : QStringLiteral("no");
}

QString decodedProcessOutput(const QByteArray &output)
{
    return QString::fromUtf8(output);
}

QString outputSection(const QString &title, const QString &content)
{
    if (content.trimmed().isEmpty()) {
        return QStringLiteral("%1:\n(empty)").arg(title);
    }

    return QStringLiteral("%1:\n%2").arg(title, content);
}

QString buildSummary(
    const CommandSpec &command,
    int exitCode,
    bool timedOut,
    const QString &stdoutText,
    const QString &stderrText,
    int maxOutputCharacters)
{
    const QString stdoutRedacted = CommandRunner::truncateOutput(
        CommandRunner::redactSensitiveOutput(stdoutText),
        maxOutputCharacters);
    const QString stderrRedacted = CommandRunner::truncateOutput(
        CommandRunner::redactSensitiveOutput(stderrText),
        maxOutputCharacters);

    return QStringLiteral(
               "Command template: %1\n"
               "Command: %2\n"
               "Working directory: %3\n"
               "Exit code: %4\n"
               "Timed out: %5\n"
               "stdout length: %6\n"
               "stderr length: %7\n\n"
               "%8\n\n"
               "%9")
        .arg(command.templateId,
             CommandPolicy::commandDisplayText(command),
             command.workingDirectory,
             QString::number(exitCode),
             yesNo(timedOut),
             QString::number(stdoutText.size()),
             QString::number(stderrText.size()),
             outputSection(QStringLiteral("STDOUT"), stdoutRedacted),
             outputSection(QStringLiteral("STDERR"), stderrRedacted));
}

} // namespace

namespace CommandRunner {

QString redactSensitiveOutput(const QString &text)
{
    QString redacted = text;
    const QVector<QRegularExpression> patterns = {
        QRegularExpression(QStringLiteral(R"((Bearer\s+)[A-Za-z0-9._\-]+)"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral(R"(((api[_-]?key|token|secret|password)\s*[:=]\s*)[^\s]+)"), QRegularExpression::CaseInsensitiveOption)
    };

    for (const QRegularExpression &pattern : patterns) {
        QRegularExpressionMatchIterator iterator = pattern.globalMatch(redacted);
        int offset = 0;
        while (iterator.hasNext()) {
            const QRegularExpressionMatch match = iterator.next();
            const int start = match.capturedStart(0) + offset;
            const int length = match.capturedLength(0);
            const QString replacement = match.captured(1) + QStringLiteral("[REDACTED]");
            redacted.replace(start, length, replacement);
            offset += replacement.size() - length;
        }
    }

    return redacted;
}

QString truncateOutput(const QString &text, int maxCharacters)
{
    if (maxCharacters <= 0 || text.size() <= maxCharacters) {
        return text;
    }

    return text.left(maxCharacters)
        + QStringLiteral("\n...[truncated %1 characters]").arg(text.size() - maxCharacters);
}

ToolResult run(const CommandSpec &command, int maxOutputCharacters)
{
    if (command.internalOnly) {
        return ToolResult::failure(QStringLiteral("Internal command cannot be executed by CommandRunner."));
    }
    if (command.program.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Command program is empty."));
    }
    if (command.workingDirectory.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Command working directory is empty."));
    }

    QProcess process;
    process.setProgram(command.program);
    process.setArguments(command.arguments);
    process.setWorkingDirectory(command.workingDirectory);
    process.setProcessChannelMode(QProcess::SeparateChannels);

    AppLogger::info(QStringLiteral("CommandRunner"),
                    QStringLiteral("Command started. templateId=%1 command=%2 cwdLength=%3 timeoutMs=%4")
                        .arg(command.templateId,
                             CommandPolicy::commandDisplayText(command),
                             QString::number(command.workingDirectory.size()),
                             QString::number(command.timeoutMs)));

    process.start();
    if (!process.waitForStarted(3000)) {
        AppLogger::warning(QStringLiteral("CommandRunner"),
                           QStringLiteral("Command failed to start. templateId=%1").arg(command.templateId));
        return ToolResult::failure(QStringLiteral("Command failed to start: %1").arg(process.errorString()));
    }

    bool timedOut = false;
    if (!process.waitForFinished(command.timeoutMs)) {
        timedOut = true;
        process.kill();
        process.waitForFinished(2000);
    }

    const int exitCode = process.exitCode();
    const QString stdoutText = decodedProcessOutput(process.readAllStandardOutput());
    const QString stderrText = decodedProcessOutput(process.readAllStandardError());
    const QString summary = buildSummary(command, exitCode, timedOut, stdoutText, stderrText, maxOutputCharacters);

    AppLogger::info(QStringLiteral("CommandRunner"),
                    QStringLiteral("Command finished. templateId=%1 exitCode=%2 timedOut=%3 stdoutLength=%4 stderrLength=%5")
                        .arg(command.templateId,
                             QString::number(exitCode),
                             timedOut ? QStringLiteral("true") : QStringLiteral("false"),
                             QString::number(stdoutText.size()),
                             QString::number(stderrText.size())));

    if (timedOut) {
        return ToolResult::failure(summary);
    }
    if (process.exitStatus() != QProcess::NormalExit || exitCode != 0) {
        return ToolResult::failure(summary);
    }

    return ToolResult::success(summary);
}

} // namespace CommandRunner
