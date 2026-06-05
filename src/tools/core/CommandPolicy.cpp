#include "tools/core/CommandPolicy.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace {

QString absoluteCleanPath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

QString comparePath(const QString &path)
{
    QString normalized = QDir::cleanPath(path);
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
#ifdef Q_OS_WIN
    normalized = normalized.toLower();
#endif
    return normalized;
}

bool pathMatchesOrStartsWith(const QString &path, const QString &root)
{
    const QString normalizedPath = comparePath(path);
    const QString normalizedRoot = comparePath(root);
    return normalizedPath == normalizedRoot || normalizedPath.startsWith(normalizedRoot + QLatin1Char('/'));
}

bool isBlockedSystemDirectory(const QString &directory)
{
    const QString normalizedDirectory = comparePath(directory);
    QStringList blockedRoots;

    const QString windowsDirectory = qEnvironmentVariable("WINDIR");
    const QString programFiles = qEnvironmentVariable("ProgramFiles");
    const QString programFilesX86 = qEnvironmentVariable("ProgramFiles(x86)");
    const QString programData = qEnvironmentVariable("ProgramData");

    if (!windowsDirectory.trimmed().isEmpty()) {
        blockedRoots.append(windowsDirectory);
    }
    if (!programFiles.trimmed().isEmpty()) {
        blockedRoots.append(programFiles);
    }
    if (!programFilesX86.trimmed().isEmpty()) {
        blockedRoots.append(programFilesX86);
    }
    if (!programData.trimmed().isEmpty()) {
        blockedRoots.append(programData);
    }

    for (const QString &blockedRoot : blockedRoots) {
        if (pathMatchesOrStartsWith(normalizedDirectory, blockedRoot)) {
            return true;
        }
    }

    return false;
}

bool hasShellMetaCharacters(const QString &value)
{
    const QStringList blockedFragments = {
        QStringLiteral("&&"),
        QStringLiteral("||"),
        QStringLiteral(";"),
        QStringLiteral("|"),
        QStringLiteral(">"),
        QStringLiteral("<"),
        QStringLiteral("`")
    };

    for (const QString &fragment : blockedFragments) {
        if (value.contains(fragment)) {
            return true;
        }
    }

    return false;
}

bool isBlockedProgramName(const QString &program)
{
    const QString normalized = program.trimmed().toLower();
    const QStringList blockedPrograms = {
        QStringLiteral("powershell"),
        QStringLiteral("powershell.exe"),
        QStringLiteral("pwsh"),
        QStringLiteral("pwsh.exe"),
        QStringLiteral("cmd"),
        QStringLiteral("cmd.exe"),
        QStringLiteral("del"),
        QStringLiteral("erase"),
        QStringLiteral("rd"),
        QStringLiteral("rmdir"),
        QStringLiteral("remove-item"),
        QStringLiteral("reg"),
        QStringLiteral("reg.exe"),
        QStringLiteral("sc"),
        QStringLiteral("sc.exe"),
        QStringLiteral("net"),
        QStringLiteral("net.exe"),
        QStringLiteral("shutdown"),
        QStringLiteral("shutdown.exe"),
        QStringLiteral("format"),
        QStringLiteral("format.com"),
        QStringLiteral("diskpart"),
        QStringLiteral("diskpart.exe")
    };

    return blockedPrograms.contains(normalized);
}

CommandSpec makeCommand(
    const QString &templateId,
    const QString &program,
    const QStringList &arguments,
    AgentToolRisk risk,
    int timeoutMs)
{
    CommandSpec command;
    command.templateId = templateId;
    command.program = program;
    command.arguments = arguments;
    command.risk = risk;
    command.timeoutMs = timeoutMs;
    return command;
}

} // namespace

