#include "app/ApplicationController.h"

#include "app/AgentCommandSkillFileService.h"
#include "app/AgentLoopPromptBuilder.h"
#include "app/AgentPlanExecutor.h"
#include "app/AgentPlanParser.h"
#include "app/AgentPlanPromptBuilder.h"
#include "app/AgentToolCallPlanBuilder.h"
#include "app/ProjectInstructionService.h"
#include "app/TokenEstimator.h"
#include "hooks/HookDefinition.h"
#include "mcp/McpRegistry.h"
#include "scheduler/ScheduledTask.h"
#include "scheduler/TaskScheduler.h"
#include "scheduler/TaskStorage.h"
#include "skills/SkillDefinition.h"

#include "support/AppLogger.h"
#include "tools/AgentToolCatalog.h"
#include "tools/AgentToolRegistry.h"
#include "tools/ProjectMemoryService.h"
#include "memory/ProjectMemoryManager.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QStringList>

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

QString commandSkillSectionForProject(const QString &projectDirectory, AppLanguage language)
{
    QVector<AgentCommandSkill> skills = AgentCommandSkillCatalog::defaultSkills();
    const AgentCommandSkillLoadResult loadResult = AgentCommandSkillFileService::loadFromProjectDirectory(
        projectDirectory,
        AgentToolRegistryFactory::defaultRegistry());

    for (const QString &error : loadResult.errors) {
        AppLogger::warning(QStringLiteral("AgentSkill"),
                           QStringLiteral("External skill load warning. %1").arg(error));
    }

    for (const AgentCommandSkill &externalSkill : loadResult.skills) {
        if (AgentCommandSkillCatalog::findSkill(skills, externalSkill.id) != nullptr) {
            AppLogger::warning(QStringLiteral("AgentSkill"),
                               QStringLiteral("External skill ignored because id already exists. skillId=%1").arg(externalSkill.id));
            continue;
        }
        skills.append(externalSkill);
    }

    return AgentCommandSkillCatalog::promptSection(skills, language);
}

} // namespace

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
{
    // 信号转发：ConfigCoordinator → ApplicationController
    connect(&m_configCoordinator, &ConfigCoordinator::configChanged,
            this, &ApplicationController::configChanged);
    connect(&m_configCoordinator, &ConfigCoordinator::promptTemplatesChanged,
            this, &ApplicationController::promptTemplatesChanged);
    connect(&m_configCoordinator, &ConfigCoordinator::startupWarning,
            this, &ApplicationController::startupWarning);

    // 信号转发：SessionCoordinator → ApplicationController
    connect(&m_sessionCoordinator, &SessionCoordinator::sessionListChanged,
            this, &ApplicationController::sessionListChanged);
    connect(&m_sessionCoordinator, &SessionCoordinator::currentSessionChanged,
            this, &ApplicationController::currentSessionChanged);
    connect(&m_sessionCoordinator, &SessionCoordinator::sessionListFilterChanged,
            this, &ApplicationController::sessionListFilterChanged);
    connect(&m_sessionCoordinator, &SessionCoordinator::currentChatCleared,
            this, &ApplicationController::currentChatCleared);
    connect(&m_sessionCoordinator, &SessionCoordinator::statusMessage,
            this, &ApplicationController::statusMessage);
    connect(&m_sessionCoordinator, &SessionCoordinator::startupWarning,
            this, &ApplicationController::startupWarning);

    // 信号转发：AgentOrchestrator → ApplicationController
    connect(&m_agentOrchestrator, &AgentOrchestrator::agentLoopIterationUpdated,
            this, &ApplicationController::agentLoopIterationUpdated);
    connect(&m_agentOrchestrator, &AgentOrchestrator::agentLoopSkillSummary,
            this, &ApplicationController::agentLoopSkillSummary);
    connect(&m_agentOrchestrator, &AgentOrchestrator::agentLoopThought,
            this, &ApplicationController::agentLoopThought);
    connect(&m_agentOrchestrator, &AgentOrchestrator::agentLoopToolFinished,
            this, &ApplicationController::agentLoopToolFinished);
    connect(&m_agentOrchestrator, &AgentOrchestrator::agentLoopPromptDebug,
            this, &ApplicationController::agentLoopPromptDebug);
    connect(&m_agentOrchestrator, &AgentOrchestrator::tokenUsageUpdated,
            this, &ApplicationController::tokenUsageUpdated);
    connect(&m_agentOrchestrator, &AgentOrchestrator::agentLoopCompleted,
            this, &ApplicationController::agentLoopCompleted);
    connect(&m_agentOrchestrator, &AgentOrchestrator::assistantMessageUpdated,
            this, &ApplicationController::assistantMessageUpdated);
    connect(&m_agentOrchestrator, &AgentOrchestrator::statusMessage,
            this, &ApplicationController::statusMessage);

    // AI 客户端信号
    connect(&m_aiClient, &OpenAICompatibleClient::textDeltaReceived, this, &ApplicationController::handleTextDelta);
    connect(&m_aiClient, &OpenAICompatibleClient::toolCallsReceived, this, &ApplicationController::handleToolCallsReceived);
    connect(&m_aiClient, &OpenAICompatibleClient::toolUseBlockComplete, this, &ApplicationController::handleToolUseBlockComplete);
    connect(&m_aiClient, &OpenAICompatibleClient::requestFinished, this, &ApplicationController::handleRequestFinished);
    connect(&m_aiClient, &OpenAICompatibleClient::requestFailed, this, &ApplicationController::handleRequestFailed);
}

ApplicationController::~ApplicationController()
{
    if (m_taskStorage && m_taskScheduler) {
        m_taskStorage->save(m_taskScheduler->allTasks());
    }
}

void ApplicationController::initialize()
{
    // 配置加载
    m_configCoordinator.initialize();

    // 会话初始化
    m_sessionCoordinator.initialize();

    // Agent 编排器初始化（上下文窗口、技能/钩子、MCP、残留状态检测）
    m_agentOrchestrator.initialize(&m_aiClient, &m_configCoordinator, &m_sessionCoordinator);

    // V15.1: 初始化调度器
    m_taskScheduler = std::make_unique<TaskScheduler>(this);
    const QString taskFilePath = QDir::cleanPath(
        m_configCoordinator.config().agentProjectDirectory + QStringLiteral("/.workbuddy/scheduled_tasks.json"));
    m_taskStorage = new TaskStorage(taskFilePath);

    auto tasks = m_taskStorage->load();
    for (auto &t : tasks)
        m_taskScheduler->addTask(t);

    connect(m_taskScheduler.get(), &TaskScheduler::taskTriggered,
            this, &ApplicationController::onScheduledTaskTriggered);

    m_taskScheduler->start();
}

