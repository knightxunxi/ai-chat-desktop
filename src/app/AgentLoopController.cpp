#include "app/AgentLoopController.h"

#include "support/AppLogger.h"

#include <QElapsedTimer>
#include <QJsonDocument>
#include <QSet>

namespace {

QString compactParameters(const QJsonObject &parameters)
{
    return QString::fromUtf8(QJsonDocument(parameters).toJson(QJsonDocument::Compact));
}

QString actionFingerprint(const AgentPlanStep &step)
{
    return QStringLiteral("%1|%2|%3").arg(step.id, step.toolId, compactParameters(step.parameters));
}

int nextExecutableStepIndex(const AgentPlan &plan, const AgentToolRegistry &registry)
{
    for (int index = 0; index < plan.steps.size(); ++index) {
        const AgentPlanStep &step = plan.steps[index];
        if (step.status == AgentPlanStepStatus::Pending && registry.canExecuteDirectly(step.toolId)) {
            return index;
        }
    }

    return -1;
}

bool stopRequested(const AgentLoopOptions &options)
{
    return options.shouldStop && options.shouldStop();
}

void logAudit(QStringList *auditTrail, const QString &line)
{
    if (auditTrail != nullptr) {
        auditTrail->append(line);
    }
    AppLogger::info(QStringLiteral("AgentLoop"), line);
}

AgentLoopRunResult finishWith(
    AgentLoopRunResult result,
    AgentLoopRunStatus status,
    const QString &error = QString())
{
    result.status = status;
    result.error = error;
    logAudit(&result.auditTrail,
             QStringLiteral("Evaluate: status=%1 executedSteps=%2 error=%3")
                 .arg(AgentLoopController::runStatusToString(status))
                 .arg(result.executedStepCount)
                 .arg(error));
    return result;
}

} // namespace

namespace AgentLoopController {

QString runStatusToString(AgentLoopRunStatus status)
{
    switch (status) {
    case AgentLoopRunStatus::Completed:
        return QStringLiteral("completed");
    case AgentLoopRunStatus::Failed:
        return QStringLiteral("failed");
    case AgentLoopRunStatus::Stopped:
        return QStringLiteral("stopped");
    case AgentLoopRunStatus::StepLimitReached:
        return QStringLiteral("step_limit_reached");
    case AgentLoopRunStatus::RuntimeLimitReached:
        return QStringLiteral("runtime_limit_reached");
    case AgentLoopRunStatus::StepTimeout:
        return QStringLiteral("step_timeout");
    case AgentLoopRunStatus::RepeatedAction:
        return QStringLiteral("repeated_action");
    }

    return QStringLiteral("failed");
}

AgentLoopRunResult runPlan(
    AgentPlan *plan,
    const AgentToolRegistry &registry,
    const AgentToolExecutionContext &context,
    const AgentLoopOptions &options,
    const AgentLoopCallbacks &callbacks)
{
    AgentLoopRunResult result;
    if (plan == nullptr) {
        return finishWith(result, AgentLoopRunStatus::Failed, QStringLiteral("Agent plan is null."));
    }

    if (options.maxSteps <= 0) {
        return finishWith(result, AgentLoopRunStatus::Failed, QStringLiteral("Maximum step count must be positive."));
    }

    QElapsedTimer runtimeTimer;
    runtimeTimer.start();
    QSet<QString> seenActions;

    while (true) {
        logAudit(&result.auditTrail,
                 QStringLiteral("Observe: completedSteps=%1 totalSteps=%2 lastTool=%3 lastOutputLength=%4")
                     .arg(result.executedStepCount)
                     .arg(plan->steps.size())
                     .arg(result.lastToolId)
                     .arg(result.lastOutput.size()));

        if (stopRequested(options)) {
            return finishWith(result, AgentLoopRunStatus::Stopped);
        }

        if (result.executedStepCount >= options.maxSteps) {
            return finishWith(result, AgentLoopRunStatus::StepLimitReached);
        }

        if (runtimeTimer.elapsed() >= options.maxRuntimeMs) {
            return finishWith(result, AgentLoopRunStatus::RuntimeLimitReached);
        }

        const int index = nextExecutableStepIndex(*plan, registry);
        if (index < 0) {
            logAudit(&result.auditTrail, QStringLiteral("Think: done=true reason=no_executable_pending_step"));
            return finishWith(result, AgentLoopRunStatus::Completed);
        }

        AgentPlanStep &step = plan->steps[index];
        const QString fingerprint = actionFingerprint(step);
        if (seenActions.contains(fingerprint)) {
            return finishWith(
                result,
                AgentLoopRunStatus::RepeatedAction,
                QStringLiteral("Repeated Agent action detected for tool %1.").arg(step.toolId));
        }
        seenActions.insert(fingerprint);

        logAudit(&result.auditTrail,
                 QStringLiteral("Think: done=false stepId=%1 toolId=%2")
                     .arg(step.id, step.toolId));

        step.status = AgentPlanStepStatus::Running;
        if (callbacks.stepStarted) {
            callbacks.stepStarted(index);
        }

        QElapsedTimer stepTimer;
        stepTimer.start();
        const ToolResult toolResult = registry.execute(step.toolId, step.parameters, context);
        const qint64 stepElapsedMs = stepTimer.elapsed();

        result.lastToolId = step.toolId;
        result.lastOutput = toolResult.output;

        if (!toolResult.ok) {
            step.status = AgentPlanStepStatus::Failed;
            step.output = QStringLiteral("Tool failed: %1").arg(toolResult.error);
            result.lastOutput = step.output;
            if (callbacks.stepFinished) {
                callbacks.stepFinished(index, toolResult);
            }
            logAudit(&result.auditTrail,
                     QStringLiteral("Act: stepId=%1 toolId=%2 ok=false elapsedMs=%3 error=%4")
                         .arg(step.id, step.toolId)
                         .arg(stepElapsedMs)
                         .arg(toolResult.error));
            return finishWith(result, AgentLoopRunStatus::Failed, toolResult.error);
        }

        step.status = AgentPlanStepStatus::Completed;
        step.output = toolResult.output;
        ++result.executedStepCount;

        if (callbacks.stepFinished) {
            callbacks.stepFinished(index, toolResult);
        }

        logAudit(&result.auditTrail,
                 QStringLiteral("Act: stepId=%1 toolId=%2 ok=true elapsedMs=%3 outputLength=%4")
                     .arg(step.id, step.toolId)
                     .arg(stepElapsedMs)
                     .arg(toolResult.output.size()));

        if (stepElapsedMs >= options.maxStepMs) {
            return finishWith(
                result,
                AgentLoopRunStatus::StepTimeout,
                QStringLiteral("Agent step exceeded the time limit."));
        }
    }
}

} // namespace AgentLoopController
