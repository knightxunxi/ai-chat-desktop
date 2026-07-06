#include "app/AgentOrchestrator.h"

#include "app/AgentCommandSkillFileService.h"
#include "app/AgentLoopActionParser.h"
#include "app/AgentLoopPromptBuilder.h"
#include "app/AgentPlanExecutor.h"
#include "app/AgentPlanParser.h"
#include "app/AgentPlanPromptBuilder.h"
#include "app/ConfigCoordinator.h"
#include "app/ContextWindowManager.h"
#include "app/SessionCoordinator.h"
#include "app/TokenEstimator.h"
#include "core/AppConfig.h"
#include "hooks/BuiltinHooks.h"
#include "hooks/HookDefinition.h"
#include "hooks/HookManager.h"
#include "memory/ProjectMemoryManager.h"
#include "mcp/McpRegistry.h"
#include "plugins/PluginManager.h"
#include "skills/SkillDefinition.h"
#include "skills/SkillManager.h"
#include "services/AIClient.h"
#include "services/OpenAICompatibleClient.h"
#include "services/PythonSidecarAIClient.h"
#include "support/AppLogger.h"
#include "tools/AgentToolCatalog.h"
#include "tools/AgentToolRegistry.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include <memory>

namespace {

// V17.3: Token 估算统一走 TokenEstimator，避免多处算法漂移。
int estimateTokenCount(const QString &text)
{
    return static_cast<int>(TokenEstimator::estimateTokens(text));
}

bool containsPythonSidecarPackage(const QString &directoryPath)
{
    const QString trimmedPath = directoryPath.trimmed();
    if (trimmedPath.isEmpty()) {
        return false;
    }

    return QDir(QDir::cleanPath(trimmedPath)).exists(QStringLiteral("agent_sidecar"));
}

QString resolvePythonSidecarDirectoryForSubAgent(const QString &configuredDirectory)
{
    const QString configured = configuredDirectory.trimmed();
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString currentDir = QDir::currentPath();

    QStringList candidates;
    if (!configured.isEmpty()) {
        candidates.append(configured);
    }
    candidates.append({
        QDir(appDir).filePath(QStringLiteral("python/agent_sidecar")),
        QDir(appDir).filePath(QStringLiteral("../python/agent_sidecar")),
        QDir(currentDir).filePath(QStringLiteral("python/agent_sidecar")),
        QDir(currentDir).filePath(QStringLiteral("../python/agent_sidecar")),
        AppConfig::defaultPythonSidecarDirectory()
    });

    for (const QString &candidate : candidates) {
        if (containsPythonSidecarPackage(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }

    return QDir::cleanPath(candidates.isEmpty() ? AppConfig::defaultPythonSidecarDirectory() : candidates.first());
}

std::unique_ptr<AIClient> createIsolatedSubAgentClient(const AppConfig &config)
{
    if (config.backendType != AIBackendType::Sidecar) {
        return std::make_unique<OpenAICompatibleClient>();
    }

    auto sidecar = std::make_unique<PythonSidecarAIClient>();
    const QString pythonExecutable = config.pythonExecutable.trimmed().isEmpty()
                                         ? AppConfig::defaultPythonExecutable()
                                         : config.pythonExecutable.trimmed();
    const QString sidecarDirectory = resolvePythonSidecarDirectoryForSubAgent(config.pythonSidecarDirectory);
    if (sidecar->startSidecar(pythonExecutable, sidecarDirectory, 5000)) {
        return sidecar;
    }

    AppLogger::warning(QStringLiteral("SubAgent"),
                       QStringLiteral("Python sidecar failed for sub-agent. executable=%1 dir=%2 error=%3")
                           .arg(pythonExecutable, sidecarDirectory, sidecar->lastError()));
    return std::make_unique<OpenAICompatibleClient>();
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

    // N4: 初始化插件管理器
    m_pluginManager = std::make_unique<PluginManager>(this);
    const QString appPluginsDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("plugins"));
    m_pluginManager->scanAndLoadPlugins(appPluginsDir);
    if (!m_configCoordinator->config().agentProjectDirectory.isEmpty()) {
        const QString pluginsDir = QDir(m_configCoordinator->config().agentProjectDirectory)
                                       .filePath(QStringLiteral("plugins"));
        m_pluginManager->scanAndLoadPlugins(pluginsDir);
    }

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
    m_actionFingerprints.clear();

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

AgentToolRegistry AgentOrchestrator::toolRegistry() const
{
    AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();

    // V17.6 P1-3: 注册轻量子代理探索工具
    if (m_aiClient != nullptr) {
        auto defs = registry.definitions();
        defs.append(createSubAgentTool());
        registry = AgentToolRegistry(defs);
    }

    if (!m_mcpRegistry) {
        return registry;
    }

    for (McpConnector *connector : m_mcpRegistry->connectors()) {
        if (connector == nullptr || !connector->isConnected()) {
            continue;
        }
        registry.registerExternalTools(connector->listTools(), connector);
    }

    // N4: 注册插件工具
    if (m_pluginManager) {
        const QVector<PluginToolInfo> pluginTools = m_pluginManager->allPluginTools();
        for (const auto &entry : m_pluginManager->allPlugins()) {
            if (entry.enabled && entry.loaded && entry.instance) {
                registry.registerPluginTools(entry.tools, entry.instance);
            }
        }
    }

    return registry;
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
    const AgentToolRegistry registry = toolRegistry();

    const QString loopPrompt = AgentLoopPromptBuilder::buildNextActionPrompt(
        m_agentLoopGoal,
        m_agentLoopObservations,
        registry.descriptors(),
        m_configCoordinator->config().language,
        m_agentLoopIteration,
        kMaxAgentLoopIterations,
        m_matchedSkills);

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

QStringList &AgentOrchestrator::loopObservationsRef()
{
    return m_agentLoopObservations;
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
    const AgentToolRegistry registry = toolRegistry();
    AgentToolExecutionContext context;
    context.workspaceDirectory = workspace;
    context.projectDirectory = workspace;
    context.sidecarClient = activeSidecarClient();
    QString execSummary;
    int successCount = 0;
    int failCount = 0;
    const int displayIteration = m_agentLoopIteration + 1;

    execSummary += QStringLiteral("\n\n---\n");
    execSummary += m_configCoordinator->text(
        QStringLiteral("**Executing plan (%1 steps)...**\n").arg(plan.steps.size()),
        QStringLiteral("**正在执行计划 (%1 步)...**\n").arg(plan.steps.size()));

    // V19 #25: 使用并行执行替换顺序执行
    const QVector<StepResult> stepResults = executePlanStepsParallel(plan, registry, context);

    for (int i = 0; i < plan.steps.size(); ++i) {
        const AgentPlanStep &step = plan.steps[i];

        // 查找对应的执行结果
        StepResult stepResult;
        stepResult.ok = false;
        stepResult.output = QStringLiteral("Step result not found");
        stepResult.stepIndex = i;
        for (const auto &sr : stepResults) {
            if (sr.stepIndex == i) {
                stepResult = sr;
                break;
            }
        }
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
            stepResult.output.left(500),
            stepResult.ok ? QStringLiteral("success") : QStringLiteral("error")));

        const QString status = stepResult.ok
            ? QStringLiteral("\u2705")
            : QStringLiteral("\u274C");

        const QString desc = step.title.isEmpty()
            ? step.toolId
            : step.title;

        QString resultPreview = stepResult.output;
        if (resultPreview.length() > 300) {
            resultPreview = resultPreview.left(300) + QStringLiteral("...");
        }

        execSummary += QStringLiteral("%1 **[%2/%3] %4**\n> %5\n")
            .arg(status)
            .arg(i + 1)
            .arg(plan.steps.size())
            .arg(desc, resultPreview);

        if (stepResult.ok) {
            ++successCount;
        } else {
            ++failCount;
            AppLogger::warning(QStringLiteral("AgentPlan"),
                               QStringLiteral("Step failed. step=%1 toolId=%2 error=%3")
                                   .arg(step.id, step.toolId, stepResult.output));
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

        // V18.6: 记录成功的工具序列 → 供下次类似任务参考
        if (successCount > 0) {
            QStringList toolIds;
            for (const auto &step : plan.steps)
                toolIds.append(step.toolId);
            AgentLoopPromptBuilder::recordToolSequence(
                m_configCoordinator->config().agentProjectDirectory,
                m_agentLoopGoal.left(80),
                toolIds);
        }
    }

    // V18.5: 自动修复闭环 — 编辑文件后自动运行构建+测试
    if (m_autoFixEnabled && successCount > 0) {
        bool didEdit = false;
        for (const auto &step : plan.steps) {
            if (step.toolId == QStringLiteral("file.edit_text")
                || step.toolId == QStringLiteral("file.save_text")
                || step.toolId == QStringLiteral("workspace.write_text")
                || step.toolId == QStringLiteral("workspace.overwrite_text")) {
                didEdit = true; break;
            }
        }
        if (didEdit && !m_configCoordinator->config().agentProjectDirectory.isEmpty()) {
            const QString projectDir = m_configCoordinator->config().agentProjectDirectory;
            QString autoFixReport;

            // 自动构建
            QProcess buildProc;
            buildProc.setWorkingDirectory(projectDir);
            buildProc.setProgram(QStringLiteral("cmake"));
            buildProc.setArguments({QStringLiteral("--build"), QStringLiteral("build")});
            buildProc.start();
            if (buildProc.waitForFinished(60000)) {
                QString buildOut = QString::fromLocal8Bit(buildProc.readAll());
                if (buildProc.exitCode() == 0) {
                    autoFixReport += QStringLiteral("[AutoFix] Build passed.\n");
                } else {
                    autoFixReport += QStringLiteral("[AutoFix] Build FAILED:\n%1\n").arg(buildOut.left(2000));
                }
            } else {
                autoFixReport += QStringLiteral("[AutoFix] Build timed out.\n");
            }

            // 自动测试
            QProcess testProc;
            testProc.setWorkingDirectory(projectDir);
            testProc.setProgram(QStringLiteral("ctest"));
            testProc.setArguments({QStringLiteral("--test-dir"), QStringLiteral("build"), QStringLiteral("--output-on-failure")});
            testProc.start();
            if (testProc.waitForFinished(60000)) {
                QString testOut = QString::fromLocal8Bit(testProc.readAll());
                if (testProc.exitCode() == 0) {
                    autoFixReport += QStringLiteral("[AutoFix] All tests passed.");
                } else {
                    autoFixReport += QStringLiteral("[AutoFix] Tests FAILED:\n%1").arg(testOut.left(2000));
                }
            } else {
                autoFixReport += QStringLiteral("[AutoFix] Tests timed out.");
            }

            appendLoopObservation(autoFixReport);
            AppLogger::info(QStringLiteral("AgentOrchestrator"), QStringLiteral("AutoFix check completed."));
        }
    }

    return (failCount == 0);
}

// V19 #25: 并行执行计划步骤
// 将无依赖的"安全并行"工具分组执行，减少整体执行时间
QVector<AgentOrchestrator::StepResult> AgentOrchestrator::executePlanStepsParallel(
    const AgentPlan &plan,
    const AgentToolRegistry &registry,
    const AgentToolExecutionContext &context)
{
    QVector<StepResult> results;
    results.reserve(plan.steps.size());

    // 安全并行工具集合（只读/轻量，无副作用）
    const QSet<QString> safeParallelTools = {
        QStringLiteral("file.read_text"),
        QStringLiteral("file.grep"),
        QStringLiteral("file.get_info"),
        QStringLiteral("file.list_directory"),
        QStringLiteral("system.screen_size"),
        QStringLiteral("input.mouse_position"),
        QStringLiteral("project.find_files"),
        QStringLiteral("command.git_status"),
    };

    // 分批执行：累积安全工具到 batch，遇到非安全工具先 flush batch
    auto flushBatch = [&](QVector<int> &batch) {
        if (batch.isEmpty()) return;
        if (batch.size() == 1) {
            // 单步执行，不走并行
            const auto &step = plan.steps[batch[0]];
            emit agentLoopThought(m_agentLoopIteration + 1, step.reason, step.toolId, step.title);
            ToolResult r = registry.execute(step.toolId, step.parameters, context, m_hookManager.get());
            results.append({batch[0], step.toolId, r.ok, r.ok ? r.output : r.error});
            return;
        }

        // 并行执行 batch
        QVector<QFuture<ToolResult>> futures;

        for (int idx : batch) {
            const AgentPlanStep step = plan.steps[idx];
            emit agentLoopThought(m_agentLoopIteration + 1, step.reason, step.toolId, step.title);

            const AgentToolExecutionContext contextCopy = context;
            futures.append(QtConcurrent::run([&registry, step, contextCopy]() -> ToolResult {
                return registry.execute(step.toolId, step.parameters, contextCopy, nullptr);
            }));
        }

        for (int fi = 0; fi < futures.size(); ++fi) {
            ToolResult r = futures[fi].result();
            const int idx = batch[fi];
            const auto &step = plan.steps[idx];
            results.append({idx, step.toolId, r.ok, r.ok ? r.output : r.error});
        }
    };

    QVector<int> currentBatch;
    for (int i = 0; i < plan.steps.size(); ++i) {
        const auto &step = plan.steps[i];
        if (safeParallelTools.contains(step.toolId)) {
            currentBatch.append(i);
        } else {
            flushBatch(currentBatch);
            currentBatch.clear();
            // 非安全工具单独执行
            QVector<int> single = {i};
            flushBatch(single);
        }
    }
    // flush 最后一组
    flushBatch(currentBatch);

    // 按原始顺序排序结果
    std::sort(results.begin(), results.end(),
              [](const StepResult &a, const StepResult &b) {
                  return a.stepIndex < b.stepIndex;
              });

    return results;
}

PythonSidecarClient *AgentOrchestrator::activeSidecarClient() const
{
    auto *sidecar = qobject_cast<PythonSidecarAIClient *>(m_aiClient);
    return sidecar == nullptr ? nullptr : sidecar->sidecarClient();
}

// V17.6 P1-3: 轻量子代理 — agent.explore 工具，使用只读工具异步探索
AgentToolDefinition AgentOrchestrator::createSubAgentTool() const
{
    const bool isChinese = (m_configCoordinator->config().language == AppLanguage::Chinese);

    AgentToolDescriptor desc;
    desc.id = QStringLiteral("agent.explore");
    desc.englishName = QStringLiteral("Explore Project");
    desc.chineseName = QStringLiteral("项目探索");
    desc.englishDescription = QStringLiteral(
        "Research a question by reading/searching project files. "
        "Use this to understand code structure, find definitions, or locate files "
        "without modifying anything. Returns findings as text.");
    desc.chineseDescription = QStringLiteral(
        "通过读取/搜索项目文件来研究问题。"
        "用于理解代码结构、查找定义或定位文件，不修改任何内容。返回文本格式的发现。");
    desc.risk = AgentToolRisk::Low;
    desc.enabledForAgent = true;
    desc.resultMayContainSensitiveContent = false;
    desc.inputPolicy = QStringLiteral("{\"type\":\"object\",\"properties\":{\"question\":{\"type\":\"string\",\"description\":\"What to search for\"}},\"required\":[\"question\"]}");

    QJsonObject paramSchema;
    paramSchema[QStringLiteral("type")] = QStringLiteral("object");
    QJsonObject props;
    props[QStringLiteral("question")] = QJsonObject{
        {QStringLiteral("type"), QStringLiteral("string")},
        {QStringLiteral("description"), isChinese
            ? QStringLiteral("要在项目中搜索研究的问题")
            : QStringLiteral("The question to research in the project")}
    };
    paramSchema[QStringLiteral("properties")] = props;
    QJsonArray required;
    required.append(QStringLiteral("question"));
    paramSchema[QStringLiteral("required")] = required;

    AppConfig subConfig = m_configCoordinator->config();

    return AgentToolDefinition{
        desc,
        paramSchema,
        AgentToolRegistryFactory::functionNameForToolId(QStringLiteral("agent.explore")),
        true,
        [subConfig, isChinese](const QJsonObject &params, const AgentToolExecutionContext &) -> ToolResult {
            const QString question = params.value(QStringLiteral("question")).toString().trimmed();
            if (question.isEmpty()) {
                return {false, QString(), QStringLiteral("question is required")};
            }

            if (!subConfig.isComplete()) {
                return {false, QString(), QStringLiteral("AI configuration is incomplete")};
            }

            // 只给只读工具：file.read_text, file.list_directory, project.find_files
            QVector<AgentToolDefinition> readOnlyTools;
            const auto allDefs = AgentToolRegistryFactory::defaultRegistry().definitions();
            for (const auto &def : allDefs) {
                if (def.descriptor.id == QStringLiteral("file.read_text")
                    || def.descriptor.id == QStringLiteral("file.list_directory")
                    || def.descriptor.id == QStringLiteral("project.find_files")
                    || def.descriptor.id == QStringLiteral("command.git_status")) {
                    readOnlyTools.append(def);
                }
            }

            const AgentToolRegistry subRegistry(readOnlyTools);
            const QString prompt = isChinese
                ? QStringLiteral("研究以下问题（只读，不要修改任何文件）：\n%1\n\n"
                                 "输出你的发现。要简短，只包含关键信息。")
                      .arg(question)
                : QStringLiteral("Research this question (read-only, do NOT modify any files):\n%1\n\n"
                                 "Output your findings. Be brief, only include key information.")
                      .arg(question);

            ChatSession session = ChatSession::createDefault();
            session.addMessage(MessageRole::User, prompt);

            QEventLoop loop;
            QString resultText;
            bool finished = false;
            QString errorMsg;
            std::unique_ptr<AIClient> subClient = createIsolatedSubAgentClient(subConfig);

            QMetaObject::Connection c1 = QObject::connect(
                subClient.get(), &AIClient::textDeltaReceived,
                [&resultText](const QString &delta) { resultText += delta; });

            QMetaObject::Connection c2 = QObject::connect(
                subClient.get(), &AIClient::requestFinished,
                [&loop, &finished]() { finished = true; loop.quit(); });

            QMetaObject::Connection c3 = QObject::connect(
                subClient.get(), &AIClient::requestFailed,
                [&loop, &errorMsg](const QString &msg, RequestErrorCategory) {
                    errorMsg = msg;
                    loop.quit();
                });

            QTimer timeoutTimer;
            timeoutTimer.setSingleShot(true);
            QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeoutTimer.start(30000); // 30s sub-agent timeout

            subClient->sendChat(subConfig, session);

            loop.exec();

            QObject::disconnect(c1);
            QObject::disconnect(c2);
            QObject::disconnect(c3);

            if (!errorMsg.isEmpty()) {
                return {false, QString(), QStringLiteral("Sub-agent failed: %1").arg(errorMsg)};
            }
            if (!finished) {
                return {false, QString(), QStringLiteral("Sub-agent timed out")};
            }

            return {true, resultText.trimmed(), QString()};
        }};
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