const AppConfig &ApplicationController::config() const
{
    return m_configCoordinator.config();
}

const ChatSession &ApplicationController::currentSession() const
{
    return m_sessionCoordinator.currentSession();
}

const QVector<ChatSession> &ApplicationController::sessionSummaries() const
{
    return m_sessionCoordinator.sessionSummaries();
}

SessionListFilter ApplicationController::sessionListFilter() const
{
    return m_sessionCoordinator.sessionListFilter();
}

const QVector<PromptTemplate> &ApplicationController::promptTemplates() const
{
    return m_configCoordinator.promptTemplates();
}

bool ApplicationController::isGenerating() const
{
    return m_isGenerating;
}

bool ApplicationController::exportCurrentSessionMarkdown(const QString &filePath, QString *error) const
{
    return m_sessionCoordinator.exportCurrentSessionMarkdown(filePath, error);
}

void ApplicationController::saveConfig(const AppConfig &config)
{
    m_configCoordinator.saveConfig(config);
}

void ApplicationController::savePromptTemplates(const QVector<PromptTemplate> &templates)
{
    m_configCoordinator.savePromptTemplates(templates);
}

void ApplicationController::setSystemPrompt(const QString &prompt)
{
    if (m_isGenerating) {
        return;
    }
    m_sessionCoordinator.setSystemPrompt(prompt);
}

void ApplicationController::renameCurrentSession(const QString &title)
{
    if (m_isGenerating) {
        return;
    }
    m_sessionCoordinator.renameCurrentSession(title);
}

void ApplicationController::searchSessions(const QString &query)
{
    m_sessionCoordinator.searchSessions(query);
}

void ApplicationController::setSessionListFilter(SessionListFilter filter)
{
    m_sessionCoordinator.setSessionListFilter(filter);
}

void ApplicationController::toggleCurrentSessionFavorite()
{
    if (m_isGenerating) {
        return;
    }
    m_sessionCoordinator.toggleCurrentSessionFavorite();
}

void ApplicationController::toggleCurrentSessionArchived()
{
    if (m_isGenerating) {
        return;
    }
    m_sessionCoordinator.toggleCurrentSessionArchived();
}

void ApplicationController::startNewChat()
{
    if (m_isGenerating) {
        return;
    }

    setRetryAvailable(false);
    m_sessionCoordinator.createNewChat();
    m_currentAssistantContent.clear();
}

void ApplicationController::switchToSession(const QString &sessionId)
{
    if (m_isGenerating || sessionId.isEmpty()) {
        return;
    }

    setRetryAvailable(false);
    if (m_sessionCoordinator.switchToSession(sessionId)) {
        m_currentAssistantContent.clear();
    }
}

void ApplicationController::deleteCurrentSession()
{
    if (m_isGenerating) {
        return;
    }

    setRetryAvailable(false);
    m_sessionCoordinator.deleteCurrentSession();
    m_currentAssistantContent.clear();
}

void ApplicationController::sendMessage(const QString &content)
{
    if (m_isGenerating) {
        return;
    }

    const QString trimmedContent = content.trimmed();
    if (trimmedContent.isEmpty()) {
        return;
    }

    setRetryAvailable(false);
    if (!m_configCoordinator.config().isComplete()) {
        emit configurationMissing();
        return;
    }

    if (m_sessionCoordinator.currentSession().messages.isEmpty()) {
        m_sessionCoordinator.currentSession().title = trimmedContent.left(36);
        emit currentChatCleared();
        m_sessionCoordinator.upsertCurrentSessionSummary(true);
        emit sessionListChanged();
    }

    m_sessionCoordinator.currentSession().addMessage(MessageRole::User, trimmedContent);
    emit userMessageAdded(trimmedContent);

    startAssistantRequest(trimmedContent);
}

// V17.1: 发送带图片附件的用户消息
void ApplicationController::sendMessageWithImages(const QString &content, const QStringList &imageBase64List)
{
    if (m_isGenerating) {
        return;
    }

    const QString trimmedContent = content.trimmed();
    if (trimmedContent.isEmpty() && imageBase64List.isEmpty()) {
        return;
    }

    setRetryAvailable(false);
    if (!m_configCoordinator.config().isComplete()) {
        emit configurationMissing();
        return;
    }

    if (m_sessionCoordinator.currentSession().messages.isEmpty()) {
        m_sessionCoordinator.currentSession().title = trimmedContent.left(36);
        emit currentChatCleared();
        m_sessionCoordinator.upsertCurrentSessionSummary(true);
        emit sessionListChanged();
    }

    m_sessionCoordinator.currentSession().addMessage(MessageRole::User, trimmedContent);
    emit userMessageAdded(trimmedContent);

    m_pendingImages = QJsonArray();
    for (const QString &img : imageBase64List) {
        m_pendingImages.append(img);
    }

    // V12.1 上下文窗口管理
    ContextWindowResult result = m_agentOrchestrator.contextWindowManager()->processMessages(
        m_sessionCoordinator.currentSession().messages, m_sessionCoordinator.currentSession().systemPrompt);

    if (result.wasTrimmed) {
        const QString hint = ContextWindowManager::buildCompressionHint(
            result.trimmedRoundCount, result.summary);
        m_sessionCoordinator.currentSession().systemPrompt = hint + QStringLiteral("\n") + m_sessionCoordinator.currentSession().systemPrompt;
        m_sessionCoordinator.currentSession().messages = result.processedMessages;
        emit statusMessage(
            QStringLiteral("Compressed %1 rounds into summary").arg(result.trimmedRoundCount),
            QStringLiteral("已压缩 %1 轮历史对话为摘要").arg(result.trimmedRoundCount),
            3000);
    }

    m_sessionCoordinator.currentSession().addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    emit assistantMessageStarted();

    setGenerating(true);
    m_activeRequestKind = ActiveRequestKind::ChatMessage;
    m_lastRequestUserContent = trimmedContent;
    m_aiClient.sendChatWithImages(m_configCoordinator.config(), m_sessionCoordinator.currentSession(), QJsonArray(), m_pendingImages);

    // V17.3: Token 估算更新
    int totalTokens = estimateTokenCount(m_sessionCoordinator.currentSession().systemPrompt);
    for (const auto &msg : m_sessionCoordinator.currentSession().messages) {
        totalTokens += estimateTokenCount(msg.content);
    }
    emit tokenUsageUpdated(totalTokens, static_cast<int>(m_agentOrchestrator.getContextWindowTokens()));
}

