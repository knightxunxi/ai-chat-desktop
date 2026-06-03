#pragma once

#include "app/AgentPlan.h"
#include "core/AppLanguage.h"
#include "services/ToolCall.h"
#include "tools/AgentToolRegistry.h"

namespace AgentToolCallPlanBuilder {

// 功能：把 Function Calling tool_calls 转换为本地 Agent 计划；使用模块：ApplicationController 的 V9.2 原生工具调用流程。
AgentPlanParseResult buildPlanFromToolCalls(
    const ToolCallList &toolCalls,
    const AgentToolRegistry &registry,
    AppLanguage language,
    int maxSteps);

} // namespace AgentToolCallPlanBuilder
