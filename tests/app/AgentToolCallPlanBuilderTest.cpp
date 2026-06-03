#include "app/AgentToolCallPlanBuilder.h"

#include <cassert>

int main()
{
    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();

    ToolCall firstCall;
    firstCall.id = QStringLiteral("call-1");
    firstCall.functionName = QStringLiteral("json_format");
    firstCall.arguments = QStringLiteral("{\"input\":\"{\\\"a\\\":1}\"}");

    AgentPlanParseResult result = AgentToolCallPlanBuilder::buildPlanFromToolCalls(
        ToolCallList{firstCall},
        registry,
        AppLanguage::Chinese,
        5);
    assert(result.ok);
    assert(result.plan.steps.size() == 1);
    assert(result.plan.steps.first().id == QStringLiteral("call-1"));
    assert(result.plan.steps.first().toolId == QStringLiteral("json.format"));
    assert(result.plan.steps.first().risk == AgentToolRisk::Low);
    assert(result.plan.steps.first().parameters.value(QStringLiteral("input")).toString() == QStringLiteral("{\"a\":1}"));
    assert(result.plan.summary.contains(QStringLiteral("原生工具调用")));

    ToolCall commandCall;
    commandCall.functionName = QStringLiteral("command_cmake_build");
    commandCall.arguments = QStringLiteral("{}");
    result = AgentToolCallPlanBuilder::buildPlanFromToolCalls(
        ToolCallList{commandCall},
        registry,
        AppLanguage::English,
        5);
    assert(result.ok);
    assert(result.plan.steps.first().id == QStringLiteral("tool-call-1"));
    assert(result.plan.steps.first().toolId == QStringLiteral("command.cmake_build"));
    assert(result.plan.steps.first().risk == AgentToolRisk::Medium);

    ToolCall duplicateIdCall = firstCall;
    result = AgentToolCallPlanBuilder::buildPlanFromToolCalls(
        ToolCallList{firstCall, duplicateIdCall},
        registry,
        AppLanguage::English,
        5);
    assert(result.ok);
    assert(result.plan.steps.at(0).id == QStringLiteral("call-1"));
    assert(result.plan.steps.at(1).id == QStringLiteral("call-1-2"));

    ToolCall unknownCall;
    unknownCall.functionName = QStringLiteral("missing_tool");
    unknownCall.arguments = QStringLiteral("{}");
    result = AgentToolCallPlanBuilder::buildPlanFromToolCalls(
        ToolCallList{unknownCall},
        registry,
        AppLanguage::English,
        5);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("not found"), Qt::CaseInsensitive));

    ToolCall invalidArgumentsCall = firstCall;
    invalidArgumentsCall.arguments = QStringLiteral("{not-json");
    result = AgentToolCallPlanBuilder::buildPlanFromToolCalls(
        ToolCallList{invalidArgumentsCall},
        registry,
        AppLanguage::English,
        5);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("parse error"), Qt::CaseInsensitive));

    result = AgentToolCallPlanBuilder::buildPlanFromToolCalls(
        ToolCallList{firstCall, firstCall},
        registry,
        AppLanguage::English,
        1);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("limit"), Qt::CaseInsensitive));

    return 0;
}
