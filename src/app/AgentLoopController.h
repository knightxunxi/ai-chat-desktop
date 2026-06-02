#pragma once

#include "app/AgentPlan.h"
#include "tools/AgentToolRegistry.h"

#include <QString>
#include <QStringList>

#include <functional>

enum class AgentLoopRunStatus {
    Completed,
    Failed,
    Stopped,
    StepLimitReached,
    RuntimeLimitReached,
    StepTimeout,
    RepeatedAction
};

struct AgentLoopOptions {
    int maxSteps = 5;             // 功能：最大连续动作数；使用模块：Agentic Loop 运行时。
    qint64 maxRuntimeMs = 60000;  // 功能：总运行耗时上限；使用模块：防止长时间占用 UI。
    qint64 maxStepMs = 30000;     // 功能：单步耗时上限；使用模块：后续命令执行安全边界。
    std::function<bool()> shouldStop; // 功能：停止请求回调；使用模块：计划窗口停止按钮。
};

struct AgentLoopCallbacks {
    std::function<void(int)> stepStarted; // 功能：步骤开始回调；使用模块：UI 刷新。
    std::function<void(int, const ToolResult &)> stepFinished; // 功能：步骤结束回调；使用模块：UI 刷新。
};

struct AgentLoopRunResult {
    AgentLoopRunStatus status = AgentLoopRunStatus::Completed; // 功能：循环结束原因；使用模块：UI 状态提示和测试。
    int executedStepCount = 0;       // 功能：已执行步骤数；使用模块：验收和日志。
    QString error;                   // 功能：失败原因；使用模块：状态提示。
    QString lastToolId;              // 功能：最后执行工具 ID；使用模块：审计日志。
    QString lastOutput;              // 功能：最后输出摘要；使用模块：后续规划输入。
    QStringList auditTrail;          // 功能：Agent 专用审计链；使用模块：测试和日志。
};

namespace AgentLoopController {

// 功能：返回运行状态稳定字符串；使用模块：日志和测试。
QString runStatusToString(AgentLoopRunStatus status);

// 功能：按观察、选择下一步、执行、评估的循环运行计划；使用模块：AgentPlanDialog 连续执行。
AgentLoopRunResult runPlan(
    AgentPlan *plan,
    const AgentToolRegistry &registry,
    const AgentToolExecutionContext &context,
    const AgentLoopOptions &options = AgentLoopOptions(),
    const AgentLoopCallbacks &callbacks = AgentLoopCallbacks());

} // namespace AgentLoopController
