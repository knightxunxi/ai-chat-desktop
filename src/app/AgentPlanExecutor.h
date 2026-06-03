#pragma once

#include "app/AgentPlan.h"
#include "tools/ToolResult.h"

#include <QString>

namespace AgentPlanExecutor {

// 功能：执行用户确认后的 Agent 计划步骤；使用模块：AgentPlanDialog。
ToolResult executeStep(const AgentPlanStep &step);

// 功能：按指定 Agent 工作目录执行计划步骤；使用模块：V8 workspace.* 文件工具。
ToolResult executeStep(const AgentPlanStep &step, const QString &workspaceDirectory);

// 功能：按指定 Agent 工作目录和项目目录执行计划步骤；使用模块：V8 workspace.* 和 V9 command.* 工具。
ToolResult executeStep(const AgentPlanStep &step, const QString &workspaceDirectory, const QString &projectDirectory);

// 功能：判断步骤是否可由 V7 计划窗口直接执行；使用模块：AgentPlanDialog 按钮状态。
bool canExecuteDirectly(const AgentPlanStep &step);

} // namespace AgentPlanExecutor
