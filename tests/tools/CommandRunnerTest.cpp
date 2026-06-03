#include "support/AppLogger.h"
#include "tools/CommandRunner.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QThread>

#include <cassert>
#include <iostream>

namespace {

CommandSpec selfCommand(const QStringList &arguments, const QString &workingDirectory, int timeoutMs)
{
    CommandSpec command;
    command.templateId = QStringLiteral("test.self");
    command.program = QCoreApplication::applicationFilePath();
    command.arguments = arguments;
    command.workingDirectory = workingDirectory;
    command.timeoutMs = timeoutMs;
    return command;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList arguments = app.arguments();
    if (arguments.contains(QStringLiteral("--child-ok"))) {
        std::cout << "child ok\n";
        std::cout << "api_key=secret-value\n";
        return 0;
    }
    if (arguments.contains(QStringLiteral("--child-fail"))) {
        std::cerr << "child failed\n";
        return 7;
    }
    if (arguments.contains(QStringLiteral("--child-sleep"))) {
        QThread::msleep(1500);
        std::cout << "woke up\n";
        return 0;
    }

    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());
    AppLogger::setLogFilePathForTests(temporaryDirectory.filePath(QStringLiteral("command-runner.log")));
    QString loggerError;
    assert(AppLogger::initialize(&loggerError));

    assert(CommandRunner::redactSensitiveOutput(QStringLiteral("Bearer abc.def")).contains(QStringLiteral("[REDACTED]")));
    assert(!CommandRunner::redactSensitiveOutput(QStringLiteral("token=abc123")).contains(QStringLiteral("abc123")));
    assert(CommandRunner::truncateOutput(QStringLiteral("abcdef"), 3).contains(QStringLiteral("truncated")));

    ToolResult result = CommandRunner::run(
        selfCommand({QStringLiteral("--child-ok")}, temporaryDirectory.path(), 3000),
        2000);
    assert(result.ok);
    assert(result.output.contains(QStringLiteral("child ok")));
    assert(result.output.contains(QStringLiteral("api_key=[REDACTED]")));
    assert(!result.output.contains(QStringLiteral("secret-value")));
    assert(result.output.contains(QStringLiteral("Exit code: 0")));

    result = CommandRunner::run(
        selfCommand({QStringLiteral("--child-fail")}, temporaryDirectory.path(), 3000),
        2000);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("Exit code: 7")));
    assert(result.error.contains(QStringLiteral("child failed")));

    result = CommandRunner::run(
        selfCommand({QStringLiteral("--child-sleep")}, temporaryDirectory.path(), 100),
        2000);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("Timed out: yes")));

    CommandSpec internalCommand;
    internalCommand.templateId = QStringLiteral("command.list_project_files");
    internalCommand.internalOnly = true;
    result = CommandRunner::run(internalCommand);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("Internal command")));

    CommandSpec missingDirectoryCommand = selfCommand({QStringLiteral("--child-ok")}, QString(), 1000);
    result = CommandRunner::run(missingDirectoryCommand);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("working directory")));

    return 0;
}