void ApplicationController::startAssistantRequest(const QString &userContentForRetry)
{
    m_activeRequestKind = ActiveRequestKind::ChatMessage;
    m_lastRequestUserContent = userContentForRetry;

    // V12.1 上下文窗口管理
    ContextWindowResult result = m_agentOrchestrator.contextWindowManager()->processMessages(
        m_sessionCoordinator.currentSession().messages, m_sessionCoordinator.currentSession().systemPrompt);

    if (result.wasTrimmed) {
        const QString hint = ContextWindowManager::buildCompressionHint(
            result.trimmedRoundCount, result.summary);
        m_sessionCoordinator.currentSession().systemPrompt = hint + QStringLiteral("\n") + m_sessionCoordinator.currentSession().systemPrompt;
        m_sessionCoordinator.currentSession().messages = result.processedMessages;
        emit statusMessage(
            QStringLiteral("Compressed %1 rounds into summary").arg(result.trimmedRoundCount),
            QStringLiteral("已压缩 %1 轮历史对话为摘要").arg(result.trimmedRoundCount),
            3000);
    }

    m_sessionCoordinator.currentSession().addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    emit assistantMessageStarted();

    setGenerating(true);
    m_aiClient.sendChat(m_configCoordinator.config(), m_sessionCoordinator.currentSession());

    // V17.3: Token 估算更新
    int totalTokens = estimateTokenCount(m_sessionCoordinator.currentSession().systemPrompt);
    for (const auto &msg : m_sessionCoordinator.currentSession().messages) {
        totalTokens += estimateTokenCount(msg.content);
    }
    emit tokenUsageUpdated(totalTokens, static_cast<int>(m_agentOrchestrator.getContextWindowTokens()));
}

void ApplicationController::generateAgentPlan(const QString &goal, int continuationDepth)
{
    if (m_isGenerating) {
        return;
    }

    const QString trimmedGoal = goal.trimmed();
    if (trimmedGoal.isEmpty()) {
        emit statusMessage(QStringLiteral("Type a goal before generating an Agent plan."),
                           QStringLiteral("生成 Agent 计划前，请先输入目标。"),
                           3000);
        return;
    }

    if (!m_configCoordinator.config().isComplete()) {
        emit configurationMissing();
        return;
    }

    if (continuationDepth > AgentPlanMaxContinuationDepth) {
        emit statusMessage(QStringLiteral("Agent continuation limit reached."),
                           QStringLiteral("Agent 继续规划次数已达到上限。"),
                           4000);
        return;
    }

    setRetryAvailable(false);
    m_agentPlanResponseBuffer.clear();
    m_agentToolCalls.clear();
    m_pendingAgentPlanContinuationDepth = continuationDepth;

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
    const QVector<AgentToolDescriptor> catalog = registry.descriptors();
    const ProjectInstructions projectInstructions = ProjectInstructionService::loadFromProjectDirectory(m_configCoordinator.config().agentProjectDirectory);
    const QString projectInstructionSection = ProjectInstructionService::promptSection(projectInstructions, m_configCoordinator.config().language);
    const ProjectMemory projectMemory = ProjectMemoryService::loadFromProjectDirectory(m_configCoordinator.config().agentProjectDirectory);
    const QString projectMemorySection = ProjectMemoryService::promptSection(projectMemory, m_configCoordinator.config().language);
    const QString commandSkillSection = commandSkillSectionForProject(m_configCoordinator.config().agentProjectDirectory, m_configCoordinator.config().language);
    const QString planningPrompt = AgentPlanPromptBuilder::buildPlanningPrompt(
        trimmedGoal,
        catalog,
        m_configCoordinator.config().language,
        AgentPlanParser::DefaultMaxPlanSteps,
        projectInstructionSection,
        commandSkillSection,
        projectMemorySection);

    ChatSession planningSession = ChatSession::createDefault();
    planningSession.title = QStringLiteral("Agent Plan");
    planningSession.addMessage(MessageRole::User, planningPrompt);
    m_pendingAgentRequestSession = planningSession;
    m_nativeToolRequestActive = true;
    m_nativeToolFallbackAttempted = false;

    m_activeRequestKind = ActiveRequestKind::AgentPlan;
    AppLogger::info(QStringLiteral("AgentPlan"),
                    QStringLiteral("Agent plan request started. goalLength=%1 tools=%2 maxSteps=%3")
                        .arg(trimmedGoal.size())
                        .arg(catalog.size())
                        .arg(AgentPlanParser::DefaultMaxPlanSteps));

    // V12.1 上下文检查
    ContextWindowResult planResult = m_agentOrchestrator.contextWindowManager()->processMessages(
        planningSession.messages, planningSession.systemPrompt);
    if (planResult.wasTrimmed) {
        planningSession.messages = planResult.processedMessages;
    }

    setGenerating(true);
    m_aiClient.sendChatWithTools(m_configCoordinator.config(), planningSession, registry.functionToolSchemas(m_configCoordinator.config().language));

    emit statusMessage(QStringLiteral("Generating Agent plan..."),
                       QStringLiteral("正在生成 Agent 计划..."),
                       3000);
}

void ApplicationController::sendUnifiedMessage(const QString &content)
{
    if (m_isGenerating) {
        return;
    }

    const QString trimmedContent = content.trimmed();
    if (trimmedContent.isEmpty()) {
        return;
    }

    setRetryAvailable(false);
    if (!m_configCoordinator.config().isComplete()) {
        emit configurationMissing();
        return;
    }

    if (m_sessionCoordinator.currentSession().messages.isEmpty()) {
        m_sessionCoordinator.currentSession().title = trimmedContent.left(36);
        emit currentChatCleared();
        m_sessionCoordinator.upsertCurrentSessionSummary(true);
        emit sessionListChanged();
    }

    m_sessionCoordinator.currentSession().addMessage(MessageRole::User, trimmedContent);
    emit userMessageAdded(trimmedContent);

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
    const QVector<AgentToolDescriptor> catalog = registry.descriptors();
    const ProjectInstructions projectInstructions = ProjectInstructionService::loadFromProjectDirectory(m_configCoordinator.config().agentProjectDirectory);
    const QString projectInstructionSection = ProjectInstructionService::promptSection(projectInstructions, m_configCoordinator.config().language);
    const ProjectMemory projectMemory = ProjectMemoryService::loadFromProjectDirectory(m_configCoordinator.config().agentProjectDirectory);
    const QString projectMemorySection = ProjectMemoryService::promptSection(projectMemory, m_configCoordinator.config().language);
    const QString commandSkillSection = commandSkillSectionForProject(m_configCoordinator.config().agentProjectDirectory, m_configCoordinator.config().language);
    const QString prompt = AgentPlanPromptBuilder::buildUnifiedPrompt(
        trimmedContent,
        catalog,
        m_configCoordinator.config().language,
        AgentPlanParser::DefaultMaxPlanSteps,
        projectInstructionSection,
        commandSkillSection,
        projectMemorySection);

    ChatSession requestSession = ChatSession::createDefault();
    requestSession.title = QStringLiteral("Unified");
    requestSession.addMessage(MessageRole::User, prompt);

    m_activeRequestKind = ActiveRequestKind::UnifiedAgent;
    m_agentPlanResponseBuffer.clear();
    m_agentToolCalls.clear();
    m_pendingAgentRequestSession = requestSession;
    m_nativeToolRequestActive = true;
    m_nativeToolFallbackAttempted = false;

    m_sessionCoordinator.currentSession().addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    emit assistantMessageStarted();

    // V12.1 上下文检查
    ContextWindowResult unifiedResult = m_agentOrchestrator.contextWindowManager()->processMessages(
        requestSession.messages, requestSession.systemPrompt);
    if (unifiedResult.wasTrimmed) {
        requestSession.messages = unifiedResult.processedMessages;
    }

    setGenerating(true);
    m_aiClient.sendChatWithTools(m_configCoordinator.config(), requestSession, registry.functionToolSchemas(m_configCoordinator.config().language));

    AppLogger::info(QStringLiteral("UnifiedAgent"),
                    QStringLiteral("Unified agent request started. messageLength=%1")
                        .arg(trimmedContent.size()));
}

