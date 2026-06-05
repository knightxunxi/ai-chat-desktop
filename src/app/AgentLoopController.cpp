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
#include <QSet>
#include <QTimer>

namespace {

// ============================================================================
// AiLoopRunner — 同步调用 AIClient::sendChat，使用 QEventLoop 等待结果
// ============================================================================

class AiLoopRunner {
public:
    // 功能：同步调用 AIClient::sendChat，封装 QEventLoop 等待；使用模块：executeLoop 单步 AI 调用。
    static AiLoopResponse call(
        AIClient *client,
        const AppConfig &config,
        const QString &prompt,
        qint64 maxStepMs)
    {
        if (client == nullptr) {
            return AiLoopResponse::failure(
                QStringLiteral("AIClient is null."), RequestErrorCategory::Unknown);
        }

        // 1. 构建只含一条 user 消息的 ChatSession
        ChatSession session = ChatSession::createDefault();
        session.addMessage(MessageRole::User, prompt);

        // 2. 使用 QEventLoop 等待异步结果
        QEventLoop loop;
        QString fullText;
        bool finished = false;
        QString errorMessage;
        RequestErrorCategory errorCategory = RequestErrorCategory::Unknown;

        // 连接文本增量信号
        QMetaObject::Connection connDelta = QObject::connect(
            client, &AIClient::textDeltaReceived,
            [&fullText](const QString &delta) {
                fullText += delta;
            });

        // 连接请求完成信号
        QMetaObject::Connection connFinished = QObject::connect(
            client, &AIClient::requestFinished,
            [&loop, &finished]() {
                finished = true;
                loop.quit();
            });

        // 连接请求失败信号
        QMetaObject::Connection connFailed = QObject::connect(
            client, &AIClient::requestFailed,
            [&loop, &errorMessage, &errorCategory](const QString &msg, RequestErrorCategory cat) {
                errorMessage = msg;
                errorCategory = cat;
                loop.quit();
            });

        // 3. 超时保护
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timeoutTimer.start(static_cast<int>(maxStepMs));

        // 4. 发起请求
        client->sendChat(config, session);

        // 5. 进入事件循环等待
        loop.exec();

        // 6. 断开所有信号连接
        QObject::disconnect(connDelta);
        QObject::disconnect(connFinished);
        QObject::disconnect(connFailed);

        // 7. 判断结果
        if (!finished && errorMessage.isEmpty()) {
            return AiLoopResponse::failure(
                QStringLiteral("AI step timed out after %1 ms.").arg(maxStepMs),
                RequestErrorCategory::Unknown);
        }

        if (!errorMessage.isEmpty()) {
            return AiLoopResponse::failure(errorMessage, errorCategory);
        }

        return AiLoopResponse::success(fullText);
    }
};

// ============================================================================
// 复用自原有 runPlan() 的辅助函数
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

} // namespace

// ============================================================================
// LoopTerminationPolicy 实现
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
    case TerminationReason::None:
        return QStringLiteral("none");
    case TerminationReason::StepLimit:
        return QStringLiteral("step_limit");
    case TerminationReason::RuntimeLimit:
        return QStringLiteral("runtime_limit");
    case TerminationReason::StepTimeout:
        return QStringLiteral("step_timeout");
    case TerminationReason::Stopped:
        return QStringLiteral("stopped");
    case TerminationReason::AIDone:
        return QStringLiteral("ai_done");
    case TerminationReason::AIError:
        return QStringLiteral("ai_error");
    case TerminationReason::ParseError:
        return QStringLiteral("parse_error");
    }

    return QStringLiteral("unknown");
}

// ============================================================================
// AiLoopResponse 静态工厂
// ============================================================================

AiLoopResponse AiLoopResponse::success(const QString &text)
{
    AiLoopResponse response;
    response.ok = true;
    response.fullText = text;
    return response;
}

AiLoopResponse AiLoopResponse::failure(const QString &msg, RequestErrorCategory category)
{
    AiLoopResponse response;
    response.ok = false;
    response.errorMessage = msg;
    response.errorCategory = category;
    return response;
}

