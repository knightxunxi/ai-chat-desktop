#pragma once

#include "app/AgentPlan.h"
#include "tools/ToolResult.h"

namespace AgentPlanExecutor {

// 功能：执行用户确认后的 Agent 计划步骤；使用模块：AgentPlanDialog。
ToolResult executeStep(const AgentPlanStep &step);

// 功能：判断步骤是否可由 V7 计划窗口直接执行；使用模块：AgentPlanDialog 按钮状态。
bool canExecuteDirectly(const AgentPlanStep &step);

} // namespace AgentPlanExecutor