// V12.6: Agent 连续循环执行入口
void ApplicationController::sendAgentLoopMessage(const QString &content)
{
    if (m_isGenerating) {
        cancelCurrentRequest();
        return;
    }

    const QString trimmedContent = content.trimmed();
    if (trimmedContent.isEmpty()) {
        return;
    }

    // V17.5: 检测"继续"命令 — 恢复中断的 Agent 任务
    const bool isContinueCmd =
        trimmedContent.compare(QStringLiteral("继续"), Qt::CaseInsensitive) == 0
        || trimmedContent.compare(QStringLiteral("continue"), Qt::CaseInsensitive) == 0
        || trimmedContent.compare(QStringLiteral("resume"), Qt::CaseInsensitive) == 0;
    if (isContinueCmd && m_agentOrchestrator.hasPendingAgentState()) {
        m_agentOrchestrator.resumeAgentLoop();
        // 继续循环：构建下一步提示词并发送
        const QString loopPrompt = m_agentOrchestrator.buildNextLoopPrompt();
        const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();

        ChatSession loopSession = m_sessionCoordinator.currentSession();
        loopSession.systemPrompt = loopPrompt;

        emit statusMessage(
            QStringLiteral("Agent loop resumed at step %1").arg(m_agentOrchestrator.agentLoopIteration() + 1),
            QStringLiteral("Agent 循环从第 %1 步恢复").arg(m_agentOrchestrator.agentLoopIteration() + 1),
            2000);

        // 添加助手占位
        m_sessionCoordinator.currentSession().addMessage(MessageRole::Assistant, QString());
        m_currentAssistantContent.clear();
        m_agentPlanResponseBuffer.clear();
        m_agentToolCalls.clear();
        m_agentOrchestrator.clearPendingToolResults();
        emit assistantMessageStarted();

        m_activeRequestKind = ActiveRequestKind::UnifiedAgent;
        m_nativeToolRequestActive = true;
        m_nativeToolFallbackAttempted = false;

        ContextWindowResult cwResult = m_agentOrchestrator.contextWindowManager()->processMessages(
            loopSession.messages, loopSession.systemPrompt);
        if (cwResult.wasTrimmed) {
            loopSession.messages = cwResult.processedMessages;
        }

        setGenerating(true);
        m_aiClient.sendChatWithTools(m_configCoordinator.config(), loopSession,
            registry.functionToolSchemas(m_configCoordinator.config().language));
        return;
    }

    if (isContinueCmd && !m_agentOrchestrator.hasPendingAgentState()) {
        emit statusMessage(
            QStringLiteral("No pending Agent task to resume."),
            QStringLiteral("没有可恢复的 Agent 任务。"),
            3000);
        return;
    }

    // 委托给 AgentOrchestrator 初始化循环状态
    m_agentOrchestrator.startAgentLoop(trimmedContent);

    // 第一轮复用 sendUnifiedMessage 的逻辑
    sendUnifiedMessage(content);
}

// V16.3: Agent 调试模式开关
void ApplicationController::setAgentDebugMode(bool enabled)
{
    m_agentOrchestrator.setAgentDebugMode(enabled);
}

bool ApplicationController::agentDebugMode() const
{
    return m_agentOrchestrator.agentDebugMode();
}

// 启动下一轮 Agent 循环 AI 请求
void ApplicationController::continueAgentLoop()
{
    const QString loopPrompt = m_agentOrchestrator.buildNextLoopPrompt();

    ChatSession loopSession = m_sessionCoordinator.currentSession();
    loopSession.systemPrompt = loopPrompt;

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();

    emit statusMessage(
        QStringLiteral("Agent loop iteration %1/%2").arg(m_agentOrchestrator.agentLoopIteration()).arg(m_agentOrchestrator.maxAgentLoopIterations()),
        QStringLiteral("Agent 循环 %1/%2").arg(m_agentOrchestrator.agentLoopIteration()).arg(m_agentOrchestrator.maxAgentLoopIterations()),
        2000);

    // 添加助手占位
    m_sessionCoordinator.currentSession().addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    m_agentPlanResponseBuffer.clear();
    m_agentToolCalls.clear();
    m_agentOrchestrator.clearPendingToolResults();
    emit assistantMessageStarted();

    // V12.1 上下文检查
    ContextWindowResult cwResult = m_agentOrchestrator.contextWindowManager()->processMessages(
        loopSession.messages, loopSession.systemPrompt);
    if (cwResult.wasTrimmed) {
        loopSession.messages = cwResult.processedMessages;
    }

    // 发送请求
    m_activeRequestKind = ActiveRequestKind::UnifiedAgent;
    m_nativeToolRequestActive = true;
    m_nativeToolFallbackAttempted = false;

    setGenerating(true);
    m_aiClient.sendChatWithTools(m_configCoordinator.config(), loopSession,
        registry.functionToolSchemas(m_configCoordinator.config().language));
}