namespace AgentLoopController {

QString runStatusToString(AgentLoopRunStatus status)
{
    switch (status) {
    case AgentLoopRunStatus::Completed:
        return QStringLiteral("completed");
    case AgentLoopRunStatus::Failed:
        return QStringLiteral("failed");
    case AgentLoopRunStatus::Stopped:
        return QStringLiteral("stopped");
    case AgentLoopRunStatus::StepLimitReached:
        return QStringLiteral("step_limit_reached");
    case AgentLoopRunStatus::RuntimeLimitReached:
        return QStringLiteral("runtime_limit_reached");
    case AgentLoopRunStatus::StepTimeout:
        return QStringLiteral("step_timeout");
    case AgentLoopRunStatus::RepeatedAction:
        return QStringLiteral("repeated_action");
    }

    return QStringLiteral("failed");
}

AgentLoopRunResult runPlan(
    AgentPlan *plan,
    const AgentToolRegistry &registry,
    const AgentToolExecutionContext &context,
    const AgentLoopOptions &options,
    const AgentLoopCallbacks &callbacks)
{
    AgentLoopRunResult result;
    if (plan == nullptr) {
        return finishWith(result, AgentLoopRunStatus::Failed, QStringLiteral("Agent plan is null."));
    }

    if (options.maxSteps <= 0) {
        return finishWith(result, AgentLoopRunStatus::Failed, QStringLiteral("Maximum step count must be positive."));
    }

    QElapsedTimer runtimeTimer;
    runtimeTimer.start();
    QSet<QString> seenActions;

    while (true) {
        logAudit(&result.auditTrail,
                 QStringLiteral("Observe: completedSteps=%1 totalSteps=%2 lastTool=%3 lastOutputLength=%4")
                     .arg(result.executedStepCount)
                     .arg(plan->steps.size())
                     .arg(result.lastToolId)
                     .arg(result.lastOutput.size()));

        if (stopRequested(options)) {
            return finishWith(result, AgentLoopRunStatus::Stopped);
        }

        if (result.executedStepCount >= options.maxSteps) {
            return finishWith(result, AgentLoopRunStatus::StepLimitReached);
        }

        if (runtimeTimer.elapsed() >= options.maxRuntimeMs) {
            return finishWith(result, AgentLoopRunStatus::RuntimeLimitReached);
        }

        const int index = nextExecutableStepIndex(*plan, registry);
        if (index < 0) {
            logAudit(&result.auditTrail, QStringLiteral("Think: done=true reason=no_executable_pending_step"));
            return finishWith(result, AgentLoopRunStatus::Completed);
        }

        AgentPlanStep &step = plan->steps[index];
        const QString fingerprint = actionFingerprint(step);
        if (seenActions.contains(fingerprint)) {
            return finishWith(
                result,
                AgentLoopRunStatus::RepeatedAction,
                QStringLiteral("Repeated Agent action detected for tool %1.").arg(step.toolId));
        }
        seenActions.insert(fingerprint);

        logAudit(&result.auditTrail,
                 QStringLiteral("Think: done=false stepId=%1 toolId=%2")
                     .arg(step.id, step.toolId));

        step.status = AgentPlanStepStatus::Running;
        if (callbacks.stepStarted) {
            callbacks.stepStarted(index);
        }

        QElapsedTimer stepTimer;
        stepTimer.start();
        const ToolResult toolResult = registry.execute(step.toolId, step.parameters, context);
        const qint64 stepElapsedMs = stepTimer.elapsed();

        result.lastToolId = step.toolId;
        result.lastOutput = toolResult.output;

        if (!toolResult.ok) {
            step.status = AgentPlanStepStatus::Failed;
            step.output = QStringLiteral("Tool failed: %1").arg(toolResult.error);
            result.lastOutput = step.output;
            if (callbacks.stepFinished) {
                callbacks.stepFinished(index, toolResult);
            }
            logAudit(&result.auditTrail,
                     QStringLiteral("Act: stepId=%1 toolId=%2 ok=false elapsedMs=%3 error=%4")
                         .arg(step.id, step.toolId)
                         .arg(stepElapsedMs)
                         .arg(toolResult.error));
            return finishWith(result, AgentLoopRunStatus::Failed, toolResult.error);
        }

        step.status = AgentPlanStepStatus::Completed;
        step.output = toolResult.output;
        ++result.executedStepCount;

        if (callbacks.stepFinished) {
            callbacks.stepFinished(index, toolResult);
        }

        logAudit(&result.auditTrail,
                 QStringLiteral("Act: stepId=%1 toolId=%2 ok=true elapsedMs=%3 outputLength=%4")
                     .arg(step.id, step.toolId)
                     .arg(stepElapsedMs)
                     .arg(toolResult.output.size()));

        if (stepElapsedMs >= options.maxStepMs) {
            return finishWith(
                result,
                AgentLoopRunStatus::StepTimeout,
                QStringLiteral("Agent step exceeded the time limit."));
        }
    }
}

// ============================================================================
// executeLoop — 每步调 AI → 解析 → 执行工具 → 收集观测 → 继续
// ============================================================================

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
    AgentLoopRunResult result;

    // 1. 校验 aiClient 非空
    if (aiClient == nullptr) {
        return finishWith(result, AgentLoopRunStatus::Failed, QStringLiteral("AIClient is null."));
    }

