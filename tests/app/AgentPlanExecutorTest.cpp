#include "app/AgentPlanExecutor.h"

#include <QJsonObject>

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

} // namespace

int main()
{
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
    assert(!AgentPlanExecutor::canExecuteDirectly(step));
    result = AgentPlanExecutor::executeStep(step);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("file picker"), Qt::CaseInsensitive));

    return 0;
}
