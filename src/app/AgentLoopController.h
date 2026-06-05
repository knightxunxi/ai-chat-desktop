#pragma once

#include "app/AgentPlan.h"
#include "core/AppLanguage.h"
#include "services/RequestErrorCategory.h"
#include "tools/AgentToolRegistry.h"

#include <QString>
#include <QStringList>

#include <functional>

// Forward declarations for executeLoop() parameters.
struct AppConfig;
class AIClient;
class HookManager;
class SkillManager;
struct SkillDefinition;

enum class AgentLoopRunStatus {
    Completed,
    Failed,
    Stopped,
    StepLimitReached,
    RuntimeLimitReached,
    StepTimeout,
    RepeatedAction
};

// 功能：描述循环终止原因；使用模块：executeLoop 终止判断和审计日志。
enum class TerminationReason {
    None,           // 无需终止
    StepLimit,      // 达到步数上限
    RuntimeLimit,   // 达到时间上限
    StepTimeout,    // 单步超时
    Stopped,        // 用户停止
    AIDone,         // AI 返回 done=true
    AIError,        // AI 调用失败
    ParseError      // 解析失败（保留，不用于终止）
};

struct AgentLoopOptions {
    int maxSteps = 10;             // 功能：最大连续动作数；使用模块：Agentic Loop 运行时。
    qint64 maxRuntimeMs = 120000;  // 功能：总运行耗时上限；使用模块：防止长时间占用 UI。
    qint64 maxStepMs = 30000;     // 功能：单步耗时上限；使用模块：后续命令执行安全边界。
    std::function<bool()> shouldStop; // 功能：停止请求回调；使用模块：计划窗口停止按钮。
};

// 功能：统一的循环终止检查策略；使用模块：executeLoop 每轮迭代前后的终止判断。
struct LoopTerminationPolicy {
    int maxSteps = 10;
    qint64 maxRuntimeMs = 120000;
    qint64 maxStepMs = 30000;
    std::function<bool()> shouldStop;

    // 功能：从 AgentLoopOptions 构造策略；使用模块：executeLoop 初始化。
    static LoopTerminationPolicy fromOptions(const AgentLoopOptions &options);

    // 功能：检查终止条件，返回原因；使用模块：executeLoop 每轮终止判断。
    // lastStepMs > 0 时同时检查单步超时。
    TerminationReason check(int completedSteps, qint64 elapsedMs, qint64 lastStepMs = 0) const;

    // 功能：终止原因的文本标签；使用模块：审计日志和错误提示。
    static QString reasonLabel(TerminationReason reason);
};

struct AgentLoopCallbacks {
    std::function<void(int)> stepStarted; // 功能：步骤开始回调；使用模块：UI 刷新。
    std::function<void(int, const ToolResult &)> stepFinished; // 功能：步骤结束回调；使用模块：UI 刷新。
};

// 功能：AI 单步调用的统一返回结构；使用模块：executeLoop 中 AiLoopRunner::call 的返回值。
struct AiLoopResponse {
    bool ok = false;
    QString fullText;
    QString errorMessage;
    RequestErrorCategory errorCategory = RequestErrorCategory::Unknown;

    static AiLoopResponse success(const QString &text);
    static AiLoopResponse failure(const QString &msg, RequestErrorCategory category);
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

// 功能：每步调 AI → 解析 → 执行工具 → 收集观测 → 继续的无限循环；使用模块：V12 Agentic Loop 核心运行时。
// V13.3: 新增 hooks/skills 参数（默认 nullptr），向后兼容 runPlan()。
AgentLoopRunResult executeLoop(
    AIClient *aiClient,
    const AppConfig &config,
    const QString &userGoal,
    const AgentToolRegistry &registry,
    const AgentToolExecutionContext &context,
    const AgentLoopOptions &options = AgentLoopOptions(),
    const AgentLoopCallbacks &callbacks = AgentLoopCallbacks(),
    AppLanguage language = AppLanguage::Chinese,
    HookManager *hooks = nullptr,
    SkillManager *skills = nullptr);

} // namespace AgentLoopController
