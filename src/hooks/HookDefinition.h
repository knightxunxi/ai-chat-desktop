#pragma once

#include <QJsonObject>
#include <QString>

// ============================================================================
// HookPoint — 6 个 Agent 生命周期 Hook 点
// ============================================================================

enum class HookPoint {
    OnAgentStart,      // Agent 循环启动时
    OnAgentStop,       // Agent 循环停止时
    PreSend,           // 发送 prompt 到 AI 之前
    PostReceive,       // 收到 AI 响应之后
    OnToolExecute,     // 工具执行前后（通过 metadata["before"] 区分）
    OnError            // 发生错误时
};

// ============================================================================
// HookAction — Hook 执行后的三种动作
// ============================================================================

enum class HookAction {
    Pass,       // 放行，context 不变
    Modify,     // 修改，使用 modifiedContext
    Reject      // 拒绝，停止后续 Hook 并终止当前操作
};

// ============================================================================
// HookContext — 传递给 Hook 的上下文信息
// ============================================================================

struct HookContext {
    HookPoint hookPoint = HookPoint::PreSend;
    QJsonObject context;      // 具体上下文数据（如 prompt、toolId、params）
    QJsonObject metadata;     // 元数据（如 iteration、sessionId、before 标记）

    // 功能：构造 PreSend 阶段上下文；使用模块：AgentLoopController 发送 prompt 前。
    static HookContext forPreSend(const QString &prompt,
                                  const QString &userGoal,
                                  int iteration,
                                  const QString &sessionId)
    {
        HookContext ctx;
        ctx.hookPoint = HookPoint::PreSend;
        ctx.context.insert(QStringLiteral("prompt"), prompt);
        ctx.context.insert(QStringLiteral("user_goal"), userGoal);
        ctx.metadata.insert(QStringLiteral("iteration"), iteration);
        ctx.metadata.insert(QStringLiteral("session_id"), sessionId);
        return ctx;
    }

    // 功能：构造 PostReceive 阶段上下文；使用模块：AgentLoopController 收到 AI 响应后。
    static HookContext forPostReceive(const QString &response, int iteration)
    {
        HookContext ctx;
        ctx.hookPoint = HookPoint::PostReceive;
        ctx.context.insert(QStringLiteral("response"), response);
        ctx.metadata.insert(QStringLiteral("iteration"), iteration);
        return ctx;
    }

    // 功能：构造 OnToolExecute 阶段上下文；使用模块：AgentLoopController 工具执行前后。
    static HookContext forToolExecute(const QString &toolId,
                                      const QJsonObject &params,
                                      bool before)
    {
        HookContext ctx;
        ctx.hookPoint = HookPoint::OnToolExecute;
        ctx.context.insert(QStringLiteral("tool_id"), toolId);
        ctx.context.insert(QStringLiteral("parameters"), params);
        ctx.metadata.insert(QStringLiteral("before"), before);
        return ctx;
    }

    // 功能：构造 OnError 阶段上下文；使用模块：AgentLoopController AI 调用失败时。
    static HookContext forError(const QString &errorMessage, int iteration)
    {
        HookContext ctx;
        ctx.hookPoint = HookPoint::OnError;
        ctx.context.insert(QStringLiteral("error_message"), errorMessage);
        ctx.metadata.insert(QStringLiteral("iteration"), iteration);
        return ctx;
    }

    // 功能：构造 OnAgentStart/OnAgentStop 生命周期上下文；使用模块：ApplicationController。
    static HookContext forAgentLifecycle(HookPoint point, const QString &userGoal)
    {
        HookContext ctx;
        ctx.hookPoint = point;
        ctx.context.insert(QStringLiteral("user_goal"), userGoal);
        return ctx;
    }
};

// ============================================================================
// HookResult — Hook 执行后的返回结果
// ============================================================================

struct HookResult {
    HookAction action = HookAction::Pass;
    QJsonObject modifiedContext;
    QString error;

    // 功能：判断是否为放行；使用模块：HookManager 执行后判断。
    bool isPass() const { return action == HookAction::Pass; }

    // 功能：判断是否为拒绝；使用模块：HookManager 遇到 Reject 停止后续。
    bool isReject() const { return action == HookAction::Reject; }

    // 功能：返回拒绝原因；使用模块：日志和状态提示。
    QString rejectionReason() const { return error.isEmpty() ? QStringLiteral("Rejected by hook") : error; }
};

// ============================================================================
// HookBase — 所有 Hook 的抽象基类
// ============================================================================

class HookBase {
public:
    virtual ~HookBase() = default;

    // 功能：执行 Hook 逻辑；使用模块：HookManager::executeHooks。
    virtual HookResult execute(const HookContext &ctx) = 0;

    // 功能：返回 Hook 名称；使用模块：日志和注册表。
    virtual QString name() const = 0;

    // 功能：返回 Hook 适用的 Hook 点；使用模块：HookManager 注册时分组。
    virtual HookPoint hookPoint() const = 0;

    // 功能：返回 Hook 类型字符串（"builtin" 或 "script"）；使用模块：日志。
    virtual QString hookType() const = 0;

    // 功能：返回超时毫秒数，默认 10000ms；使用模块：脚本 Hook 超时控制。
    virtual int timeoutMs() const { return 10000; }
};
