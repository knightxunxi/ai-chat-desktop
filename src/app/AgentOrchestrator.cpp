#include "app/AgentOrchestrator.h"

#include "app/AgentCommandSkillFileService.h"
#include "app/AgentLoopPromptBuilder.h"
#include "app/AgentPlanExecutor.h"
#include "app/AgentPlanParser.h"
#include "app/AgentPlanPromptBuilder.h"
#include "app/ConfigCoordinator.h"
#include "app/ContextWindowManager.h"
#include "app/SessionCoordinator.h"
#include "app/TokenEstimator.h"
#include "hooks/BuiltinHooks.h"
#include "hooks/HookDefinition.h"
#include "hooks/HookManager.h"
#include "mcp/McpRegistry.h"
#include "skills/SkillDefinition.h"
#include "skills/SkillManager.h"
#include "support/AppLogger.h"
#include "tools/AgentToolCatalog.h"
#include "tools/AgentToolRegistry.h"
#include "memory/ProjectMemoryManager.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>

namespace {

// V17.3: 粗略 Token 估算，用于 TokenBar 显示。
int estimateTokenCount(const QString &text)
{
    if (text.isEmpty()) {
        return 0;
    }

    int tokens = 0;
    for (const QChar &ch : text) {
        if (ch.unicode() > 0x7F) {
            tokens += 2;
        } else {
            tokens += 1;
        }
    }
    return tokens / 3;
}

} // namespace

AgentOrchestrator::AgentOrchestrator(QObject *parent)
    : QObject(parent)
{
}

void AgentOrchestrator::initialize(AIClient *aiClient, ConfigCoordinator *configCoordinator, SessionCoordinator *sessionCoordinator)
{
    m_aiClient = aiClient;
    m_configCoordinator = configCoordinator;
    m_sessionCoordinator = sessionCoordinator;

    // V12.1 上下文窗口管理
    m_summaryClient.reconfigure(m_configCoordinator->config());
    m_contextWindowManager = ContextWindowManager(getContextWindowTokens());
    m_contextWindowManager.setSummaryClient(&m_summaryClient);

    // V13.3: Skills + Hooks 系统初始化
    m_hookManager = std::make_unique<HookManager>();

    // 注册内置 Hook
    m_hookManager->registerHook(new TimestampHook());
    m_hookManager->registerHook(new RateLimitHook());
    m_hookManager->registerHook(new SensitiveFilterHook());

    m_skillManager = std::make_unique<SkillManager>();

    // 设置双目录
    const QString userSkillsDir = QDir::homePath() + QStringLiteral("/.codex/skills");
    QString projectSkillsDir;
    if (!m_configCoordinator->config().agentProjectDirectory.isEmpty()) {
        projectSkillsDir = QDir::cleanPath(m_configCoordinator->config().agentProjectDirectory + QStringLiteral("/.workbuddy/skills"));
    }
    m_skillManager->initialize(userSkillsDir, projectSkillsDir);

    // V15.4: 初始化 MCP 注册表
    m_mcpRegistry = std::make_unique<McpRegistry>(this);

    if (!m_configCoordinator->config().agentProjectDirectory.isEmpty()) {
        const QString mcpConfigPath = QDir::cleanPath(
            m_configCoordinator->config().agentProjectDirectory + QStringLiteral("/.workbuddy/mcp_servers.json"));
        m_mcpRegistry->loadConfig(mcpConfigPath);
    }

    m_mcpRegistry->connectAll();

    // V17.5: 静默检测残留 Agent 状态（不弹窗，用户用"继续"命令触发恢复）
    QFile stateFile(agentStateFilePath());
    if (stateFile.exists() && stateFile.open(QIODevice::ReadOnly)) {
        m_agentLoopState = AgentLoopState::fromJson(stateFile.readAll());
        stateFile.close();
    }
}

size_t AgentOrchestrator::getContextWindowTokens() const
{
    if (m_configCoordinator->config().maxTokens.has_value()) {
        return static_cast<size_t>(m_configCoordinator->config().maxTokens.value());
    }
    return ContextWindowManager::contextWindowForModel(m_configCoordinator->config().modelName);
}

// ─── Agent 循环入口 ─────────────────────────────────────────────────

