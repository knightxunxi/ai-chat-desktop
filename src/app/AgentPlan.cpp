#include "app/AgentPlan.h"

AgentPlanParseResult AgentPlanParseResult::success(const AgentPlan &plan)
{
    AgentPlanParseResult result;
    result.ok = true;
    result.plan = plan;
    return result;
}

AgentPlanParseResult AgentPlanParseResult::failure(const QString &error)
{
    AgentPlanParseResult result;
    result.ok = false;
    result.error = error;
    return result;
}

QString agentPlanStepStatusToString(AgentPlanStepStatus status)
{
    switch (status) {
    case AgentPlanStepStatus::Pending:
        return QStringLiteral("pending");
    case AgentPlanStepStatus::Confirmed:
        return QStringLiteral("confirmed");
    case AgentPlanStepStatus::Skipped:
        return QStringLiteral("skipped");
    case AgentPlanStepStatus::Running:
        return QStringLiteral("running");
    case AgentPlanStepStatus::Completed:
        return QStringLiteral("completed");
    case AgentPlanStepStatus::Failed:
        return QStringLiteral("failed");
    case AgentPlanStepStatus::Canceled:
        return QStringLiteral("canceled");
    }

    return QStringLiteral("failed");
}