    if (options.maxSteps <= 0) {
        return finishWith(result, AgentLoopRunStatus::Failed, QStringLiteral("Maximum step count must be positive."));
    }

    // V13.3: on_agent_start Hook
    if (hooks != nullptr) {
        hooks->executeHooks(HookPoint::OnAgentStart,
                            HookContext::forAgentLifecycle(HookPoint::OnAgentStart, userGoal));
    }

    // 2. 初始化计时器和收集容器
    QElapsedTimer runtimeTimer;
    runtimeTimer.start();
    QStringList observations;
    int completedSteps = 0;

    // 3. 从 options 构造终止策略
    LoopTerminationPolicy policy = LoopTerminationPolicy::fromOptions(options);

    // 4. 提取仅 enabledForAgent 的工具目录
    const QVector<AgentToolDescriptor> allDescriptors = registry.descriptors();
    QVector<AgentToolDescriptor> toolCatalog;
    for (const AgentToolDescriptor &desc : allDescriptors) {
        if (desc.enabledForAgent) {
            toolCatalog.append(desc);
        }
    }

    // V13.3: 技能匹配（pre_send 阶段）
    QVector<SkillDefinition> matchedSkills;
    if (skills != nullptr) {
        matchedSkills = skills->matchSkills(userGoal);
    }

    // 5. 主循环
    while (true) {
        const qint64 elapsedMs = runtimeTimer.elapsed();

        // ----- 观测阶段 -----
        logAudit(&result.auditTrail,
                 QStringLiteral("Observe: completedSteps=%1 observations=%2")
                     .arg(completedSteps)
                     .arg(observations.size()));

        // a. 检查用户停止请求
        if (policy.shouldStop && policy.shouldStop()) {
            // V13.3: on_agent_stop
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(result, AgentLoopRunStatus::Stopped,
                              QStringLiteral("User requested stop."));
        }

        // b. 检查步数上限和运行时间上限
        const TerminationReason preCheck = policy.check(completedSteps, elapsedMs);
        if (preCheck == TerminationReason::StepLimit) {
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(result, AgentLoopRunStatus::StepLimitReached,
                              QStringLiteral("Step limit of %1 reached.").arg(policy.maxSteps));
        }
        if (preCheck == TerminationReason::RuntimeLimit) {
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(result, AgentLoopRunStatus::RuntimeLimitReached,
                              QStringLiteral("Runtime limit of %1 ms reached.").arg(policy.maxRuntimeMs));
        }

        // ----- 思考阶段：构建提示词 -----
        QString prompt = AgentLoopPromptBuilder::buildNextActionPrompt(
            userGoal, observations, toolCatalog, language, completedSteps, policy.maxSteps, matchedSkills);

        // V13.3: pre_send Hook
        if (hooks != nullptr) {
            HookContext preSendCtx = HookContext::forPreSend(prompt, userGoal, completedSteps, QString());
            QVector<HookResult> preSendResults = hooks->executeHooks(HookPoint::PreSend, preSendCtx);

            // 检查是否有 Reject
            for (const HookResult &hr : preSendResults) {
                if (hr.isReject()) {
                    if (hooks != nullptr) {
                        hooks->executeHooks(HookPoint::OnAgentStop,
                                            HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
                    }
                    return finishWith(result, AgentLoopRunStatus::Failed,
                                      QStringLiteral("PreSend hook rejected: %1").arg(hr.rejectionReason()));
                }
                // 应用 Modify
                if (hr.action == HookAction::Modify && hr.modifiedContext.contains(QStringLiteral("prompt"))) {
                    prompt = hr.modifiedContext.value(QStringLiteral("prompt")).toString();
                }
            }
        }

        // c. 调用 AI
        const AiLoopResponse aiResponse = AiLoopRunner::call(aiClient, config, prompt, policy.maxStepMs);

        // d. AI 调用失败则终止
        if (!aiResponse.ok) {
            logAudit(&result.auditTrail,
                     QStringLiteral("Think: ai_error=%1 category=%2")
                         .arg(aiResponse.errorMessage, RequestErrorCategoryHelpers::toString(aiResponse.errorCategory)));

            // V13.3: on_error Hook
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnError,
                                    HookContext::forError(aiResponse.errorMessage, completedSteps));
            }

            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(result, AgentLoopRunStatus::Failed, aiResponse.errorMessage);
        }

        // ----- 解析阶段 -----
        logAudit(&result.auditTrail, QStringLiteral("Think: ai_response_length=%1").arg(aiResponse.fullText.size()));

