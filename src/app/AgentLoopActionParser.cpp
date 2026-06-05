#include "app/AgentLoopActionParser.h"
#include "app/AgentJsonHelpers.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

namespace {

AgentLoopActionParseResult parseStepObject(
    const QJsonObject &stepObject,
    const QVector<AgentToolDescriptor> &toolCatalog,
    AgentLoopAction action)
{
    bool fieldOk = true;
    AgentPlanStep step;
    step.id = requiredString(stepObject, QStringLiteral("id"), &fieldOk);
    if (!fieldOk) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step.id must be a non-empty string."));
    }

    fieldOk = true;
    step.title = requiredString(stepObject, QStringLiteral("title"), &fieldOk);
    if (!fieldOk) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step.title must be a non-empty string."));
    }

    fieldOk = true;
    step.toolId = requiredString(stepObject, QStringLiteral("toolId"), &fieldOk);
    if (!fieldOk) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step.toolId must be a non-empty string."));
    }

    const AgentToolDescriptor *descriptor = findAgentToolDescriptor(toolCatalog, step.toolId);
    if (descriptor == nullptr || !descriptor->enabledForAgent) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step.toolId is not available in the Agent tool catalog."));
    }

    fieldOk = true;
    step.reason = requiredString(stepObject, QStringLiteral("reason"), &fieldOk);
    if (!fieldOk) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step.reason must be a non-empty string."));
    }

    fieldOk = true;
    const QString riskText = requiredString(stepObject, QStringLiteral("risk"), &fieldOk);
    if (!fieldOk) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step.risk must be a non-empty string."));
    }

    AgentToolRisk aiRisk = AgentToolRisk::Low;
    if (!agentToolRiskFromString(riskText, &aiRisk)) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step.risk must be one of low, medium, or high."));
    }
    step.risk = maxAgentToolRisk(aiRisk, descriptor->risk);

    const QJsonValue parametersValue = stepObject.value(QStringLiteral("parameters"));
    if (parametersValue.isUndefined()) {
        step.parameters = QJsonObject();
    } else if (!parametersValue.isObject()) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step.parameters must be an object when provided."));
    } else {
        step.parameters = parametersValue.toObject();
    }

    action.step = step;
    return AgentLoopActionParseResult::success(action);
}

} // namespace

AgentLoopActionParseResult AgentLoopActionParseResult::success(const AgentLoopAction &action)
{
    AgentLoopActionParseResult result;
    result.ok = true;
    result.action = action;
    return result;
}

AgentLoopActionParseResult AgentLoopActionParseResult::failure(const QString &error)
{
    AgentLoopActionParseResult result;
    result.ok = false;
    result.error = error;
    return result;
}

namespace AgentLoopActionParser {

AgentLoopActionParseResult parseJsonAction(
    const QString &jsonText,
    const QVector<AgentToolDescriptor> &toolCatalog)
{
    const QString candidateJson = extractJsonObjectCandidate(jsonText);
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(candidateJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return AgentLoopActionParseResult::failure(
            QStringLiteral("Action JSON parse error at offset %1: %2")
                .arg(parseError.offset)
                .arg(parseError.errorString()));
    }

    if (!document.isObject()) {
        return AgentLoopActionParseResult::failure(QStringLiteral("Action root must be a JSON object."));
    }

    const QJsonObject root = document.object();
    const QJsonValue doneValue = root.value(QStringLiteral("done"));
    if (!doneValue.isBool()) {
        return AgentLoopActionParseResult::failure(QStringLiteral("done must be a boolean."));
    }

    AgentLoopAction action;
    action.done = doneValue.toBool();
    action.message = optionalString(root, QStringLiteral("message"));

    const QJsonValue stepValue = root.value(QStringLiteral("step"));
    if (action.done) {
        if (stepValue.isObject()) {
            return AgentLoopActionParseResult::failure(QStringLiteral("step must be omitted when done is true."));
        }
        return AgentLoopActionParseResult::success(action);
    }

    if (!stepValue.isObject()) {
        return AgentLoopActionParseResult::failure(QStringLiteral("step must be an object when done is false."));
    }

    return parseStepObject(stepValue.toObject(), toolCatalog, action);
}

} // namespace AgentLoopActionParser
