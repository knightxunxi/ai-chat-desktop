#include "app/AgentLoopController.h"

#include "support/AppLogger.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>

namespace {

AgentPlanStep makeStep(const QString &id, const QString &toolId, const QJsonObject &parameters)
{
    AgentPlanStep step;
    step.id = id;
    step.title = QStringLiteral("Run %1").arg(toolId);
    step.toolId = toolId;
    step.reason = QStringLiteral("Test loop execution.");
    step.risk = toolId == QStringLiteral("workspace.delete_file") ? AgentToolRisk::High : AgentToolRisk::Medium;
    step.parameters = parameters;
    return step;
}

QJsonObject writeParameters(const QString &path, const QString &content)
{
    QJsonObject parameters;
    parameters.insert(QStringLiteral("path"), path);
    parameters.insert(QStringLiteral("content"), content);
    return parameters;
}

QJsonObject readParameters(const QString &path)
{
    QJsonObject parameters;
    parameters.insert(QStringLiteral("path"), path);
    return parameters;
}

QJsonObject textParameters(const QString &input)
{
    QJsonObject parameters;
    parameters.insert(QStringLiteral("input"), input);
    return parameters;
}

AgentPlan makeWorkspacePlan()
{
    AgentPlan plan;
    plan.summary = QStringLiteral("Create and read a file.");
    plan.steps.append(makeStep(QStringLiteral("step-1"), QStringLiteral("workspace.write_text"), writeParameters(QStringLiteral("notes/a.txt"), QStringLiteral("hello"))));
    plan.steps.append(makeStep(QStringLiteral("step-2"), QStringLiteral("workspace.read_text"), readParameters(QStringLiteral("notes/a.txt"))));
    return plan;
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
    AppLogger::setLogFilePathForTests(temporaryDirectory.filePath(QStringLiteral("agent-loop.log")));
    QString loggerError;
    assert(AppLogger::initialize(&loggerError));

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
    AgentToolExecutionContext context;
    context.workspaceDirectory = temporaryDirectory.filePath(QStringLiteral("workspace"));

    AgentPlan plan = makeWorkspacePlan();
    int startedCount = 0;
    int finishedCount = 0;
    AgentLoopCallbacks callbacks;
    callbacks.stepStarted = [&startedCount](int) {
        ++startedCount;
    };
    callbacks.stepFinished = [&finishedCount](int, const ToolResult &) {
        ++finishedCount;
    };
    AgentLoopRunResult result = AgentLoopController::runPlan(&plan, registry, context, AgentLoopOptions(), callbacks);
    assert(result.status == AgentLoopRunStatus::Completed);
    assert(result.executedStepCount == 2);
    assert(startedCount == 2);
    assert(finishedCount == 2);
    assert(plan.steps[0].status == AgentPlanStepStatus::Completed);
    assert(plan.steps[1].status == AgentPlanStepStatus::Completed);
    assert(readFile(QDir(context.workspaceDirectory).filePath(QStringLiteral("notes/a.txt"))) == QStringLiteral("hello"));
    assert(result.auditTrail.join(QLatin1Char('\n')).contains(QStringLiteral("Observe:")));
    assert(result.auditTrail.join(QLatin1Char('\n')).contains(QStringLiteral("Act:")));
    assert(result.auditTrail.join(QLatin1Char('\n')).contains(QStringLiteral("Evaluate:")));

    plan = makeWorkspacePlan();
    context.workspaceDirectory = temporaryDirectory.filePath(QStringLiteral("workspace-limit"));
    AgentLoopOptions limitedOptions;
    limitedOptions.maxSteps = 1;
    result = AgentLoopController::runPlan(&plan, registry, context, limitedOptions);
    assert(result.status == AgentLoopRunStatus::StepLimitReached);
    assert(result.executedStepCount == 1);
    assert(plan.steps[0].status == AgentPlanStepStatus::Completed);
    assert(plan.steps[1].status == AgentPlanStepStatus::Pending);

    plan = makeWorkspacePlan();
    context.workspaceDirectory = temporaryDirectory.filePath(QStringLiteral("workspace-stop"));
    int stopChecks = 0;
    AgentLoopOptions stopOptions;
    stopOptions.shouldStop = [&stopChecks]() {
        return stopChecks++ > 0;
    };
    result = AgentLoopController::runPlan(&plan, registry, context, stopOptions);
    assert(result.status == AgentLoopRunStatus::Stopped);
    assert(result.executedStepCount == 1);
    assert(plan.steps[1].status == AgentPlanStepStatus::Pending);

    AgentPlan failingPlan;
    context.workspaceDirectory = temporaryDirectory.filePath(QStringLiteral("workspace-fail"));
    // V17.4: 沙箱已移除 — 改为使用受保护路径 (.env) 来触发失败
    failingPlan.summary = QStringLiteral("Fail on protected file.");
    failingPlan.steps.append(makeStep(QStringLiteral("step-1"), QStringLiteral("workspace.write_text"), writeParameters(QStringLiteral(".env"), QStringLiteral("SECRET=bad"))));
    result = AgentLoopController::runPlan(&failingPlan, registry, context);
    assert(result.status == AgentLoopRunStatus::Failed);
    assert(result.executedStepCount == 0);
    assert(failingPlan.steps[0].status == AgentPlanStepStatus::Failed);
    assert(failingPlan.steps[0].output.contains(QStringLiteral("Tool failed")));
    assert(result.error.contains(QStringLiteral("Protected"), Qt::CaseInsensitive));

    AgentPlan repeatedPlan;
    context.workspaceDirectory = temporaryDirectory.filePath(QStringLiteral("workspace-repeat"));
    repeatedPlan.summary = QStringLiteral("Repeat same action.");
    repeatedPlan.steps.append(makeStep(QStringLiteral("repeat-step"), QStringLiteral("text.cleanup"), textParameters(QStringLiteral("A\n\n\nB"))));
    repeatedPlan.steps.append(makeStep(QStringLiteral("repeat-step"), QStringLiteral("text.cleanup"), textParameters(QStringLiteral("A\n\n\nB"))));
    result = AgentLoopController::runPlan(&repeatedPlan, registry, context);
    assert(result.status == AgentLoopRunStatus::RepeatedAction);
    assert(result.executedStepCount == 1);
    assert(repeatedPlan.steps[0].status == AgentPlanStepStatus::Completed);
    assert(repeatedPlan.steps[1].status == AgentPlanStepStatus::Pending);

    assert(AgentLoopController::runStatusToString(AgentLoopRunStatus::Completed) == QStringLiteral("completed"));
    assert(AgentLoopController::runStatusToString(AgentLoopRunStatus::StepTimeout) == QStringLiteral("step_timeout"));

    const QString logs = readFile(temporaryDirectory.filePath(QStringLiteral("agent-loop.log")));
    assert(logs.contains(QStringLiteral("AgentLoop")));
    assert(!logs.contains(QStringLiteral("bad")));

    return 0;
}
