#include "app/AgentPlanParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>

namespace {

QString requiredString(const QJsonObject &object, const QString &fieldName, bool *ok)
{
    const QJsonValue value = object.value(fieldName);
    if (!value.isString()) {
        *ok = false;
        return QString();
    }

    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        *ok = false;
        return QString();
    }

    return text;
}

AgentPlanParseResult failureAtStep(int stepIndex, const QString &message)
{
    return AgentPlanParseResult::failure(QStringLiteral("Step %1: %2").arg(stepIndex + 1).arg(message));
}

QString stripMarkdownFence(QString text)
{
    text = text.trimmed();
    if (!text.startsWith(QStringLiteral("```"))) {
        return text;
    }

    const int firstLineEnd = text.indexOf(QLatin1Char('\n'));
    if (firstLineEnd < 0) {
        return text;
    }

    text = text.mid(firstLineEnd + 1).trimmed();
    if (text.endsWith(QStringLiteral("```"))) {
        text.chop(3);
    }

    return text.trimmed();
}

QString extractJsonObjectCandidate(const QString &text)
{
    const QString unfenced = stripMarkdownFence(text);
    const int firstBrace = unfenced.indexOf(QLatin1Char('{'));
    const int lastBrace = unfenced.lastIndexOf(QLatin1Char('}'));
    if (firstBrace < 0 || lastBrace < firstBrace) {
        return unfenced;
    }

    return unfenced.mid(firstBrace, lastBrace - firstBrace + 1).trimmed();
}

} // namespace

namespace AgentPlanParser {

AgentPlanParseResult parseJsonPlan(
    const QString &jsonText,
    const QVector<AgentToolDescriptor> &toolCatalog,
    int maxSteps)
{
    if (maxSteps <= 0) {
        return AgentPlanParseResult::failure(QStringLiteral("Maximum step count must be positive."));
    }

    const QString candidateJson = extractJsonObjectCandidate(jsonText);
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(candidateJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return AgentPlanParseResult::failure(
            QStringLiteral("Plan JSON parse error at offset %1: %2")
                .arg(parseError.offset)
                .arg(parseError.errorString()));
    }

    if (!document.isObject()) {
        return AgentPlanParseResult::failure(QStringLiteral("Plan root must be a JSON object."));
    }

    const QJsonObject root = document.object();
    bool fieldOk = true;
    const QString summary = requiredString(root, QStringLiteral("summary"), &fieldOk);
    if (!fieldOk) {
        return AgentPlanParseResult::failure(QStringLiteral("Plan summary must be a non-empty string."));
    }

    const QJsonValue stepsValue = root.value(QStringLiteral("steps"));
    if (!stepsValue.isArray()) {
        return AgentPlanParseResult::failure(QStringLiteral("Plan steps must be an array."));
    }

    const QJsonArray stepsArray = stepsValue.toArray();
    if (stepsArray.isEmpty()) {
        return AgentPlanParseResult::failure(QStringLiteral("Plan must contain at least one step."));
    }

    if (stepsArray.size() > maxSteps) {
        return AgentPlanParseResult::failure(
            QStringLiteral("Plan contains %1 steps, but the limit is %2.")
                .arg(stepsArray.size())
                .arg(maxSteps));
    }

    AgentPlan plan;
    plan.summary = summary;
    QSet<QString> stepIds;

    for (int index = 0; index < stepsArray.size(); ++index) {
        const QJsonValue stepValue = stepsArray.at(index);
        if (!stepValue.isObject()) {
            return failureAtStep(index, QStringLiteral("Step must be an object."));
        }

        const QJsonObject stepObject = stepValue.toObject();
        fieldOk = true;
        AgentPlanStep step;
        step.id = requiredString(stepObject, QStringLiteral("id"), &fieldOk);
        if (!fieldOk) {
            return failureAtStep(index, QStringLiteral("id must be a non-empty string."));
        }

        if (stepIds.contains(step.id)) {
            return failureAtStep(index, QStringLiteral("id must be unique."));
        }
        stepIds.insert(step.id);

        fieldOk = true;
        step.title = requiredString(stepObject, QStringLiteral("title"), &fieldOk);
        if (!fieldOk) {
            return failureAtStep(index, QStringLiteral("title must be a non-empty string."));
        }

        fieldOk = true;
        step.toolId = requiredString(stepObject, QStringLiteral("toolId"), &fieldOk);
        if (!fieldOk) {
            return failureAtStep(index, QStringLiteral("toolId must be a non-empty string."));
        }

        const AgentToolDescriptor *toolDescriptor = findAgentToolDescriptor(toolCatalog, step.toolId);
        if (toolDescriptor == nullptr || !toolDescriptor->enabledForAgent) {
            return failureAtStep(index, QStringLiteral("toolId is not available in the Agent tool catalog."));
        }

        fieldOk = true;
        step.reason = requiredString(stepObject, QStringLiteral("reason"), &fieldOk);
        if (!fieldOk) {
            return failureAtStep(index, QStringLiteral("reason must be a non-empty string."));
        }

        fieldOk = true;
        const QString riskText = requiredString(stepObject, QStringLiteral("risk"), &fieldOk);
        if (!fieldOk) {
            return failureAtStep(index, QStringLiteral("risk must be a non-empty string."));
        }

        AgentToolRisk aiRisk = AgentToolRisk::Low;
        if (!agentToolRiskFromString(riskText, &aiRisk)) {
            return failureAtStep(index, QStringLiteral("risk must be one of low, medium, or high."));
        }

        step.risk = maxAgentToolRisk(aiRisk, toolDescriptor->risk);

        const QJsonValue parametersValue = stepObject.value(QStringLiteral("parameters"));
        if (parametersValue.isUndefined()) {
            step.parameters = QJsonObject();
        } else if (!parametersValue.isObject()) {
            return failureAtStep(index, QStringLiteral("parameters must be an object when provided."));
        } else {
            step.parameters = parametersValue.toObject();
        }

        plan.steps.append(step);
    }

    return AgentPlanParseResult::success(plan);
}

} // namespace AgentPlanParser