void ApplicationController::cancelCurrentRequest()
{
    if (!m_isGenerating) {
        return;
    }

    const ActiveRequestKind requestKind = m_activeRequestKind;
    m_aiClient.cancel();
    setGenerating(false);
    m_agentOrchestrator.clearPendingToolResults();

    // 取消 Agent 循环
    m_agentOrchestrator.cancelAgentLoop();

    if (requestKind == ActiveRequestKind::AgentPlan || requestKind == ActiveRequestKind::UnifiedAgent) {
        if (!m_sessionCoordinator.currentSession().messages.isEmpty() && m_sessionCoordinator.currentSession().messages.last().content.isEmpty()) {
            m_sessionCoordinator.currentSession().messages.last().content = m_configCoordinator.text(QStringLiteral("(Stopped)"), QStringLiteral("（已停止）"));
            emit assistantMessageUpdated(m_sessionCoordinator.currentSession().messages.last().content);
        }
        m_agentPlanResponseBuffer.clear();
        m_agentToolCalls.clear();
        m_pendingAgentRequestSession = ChatSession();
        m_nativeToolRequestActive = false;
        m_nativeToolFallbackAttempted = false;
        m_pendingAgentPlanContinuationDepth = 0;
        m_activeRequestKind = ActiveRequestKind::None;
        m_sessionCoordinator.saveCurrentSession();
        emit statusMessage(QStringLiteral("Agent generation stopped."),
                           QStringLiteral("Agent 生成已停止。"),
                           2500);
        return;
    }

    if (!m_sessionCoordinator.currentSession().messages.isEmpty() && m_sessionCoordinator.currentSession().messages.last().content.isEmpty()) {
        m_sessionCoordinator.currentSession().messages.last().content = m_configCoordinator.text(QStringLiteral("(Stopped)"), QStringLiteral("（已停止）"));
        emit assistantMessageUpdated(m_sessionCoordinator.currentSession().messages.last().content);
    }
    setRetryAvailable(false);
    m_sessionCoordinator.saveCurrentSession();
    m_activeRequestKind = ActiveRequestKind::None;
    emit statusMessage(QStringLiteral("Generation stopped."),
                       QStringLiteral("已停止生成。"),
                       2500);
}

void ApplicationController::retryLastRequest()
{
    if (m_isGenerating || !m_retryAvailable || m_retryUserContent.trimmed().isEmpty()) {
        return;
    }

    if (!m_configCoordinator.config().isComplete()) {
        emit configurationMissing();
        return;
    }

    const QString retryContent = m_retryUserContent;
    setRetryAvailable(false);

    if (!m_sessionCoordinator.currentSession().messages.isEmpty() && m_sessionCoordinator.currentSession().messages.last().role == MessageRole::Assistant) {
        m_sessionCoordinator.currentSession().messages.removeLast();
    }

    m_currentAssistantContent.clear();
    emit currentSessionChanged();
    startAssistantRequest(retryContent);
}

void ApplicationController::handleTextDelta(const QString &delta)
{
    if (m_activeRequestKind == ActiveRequestKind::AgentPlan ||
        m_activeRequestKind == ActiveRequestKind::UnifiedAgent) {
        m_agentPlanResponseBuffer += delta;
        return;
    }

    m_currentAssistantContent += delta;
    if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
        m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
    }

    emit assistantMessageUpdated(m_currentAssistantContent);

    // V17.3: 实时更新 token 估算
    int totalTokens = estimateTokenCount(m_sessionCoordinator.currentSession().systemPrompt);
    for (const auto &msg : m_sessionCoordinator.currentSession().messages) {
        totalTokens += estimateTokenCount(msg.content);
    }
    emit tokenUsageUpdated(totalTokens, static_cast<int>(m_agentOrchestrator.getContextWindowTokens()));
}

void ApplicationController::handleToolCallsReceived(const ToolCallList &toolCalls)
{
    if (m_activeRequestKind != ActiveRequestKind::AgentPlan &&
        m_activeRequestKind != ActiveRequestKind::UnifiedAgent) {
        return;
    }

    m_agentToolCalls += toolCalls;
    AppLogger::info(QStringLiteral("AgentPlan"),
                    QStringLiteral("Native tool calls received. count=%1")
                        .arg(toolCalls.size()));
}

void ApplicationController::handleToolUseBlockComplete(
    const QString &toolName, const QJsonObject &arguments)
{
    if (m_activeRequestKind != ActiveRequestKind::AgentPlan &&
        m_activeRequestKind != ActiveRequestKind::UnifiedAgent) {
        return;
    }

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
    const AgentToolExecutionContext context{
        m_configCoordinator.config().agentProjectDirectory,
        m_configCoordinator.config().agentProjectDirectory};

    const AgentToolDefinition *def = registry.findByFunctionName(toolName);
    QString result;
    bool success = false;
    if (def != nullptr) {
        const ToolResult toolResult = registry.execute(
            def->descriptor.id, arguments, context);
        result = toolResult.output;
        success = toolResult.ok;
    } else {
        result = QStringLiteral("Tool not found: %1").arg(toolName);
        success = false;
    }

    m_agentOrchestrator.addPendingToolResult(toolName, arguments, result, success);

    AppLogger::info(QStringLiteral("StreamingTool"),
                    QStringLiteral("Tool executed during streaming: %1 (ok=%2)")
                        .arg(toolName)
                        .arg(success ? QStringLiteral("true") : QStringLiteral("false")));
}

