// D-2: Agent 循环控制器实现
// AgentLoopEngine — 异步引擎，使用 QObject 信号/槽 + QTimer 驱动
// runPlan — 同步计划执行器，用于测试路径
// executeLoop — 同步包装，内部使用 QEventLoop 等 AgentLoopEngine 结果

#include "app/AgentLoopController.h"

#include "app/AgentLoopActionParser.h"
#include "app/AgentLoopPromptBuilder.h"
#include "core/AppConfig.h"
#include "core/ChatSession.h"
#include "hooks/HookDefinition.h"
#include "hooks/HookManager.h"
#include "services/AIClient.h"
#include "services/RequestErrorCategory.h"
#include "skills/SkillDefinition.h"
#include "skills/SkillManager.h"
#include "support/AppLogger.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTimer>

// ============================================================================
// 辅助函数
// ============================================================================

QString compactParameters(const QJsonObject &parameters)
{
    return QString::fromUtf8(QJsonDocument(parameters).toJson(QJsonDocument::Compact));
}

QString actionFingerprint(const AgentPlanStep &step)
{
    return QStringLiteral("%1|%2|%3").arg(step.id, step.toolId, compactParameters(step.parameters));
}

int nextExecutableStepIndex(const AgentPlan &plan, const AgentToolRegistry &registry)
{
    for (int index = 0; index < plan.steps.size(); ++index) {
        const AgentPlanStep &step = plan.steps[index];
        if (step.status == AgentPlanStepStatus::Pending && registry.canExecuteDirectly(step.toolId)) {
            return index;
        }
    }
    return -1;
}

bool stopRequested(const AgentLoopOptions &options)
{
    return options.shouldStop && options.shouldStop();
}

void logAudit(QStringList *auditTrail, const QString &line)
{
    if (auditTrail != nullptr) {
        auditTrail->append(line);
    }
    AppLogger::info(QStringLiteral("AgentLoop"), line);
}

AgentLoopRunResult finishWith(
    AgentLoopRunResult result,
    AgentLoopRunStatus status,
    const QString &error = QString())
{
    result.status = status;
    result.error = error;
    logAudit(&result.auditTrail,
             QStringLiteral("Evaluate: status=%1 executedSteps=%2 error=%3")
                 .arg(AgentLoopController::runStatusToString(status))
                 .arg(result.executedStepCount)
                 .arg(error));
    return result;
}

QString trimObservationOutput(const QString &output)
{
    constexpr int kMaxObservationLines = 50;
    constexpr int kKeepHeadLines = 20;
    constexpr int kKeepTailLines = 20;
    constexpr int kMaxObservationChars = 3000;

    const QStringList lines = output.split(QLatin1Char('\n'));
    if (lines.size() <= kMaxObservationLines && output.size() <= kMaxObservationChars) {
        return output;
    }

    if (lines.size() > kMaxObservationLines) {
        QStringList trimmed;
        trimmed.reserve(kKeepHeadLines + kKeepTailLines + 1);
        for (int i = 0; i < kKeepHeadLines && i < lines.size(); ++i) {
            trimmed.append(lines[i]);
        }
        trimmed.append(QStringLiteral("... [%1 lines trimmed] ...").arg(lines.size() - kKeepHeadLines - kKeepTailLines));
        for (int i = lines.size() - kKeepTailLines; i < lines.size(); ++i) {
            if (i >= 0) trimmed.append(lines[i]);
        }
        return trimmed.join(QLatin1Char('\n'));
    }

    return output.left(kMaxObservationChars)
           + QStringLiteral("\n... [trimmed to %1 chars]").arg(kMaxObservationChars);
}

// ============================================================================
// D-2: AgentLoopEngine — 基于 QObject 信号/槽的异步循环引擎
// ============================================================================

LoopTerminationPolicy LoopTerminationPolicy::fromOptions(const AgentLoopOptions &options)
{
    LoopTerminationPolicy policy;
    policy.maxSteps = options.maxSteps;
    policy.maxRuntimeMs = options.maxRuntimeMs;
    policy.maxStepMs = options.maxStepMs;
    policy.shouldStop = options.shouldStop;
    return policy;
}