void AgentOrchestrator::startAgentLoop(const QString &goal)
{
    const QString trimmedGoal = goal.trimmed();
    if (trimmedGoal.isEmpty()) {
        return;
    }

    m_isAgentLoopActive = true;
    m_agentLoopGoal = trimmedGoal;
    m_agentLoopIteration = 0;
    m_agentLoopObservations.clear();

    // V17.2: 初始化 Agent 循环状态快照
    m_agentLoopState = AgentLoopState{};
    m_agentLoopState.goal = trimmedGoal;
    m_agentLoopState.sessionId = m_sessionCoordinator->currentSession().id;
    m_agentLoopState.startedAt = QDateTime::currentDateTimeUtc();
    m_agentLoopState.maxSteps = kMaxAgentLoopIterations;
    saveAgentLoopState();

    // V13.3: 匹配技能
    matchSkills(trimmedGoal);
}

void AgentOrchestrator::resumeAgentLoop()
{
    if (!m_agentLoopState.isValid()) {
        return;
    }

    // 注入已完成步骤的上下文到 session
    QString resumeContext = m_configCoordinator->text(
        QString("Previous steps completed (step 1-%1):\n").arg(m_agentLoopState.stepIndex),
        QString("之前的步骤已完成 (第 1-%1 步):\n").arg(m_agentLoopState.stepIndex));
    for (const auto &r : m_agentLoopState.accumulatedResults) {
        resumeContext += r.toString() + QStringLiteral("\n");
    }
    resumeContext += m_configCoordinator->text(
        QStringLiteral("\nContinue from step ") + QString::number(m_agentLoopState.stepIndex + 1),
        QStringLiteral("\n从第 ") + QString::number(m_agentLoopState.stepIndex + 1) + QStringLiteral(" 步继续"));

    // 追加为 system 消息
    m_sessionCoordinator->currentSession().addMessage(MessageRole::System, resumeContext);

    // 恢复循环状态
    m_isAgentLoopActive = true;
    m_agentLoopGoal = m_agentLoopState.goal;
    m_agentLoopIteration = m_agentLoopState.stepIndex;
}

void AgentOrchestrator::cancelAgentLoop()
{
    m_isAgentLoopActive = false;
    m_agentLoopGoal.clear();
    m_agentLoopObservations.clear();
    m_agentLoopIteration = 0;
    // V17.5: 中断时保留状态文件，供后续"继续"命令恢复
    // clearAgentLoopState() 仅在任务成功完成或新任务启动时调用
}

void AgentOrchestrator::clearAgentLoopState()
{
    m_agentLoopState = AgentLoopState{};
    QFile stateFile(agentStateFilePath());
    if (stateFile.exists()) {
        stateFile.remove();
    }
}

// ─── 状态查询 ───────────────────────────────────────────────────────

bool AgentOrchestrator::isAgentLoopActive() const
{
    return m_isAgentLoopActive;
}

int AgentOrchestrator::agentLoopIteration() const
{
    return m_agentLoopIteration;
}

int AgentOrchestrator::maxAgentLoopIterations() const
{
    return kMaxAgentLoopIterations;
}

bool AgentOrchestrator::hasPendingAgentState() const
{
    return m_agentLoopState.isValid();
}

AgentLoopState AgentOrchestrator::pendingAgentState() const
{
    return m_agentLoopState;
}

bool AgentOrchestrator::agentDebugMode() const
{
    return m_agentDebugMode;
}

void AgentOrchestrator::setAgentDebugMode(bool enabled)
{
    m_agentDebugMode = enabled;
}

// ─── 管理器访问器 ───────────────────────────────────────────────────

SkillManager *AgentOrchestrator::skillManager()
{
    return m_skillManager.get();
}

HookManager *AgentOrchestrator::hookManager()
{
    return m_hookManager.get();
}

McpRegistry *AgentOrchestrator::mcpRegistry()
{
    return m_mcpRegistry.get();
}

ContextWindowManager *AgentOrchestrator::contextWindowManager()
{
    return &m_contextWindowManager;
}

SummaryAPIClient *AgentOrchestrator::summaryClient()
{
    return &m_summaryClient;
}

// ─── 技能匹配 ───────────────────────────────────────────────────────