void ApplicationController::handleRequestFinished()
{
    if (m_activeRequestKind == ActiveRequestKind::AgentPlan) {
        setGenerating(false);
        const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
        const AgentPlanParseResult parseResult = !m_agentToolCalls.isEmpty()
            ? AgentToolCallPlanBuilder::buildPlanFromToolCalls(
                  m_agentToolCalls,
                  registry,
                  m_configCoordinator.config().language,
                  AgentPlanParser::DefaultMaxPlanSteps)
            : AgentPlanParser::parseJsonPlan(
                  m_agentPlanResponseBuffer,
                  registry.descriptors());
        m_agentPlanResponseBuffer.clear();
        m_agentToolCalls.clear();
        m_pendingAgentRequestSession = ChatSession();
        m_nativeToolRequestActive = false;
        m_nativeToolFallbackAttempted = false;
        m_activeRequestKind = ActiveRequestKind::None;

        if (!parseResult.ok) {
            m_pendingAgentPlanContinuationDepth = 0;
            AppLogger::warning(QStringLiteral("AgentPlan"),
                               QStringLiteral("Agent plan parse failed. error=%1")
                                   .arg(parseResult.error));
            emit statusMessage(QStringLiteral("Agent plan parsing failed: %1").arg(parseResult.error),
                               QStringLiteral("Agent 计划解析失败：%1").arg(parseResult.error),
                               7000);
            return;
        }

        AgentPlan plan = parseResult.plan;
        plan.continuationDepth = m_pendingAgentPlanContinuationDepth;
        m_pendingAgentPlanContinuationDepth = 0;

        AppLogger::info(QStringLiteral("AgentPlan"),
                        QStringLiteral("Agent plan parsed. Auto-executing. steps=%1")
                            .arg(plan.steps.size()));

        // 委托给 AgentOrchestrator 执行计划
        const bool planAllSuccess = m_agentOrchestrator.executePlanAndReportToChat(plan);

        // V12.6: 循环继续 — 仅当计划有失败步骤时才进入 OODA 循环
        if (planAllSuccess) {
            // 全部成功 → 直接结束循环
            m_agentOrchestrator.cancelAgentLoop();
            emit statusMessage("Agent plan completed", "Agent 计划执行完成", 3000);
            return;
        }
        if (m_agentOrchestrator.isAgentLoopActive()) {
            QString observation;
            for (const AgentPlanStep &step : plan.steps) {
                observation += QStringLiteral("[%1] %2\n").arg(step.toolId, step.title);
            }
            m_agentOrchestrator.appendLoopObservation(observation);
            if (m_agentOrchestrator.executeAgentLoopIteration()) {
                continueAgentLoop();
            }
        }
        return;
    }

    if (m_activeRequestKind == ActiveRequestKind::UnifiedAgent) {
        setGenerating(false);
        if (!m_agentToolCalls.isEmpty()) {
            const AgentPlanParseResult parseResult = AgentToolCallPlanBuilder::buildPlanFromToolCalls(
                m_agentToolCalls,
                AgentToolRegistryFactory::defaultRegistry(),
                m_configCoordinator.config().language,
                AgentPlanParser::DefaultMaxPlanSteps);
            m_agentPlanResponseBuffer.clear();
            m_agentToolCalls.clear();
            m_pendingAgentRequestSession = ChatSession();
            m_nativeToolRequestActive = false;
            m_nativeToolFallbackAttempted = false;
            m_activeRequestKind = ActiveRequestKind::None;

            // V12.3: 组合流式工具执行结果
            QVector<PendingToolResult> streamResults = m_agentOrchestrator.takePendingToolResults();
            QString streamingResultsSummary;
            if (!streamResults.isEmpty()) {
                streamingResultsSummary = QStringLiteral("\n\n[Streaming results (executed early)]");
                for (const PendingToolResult &pending : streamResults) {
                    streamingResultsSummary += QStringLiteral("\ntool=%1: %2")
                        .arg(pending.toolName, pending.result);
                }
            }

            if (!parseResult.ok) {
                m_currentAssistantContent = m_configCoordinator.text(
                    QStringLiteral("Tool call parsing failed: %1").arg(parseResult.error),
                    QStringLiteral("工具调用解析失败：%1").arg(parseResult.error))
                    + streamingResultsSummary;
                if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
                    m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
                }
                emit assistantMessageUpdated(m_currentAssistantContent);
                m_sessionCoordinator.saveCurrentSession();
                return;
            }

            m_currentAssistantContent = m_configCoordinator.text(
                QStringLiteral("Agent plan generated. Review before executing steps."),
                QStringLiteral("Agent 计划已生成。请先检查再执行步骤。"))
                + streamingResultsSummary;
            if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
                m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
            }
            emit assistantMessageUpdated(m_currentAssistantContent);
            m_sessionCoordinator.saveCurrentSession();

            // 委托给 AgentOrchestrator 执行计划
            const bool planAllSuccess = m_agentOrchestrator.executePlanAndReportToChat(parseResult.plan);

            // V12.6: 循环继续 — 仅当计划步骤有失败时才进入 OODA 循环尝试恢复
            if (m_agentOrchestrator.isAgentLoopActive() && !planAllSuccess) {
                QString observation;
                for (const AgentPlanStep &step : parseResult.plan.steps) {
                    observation += QStringLiteral("[%1] %2\n").arg(step.toolId, step.title);
                }
                m_agentOrchestrator.appendLoopObservation(observation);
                if (m_agentOrchestrator.executeAgentLoopIteration()) {
                    continueAgentLoop();
                }
            } else if (m_agentOrchestrator.isAgentLoopActive()) {
                // 全部成功 → 直接结束循环
                m_agentOrchestrator.cancelAgentLoop();
                m_currentAssistantContent += 
                    QStringLiteral("\n\n---\n")
                    + m_configCoordinator.text("Task completed.", "任务完成。");
                if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
                    m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
                }
                emit assistantMessageUpdated(m_currentAssistantContent);
                m_sessionCoordinator.saveCurrentSession();
                emit statusMessage("Agent plan completed", "Agent 计划执行完成", 3000);
            }
            return;
        }

        const auto response = UnifiedResponseParser::parse(m_agentPlanResponseBuffer, m_configCoordinator.config().language);
        m_agentPlanResponseBuffer.clear();
        m_agentToolCalls.clear();
        m_pendingAgentRequestSession = ChatSession();
        m_nativeToolRequestActive = false;
        m_nativeToolFallbackAttempted = false;
        m_activeRequestKind = ActiveRequestKind::None;

        // V12.3: 组合流式工具执行结果
        QVector<PendingToolResult> streamResults = m_agentOrchestrator.takePendingToolResults();
        QString streamingResultsSummary;
        if (!streamResults.isEmpty()) {
            streamingResultsSummary = QStringLiteral("\n\n[Streaming results (executed early)]");
            for (const PendingToolResult &pending : streamResults) {
                streamingResultsSummary += QStringLiteral("\ntool=%1: %2")
                    .arg(pending.toolName, pending.result);
            }
        }

        if (response.kind == UnifiedResponseKind::Chat) {
            // Chat 响应 = AI 判断任务完成
            if (m_agentOrchestrator.isAgentLoopActive()) {
                // Agent 循环完成
                m_agentOrchestrator.cancelAgentLoop(); // 清空循环状态

                // V13.3: 发送技能摘要
                auto skills = m_agentOrchestrator.matchedSkills();
                if (m_agentOrchestrator.skillManager() && !skills.isEmpty()) {
                    const QString summary = m_agentOrchestrator.skillManager()->skillSummary(skills);
                    emit agentLoopSkillSummary(summary);
                    emit statusMessage(summary, summary, 5000);
                }

                m_currentAssistantContent = response.chatMessage + streamingResultsSummary
                    + QStringLiteral("\n\n---\n")
                    + m_configCoordinator.text("Task completed.", "任务完成。");
                if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
                    m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
                }
                emit assistantMessageUpdated(m_currentAssistantContent);
                m_sessionCoordinator.saveCurrentSession();
                emit statusMessage("Agent loop finished", "Agent 循环完成", 3000);
                return;
            }

            // 聊天回复 → 显示在消息流中
            m_currentAssistantContent = response.chatMessage + streamingResultsSummary;
            if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
                m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
            }
            emit assistantMessageUpdated(m_currentAssistantContent);
            m_sessionCoordinator.saveCurrentSession();
            AppLogger::info(QStringLiteral("UnifiedAgent"),
                            QStringLiteral("Unified agent returned chat reply."));
            return;
        }

        if (response.kind == UnifiedResponseKind::Plan) {
            const AgentPlanParseResult parseResult = AgentPlanParser::parseJsonPlan(
                response.planJson, defaultAgentToolCatalog());

            if (!parseResult.ok) {
                m_currentAssistantContent = m_configCoordinator.text(
                    QStringLiteral("Plan parsing failed: %1").arg(parseResult.error),
                    QStringLiteral("计划解析失败：%1").arg(parseResult.error))
                    + streamingResultsSummary;
                if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
                    m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
                }
                emit assistantMessageUpdated(m_currentAssistantContent);
                m_sessionCoordinator.saveCurrentSession();
                return;
            }

            AppLogger::info(QStringLiteral("UnifiedAgent"),
                            QStringLiteral("Unified agent plan parsed and ready. steps=%1")
                                .arg(parseResult.plan.steps.size()));
            m_currentAssistantContent = m_configCoordinator.text(
                QStringLiteral("Agent plan generated. Review before executing steps."),
                QStringLiteral("Agent 计划已生成。请先检查再执行步骤。"))
                + streamingResultsSummary;
            if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
                m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
            }
            emit assistantMessageUpdated(m_currentAssistantContent);
            m_sessionCoordinator.saveCurrentSession();

            // 委托给 AgentOrchestrator 执行计划
            const bool planAllSuccess = m_agentOrchestrator.executePlanAndReportToChat(parseResult.plan);

            // V12.6: 循环继续 — 仅当计划有失败步骤时才进入 OODA 循环
            if (planAllSuccess) {
                // 全部成功 → 直接结束循环
                m_agentOrchestrator.cancelAgentLoop();
                m_currentAssistantContent += 
                    QStringLiteral("\n\n---\n")
                    + m_configCoordinator.text("Task completed.", "任务完成。");
                if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
                    m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
                }
                emit assistantMessageUpdated(m_currentAssistantContent);
                m_sessionCoordinator.saveCurrentSession();
                emit statusMessage("Agent plan completed", "Agent 计划执行完成", 3000);
                return;
            }
            if (m_agentOrchestrator.isAgentLoopActive()) {
                QString observation;
                for (const AgentPlanStep &step : parseResult.plan.steps) {
                    observation += QStringLiteral("[%1] %2\n").arg(step.toolId, step.title);
                }
                m_agentOrchestrator.appendLoopObservation(observation);
                if (m_agentOrchestrator.executeAgentLoopIteration()) {
                    continueAgentLoop();
                }
            }
            return;
        }

        // 解析失败 → 当作聊天回复兜底
        m_currentAssistantContent = response.rawResponse + streamingResultsSummary;
        if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
            m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
        }
        emit assistantMessageUpdated(m_currentAssistantContent);
        m_sessionCoordinator.saveCurrentSession();
        return;
    }

    setGenerating(false);
    setRetryAvailable(false);

    m_agentOrchestrator.clearPendingToolResults();

    if (!m_sessionCoordinator.currentSession().messages.isEmpty() && m_sessionCoordinator.currentSession().messages.last().content.isEmpty()) {
        m_sessionCoordinator.currentSession().messages.last().content = m_configCoordinator.text(QStringLiteral("(Empty response)"), QStringLiteral("（空回复）"));
        emit assistantMessageUpdated(m_sessionCoordinator.currentSession().messages.last().content);
    }

    m_sessionCoordinator.saveCurrentSession();
    m_activeRequestKind = ActiveRequestKind::None;
}

