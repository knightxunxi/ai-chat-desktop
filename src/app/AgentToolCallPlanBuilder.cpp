#include "app/AgentToolCallPlanBuilder.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

namespace {

QString failureAtCall(int index, const QString &message)
{
    return QStringLiteral("Tool call %1: %2").arg(index + 1).arg(message);
}

QString uniqueStepId(const QString &candidateId, int index, QSet<QString> *usedIds)
{
    QString id = candidateId.trimmed();
    if (id.isEmpty()) {
        id = QStringLiteral("tool-call-%1").arg(index + 1);
    }

    const QString baseId = id;
    int suffix = 2;
    while (usedIds->contains(id)) {
        id = QStringLiteral("%1-%2").arg(baseId).arg(suffix);
        ++suffix;
    }

    usedIds->insert(id);
    return id;
}

AgentPlanParseResult parseArguments(const QString &arguments, QJsonObject *parameters)
{
    const QString trimmed = arguments.trimmed();
    if (trimmed.isEmpty()) {
        *parameters = QJsonObject();
        return AgentPlanParseResult::success(AgentPlan());
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return AgentPlanParseResult::failure(
            QStringLiteral("arguments JSON parse error at offset %1: %2")
                .arg(parseError.offset)
                .arg(parseError.errorString()));
    }

    if (!document.isObject()) {
        return AgentPlanParseResult::failure(QStringLiteral("arguments must be a JSON object."));
    }

    *parameters = document.object();
    return AgentPlanParseResult::success(AgentPlan());
}

} // namespace

namespace AgentToolCallPlanBuilder {

AgentPlanParseResult buildPlanFromToolCalls(
    const ToolCallList &toolCalls,
    const AgentToolRegistry &registry,
    AppLanguage language,
    int maxSteps)
{
    if (maxSteps <= 0) {
        return AgentPlanParseResult::failure(QStringLiteral("Maximum step count must be positive."));
    }

    if (toolCalls.isEmpty()) {
        return AgentPlanParseResult::failure(QStringLiteral("Tool calls must not be empty."));
    }

    if (toolCalls.size() > maxSteps) {
        return AgentPlanParseResult::failure(
            QStringLiteral("Tool calls contain %1 steps, but the limit is %2.")
                .arg(toolCalls.size())
                .arg(maxSteps));
    }

    AgentPlan plan;
    plan.summary = language == AppLanguage::Chinese
                       ? QStringLiteral("模型通过原生工具调用生成了执行计划。")
                       : QStringLiteral("The model generated an execution plan using native tool calls.");

    QSet<QString> usedIds;
    for (int index = 0; index < toolCalls.size(); ++index) {
        const ToolCall &toolCall = toolCalls.at(index);
        const QString functionName = toolCall.functionName.trimmed();
        if (functionName.isEmpty()) {
            return AgentPlanParseResult::failure(failureAtCall(index, QStringLiteral("function name must not be empty.")));
        }

        const AgentToolDefinition *definition = registry.findByFunctionName(functionName);
        if (definition == nullptr || !definition->descriptor.enabledForAgent || !definition->executableFromPlanPreview) {
            return AgentPlanParseResult::failure(
                failureAtCall(index, QStringLiteral("function name \"%1\" not found. Try using tool IDs with underscores instead of dots (e.g. system_path).").arg(functionName)));
        }

        QJsonObject parameters;
        const AgentPlanParseResult argumentResult = parseArguments(toolCall.arguments, &parameters);
        if (!argumentResult.ok) {
            return AgentPlanParseResult::failure(failureAtCall(index, argumentResult.error));
        }

        AgentPlanStep step;
        step.id = uniqueStepId(toolCall.id, index, &usedIds);
        step.title = agentToolDisplayName(definition->descriptor, language);
        step.toolId = definition->descriptor.id;
        step.reason = language == AppLanguage::Chinese
                          ? QStringLiteral("模型通过 Function Calling 选择了这个工具。")
                          : QStringLiteral("The model selected this tool through Function Calling.");
        step.risk = definition->descriptor.risk;
        step.parameters = parameters;
        plan.steps.append(step);
    }

    return AgentPlanParseResult::success(plan);
}

} // namespace AgentToolCallPlanBuilder