TerminationReason LoopTerminationPolicy::check(int completedSteps, qint64 elapsedMs, qint64 lastStepMs) const
{
    if (shouldStop && shouldStop()) {
        return TerminationReason::Stopped;
    }
    if (completedSteps >= maxSteps) {
        return TerminationReason::StepLimit;
    }
    if (elapsedMs >= maxRuntimeMs) {
        return TerminationReason::RuntimeLimit;
    }
    if (lastStepMs > 0 && lastStepMs >= maxStepMs) {
        return TerminationReason::StepTimeout;
    }
    return TerminationReason::None;
}

QString LoopTerminationPolicy::reasonLabel(TerminationReason reason)
{
    switch (reason) {
    case TerminationReason::None: return QStringLiteral("None");
    case TerminationReason::StepLimit: return QStringLiteral("StepLimit");
    case TerminationReason::RuntimeLimit: return QStringLiteral("RuntimeLimit");
    case TerminationReason::StepTimeout: return QStringLiteral("StepTimeout");
    case TerminationReason::Stopped: return QStringLiteral("Stopped");
    case TerminationReason::AIDone: return QStringLiteral("AIDone");
    case TerminationReason::AIError: return QStringLiteral("AIError");
    case TerminationReason::ParseError: return QStringLiteral("ParseError");
    }
    return QStringLiteral("Unknown");
}

AiLoopResponse AiLoopResponse::success(const QString &text)
{
    AiLoopResponse resp;
    resp.ok = true;
    resp.fullText = text;
    return resp;
}

AiLoopResponse AiLoopResponse::failure(const QString &msg, RequestErrorCategory category)
{
    AiLoopResponse resp;
    resp.ok = false;
    resp.errorMessage = msg;
    resp.errorCategory = category;
    return resp;
}

AgentLoopEngine::AgentLoopEngine(QObject *parent)
    : QObject(parent)
{
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this]() {
        finish(AgentLoopRunStatus::StepTimeout,
               QStringLiteral("AI step timed out after %1 ms.").arg(m_options.maxStepMs));
    });
}

void AgentLoopEngine::start(AIClient *aiClient, const AppConfig &config, const QString &userGoal,
                              const AgentToolRegistry &registry, const AgentToolExecutionContext &context,
                              const AgentLoopOptions &options,
                              const AgentLoopCallbacks &callbacks,
                              AppLanguage language,
                              HookManager *hooks, SkillManager *skills)
{
    if (m_running) return;

    m_aiClient = aiClient;
    m_config = &config;
    m_userGoal = userGoal;
    m_registry = registry;
    m_context = context;
    m_options = options;
    m_callbacks = callbacks;
    m_language = language;
    m_hooks = hooks;
    m_skills = skills;

    m_running = true;
    m_stepCount = 0;
    m_runtime.start();
    m_observations.clear();
    m_actionFingerprints.clear();
    m_result = {};

    // 异步启动第一轮
    QTimer::singleShot(0, this, &AgentLoopEngine::doIteration);
}

void AgentLoopEngine::stop()
{
    if (!m_running) return;
    finish(AgentLoopRunStatus::Stopped);
}

bool AgentLoopEngine::isRunning() const
{
    return m_running;
}

