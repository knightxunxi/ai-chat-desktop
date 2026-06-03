#include "app/AgentPlanExecutor.h"

#include "support/AppLogger.h"
#include "tools/AgentToolRegistry.h"
#include "tools/WorkspacePolicy.h"

namespace AgentPlanExecutor {

bool canExecuteDirectly(const AgentPlanStep &step)
{
    return AgentToolRegistryFactory::defaultRegistry().canExecuteDirectly(step.toolId);
}

ToolResult executeStep(const AgentPlanStep &step)
{
    return executeStep(step, WorkspacePolicy::defaultWorkspaceDirectory());
}

ToolResult executeStep(const AgentPlanStep &step, const QString &workspaceDirectory)
{
    return executeStep(step, workspaceDirectory, QString());
}

ToolResult executeStep(const AgentPlanStep &step, const QString &workspaceDirectory, const QString &projectDirectory)
{
    AgentToolExecutionContext context;
    context.workspaceDirectory = workspaceDirectory;
    context.projectDirectory = projectDirectory;

    const ToolResult result = AgentToolRegistryFactory::defaultRegistry().execute(step.toolId, step.parameters, context);
    AppLogger::info(QStringLiteral("AgentPlan"),
                    QStringLiteral("Registry step executed. stepId=%1 toolId=%2 ok=%3 outputLength=%4")
                        .arg(step.id, step.toolId, result.ok ? QStringLiteral("true") : QStringLiteral("false"))
                        .arg(result.output.size()));
    return result;
}

} // namespace AgentPlanExecutor