namespace CommandPolicy {

QVector<CommandSpec> allowedCommandTemplates()
{
    QVector<CommandSpec> commands;
    commands.reserve(6);
    commands.append(makeCommand(
        QStringLiteral("command.git_status"),
        QStringLiteral("git"),
        {QStringLiteral("status"), QStringLiteral("--short"), QStringLiteral("--branch")},
        AgentToolRisk::Low,
        15000));
    commands.append(makeCommand(
        QStringLiteral("command.git_diff_check"),
        QStringLiteral("git"),
        {QStringLiteral("diff"), QStringLiteral("--check")},
        AgentToolRisk::Low,
        20000));
    commands.append(makeCommand(
        QStringLiteral("command.git_diff_stat"),
        QStringLiteral("git"),
        {QStringLiteral("diff"), QStringLiteral("--stat")},
        AgentToolRisk::Low,
        20000));
    commands.append(makeCommand(
        QStringLiteral("command.cmake_build"),
        QStringLiteral("cmake"),
        {QStringLiteral("--build"), QStringLiteral("build-qt")},
        AgentToolRisk::Medium,
        60000));
    commands.append(makeCommand(
        QStringLiteral("command.ctest"),
        QStringLiteral("ctest"),
        {QStringLiteral("--test-dir"), QStringLiteral("build-qt"), QStringLiteral("--output-on-failure")},
        AgentToolRisk::Medium,
        60000));

    CommandSpec listProjectFiles;
    listProjectFiles.templateId = QStringLiteral("command.list_project_files");
    listProjectFiles.risk = AgentToolRisk::Low;
    listProjectFiles.internalOnly = true;
    commands.append(listProjectFiles);
    return commands;
}

std::optional<CommandSpec> commandTemplate(const QString &templateId)
{
    const QString normalizedId = templateId.trimmed();
    for (const CommandSpec &command : allowedCommandTemplates()) {
        if (command.templateId == normalizedId) {
            return command;
        }
    }

    return std::nullopt;
}

bool isSafeWorkingDirectory(const QString &workingDirectory)
{
    const QString trimmedDirectory = workingDirectory.trimmed();
    if (trimmedDirectory.isEmpty()) {
        return false;
    }

    const QFileInfo directoryInfo(absoluteCleanPath(trimmedDirectory));
    if (!directoryInfo.exists() || !directoryInfo.isDir()) {
        return false;
    }

    const QString directory = absoluteCleanPath(trimmedDirectory);
    if (comparePath(directory) == comparePath(QDir::rootPath())) {
        return false;
    }

    if (comparePath(directory) == comparePath(QDir::homePath())) {
        return false;
    }

    if (isBlockedSystemDirectory(directory)) {
        return false;
    }

    return true;
}

CommandPolicyDecision evaluateCommand(const QString &templateId, const QString &projectDirectory)
{
    CommandPolicyDecision decision;
    const std::optional<CommandSpec> commandTemplateValue = commandTemplate(templateId);
    if (!commandTemplateValue.has_value()) {
        decision.reason = QStringLiteral("Command template is not allowed.");
        return decision;
    }

    CommandSpec command = commandTemplateValue.value();
    if (!command.internalOnly) {
        if (command.program.trimmed().isEmpty()) {
            decision.reason = QStringLiteral("Command program is empty.");
            return decision;
        }
        if (command.program.contains(QLatin1Char('/')) || command.program.contains(QLatin1Char('\\'))) {
            decision.reason = QStringLiteral("Command program must be a fixed executable name, not a path.");
            return decision;
        }
        if (isBlockedProgramName(command.program)) {
            decision.reason = QStringLiteral("Command program is blocked.");
            return decision;
        }
    }

    if (!isSafeWorkingDirectory(projectDirectory)) {
        decision.reason = QStringLiteral("Command working directory is unsafe.");
        return decision;
    }

    for (const QString &argument : command.arguments) {
        if (hasShellMetaCharacters(argument)) {
            decision.reason = QStringLiteral("Command arguments must not contain shell metacharacters.");
            return decision;
        }
    }

    command.workingDirectory = absoluteCleanPath(projectDirectory);
    decision.allowed = true;
    decision.reason = QStringLiteral("Command is allowed by the local command policy.");
    decision.command = command;
    return decision;
}

QString commandDisplayText(const CommandSpec &command)
{
    QStringList parts;
    if (!command.program.trimmed().isEmpty()) {
        parts.append(command.program);
    }
    parts.append(command.arguments);
    return parts.join(QLatin1Char(' ')).trimmed();
}

} // namespace CommandPolicy