void AgentLoopEngine::doIteration()
{
    if (!m_running) return;

    // 检查终止条件
    const qint64 elapsed = m_runtime.elapsed();
    LoopTerminationPolicy policy = LoopTerminationPolicy::fromOptions(m_options);
    TerminationReason reason = policy.check(m_stepCount, elapsed);
    if (reason != TerminationReason::None) {
        AgentLoopRunStatus status = AgentLoopRunStatus::Failed;
        if (reason == TerminationReason::StepLimit) status = AgentLoopRunStatus::StepLimitReached;
        else if (reason == TerminationReason::RuntimeLimit) status = AgentLoopRunStatus::RuntimeLimitReached;
        else if (reason == TerminationReason::Stopped) status = AgentLoopRunStatus::Stopped;
        finish(status);
        return;
    }

    emit stepStarted(m_stepCount);
    if (m_callbacks.stepStarted) m_callbacks.stepStarted(m_stepCount);

    // 构建提示词 — 简化版，完整的提示词构建由 AgentOrchestrator 负责
    const QString prompt = m_userGoal + QStringLiteral("\n\nObservations:\n")
                           + m_observations.join(QStringLiteral("\n"));

    if (!m_aiClient) {
        finish(AgentLoopRunStatus::Failed, QStringLiteral("AIClient is null."));
        return;
    }

    // JIT skill 匹配
    if (m_skills) {
        m_skills->matchSkills(m_userGoal);
    }

    // 发送 AI 请求（异步）
    ChatSession session = ChatSession::createDefault();
    session.addMessage(MessageRole::User, prompt);

    // 断开旧连接
    if (m_connDelta) QObject::disconnect(m_connDelta);
    if (m_connFinished) QObject::disconnect(m_connFinished);
    if (m_connFailed) QObject::disconnect(m_connFailed);

    m_currentResponseText.clear();
    m_connDelta = connect(m_aiClient, &AIClient::textDeltaReceived, this,
        [this](const QString &delta) {
            m_currentResponseText += delta;
            emit aiResponseReceived(m_currentResponseText);
        });

    m_connFinished = connect(m_aiClient, &AIClient::requestFinished, this,
        [this]() {
            m_timeoutTimer.stop();
            handleAiResponse(AiLoopResponse::success(m_currentResponseText));
        });

    m_connFailed = connect(m_aiClient, &AIClient::requestFailed, this,
        [this](const QString &msg, RequestErrorCategory cat) {
            m_timeoutTimer.stop();
            AiLoopResponse resp;
            resp.ok = false;
            resp.errorMessage = msg;
            resp.errorCategory = cat;
            handleAiResponse(resp);
        });

    // 超时保护
    m_timeoutTimer.start(static_cast<int>(m_options.maxStepMs));

    m_aiClient->sendChat(*m_config, session);
}

void AgentLoopEngine::handleAiResponse(const AiLoopResponse &response)
{
    if (!m_running) return;

    if (!response.ok) {
        finish(AgentLoopRunStatus::Failed, response.errorMessage);
        return;
    }

    emit aiResponseReceived(response.fullText);
    m_result.lastOutput = response.fullText;
    m_result.executedStepCount = m_stepCount;

    AgentPlan plan;  // stub — actual parsing done by caller
    // D-2: AgentLoopEngine 只负责 AI 调用和超时管理。
    // 真正的响应解析和工具执行由 AgentOrchestrator（已异步）处理。
    // 这里直接将响应文本通过信号传出供测试/tools调用方处理。
    finish(AgentLoopRunStatus::Completed);
}

void AgentLoopEngine::finish(AgentLoopRunStatus status, const QString &error)
{
    if (!m_running) return;

    m_running = false;
    m_timeoutTimer.stop();

    m_result.status = status;
    m_result.error = error;

    logAudit(&m_result.auditTrail,
             QStringLiteral("Finished: status=%1 steps=%2 error=%3")
                 .arg(AgentLoopController::runStatusToString(status))
                 .arg(m_result.executedStepCount)
                 .arg(error));

    emit finished(m_result);
}

// ============================================================================
// 静态辅助
// ============================================================================

