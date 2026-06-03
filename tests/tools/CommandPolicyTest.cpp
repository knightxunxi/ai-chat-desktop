#include "tools/CommandPolicy.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    const QVector<CommandSpec> commands = CommandPolicy::allowedCommandTemplates();
    assert(commands.size() == 6);

    QSet<QString> ids;
    for (const CommandSpec &command : commands) {
        assert(!command.templateId.isEmpty());
        assert(!ids.contains(command.templateId));
        ids.insert(command.templateId);
        if (!command.internalOnly) {
            assert(!command.program.isEmpty());
            assert(!command.program.contains(QLatin1Char('/')));
            assert(!command.program.contains(QLatin1Char('\\')));
            assert(command.timeoutMs > 0);
        }
    }

    assert(CommandPolicy::commandTemplate(QStringLiteral("command.git_status")).has_value());
    assert(!CommandPolicy::commandTemplate(QStringLiteral("command.git_push")).has_value());
    assert(!CommandPolicy::commandTemplate(QStringLiteral("powershell")).has_value());

    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());
    const QString projectDirectory = temporaryDirectory.filePath(QStringLiteral("project"));
    assert(QDir().mkpath(projectDirectory));

    CommandPolicyDecision decision = CommandPolicy::evaluateCommand(
        QStringLiteral("command.git_status"),
        projectDirectory);
    assert(decision.allowed);
    assert(decision.command.templateId == QStringLiteral("command.git_status"));
    assert(decision.command.program == QStringLiteral("git"));
    assert(decision.command.arguments.contains(QStringLiteral("status")));
    assert(decision.command.workingDirectory == QDir::cleanPath(QFileInfo(projectDirectory).absoluteFilePath()));
    assert(CommandPolicy::commandDisplayText(decision.command) == QStringLiteral("git status --short --branch"));

    decision = CommandPolicy::evaluateCommand(QStringLiteral("command.cmake_build"), projectDirectory);
    assert(decision.allowed);
    assert(decision.command.risk == AgentToolRisk::Medium);
    assert(decision.command.timeoutMs == 60000);

    decision = CommandPolicy::evaluateCommand(QStringLiteral("command.list_project_files"), projectDirectory);
    assert(decision.allowed);
    assert(decision.command.internalOnly);
    assert(CommandPolicy::commandDisplayText(decision.command).isEmpty());

    decision = CommandPolicy::evaluateCommand(QStringLiteral("command.git_push"), projectDirectory);
    assert(!decision.allowed);
    assert(decision.reason.contains(QStringLiteral("not allowed"), Qt::CaseInsensitive));

    decision = CommandPolicy::evaluateCommand(QStringLiteral("command.git_status"), QString());
    assert(!decision.allowed);
    assert(decision.reason.contains(QStringLiteral("working directory"), Qt::CaseInsensitive));

    decision = CommandPolicy::evaluateCommand(QStringLiteral("command.git_status"), temporaryDirectory.filePath(QStringLiteral("missing")));
    assert(!decision.allowed);

    decision = CommandPolicy::evaluateCommand(QStringLiteral("command.git_status"), QDir::rootPath());
    assert(!decision.allowed);

    decision = CommandPolicy::evaluateCommand(QStringLiteral("command.git_status"), QDir::homePath());
    assert(!decision.allowed);

    return 0;
}
