#include "app/AgentPlanExecutor.h"

#include "support/AppLogger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QTemporaryDir>

#include <cassert>

namespace {

AgentPlanStep makeStep(const QString &toolId, const QString &input)
{
    AgentPlanStep step;
    step.id = QStringLiteral("step-1");
    step.title = QStringLiteral("Run tool");
    step.toolId = toolId;
    step.reason = QStringLiteral("Test execution.");
    step.risk = AgentToolRisk::Low;
    step.parameters.insert(QStringLiteral("input"), input);
    return step;
}

AgentPlanStep makeWorkspaceStep(const QString &toolId, const QString &path, const QString &content = QString())
{
    AgentPlanStep step;
    step.id = QStringLiteral("workspace-step");
    step.title = QStringLiteral("Run workspace tool");
    step.toolId = toolId;
    step.reason = QStringLiteral("Test workspace execution.");
    step.risk = AgentToolRisk::Medium;
    step.parameters.insert(QStringLiteral("path"), path);
    if (toolId == QStringLiteral("workspace.write_text") || toolId == QStringLiteral("workspace.overwrite_text")) {
        step.parameters.insert(QStringLiteral("content"), content);
    }
    return step;
}

AgentPlanStep makeCommandStep(const QString &toolId)
{
    AgentPlanStep step;
    step.id = QStringLiteral("command-step");
    step.title = QStringLiteral("Run command");
    step.toolId = toolId;
    step.reason = QStringLiteral("Test command execution.");
    step.risk = AgentToolRisk::Low;
    return step;
}

QString readFile(const QString &path)
{
    QFile file(path);
    assert(file.open(QFile::ReadOnly | QFile::Text));
    return QString::fromUtf8(file.readAll());
}

} // namespace

int main()
{
    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());
    const QString logPath = temporaryDirectory.filePath(QStringLiteral("agent-plan-executor.log"));
    AppLogger::setLogFilePathForTests(logPath);
    QString loggerError;
    assert(AppLogger::initialize(&loggerError));

    AgentPlanStep step = makeStep(QStringLiteral("json.format"), QStringLiteral("{\"name\":\"test\"}"));
    assert(AgentPlanExecutor::canExecuteDirectly(step));

    ToolResult result = AgentPlanExecutor::executeStep(step);
    assert(result.ok);
    assert(result.output.contains(QStringLiteral("\"name\": \"test\"")));

    step = makeStep(QStringLiteral("text.cleanup"), QStringLiteral(" A \n\n\n B "));
    result = AgentPlanExecutor::executeStep(step);
    assert(result.ok);
    assert(result.output == QStringLiteral("A\n\nB"));

    step = makeStep(QStringLiteral("json.format"), QString());
    result = AgentPlanExecutor::executeStep(step);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("input")));

    step = makeStep(QStringLiteral("file.read_text"), QStringLiteral("ignored"));
    assert(AgentPlanExecutor::canExecuteDirectly(step));
    result = AgentPlanExecutor::executeStep(step);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("path"), Qt::CaseInsensitive));

    const QString workspace = temporaryDirectory.filePath(QStringLiteral("workspace"));
    step = makeWorkspaceStep(QStringLiteral("workspace.write_text"), QStringLiteral("notes/hello.txt"), QStringLiteral("hello"));
    assert(AgentPlanExecutor::canExecuteDirectly(step));
    result = AgentPlanExecutor::executeStep(step, workspace);
    assert(result.ok);
    assert(readFile(QDir(workspace).filePath(QStringLiteral("notes/hello.txt"))) == QStringLiteral("hello"));

    step = makeWorkspaceStep(QStringLiteral("workspace.read_text"), QStringLiteral("notes/hello.txt"));
    result = AgentPlanExecutor::executeStep(step, workspace);
    assert(result.ok);
    assert(result.output.contains(QStringLiteral("UNTRUSTED WORKSPACE FILE DATA")));
    assert(result.output.contains(QStringLiteral("hello")));

    step = makeWorkspaceStep(QStringLiteral("workspace.overwrite_text"), QStringLiteral("notes/hello.txt"), QStringLiteral("new text"));
    step.risk = AgentToolRisk::High;
    result = AgentPlanExecutor::executeStep(step, workspace);
    assert(result.ok);
    assert(readFile(QDir(workspace).filePath(QStringLiteral("notes/hello.txt"))) == QStringLiteral("new text"));
    assert(readFile(QDir(workspace).filePath(QStringLiteral("notes/hello.txt.bak"))) == QStringLiteral("hello"));

    step = makeWorkspaceStep(QStringLiteral("workspace.delete_file"), QStringLiteral("notes/hello.txt"));
    step.risk = AgentToolRisk::High;
    result = AgentPlanExecutor::executeStep(step, workspace);
    assert(result.ok);
    assert(!QFileInfo::exists(QDir(workspace).filePath(QStringLiteral("notes/hello.txt"))));
    assert(QFileInfo::exists(QDir(workspace).filePath(QStringLiteral(".trash/notes/hello.txt"))));

    step = makeWorkspaceStep(QStringLiteral("workspace.write_text"), QStringLiteral("../outside.txt"), QStringLiteral("outside"));
    result = AgentPlanExecutor::executeStep(step, workspace);
    assert(!result.ok);

    step = makeWorkspaceStep(QStringLiteral("workspace.write_text"), QStringLiteral("missing-content.txt"));
    step.parameters.remove(QStringLiteral("content"));
    result = AgentPlanExecutor::executeStep(step, workspace);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("content")));

    step = makeWorkspaceStep(QStringLiteral("workspace.read_text"), QString());
    result = AgentPlanExecutor::executeStep(step, workspace);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("path")));

    const QString projectDirectory = temporaryDirectory.filePath(QStringLiteral("project"));
    assert(QDir().mkpath(projectDirectory));
    QFile readme(QDir(projectDirectory).filePath(QStringLiteral("README.md")));
    assert(readme.open(QFile::WriteOnly | QFile::Text));
    assert(readme.write("project") == 7);
    readme.close();

    step = makeCommandStep(QStringLiteral("command.list_project_files"));
    assert(AgentPlanExecutor::canExecuteDirectly(step));
    result = AgentPlanExecutor::executeStep(step, workspace, projectDirectory);
    assert(result.ok);
    assert(result.output.contains(QStringLiteral("[FILE] README.md")));

    result = AgentPlanExecutor::executeStep(step, workspace);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("working directory"), Qt::CaseInsensitive));

    return 0;
}