namespace AgentLoopController {

QString runStatusToString(AgentLoopRunStatus status)
{
    switch (status) {
    case AgentLoopRunStatus::Completed:
        return QStringLiteral("Completed");
    case AgentLoopRunStatus::Failed:
        return QStringLiteral("Failed");
    case AgentLoopRunStatus::Stopped:
        return QStringLiteral("Stopped");
    case AgentLoopRunStatus::StepLimitReached:
        return QStringLiteral("StepLimitReached");
    case AgentLoopRunStatus::RuntimeLimitReached:
        return QStringLiteral("RuntimeLimitReached");
    case AgentLoopRunStatus::StepTimeout:
        return QStringLiteral("StepTimeout");
    case AgentLoopRunStatus::RepeatedAction:
        return QStringLiteral("RepeatedAction");
    }
    return QStringLiteral("Unknown");
}

// ── 同步包装（供测试使用，内部用 QEventLoop 等 AgentLoopEngine 结果） ───────

AgentLoopRunResult runPlan(
    AgentPlan *plan,
    const AgentToolRegistry &registry,
    const AgentToolExecutionContext &context,
    const AgentLoopOptions &options,
    const AgentLoopCallbacks &callbacks)
{
    if (plan == nullptr) {
        return finishWith(AgentLoopRunResult{}, AgentLoopRunStatus::Failed,
                          QStringLiteral("Plan is null."));
    }

    int successCount = 0;
    int failCount = 0;
    int executedSteps = 0;
    AgentLoopRunResult result;
    QSet<QString> fingerprints;

    for (int i = 0; i < plan->steps.size(); ++i) {
        AgentPlanStep &step = plan->steps[i];

        if (stopRequested(options)) {
            result.executedStepCount = executedSteps;
            return finishWith(result, AgentLoopRunStatus::Stopped);
        }

        // D-2: 检查步数限制
        if (executedSteps >= options.maxSteps) {
            result.executedStepCount = executedSteps;
            return finishWith(result, AgentLoopRunStatus::StepLimitReached);
        }

        // D-2: 重复动作检测
        const QString fp = actionFingerprint(step);
        if (fingerprints.contains(fp)) {
            result.executedStepCount = executedSteps;
            return finishWith(result, AgentLoopRunStatus::RepeatedAction);
        }
        fingerprints.insert(fp);

        if (callbacks.stepStarted) callbacks.stepStarted(i);

        const ToolResult toolResult = registry.execute(
            step.toolId, step.parameters, context, nullptr);

        ++executedSteps;

        if (callbacks.stepFinished) callbacks.stepFinished(i, toolResult);

        result.lastToolId = step.toolId;
        result.lastOutput = toolResult.ok ? toolResult.output : toolResult.error;

        if (toolResult.ok) {
            ++successCount;
            step.status = AgentPlanStepStatus::Completed;
        } else {
            ++failCount;
            step.status = AgentPlanStepStatus::Failed;
            result.error = toolResult.error;
        }

        result.auditTrail.append(
            QStringLiteral("[Step %1] %2 (%3): %4")
                .arg(i + 1).arg(toolResult.ok ? QStringLiteral("OK") : QStringLiteral("FAIL"))
                .arg(step.toolId)
                .arg(toolResult.output.left(200)));
    }

    result.executedStepCount = executedSteps;
    result.status = (failCount > 0) ? AgentLoopRunStatus::Failed : AgentLoopRunStatus::Completed;

    result.auditTrail.append(
        QStringLiteral("Evaluate: status=%1 executedSteps=%2 error=%3")
            .arg(AgentLoopController::runStatusToString(result.status))
            .arg(result.executedStepCount)
            .arg(result.error));

    return result;
}

AgentLoopRunResult executeLoop(
    AIClient *aiClient,
    const AppConfig &config,
    const QString &userGoal,
    const AgentToolRegistry &registry,
    const AgentToolExecutionContext &context,
    const AgentLoopOptions &options,
    const AgentLoopCallbacks &callbacks,
    AppLanguage language,
    HookManager *hooks,
    SkillManager *skills)
{
    // 使用 QEventLoop 同步等待 AgentLoopEngine 异步执行完成
    QEventLoop loop;
    AgentLoopRunResult result;

    AgentLoopEngine *engine = new AgentLoopEngine();
    QObject::connect(engine, &AgentLoopEngine::finished,
        [&](const AgentLoopRunResult &r) {
            result = r;
            loop.quit();
        });

    engine->start(aiClient, config, userGoal, registry, context,
                   options, callbacks, language, hooks, skills);

    loop.exec();
    delete engine;

    return result;
}

} // namespace AgentLoopController