QVector<SkillDefinition> AgentOrchestrator::matchedSkills() const
{
    return m_matchedSkills;
}

void AgentOrchestrator::matchSkills(const QString &goal)
{
    if (m_skillManager) {
        m_matchedSkills = m_skillManager->matchSkills(goal);
    }
}

// ─── Agent 循环迭代 ─────────────────────────────────────────────────

bool AgentOrchestrator::executeAgentLoopIteration()
{
    m_agentLoopIteration++;

    // V17.2: 保存当前进度
    m_agentLoopState.stepIndex = m_agentLoopIteration;
    m_agentLoopState.updatedAt = QDateTime::currentDateTimeUtc();
    saveAgentLoopState();

    if (m_agentLoopIteration >= kMaxAgentLoopIterations) {
        m_isAgentLoopActive = false;
        clearAgentLoopState();

        // V13.3: 发送技能摘要
        if (m_skillManager && !m_matchedSkills.isEmpty()) {
            const QString summary = m_skillManager->skillSummary(m_matchedSkills);
            emit agentLoopSkillSummary(summary);
            emit statusMessage(summary, summary, 5000);
        }

        emit statusMessage(
            QStringLiteral("Agent loop reached max iterations (%1)").arg(kMaxAgentLoopIterations),
            QStringLiteral("Agent 循环已达最大轮次 (%1)").arg(kMaxAgentLoopIterations),
            5000);
        emit agentLoopCompleted();
        return false;
    }

    emit agentLoopIterationUpdated(m_agentLoopIteration, kMaxAgentLoopIterations);
    return true;
}

QString AgentOrchestrator::buildNextLoopPrompt() const
{
    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();

    const QString loopPrompt = AgentLoopPromptBuilder::buildNextActionPrompt(
        m_agentLoopGoal,
        m_agentLoopObservations,
        registry.descriptors(),
        m_configCoordinator->config().language,
        m_agentLoopIteration,
        kMaxAgentLoopIterations);

    // V16.3: Agent 调试 — 暴露完整的循环提示词
    if (m_agentDebugMode) {
        // emit is const-cast safe here since debug flag doesn't change logical state
        const_cast<AgentOrchestrator *>(this)->emit agentLoopPromptDebug(loopPrompt);
    }

    QString prompt = loopPrompt;

    // V13.1: 注入三层记忆到循环提示词
    if (!m_configCoordinator->config().agentProjectDirectory.isEmpty()) {
        ProjectMemoryManager memoryMgr(m_configCoordinator->config().agentProjectDirectory);
        prompt += QStringLiteral("\n\n") + memoryMgr.buildMemorySection();
    }

    return prompt;
}

void AgentOrchestrator::appendLoopObservation(const QString &observation)
{
    m_agentLoopObservations.append(observation);
}

// ─── 流式工具结果 ───────────────────────────────────────────────────

void AgentOrchestrator::addPendingToolResult(const QString &toolName, const QJsonObject &arguments,
                                              const QString &result, bool success)
{
    PendingToolResult pending;
    pending.toolName = toolName;
    pending.arguments = arguments;
    pending.result = result;
    pending.success = success;
    m_pendingToolResults.append(pending);
}

QVector<PendingToolResult> AgentOrchestrator::takePendingToolResults()
{
    QVector<PendingToolResult> results = m_pendingToolResults;
    m_pendingToolResults.clear();
    return results;
}

void AgentOrchestrator::clearPendingToolResults()
{
    m_pendingToolResults.clear();
}

// ─── Agent 计划执行 ─────────────────────────────────────────────────

