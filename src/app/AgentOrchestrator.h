#pragma once

#include "app/ContextWindowManager.h"
#include "app/SummaryAPIClient.h"
#include "core/AgentLoopState.h"
#include "hooks/HookManager.h"
#include "mcp/McpRegistry.h"
#include "plugins/PluginManager.h"
#include "services/ToolCall.h"
#include "skills/SkillManager.h"
#include "tools/AgentToolRegistry.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <memory>

// V12.3 流式工具执行 — 待处理的结果缓存
struct PendingToolResult {
    QString toolName;
    QJsonObject arguments;
    QString result;
    bool success = false;
};

// 前向声明
class AIClient;
class ConfigCoordinator;
class PythonSidecarClient;
class SessionCoordinator;
struct AgentPlan;
struct SkillDefinition;
struct ScheduledTask;

// 学习注释：Agent 编排器，管理 Agent 循环、工具执行、技能/钩子系统、MCP、上下文窗口。
// 从 ApplicationController 分离，遵循单一职责原则。
class AgentOrchestrator : public QObject
{
    Q_OBJECT

public:
    explicit AgentOrchestrator(QObject *parent = nullptr);

    // 功能：初始化上下文窗口、摘要客户端、技能/钩子、MCP；使用模块：ApplicationController::initialize。
    void initialize(AIClient *aiClient, ConfigCoordinator *configCoordinator, SessionCoordinator *sessionCoordinator);

    // ── Agent 循环入口 ──
    // 功能：启动 Agent 连续循环；使用模块：ApplicationController::sendAgentLoopMessage。
    void startAgentLoop(const QString &goal);
    // 功能：恢复暂停的 Agent 循环；使用模块：ApplicationController::resumeAgentLoop。
    void resumeAgentLoop();
    // 功能：取消 Agent 循环；使用模块：ApplicationController::cancelCurrentRequest。
    void cancelAgentLoop();
    // 功能：清理 Agent 循环状态快照；使用模块：ApplicationController::cancelCurrentRequest。
    void clearAgentLoopState();

    // ── 状态查询 ──
    bool isAgentLoopActive() const;
    int agentLoopIteration() const;
    int maxAgentLoopIterations() const;
    bool hasPendingAgentState() const;
    AgentLoopState pendingAgentState() const;
    bool agentDebugMode() const;
    void setAgentDebugMode(bool enabled);

    // ── 管理器访问器 ──
    SkillManager *skillManager();
    HookManager *hookManager();
    McpRegistry *mcpRegistry();
    AgentToolRegistry toolRegistry() const;
    ContextWindowManager *contextWindowManager();
    SummaryAPIClient *summaryClient();

    // ── Agent 循环迭代 ──
    // 功能：执行一次循环迭代（增量、状态保存、上限检查）；返回 true 表示应继续。
    // 使用模块：ApplicationController::handleRequestFinished（Agent 路径）。
    bool executeAgentLoopIteration();
    // 功能：构建下一轮循环提示词（含调试输出和记忆注入）。
    // 使用模块：ApplicationController::continueAgentLoop。
    QString buildNextLoopPrompt() const;
    // 功能：追加 Agent 循环观察记录；使用模块：handleRequestFinished 路径。
    void appendLoopObservation(const QString &observation);
    // V17.6 P2-1: 获取 observations 的可变引用，用于 Reactive 压缩。
    QStringList &loopObservationsRef();

    // ── 流式工具结果 ──
    void addPendingToolResult(const QString &toolName, const QJsonObject &arguments,
                              const QString &result, bool success);
    QVector<PendingToolResult> takePendingToolResults();
    void clearPendingToolResults();

    // ── Agent 计划执行 ──
    // 功能：执行 Agent 计划步骤，追加结果到当前会话；返回 true 表示全部步骤成功。使用模块：ApplicationController::handleRequestFinished。
    bool executePlanAndReportToChat(const AgentPlan &plan);

    // V19 #25: 并行执行计划步骤（无依赖步骤同时进行）
    struct StepResult {
        int stepIndex;
        QString toolId;
        bool ok;
        QString output;
    };
    QVector<StepResult> executePlanStepsParallel(
        const AgentPlan &plan,
        const AgentToolRegistry &registry,
        const AgentToolExecutionContext &context);

    // ── Token 上下文 ──
    size_t getContextWindowTokens() const;

    // ── Agent 状态持久化 ──
    QString agentStateFilePath() const;
    void saveAgentLoopState();

    // ── 技能匹配 ──
    QVector<SkillDefinition> matchedSkills() const;
    void matchSkills(const QString &goal);

signals:
    void agentLoopIterationUpdated(int iteration, int maxIterations);
    void agentLoopSkillSummary(const QString &summary);
    void agentLoopThought(int iteration, const QString &reasoning, const QString &toolId, const QString &title);
    void agentLoopToolFinished(int iteration, const QString &toolId, bool ok, const QString &outputPreview);
    void agentLoopPromptDebug(const QString &prompt);
    void tokenUsageUpdated(int used, int limit);
    void agentLoopCompleted();
    void assistantMessageUpdated(const QString &content);
    void statusMessage(const QString &english, const QString &chinese, int timeoutMs);

private:
    // ── 上下文窗口 ──
    ContextWindowManager m_contextWindowManager;
    SummaryAPIClient m_summaryClient;

    // ── 流式工具结果 ──
    QVector<PendingToolResult> m_pendingToolResults;

    // ── Agent 循环状态 ──
    bool m_isAgentLoopActive = false;
    QString m_agentLoopGoal;
    QStringList m_agentLoopObservations;
    int m_agentLoopIteration = 0;
    static constexpr int kMaxAgentLoopIterations = 50;

    // ── V13.3 Skills + Hooks ──
    std::unique_ptr<SkillManager> m_skillManager;
    std::unique_ptr<HookManager> m_hookManager;
    QVector<SkillDefinition> m_matchedSkills;

    // ── V15.4 MCP ──
    std::unique_ptr<McpRegistry> m_mcpRegistry;

    // ── N4: 插件系统 ──
    std::unique_ptr<class PluginManager> m_pluginManager;

    // ── V16.3 调试 ──
    bool m_agentDebugMode = false;

    // ── V17.2 Agent 状态快照 ──
    AgentLoopState m_agentLoopState;

    // ── V17.6 P0-2: 重复动作检测 ──
    QSet<QString> m_actionFingerprints;
    static constexpr int kMaxRepeatedActions = 3;

    // V18.5: 自动修复闭环
    bool m_autoFixEnabled = true;

    // ── 外部依赖指针 ──
    AIClient *m_aiClient = nullptr;
    ConfigCoordinator *m_configCoordinator = nullptr;
    SessionCoordinator *m_sessionCoordinator = nullptr;

    // V17.6 P1-3: 轻量子代理工具
    AgentToolDefinition createSubAgentTool() const;
    // 功能：获取当前 Python sidecar 客户端；使用模块：工具执行上下文。
    PythonSidecarClient *activeSidecarClient() const;

    // V12.4: 测试友元
    friend struct ChatToolExecutionTestAccessor;
    // V12.6: 测试友元
    friend struct AgentLoopExecutionTestAccessor;
};
