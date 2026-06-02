#include "app/AgentPlanParser.h"
#include "tools/AgentToolCatalog.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cassert>

namespace {

QString compactJson(const QJsonObject &object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QJsonObject stepObject(
    const QString &id,
    const QString &toolId,
    const QString &risk = QStringLiteral("low"),
    const QJsonValue &parameters = QJsonObject())
{
    QJsonObject step;
    step.insert(QStringLiteral("id"), id);
    step.insert(QStringLiteral("title"), QStringLiteral("Prepare text"));
    step.insert(QStringLiteral("toolId"), toolId);
    step.insert(QStringLiteral("reason"), QStringLiteral("The user asked for a structured plan."));
    step.insert(QStringLiteral("risk"), risk);
    step.insert(QStringLiteral("parameters"), parameters);
    return step;
}

QJsonObject planObject(const QJsonArray &steps)
{
    QJsonObject plan;
    plan.insert(QStringLiteral("summary"), QStringLiteral("Clean and inspect user input."));
    plan.insert(QStringLiteral("steps"), steps);
    return plan;
}

AgentPlanParseResult parse(const QJsonObject &object, int maxSteps = AgentPlanParser::DefaultMaxPlanSteps)
{
    return AgentPlanParser::parseJsonPlan(compactJson(object), defaultAgentToolCatalog(), maxSteps);
}

} // namespace

int main()
{
    QJsonObject parameters;
    parameters.insert(QStringLiteral("inputSource"), QStringLiteral("chat"));
    AgentPlanParseResult result = parse(planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("json.format"), QStringLiteral("low"), parameters)}));
    assert(result.ok);
    assert(result.error.isEmpty());
    assert(result.plan.summary == QStringLiteral("Clean and inspect user input."));
    assert(result.plan.steps.size() == 1);
    assert(result.plan.steps.first().id == QStringLiteral("step-1"));
    assert(result.plan.steps.first().toolId == QStringLiteral("json.format"));
    assert(result.plan.steps.first().risk == AgentToolRisk::Low);
    assert(result.plan.steps.first().parameters.value(QStringLiteral("inputSource")).toString() == QStringLiteral("chat"));
    assert(result.plan.steps.first().status == AgentPlanStepStatus::Pending);

    const QString fencedPlan = QStringLiteral("```json\n%1\n```").arg(compactJson(planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("json.format"))})));
    result = AgentPlanParser::parseJsonPlan(fencedPlan, defaultAgentToolCatalog());
    assert(result.ok);
    assert(result.plan.steps.first().toolId == QStringLiteral("json.format"));

    const QString commentedPlan = QStringLiteral("Here is the plan:\n%1\nPlease review it.")
                                      .arg(compactJson(planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("text.cleanup"))})));
    result = AgentPlanParser::parseJsonPlan(commentedPlan, defaultAgentToolCatalog());
    assert(result.ok);
    assert(result.plan.steps.first().toolId == QStringLiteral("text.cleanup"));

    result = parse(planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("file.read_text"), QStringLiteral("low"))}));
    assert(result.ok);
    assert(result.plan.steps.first().risk == AgentToolRisk::Medium);

    result = AgentPlanParser::parseJsonPlan(QStringLiteral("not-json"), defaultAgentToolCatalog());
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("parse error"), Qt::CaseInsensitive));

    QJsonObject invalidPlan = planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("json.format"))});
    invalidPlan.remove(QStringLiteral("summary"));
    result = parse(invalidPlan);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("summary")));

    QJsonObject missingFieldPlan = planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("json.format"))});
    QJsonArray missingFieldSteps = missingFieldPlan.value(QStringLiteral("steps")).toArray();
    QJsonObject missingFieldStep = missingFieldSteps.first().toObject();
    missingFieldStep.remove(QStringLiteral("toolId"));
    missingFieldSteps.replace(0, missingFieldStep);
    missingFieldPlan.insert(QStringLiteral("steps"), missingFieldSteps);
    result = parse(missingFieldPlan);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("toolId")));

    result = parse(planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("unknown.tool"))}));
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("toolId")));

    result = parse(planObject(QJsonArray{
        stepObject(QStringLiteral("step-1"), QStringLiteral("json.format")),
        stepObject(QStringLiteral("step-2"), QStringLiteral("json.compact")),
        stepObject(QStringLiteral("step-3"), QStringLiteral("text.cleanup"))
    }), 2);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("limit")));

    result = parse(planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("json.format"), QStringLiteral("critical"))}));
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("risk")));

    result = parse(planObject(QJsonArray{stepObject(QStringLiteral("step-1"), QStringLiteral("json.format"), QStringLiteral("low"), QJsonArray{})}));
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("parameters")));

    result = parse(planObject(QJsonArray{
        stepObject(QStringLiteral("step-1"), QStringLiteral("json.format")),
        stepObject(QStringLiteral("step-1"), QStringLiteral("json.compact"))
    }));
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("unique")));

    QVector<AgentToolDescriptor> catalog = defaultAgentToolCatalog();
    catalog[0].enabledForAgent = false;
    result = AgentPlanParser::parseJsonPlan(
        compactJson(planObject(QJsonArray{stepObject(QStringLiteral("step-1"), catalog[0].id)})),
        catalog);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("catalog")));

    assert(agentPlanStepStatusToString(AgentPlanStepStatus::Pending) == QStringLiteral("pending"));
    assert(agentPlanStepStatusToString(AgentPlanStepStatus::Completed) == QStringLiteral("completed"));

    return 0;
}
