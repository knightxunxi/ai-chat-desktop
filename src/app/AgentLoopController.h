#pragma once

// 功能：Agent 循环控制器 — 定义 Agent 异步循环引擎 (AgentLoopEngine) 和
//        同步调用包装 (executeLoop / runPlan)。
// 异步引擎：AgentLoopEngine 基于 QObject 信号/槽，每轮通过 QTimer::singleShot(0)
//           释放调用栈，不阻塞 UI。
// 同步包装：executeLoop() 保持原同步接口，供测试和旧调用方使用。
// V19 D-2: 新增 AgentLoopEngine，替代原有的 QEventLoop AiLoopRunner。

#include "app/AgentPlan.h"
#include "core/AppLanguage.h"
#include "services/RequestErrorCategory.h"
#include "tools/AgentToolRegistry.h"

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

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
    None,
    StepLimit,
    RuntimeLimit,
    StepTimeout,
    Stopped,
    AIDone,
    AIError,
    ParseError
};

struct AgentLoopOptions {
    int maxSteps = 10;
    qint64 maxRuntimeMs = 120000;
    qint64 maxStepMs = 30000;
    std::function<bool()> shouldStop;
};

struct LoopTerminationPolicy {
    int maxSteps = 10;
    qint64 maxRuntimeMs = 120000;
    qint64 maxStepMs = 30000;
    std::function<bool()> shouldStop;

    static LoopTerminationPolicy fromOptions(const AgentLoopOptions &options);
    TerminationReason check(int completedSteps, qint64 elapsedMs, qint64 lastStepMs = 0) const;
    static QString reasonLabel(TerminationReason reason);
};

struct AgentLoopCallbacks {
    std::function<void(int)> stepStarted;
    std::function<void(int, const ToolResult &)> stepFinished;
};

struct AiLoopResponse {
    bool ok = false;
    QString fullText;
    QString errorMessage;
    RequestErrorCategory errorCategory = RequestErrorCategory::Unknown;

    static AiLoopResponse success(const QString &text);
    static AiLoopResponse failure(const QString &msg, RequestErrorCategory category);
};

struct AgentLoopRunResult {
    AgentLoopRunStatus status = AgentLoopRunStatus::Completed;
    int executedStepCount = 0;
    QString error;
    QString lastToolId;
    QString lastOutput;
    QStringList auditTrail;
};

// D-2: 异步 Agent Loop 引擎 — 继承 QObject 使用信号/槽代替 QEventLoop
class AgentLoopEngine : public QObject {
    Q_OBJECT
public:
    explicit AgentLoopEngine(QObject *parent = nullptr);

    // 功能：启动异步循环；使用模块：AgentOrchestrator 或测试。
    void start(AIClient *aiClient, const AppConfig &config, const QString &userGoal,
               const AgentToolRegistry &registry, const AgentToolExecutionContext &context,
               const AgentLoopOptions &options = AgentLoopOptions(),
               const AgentLoopCallbacks &callbacks = AgentLoopCallbacks(),
               AppLanguage language = AppLanguage::Chinese,
               HookManager *hooks = nullptr, SkillManager *skills = nullptr);
    // 功能：停止循环；使用模块：取消请求。
    void stop();
    bool isRunning() const;

signals:
    void finished(const AgentLoopRunResult &result);
    void stepStarted(int step);
    void stepFinished(int step, const ToolResult &result);
    void aiResponseReceived(const QString &fullText);

private:
    void doIteration();
    void handleAiResponse(const AiLoopResponse &response);
    void finish(AgentLoopRunStatus status, const QString &error = QString());

    AIClient *m_aiClient = nullptr;
    const AppConfig *m_config = nullptr;
    QString m_userGoal;
    AgentToolRegistry m_registry;
    AgentToolExecutionContext m_context;
    AgentLoopOptions m_options;
    AgentLoopCallbacks m_callbacks;
    AppLanguage m_language = AppLanguage::Chinese;
    HookManager *m_hooks = nullptr;
    SkillManager *m_skills = nullptr;

    // 运行状态
    bool m_running = false;
    int m_stepCount = 0;
    QElapsedTimer m_runtime;
    AgentLoopRunResult m_result;
    QStringList m_observations;
    QSet<QString> m_actionFingerprints;
    QString m_currentResponseText; // 功能：跨异步信号保存本轮 AI 回复，避免 lambda 捕获栈变量。

    // AI 回复信号连接
    QMetaObject::Connection m_connDelta;
    QMetaObject::Connection m_connFinished;
    QMetaObject::Connection m_connFailed;
    QTimer m_timeoutTimer;
};

namespace AgentLoopController {

QString runStatusToString(AgentLoopRunStatus status);

// 功能：仍保留同步调用（内部使用 QEventLoop+AgentLoopEngine 包装）；使用模块：测试。
AgentLoopRunResult runPlan(
    AgentPlan *plan,
    const AgentToolRegistry &registry,
    const AgentToolExecutionContext &context,
    const AgentLoopOptions &options = AgentLoopOptions(),
    const AgentLoopCallbacks &callbacks = AgentLoopCallbacks());

// 功能：同步便捷包装（内部使用 QEventLoop 等 AgentLoopEngine 结果）；使用模块：测试。
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
