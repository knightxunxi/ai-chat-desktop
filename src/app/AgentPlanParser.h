#pragma once

#include "app/AgentPlan.h"
#include "tools/AgentToolCatalog.h"

#include <QString>
#include <QVector>

namespace AgentPlanParser {

constexpr int DefaultMaxPlanSteps = 5; // 功能：默认最大计划步数；使用模块：解析器防止无限计划。

// 功能：解析并校验 AI 返回的 JSON 计划；使用模块：V7 计划生成入口和测试。
AgentPlanParseResult parseJsonPlan(
    const QString &jsonText,
    const QVector<AgentToolDescriptor> &toolCatalog,
    int maxSteps = DefaultMaxPlanSteps);

} // namespace AgentPlanParser