void ApplicationController::handleRequestFailed(const QString &message, RequestErrorCategory category)
{
    if (m_activeRequestKind == ActiveRequestKind::AgentPlan) {
        if (retryAgentRequestWithoutNativeTools(category)) {
            return;
        }

        setGenerating(false);
        m_agentPlanResponseBuffer.clear();
        m_agentToolCalls.clear();
        m_pendingAgentRequestSession = ChatSession();
        m_nativeToolRequestActive = false;
        m_nativeToolFallbackAttempted = false;
        m_pendingAgentPlanContinuationDepth = 0;
        m_activeRequestKind = ActiveRequestKind::None;
        setRetryAvailable(false);
        emit statusMessage(QStringLiteral("Agent plan request failed: %1").arg(message),
                           QStringLiteral("Agent 计划请求失败：%1").arg(message),
                           7000);
        return;
    }

    if (m_activeRequestKind == ActiveRequestKind::UnifiedAgent) {
        if (retryAgentRequestWithoutNativeTools(category)) {
            return;
        }

        setGenerating(false);
        m_agentPlanResponseBuffer.clear();
        m_agentToolCalls.clear();
        m_pendingAgentRequestSession = ChatSession();
        m_nativeToolRequestActive = false;
        m_nativeToolFallbackAttempted = false;
        m_activeRequestKind = ActiveRequestKind::None;
        setRetryAvailable(false);
        m_currentAssistantContent = m_configCoordinator.text(
            QStringLiteral("Agent request failed: %1").arg(message),
            QStringLiteral("Agent 请求失败：%1").arg(message));
        if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
            m_sessionCoordinator.currentSession().messages.last().content = m_currentAssistantContent;
        }
        emit assistantMessageUpdated(m_currentAssistantContent);
        m_sessionCoordinator.saveCurrentSession();
        emit statusMessage(QStringLiteral("Agent request failed: %1").arg(message),
                           QStringLiteral("Agent 请求失败：%1").arg(message),
                           7000);
        return;
    }

    setGenerating(false);
    const QString errorMessage = requestFailureMessage(category, message);
    const QString displayMessage = m_currentAssistantContent.isEmpty()
                                       ? errorMessage
                                       : QStringLiteral("%1\n\n%2").arg(m_currentAssistantContent, errorMessage);

    if (!m_sessionCoordinator.currentSession().messages.isEmpty()) {
        m_sessionCoordinator.currentSession().messages.last().content = displayMessage;
    }

    emit assistantMessageUpdated(displayMessage);
    setRetryAvailable(true, m_lastRequestUserContent);
    emit statusMessage(QStringLiteral("Request failed. You can retry the last message."),
                       QStringLiteral("请求失败，可以重试上一条消息。"),
                       5000);
    m_sessionCoordinator.saveCurrentSession();
    m_agentToolCalls.clear();
    m_pendingAgentRequestSession = ChatSession();
    m_nativeToolRequestActive = false;
    m_nativeToolFallbackAttempted = false;
    m_activeRequestKind = ActiveRequestKind::None;
}