        // V13.3: post_receive Hook
        QString aiResponseText = aiResponse.fullText;
        if (hooks != nullptr) {
            HookContext postReceiveCtx = HookContext::forPostReceive(aiResponseText, completedSteps);
            QVector<HookResult> postResults = hooks->executeHooks(HookPoint::PostReceive, postReceiveCtx);

            for (const HookResult &hr : postResults) {
                if (hr.action == HookAction::Modify && hr.modifiedContext.contains(QStringLiteral("response"))) {
                    aiResponseText = hr.modifiedContext.value(QStringLiteral("response")).toString();
                }
            }
        }

        const AgentLoopActionParseResult parseResult =
            AgentLoopActionParser::parseJsonAction(aiResponseText, toolCatalog);

        // e. 解析失败不终止，将错误作为 observation 注入继续循环
        if (!parseResult.ok) {
            const QString parseObservation = QStringLiteral("[parse error: %1]").arg(parseResult.error);
            observations.append(parseObservation);
            logAudit(&result.auditTrail,
                     QStringLiteral("Think: parse_error=%1").arg(parseResult.error));
            continue;
        }

        const AgentLoopAction &action = parseResult.action;

        // f. AI 声明任务完成
        if (action.done) {
            logAudit(&result.auditTrail,
                     QStringLiteral("Think: done=true message=%1").arg(action.message));
            result.lastOutput = action.message;
            // V13.3: on_agent_stop
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(result, AgentLoopRunStatus::Completed);
        }

        // ----- 执行阶段 -----
        logAudit(&result.auditTrail,
                 QStringLiteral("Think: done=false toolId=%1 reason=%2")
                     .arg(action.step.toolId, action.step.reason));

        // g. 步骤开始回调
        if (callbacks.stepStarted) {
            callbacks.stepStarted(completedSteps);
        }

        // V13.3: on_tool_execute (before)
        if (hooks != nullptr) {
            hooks->executeHooks(HookPoint::OnToolExecute,
                                HookContext::forToolExecute(action.step.toolId, action.step.parameters, true));
        }

        // h. 执行工具
        QElapsedTimer stepTimer;
        stepTimer.start();
        const ToolResult toolResult = registry.execute(
            action.step.toolId, action.step.parameters, context, hooks);
        const qint64 stepElapsedMs = stepTimer.elapsed();

        // V13.3: on_tool_execute (after)
        if (hooks != nullptr) {
            hooks->executeHooks(HookPoint::OnToolExecute,
                                HookContext::forToolExecute(action.step.toolId, action.step.parameters, false));
        }

        result.lastToolId = action.step.toolId;
        result.lastOutput = toolResult.output;

        // i. 步骤结束回调
        if (callbacks.stepFinished) {
            callbacks.stepFinished(completedSteps, toolResult);
        }

        // j. 工具执行失败则终止
        if (!toolResult.ok) {
            logAudit(&result.auditTrail,
                     QStringLiteral("Act: toolId=%1 ok=false elapsedMs=%2 error=%3")
                         .arg(action.step.toolId)
                         .arg(stepElapsedMs)
                         .arg(toolResult.error));
            // V13.3: on_error Hook
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnError,
                                    HookContext::forError(toolResult.error, completedSteps));
            }
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(result, AgentLoopRunStatus::Failed, toolResult.error);
        }

        logAudit(&result.auditTrail,
                 QStringLiteral("Act: toolId=%1 ok=true elapsedMs=%2 outputLength=%3")
                     .arg(action.step.toolId)
                     .arg(stepElapsedMs)
                     .arg(toolResult.output.size()));

        // k. 收集观测
        observations.append(QStringLiteral("%1: %2").arg(action.step.toolId, toolResult.output));
        completedSteps++;

        // l. 单步超时检查
        if (stepElapsedMs >= policy.maxStepMs) {
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(
                result,
                AgentLoopRunStatus::StepTimeout,
                QStringLiteral("Agent step exceeded the time limit of %1 ms.").arg(policy.maxStepMs));
        }

        // m. 精确实时终止检查（含单步时间）
        const qint64 updatedElapsedMs = runtimeTimer.elapsed();
        const TerminationReason postCheck = policy.check(completedSteps, updatedElapsedMs, stepElapsedMs);
        if (postCheck == TerminationReason::StepLimit) {
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(result, AgentLoopRunStatus::StepLimitReached,
                              QStringLiteral("Step limit of %1 reached.").arg(policy.maxSteps));
        }
        if (postCheck == TerminationReason::RuntimeLimit) {
            if (hooks != nullptr) {
                hooks->executeHooks(HookPoint::OnAgentStop,
                                    HookContext::forAgentLifecycle(HookPoint::OnAgentStop, userGoal));
            }
            return finishWith(result, AgentLoopRunStatus::RuntimeLimitReached,
                              QStringLiteral("Runtime limit of %1 ms reached.").arg(policy.maxRuntimeMs));
        }
    }
}

} // namespace AgentLoopController