bool AgentOrchestrator::executePlanAndReportToChat(const AgentPlan &plan)
{
    const QString &workspace = m_configCoordinator->config().agentProjectDirectory;
    QString execSummary;
    int successCount = 0;
    int failCount = 0;

    execSummary += QStringLiteral("\n\n---\n");
    execSummary += m_configCoordinator->text(
        QStringLiteral("**Executing plan (%1 steps)...**\n").arg(plan.steps.size()),
        QStringLiteral("**正在执行计划 (%1 步)...**\n").arg(plan.steps.size()));

    for (int i = 0; i < plan.steps.size(); ++i) {
        const AgentPlanStep &step = plan.steps[i];

        // V16.1: 发送思考信号
        emit agentLoopThought(m_agentLoopIteration, step.reason, step.toolId, step.title);

        const ToolResult result = AgentPlanExecutor::executeStep(
            step, workspace, workspace);

        // V16.1: 发送工具执行完成信号
        emit agentLoopToolFinished(m_agentLoopIteration, step.toolId, result.ok,
            result.output.left(80));

        // AG-4: 记录 Agent 执行步骤
        QString argsStr;
        {
            QJsonDocument argsDoc(step.parameters);
            argsStr = QString::fromUtf8(argsDoc.toJson(QJsonDocument::Compact));
        }
        m_sessionCoordinator->currentSession().agentSteps.append(AgentStepRecord::create(
            i + 1,
            step.reason,
            step.toolId,
            argsStr,
            result.output.left(500),
            result.ok ? QStringLiteral("success") : QStringLiteral("error")));

        const QString status = result.ok
            ? QStringLiteral("\u2705")
            : QStringLiteral("\u274C");

        const QString desc = step.title.isEmpty()
            ? step.toolId
            : step.title;

        QString resultPreview = result.output;
        if (resultPreview.length() > 300) {
            resultPreview = resultPreview.left(300) + QStringLiteral("...");
        }

        execSummary += QStringLiteral("%1 **[%2/%3] %4**\n> %5\n")
            .arg(status)
            .arg(i + 1)
            .arg(plan.steps.size())
            .arg(desc, resultPreview);

        if (result.ok) {
            ++successCount;
        } else {
            ++failCount;
            AppLogger::warning(QStringLiteral("AgentPlan"),
                               QStringLiteral("Step failed. step=%1 toolId=%2 error=%3")
                                   .arg(step.id, step.toolId, result.output));
        }
    }

    // V12.3: 追加流式工具执行结果
    if (!m_pendingToolResults.isEmpty()) {
        execSummary += QStringLiteral("\n**[Streaming results (executed early)]**\n");
        for (const PendingToolResult &pending : m_pendingToolResults) {
            const QString s = pending.success ? QStringLiteral("\u2705") : QStringLiteral("\u26A0\uFE0F");
            execSummary += QStringLiteral("%1 **%2**: %3\n")
                .arg(s, pending.toolName, pending.result);
        }
    }
    m_pendingToolResults.clear();

    execSummary += QStringLiteral("\n---");
    execSummary += m_configCoordinator->text(
        QStringLiteral("\n**Result**: %1/%2 steps succeeded").arg(successCount).arg(plan.steps.size()),
        QStringLiteral("\n**结果**: %1/%2 步成功").arg(successCount).arg(plan.steps.size()));

    // 追加到当前助手消息并通知 UI
    if (!m_sessionCoordinator->currentSession().messages.isEmpty()) {
        m_sessionCoordinator->currentSession().messages.last().content += execSummary;
    }
    emit assistantMessageUpdated(m_sessionCoordinator->currentSession().messages.last().content);
    m_sessionCoordinator->saveCurrentSession();

    // 状态栏提示
    emit statusMessage(
        QStringLiteral("Executed %1/%2 steps").arg(successCount).arg(plan.steps.size()),
        QStringLiteral("已执行 %1/%2 步").arg(successCount).arg(plan.steps.size()),
        5000);

    // V13.1: Agent 执行完成后自动追加每日日志
    if (!m_configCoordinator->config().agentProjectDirectory.isEmpty()) {
        ProjectMemoryManager memoryMgr(m_configCoordinator->config().agentProjectDirectory);
        QString logEntry = QStringLiteral("执行计划: %1/%2 步成功")
            .arg(successCount).arg(plan.steps.size());
        memoryMgr.appendDailyLog(QStringLiteral("log"), logEntry);
    }

    return (failCount == 0);
}

// ─── Agent 状态持久化 ───────────────────────────────────────────────

void AgentOrchestrator::saveAgentLoopState()
{
    QFile stateFile(agentStateFilePath());
    if (stateFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        stateFile.write(m_agentLoopState.toJson());
        stateFile.close();
    }
}

QString AgentOrchestrator::agentStateFilePath() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("agent_state.json"));
}