bool ApplicationController::retryAgentRequestWithoutNativeTools(RequestErrorCategory category)
{
    if (!m_nativeToolRequestActive ||
        m_nativeToolFallbackAttempted ||
        category != RequestErrorCategory::Model ||
        m_pendingAgentRequestSession.messages.isEmpty()) {
        return false;
    }

    m_nativeToolFallbackAttempted = true;
    m_nativeToolRequestActive = false;
    m_agentPlanResponseBuffer.clear();
    m_agentToolCalls.clear();

    AppLogger::warning(QStringLiteral("AgentPlan"),
                       QStringLiteral("Native tool request failed; retrying with JSON plan fallback."));
    emit statusMessage(QStringLiteral("Native tool call request failed. Retrying with JSON plan fallback..."),
                       QStringLiteral("原生工具调用请求失败，正在退回 JSON 计划模式..."),
                       4000);
    m_aiClient.sendChat(m_configCoordinator.config(), m_pendingAgentRequestSession);
    return true;
}

void ApplicationController::setGenerating(bool generating)
{
    if (m_isGenerating == generating) {
        return;
    }

    m_isGenerating = generating;
    emit generatingChanged(m_isGenerating);
}

void ApplicationController::setRetryAvailable(bool available, const QString &userContent)
{
    const bool nextAvailable = available && !userContent.trimmed().isEmpty();
    const QString nextUserContent = nextAvailable ? userContent : QString();
    if (m_retryAvailable == nextAvailable && m_retryUserContent == nextUserContent) {
        return;
    }

    m_retryAvailable = nextAvailable;
    m_retryUserContent = nextUserContent;
    emit retryAvailableChanged(m_retryAvailable);
}

QString ApplicationController::requestFailureMessage(RequestErrorCategory category, const QString &detail) const
{
    QString summaryEnglish;
    QString summaryChinese;

    switch (category) {
    case RequestErrorCategory::Network:
        summaryEnglish = QStringLiteral("Network error. Check your connection or Base URL, then retry.");
        summaryChinese = QStringLiteral("网络错误。请检查网络连接或 Base URL，然后重试。");
        break;
    case RequestErrorCategory::Authentication:
        summaryEnglish = QStringLiteral("Authentication failed. Check whether the API Key is valid.");
        summaryChinese = QStringLiteral("认证失败。请检查 API Key 是否有效。");
        break;
    case RequestErrorCategory::Quota:
        summaryEnglish = QStringLiteral("Quota or rate limit reached. Check account quota or try again later.");
        summaryChinese = QStringLiteral("额度不足或请求过于频繁。请检查账户额度，或稍后再试。");
        break;
    case RequestErrorCategory::Model:
        summaryEnglish = QStringLiteral("Model or request configuration error. Check the model name and optional parameters.");
        summaryChinese = QStringLiteral("模型或请求配置错误。请检查模型名称和可选参数。");
        break;
    case RequestErrorCategory::Server:
        summaryEnglish = QStringLiteral("Service provider error. The server response was incomplete or unavailable.");
        summaryChinese = QStringLiteral("服务商错误。服务器响应不完整或暂时不可用。");
        break;
    case RequestErrorCategory::Unknown:
        summaryEnglish = QStringLiteral("Request failed.");
        summaryChinese = QStringLiteral("请求失败。");
        break;
    }

    const QString summary = m_configCoordinator.text(summaryEnglish, summaryChinese);
    const QString trimmedDetail = detail.trimmed();
    if (trimmedDetail.isEmpty()) {
        return summary;
    }

    return m_configCoordinator.text(QStringLiteral("%1\n\nDetails: %2"), QStringLiteral("%1\n\n详细信息：%2")).arg(summary, trimmedDetail);
}

bool ApplicationController::saveCurrentSession(bool moveToTop)
{
    return m_sessionCoordinator.saveCurrentSession(moveToTop);
}

void ApplicationController::clearAgentLoopState()
{
    m_agentOrchestrator.clearAgentLoopState();
}

bool ApplicationController::hasPendingAgentState() const
{
    return m_agentOrchestrator.hasPendingAgentState();
}

AgentLoopState ApplicationController::pendingAgentState() const
{
    return m_agentOrchestrator.pendingAgentState();
}

void ApplicationController::resumeAgentLoop()
{
    if (!m_agentOrchestrator.hasPendingAgentState()) {
        return;
    }

    m_agentOrchestrator.resumeAgentLoop();

    // 继续循环：构建提示词并发送
    continueAgentLoop();
}

// V17.4: 对话分支
void ApplicationController::createMessageBranch(const QString &parentMessageId)
{
    m_sessionCoordinator.createMessageBranch(parentMessageId);
}

// V15.1: 定时任务调度

void ApplicationController::addScheduledTask(const QString &name, const QString &cron, const QString &prompt)
{
    auto task = ScheduledTask::create(name, cron, prompt);
    m_taskScheduler->addTask(task);
    if (m_taskStorage)
        m_taskStorage->save(m_taskScheduler->allTasks());
}

void ApplicationController::removeScheduledTask(const QString &taskId)
{
    m_taskScheduler->removeTask(taskId);
    if (m_taskStorage)
        m_taskStorage->save(m_taskScheduler->allTasks());
}

void ApplicationController::updateScheduledTask(const ScheduledTask &task)
{
    m_taskScheduler->updateTask(task);
    if (m_taskStorage)
        m_taskStorage->save(m_taskScheduler->allTasks());
}

QVector<ScheduledTask> ApplicationController::scheduledTasks() const
{
    return m_taskScheduler ? m_taskScheduler->allTasks() : QVector<ScheduledTask>();
}

// V15.2: 任务触发后持久化更新后的状态
void ApplicationController::onScheduledTaskTriggered(const ScheduledTask &task)
{
    AppLogger::info("Scheduler", "Task triggered: " + task.name);
    sendAgentLoopMessage(task.agentPrompt);
    if (m_taskStorage)
        m_taskStorage->save(m_taskScheduler->allTasks());
}

// ─── CH-8: 公开编辑/截断接口 ─────────────────────────────────────────

bool ApplicationController::editCurrentMessage(const QString &messageId, const QString &newContent)
{
    return m_sessionCoordinator.editCurrentMessage(messageId, newContent);
}

void ApplicationController::truncateCurrentSessionFrom(const QString &messageId)
{
    m_sessionCoordinator.truncateCurrentSessionFrom(messageId);
}
