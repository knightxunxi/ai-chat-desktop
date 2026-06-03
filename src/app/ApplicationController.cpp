#include "app/ApplicationController.h"

#include "app/AgentCommandSkillFileService.h"
#include "app/AgentLoopPromptBuilder.h"
#include "app/AgentPlanExecutor.h"
#include "app/AgentPlanParser.h"
#include "app/AgentPlanPromptBuilder.h"
#include "app/AgentToolCallPlanBuilder.h"
#include "app/ContextWindowManager.h"
#include "app/ProjectInstructionService.h"
#include "app/SessionSummaryList.h"
#include "app/TokenEstimator.h"

#include "storage/ChatSessionExporter.h"
#include "support/AppLogger.h"
#include "tools/AgentToolCatalog.h"
#include "tools/AgentToolRegistry.h"
#include "tools/ProjectMemoryService.h"
#include "memory/ProjectMemoryManager.h"

#include <QDateTime>
#include <QStringList>

namespace {

bool sessionMatchesFilter(const ChatSession &session, SessionListFilter filter)
{
    switch (filter) {
    case SessionListFilter::Active:
        return !session.isArchived;
    case SessionListFilter::Favorite:
        return session.isFavorite && !session.isArchived;
    case SessionListFilter::Archived:
        return session.isArchived;
    }

    return !session.isArchived;
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
    connect(&m_aiClient, &OpenAICompatibleClient::textDeltaReceived, this, &ApplicationController::handleTextDelta);
    connect(&m_aiClient, &OpenAICompatibleClient::toolCallsReceived, this, &ApplicationController::handleToolCallsReceived);
    connect(&m_aiClient, &OpenAICompatibleClient::toolUseBlockComplete, this, &ApplicationController::handleToolUseBlockComplete);
    connect(&m_aiClient, &OpenAICompatibleClient::requestFinished, this, &ApplicationController::handleRequestFinished);
    connect(&m_aiClient, &OpenAICompatibleClient::requestFailed, this, &ApplicationController::handleRequestFailed);
}

void ApplicationController::initialize()
{
    m_config = m_configStorage.load();
    emit configChanged();

    QString templateError;
    m_promptTemplates = m_promptTemplateStorage.load(&templateError);
    if (!templateError.isEmpty()) {
        emit startupWarning(
            QStringLiteral("Prompt templates are unavailable. Default templates will be used.\n\n%1").arg(templateError),
            QStringLiteral("角色提示词模板不可用，将使用默认模板。\n\n%1").arg(templateError));
    }
    emit promptTemplatesChanged();

    QString error;
    if (!m_chatHistoryStorage.initialize(&error)) {
        m_session = ChatSession::createDefault();
        m_sessionSummaries.clear();
        m_historyAvailable = false;
        emit startupWarning(
            QStringLiteral("Chat history is unavailable. New messages will not be restored after restart.\n\n%1").arg(error),
            QStringLiteral("聊天记录不可用。新消息在重启后将无法恢复。\n\n%1").arg(error));
        emit sessionListChanged();
        emit currentSessionChanged();
        return;
    }

    m_historyAvailable = true;
    reloadSessionSummaries(&error);
    if (m_sessionSummaries.isEmpty()) {
        m_session = ChatSession::createDefault();
        if (!error.isEmpty()) {
            emit startupWarning(
                QStringLiteral("Failed to load the latest chat history.\n\n%1").arg(error),
                QStringLiteral("加载最近聊天记录失败。\n\n%1").arg(error));
        }
    } else {
        const std::optional<ChatSession> latestSession = m_chatHistoryStorage.loadSession(m_sessionSummaries.first().id, &error);
        m_session = latestSession.value_or(ChatSession::createDefault());
        if (!latestSession.has_value() && !error.isEmpty()) {
            emit startupWarning(
                QStringLiteral("Failed to load the latest chat history.\n\n%1").arg(error),
                QStringLiteral("加载最近聊天记录失败。\n\n%1").arg(error));
        }
    }

    emit sessionListChanged();
    emit currentSessionChanged();

    // V12.1 上下文窗口管理 — 根据加载的配置初始化
    m_summaryClient.reconfigure(m_config);
    m_contextWindowManager = ContextWindowManager(getContextWindowTokens());
    m_contextWindowManager.setSummaryClient(&m_summaryClient);
}

const AppConfig &ApplicationController::config() const
{
    return m_config;
}

const ChatSession &ApplicationController::currentSession() const
{
    return m_session;
}

const QVector<ChatSession> &ApplicationController::sessionSummaries() const
{
    return m_sessionSummaries;
}

SessionListFilter ApplicationController::sessionListFilter() const
{
    return m_sessionListFilter;
}

const QVector<PromptTemplate> &ApplicationController::promptTemplates() const
{
    return m_promptTemplates;
}

bool ApplicationController::isGenerating() const
{
    return m_isGenerating;
}

bool ApplicationController::exportCurrentSessionMarkdown(const QString &filePath, QString *error) const
{
    return ChatSessionExporter::writeMarkdown(m_session, filePath, error);
}

size_t ApplicationController::getContextWindowTokens() const
{
    if (m_config.maxTokens.has_value()) {
        return static_cast<size_t>(m_config.maxTokens.value());
    }
    return ContextWindowManager::contextWindowForModel(m_config.modelName);
}

void ApplicationController::saveConfig(const AppConfig &config)
{
    m_config = config;
    QString error;
    if (!m_configStorage.save(m_config, &error)) {
        emit statusMessage(QStringLiteral("Failed to save API Key securely: %1").arg(error),
                           QStringLiteral("API Key 安全保存失败：%1").arg(error),
                           6000);
    }
    emit configChanged();
}

void ApplicationController::savePromptTemplates(const QVector<PromptTemplate> &templates)
{
    m_promptTemplates = templates;

    QString error;
    if (!m_promptTemplateStorage.save(m_promptTemplates, &error)) {
        emit statusMessage(QStringLiteral("Failed to save prompt templates: %1").arg(error),
                           QStringLiteral("保存角色提示词模板失败：%1").arg(error),
                           6000);
        return;
    }

    emit promptTemplatesChanged();
}

void ApplicationController::setSystemPrompt(const QString &prompt)
{
    if (m_isGenerating) {
        return;
    }

    m_session.systemPrompt = prompt.trimmed();
    m_session.updatedAt = QDateTime::currentDateTimeUtc();
    if (!saveCurrentSession()) {
        emit statusMessage(QStringLiteral("Failed to save the role prompt."),
                           QStringLiteral("保存角色提示词失败。"),
                           6000);
    }

    emit currentSessionChanged();
}

void ApplicationController::renameCurrentSession(const QString &title)
{
    if (m_isGenerating) {
        return;
    }

    const QString trimmedTitle = title.trimmed();
    if (trimmedTitle.isEmpty()) {
        emit statusMessage(QStringLiteral("Chat title cannot be empty."),
                           QStringLiteral("会话标题不能为空。"),
                           3000);
        return;
    }

    if (m_session.title == trimmedTitle) {
        emit currentSessionChanged();
        return;
    }

    const QString previousTitle = m_session.title;
    m_session.title = trimmedTitle;
    if (!saveCurrentSession(false)) {
        m_session.title = previousTitle;
        emit statusMessage(QStringLiteral("Failed to rename the chat session."),
                           QStringLiteral("重命名会话失败。"),
                           6000);
        return;
    }

    emit currentSessionChanged();
}

void ApplicationController::searchSessions(const QString &query)
{
    m_sessionSearchQuery = query.trimmed();
    if (!m_historyAvailable) {
        m_sessionSummaries.clear();
        emit sessionListChanged();
        return;
    }

    QString error;
    if (!reloadSessionSummaries(&error) && !error.isEmpty()) {
        emit statusMessage(QStringLiteral("Failed to search chat sessions: %1").arg(error),
                           QStringLiteral("搜索会话失败：%1").arg(error),
                           6000);
    }

    emit sessionListChanged();
}

void ApplicationController::setSessionListFilter(SessionListFilter filter)
{
    if (m_sessionListFilter == filter) {
        return;
    }

    m_sessionListFilter = filter;
    QString error;
    if (!reloadSessionSummaries(&error) && !error.isEmpty()) {
        emit statusMessage(QStringLiteral("Failed to load chat sessions: %1").arg(error),
                           QStringLiteral("加载会话列表失败：%1").arg(error),
                           6000);
    }

    emit sessionListFilterChanged();
    emit sessionListChanged();
}

void ApplicationController::toggleCurrentSessionFavorite()
{
    if (m_isGenerating) {
        return;
    }

    const bool previousFavorite = m_session.isFavorite;
    const SessionListFilter previousFilter = m_sessionListFilter;
    m_session.isFavorite = !m_session.isFavorite;
    if (!m_session.isFavorite && m_sessionListFilter == SessionListFilter::Favorite) {
        m_sessionListFilter = SessionListFilter::Active;
    }

    if (!saveCurrentSession(false)) {
        m_session.isFavorite = previousFavorite;
        m_sessionListFilter = previousFilter;
        emit sessionListFilterChanged();
        emit currentSessionChanged();
        return;
    }

    if (m_sessionListFilter != previousFilter) {
        emit sessionListFilterChanged();
    }
    emit currentSessionChanged();
}

void ApplicationController::toggleCurrentSessionArchived()
{
    if (m_isGenerating) {
        return;
    }

    const bool previousArchived = m_session.isArchived;
    const SessionListFilter previousFilter = m_sessionListFilter;
    m_session.isArchived = !m_session.isArchived;
    if (m_session.isArchived) {
        m_sessionListFilter = SessionListFilter::Archived;
    } else if (m_sessionListFilter == SessionListFilter::Archived) {
        m_sessionListFilter = SessionListFilter::Active;
    }

    if (!saveCurrentSession(false)) {
        m_session.isArchived = previousArchived;
        m_sessionListFilter = previousFilter;
        emit sessionListFilterChanged();
        emit currentSessionChanged();
        return;
    }

    if (m_sessionListFilter != previousFilter) {
        emit sessionListFilterChanged();
    }
    emit currentSessionChanged();
}

void ApplicationController::startNewChat()
{
    if (m_isGenerating) {
        return;
    }

    setRetryAvailable(false);
    if (m_sessionListFilter != SessionListFilter::Active) {
        m_sessionListFilter = SessionListFilter::Active;
        QString error;
        reloadSessionSummaries(&error);
        emit sessionListFilterChanged();
        emit sessionListChanged();
    }

    if (!hasPersistableCurrentSession()) {
        emit currentSessionChanged();
        return;
    }

    saveCurrentSession(false);

    m_session = ChatSession::createDefault();
    m_currentAssistantContent.clear();
    saveCurrentSession();

    emit sessionListChanged();
    emit currentSessionChanged();
}

void ApplicationController::switchToSession(const QString &sessionId)
{
    if (m_isGenerating || sessionId.isEmpty()) {
        return;
    }

    setRetryAvailable(false);
    if (sessionId == m_session.id) {
        emit currentSessionChanged();
        return;
    }

    if (hasPersistableCurrentSession()) {
        saveCurrentSession(false);
    }

    QString error;
    const std::optional<ChatSession> loaded = m_chatHistoryStorage.loadSession(sessionId, &error);
    if (!loaded.has_value()) {
        emit statusMessage(QStringLiteral("Failed to load chat session: %1").arg(error.isEmpty() ? sessionId : error),
                           QStringLiteral("加载会话失败：%1").arg(error.isEmpty() ? sessionId : error),
                           6000);
        emit currentSessionChanged();
        return;
    }

    m_session = loaded.value();
    m_currentAssistantContent.clear();
    emit currentSessionChanged();
}

void ApplicationController::deleteCurrentSession()
{
    if (m_isGenerating) {
        return;
    }

    setRetryAvailable(false);
    const QString deletedSessionId = m_session.id;
    if (m_historyAvailable) {
        QString error;
        if (!m_chatHistoryStorage.clearSession(deletedSessionId, &error)) {
            emit statusMessage(QStringLiteral("Failed to delete chat session: %1").arg(error),
                               QStringLiteral("删除会话失败：%1").arg(error),
                               6000);
            return;
        }
    }

    for (int index = 0; index < m_sessionSummaries.size(); ++index) {
        if (m_sessionSummaries[index].id == deletedSessionId) {
            m_sessionSummaries.removeAt(index);
            break;
        }
    }

    m_currentAssistantContent.clear();

    QString error;
    if (!m_sessionSearchQuery.isEmpty()) {
        reloadSessionSummaries(&error);
    }

    if (m_sessionSummaries.isEmpty()) {
        if (m_sessionListFilter != SessionListFilter::Active) {
            m_sessionListFilter = SessionListFilter::Active;
            emit sessionListFilterChanged();
            reloadSessionSummaries(&error);
        }
    }

    if (m_sessionSummaries.isEmpty()) {
        m_session = ChatSession::createDefault();
        saveCurrentSession();
    } else {
        const std::optional<ChatSession> nextSession = m_chatHistoryStorage.loadSession(m_sessionSummaries.first().id, &error);
        if (nextSession.has_value()) {
            m_session = nextSession.value();
        } else {
            m_session = ChatSession::createDefault();
            emit statusMessage(QStringLiteral("Failed to load the next chat session: %1").arg(error),
                               QStringLiteral("加载下一个会话失败：%1").arg(error),
                               6000);
        }
    }

    emit sessionListChanged();
    emit currentSessionChanged();
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
    if (!m_config.isComplete()) {
        emit configurationMissing();
        return;
    }

    if (m_session.messages.isEmpty()) {
        m_session.title = trimmedContent.left(36);
        emit currentChatCleared();
        upsertCurrentSessionSummary(true);
        emit sessionListChanged();
    }

    m_session.addMessage(MessageRole::User, trimmedContent);
    emit userMessageAdded(trimmedContent);

    startAssistantRequest(trimmedContent);
}

void ApplicationController::startAssistantRequest(const QString &userContentForRetry)
{
    m_activeRequestKind = ActiveRequestKind::ChatMessage;
    m_lastRequestUserContent = userContentForRetry;
    m_chatAutoExecute = false;  // V12.4: 纯 Chat 请求不带工具

    // V12.1 上下文窗口管理 — 检查并压缩
    ContextWindowResult result = m_contextWindowManager.processMessages(
        m_session.messages, m_session.systemPrompt);

    if (result.wasTrimmed) {
        // 注入压缩提示到 systemPrompt
        const QString hint = ContextWindowManager::buildCompressionHint(
            result.trimmedRoundCount, result.summary);
        m_session.systemPrompt = hint + QStringLiteral("\n") + m_session.systemPrompt;
        // 替换消息列表（仅内存，不持久化裁剪后的消息列表）
        m_session.messages = result.processedMessages;
        // 通知用户
        emit statusMessage(
            QStringLiteral("Compressed %1 rounds into summary").arg(result.trimmedRoundCount),
            QStringLiteral("已压缩 %1 轮历史对话为摘要").arg(result.trimmedRoundCount),
            3000);
    }

    m_session.addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    emit assistantMessageStarted();

    setGenerating(true);
    m_aiClient.sendChat(m_config, m_session);
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

    if (!m_config.isComplete()) {
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
    const ProjectInstructions projectInstructions = ProjectInstructionService::loadFromProjectDirectory(m_config.agentProjectDirectory);
    const QString projectInstructionSection = ProjectInstructionService::promptSection(projectInstructions, m_config.language);
    const ProjectMemory projectMemory = ProjectMemoryService::loadFromProjectDirectory(m_config.agentProjectDirectory);
    const QString projectMemorySection = ProjectMemoryService::promptSection(projectMemory, m_config.language);
    const QString commandSkillSection = commandSkillSectionForProject(m_config.agentProjectDirectory, m_config.language);
    const QString planningPrompt = AgentPlanPromptBuilder::buildPlanningPrompt(
        trimmedGoal,
        catalog,
        m_config.language,
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
    ContextWindowResult planResult = m_contextWindowManager.processMessages(
        planningSession.messages, planningSession.systemPrompt);
    if (planResult.wasTrimmed) {
        planningSession.messages = planResult.processedMessages;
    }

    setGenerating(true);
    m_aiClient.sendChatWithTools(m_config, planningSession, registry.functionToolSchemas(m_config.language));

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
    if (!m_config.isComplete()) {
        emit configurationMissing();
        return;
    }

    if (m_session.messages.isEmpty()) {
        m_session.title = trimmedContent.left(36);
        emit currentChatCleared();
        upsertCurrentSessionSummary(true);
        emit sessionListChanged();
    }

    m_session.addMessage(MessageRole::User, trimmedContent);
    emit userMessageAdded(trimmedContent);

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
    const QVector<AgentToolDescriptor> catalog = registry.descriptors();
    const ProjectInstructions projectInstructions = ProjectInstructionService::loadFromProjectDirectory(m_config.agentProjectDirectory);
    const QString projectInstructionSection = ProjectInstructionService::promptSection(projectInstructions, m_config.language);
    const ProjectMemory projectMemory = ProjectMemoryService::loadFromProjectDirectory(m_config.agentProjectDirectory);
    const QString projectMemorySection = ProjectMemoryService::promptSection(projectMemory, m_config.language);
    const QString commandSkillSection = commandSkillSectionForProject(m_config.agentProjectDirectory, m_config.language);
    const QString prompt = AgentPlanPromptBuilder::buildUnifiedPrompt(
        trimmedContent,
        catalog,
        m_config.language,
        AgentPlanParser::DefaultMaxPlanSteps,
        projectInstructionSection,
        commandSkillSection,
        projectMemorySection);

    // 构建一个临时会话，只包含统一 prompt
    ChatSession requestSession = ChatSession::createDefault();
    requestSession.title = QStringLiteral("Unified");
    requestSession.addMessage(MessageRole::User, prompt);

    m_activeRequestKind = ActiveRequestKind::UnifiedAgent;
    m_agentPlanResponseBuffer.clear();
    m_agentToolCalls.clear();
    m_pendingAgentRequestSession = requestSession;
    m_nativeToolRequestActive = true;
    m_nativeToolFallbackAttempted = false;

    m_session.addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    emit assistantMessageStarted();

    // V12.1 上下文检查
    ContextWindowResult unifiedResult = m_contextWindowManager.processMessages(
        requestSession.messages, requestSession.systemPrompt);
    if (unifiedResult.wasTrimmed) {
        requestSession.messages = unifiedResult.processedMessages;
    }

    setGenerating(true);
    m_aiClient.sendChatWithTools(m_config, requestSession, registry.functionToolSchemas(m_config.language));

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

    // 初始化循环状态
    m_isAgentLoopActive = true;
    m_agentLoopGoal = trimmedContent;
    m_agentLoopIteration = 0;
    m_agentLoopObservations.clear();

    // 第一轮复用 sendUnifiedMessage 的逻辑
    sendUnifiedMessage(content);
}

// V12.4: Chat 模式带工具的请求 — 复用 startAssistantRequest 逻辑，但携带 tools schema
void ApplicationController::sendMessageWithTools(const QString &content)
{
    if (m_isGenerating) {
        return;
    }

    const QString trimmedContent = content.trimmed();
    if (trimmedContent.isEmpty()) {
        return;
    }

    setRetryAvailable(false);
    if (!m_config.isComplete()) {
        emit configurationMissing();
        return;
    }

    if (m_session.messages.isEmpty()) {
        m_session.title = trimmedContent.left(36);
        emit currentChatCleared();
        upsertCurrentSessionSummary(true);
        emit sessionListChanged();
    }

    m_session.addMessage(MessageRole::User, trimmedContent);
    emit userMessageAdded(trimmedContent);

    m_activeRequestKind = ActiveRequestKind::ChatMessage;
    m_chatAutoExecute = true;
    m_lastRequestUserContent = trimmedContent;

    // V12.1 上下文窗口管理 — 检查并压缩
    ContextWindowResult result = m_contextWindowManager.processMessages(
        m_session.messages, m_session.systemPrompt);

    if (result.wasTrimmed) {
        const QString hint = ContextWindowManager::buildCompressionHint(
            result.trimmedRoundCount, result.summary);
        m_session.systemPrompt = hint + QStringLiteral("\n") + m_session.systemPrompt;
        m_session.messages = result.processedMessages;
        emit statusMessage(
            QStringLiteral("Compressed %1 rounds into summary").arg(result.trimmedRoundCount),
            QStringLiteral("已压缩 %1 轮历史对话为摘要").arg(result.trimmedRoundCount),
            3000);
    }

    m_session.addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    m_agentToolCalls.clear();
    m_pendingToolResults.clear();
    m_agentPlanResponseBuffer.clear();
    emit assistantMessageStarted();

    setGenerating(true);

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
    m_aiClient.sendChatWithTools(m_config, m_session, registry.functionToolSchemas(m_config.language));
}

// V12.4: 设置 Chat 模式自动执行工具开关
void ApplicationController::setChatAutoExecute(bool enabled)
{
    m_chatAutoExecute = enabled;
}

// V12.4: 设置高权限模式
void ApplicationController::setHighPermissionMode(bool enabled)
{
    m_highPermissionMode = enabled;
}

// V12.6: 执行一次循环迭代（被 handleRequestFinished 调用）
void ApplicationController::executeAgentLoopIteration()
{
    m_agentLoopIteration++;

    if (m_agentLoopIteration >= kMaxAgentLoopIterations) {
        m_isAgentLoopActive = false;
        emit statusMessage(
            QStringLiteral("Agent loop reached max iterations (%1)").arg(kMaxAgentLoopIterations),
            QStringLiteral("Agent 循环已达最大轮次 (%1)").arg(kMaxAgentLoopIterations),
            5000);
        return;
    }

    emit agentLoopIterationUpdated(m_agentLoopIteration, kMaxAgentLoopIterations);
    continueAgentLoop();
}

// V12.6: 启动下一轮 AI 请求（构建提示词 + 发送）
void ApplicationController::continueAgentLoop()
{
    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();

    // 构建循环提示词
    const QString loopPrompt = AgentLoopPromptBuilder::buildNextActionPrompt(
        m_agentLoopGoal,
        m_agentLoopObservations,
        registry.descriptors(),
        m_config.language,
        m_agentLoopIteration,
        kMaxAgentLoopIterations);

    // V12.6-fix: 准备循环会话，提示词作为系统提示词发送到 API，不显示在聊天中
    ChatSession loopSession = m_session;
    loopSession.systemPrompt = loopPrompt;

    // V13.1: 注入三层记忆到循环提示词
    if (!m_config.agentProjectDirectory.isEmpty()) {
        ProjectMemoryManager memoryMgr(m_config.agentProjectDirectory);
        loopSession.systemPrompt += QStringLiteral("\n\n") + memoryMgr.buildMemorySection();
    }

    emit statusMessage(QStringLiteral("Agent loop iteration %1/%2").arg(m_agentLoopIteration).arg(kMaxAgentLoopIterations),
                       QStringLiteral("Agent 循环 %1/%2").arg(m_agentLoopIteration).arg(kMaxAgentLoopIterations),
                       2000);

    // 添加助手占位
    m_session.addMessage(MessageRole::Assistant, QString());
    m_currentAssistantContent.clear();
    m_agentPlanResponseBuffer.clear();
    m_agentToolCalls.clear();
    m_pendingToolResults.clear();
    emit assistantMessageStarted();

    // V12.1 上下文检查
    ContextWindowResult cwResult = m_contextWindowManager.processMessages(
        loopSession.messages, loopSession.systemPrompt);
    if (cwResult.wasTrimmed) {
        loopSession.messages = cwResult.processedMessages;
    }

    // 发送请求
    m_activeRequestKind = ActiveRequestKind::UnifiedAgent;
    m_nativeToolRequestActive = true;
    m_nativeToolFallbackAttempted = false;

    setGenerating(true);
    m_aiClient.sendChatWithTools(m_config, loopSession,
        registry.functionToolSchemas(m_config.language));
}

void ApplicationController::cancelCurrentRequest()
{
    if (!m_isGenerating) {
        return;
    }

    const ActiveRequestKind requestKind = m_activeRequestKind;
    m_aiClient.cancel();
    setGenerating(false);
    m_pendingToolResults.clear();
    m_chatAutoExecute = false;  // V12.4: 每次请求结束重置

    // V12.6: 清理循环状态
    m_isAgentLoopActive = false;
    m_agentLoopGoal.clear();
    m_agentLoopObservations.clear();
    m_agentLoopIteration = 0;

    if (requestKind == ActiveRequestKind::AgentPlan || requestKind == ActiveRequestKind::UnifiedAgent) {
        if (!m_session.messages.isEmpty() && m_session.messages.last().content.isEmpty()) {
            m_session.messages.last().content = text(QStringLiteral("(Stopped)"), QStringLiteral("（已停止）"));
            emit assistantMessageUpdated(m_session.messages.last().content);
        }
        m_agentPlanResponseBuffer.clear();
        m_agentToolCalls.clear();
        m_pendingAgentRequestSession = ChatSession();
        m_nativeToolRequestActive = false;
        m_nativeToolFallbackAttempted = false;
        m_pendingAgentPlanContinuationDepth = 0;
        m_activeRequestKind = ActiveRequestKind::None;
        saveCurrentSession();
        emit statusMessage(QStringLiteral("Agent generation stopped."),
                           QStringLiteral("Agent 生成已停止。"),
                           2500);
        return;
    }

    if (!m_session.messages.isEmpty() && m_session.messages.last().content.isEmpty()) {
        m_session.messages.last().content = text(QStringLiteral("(Stopped)"), QStringLiteral("（已停止）"));
        emit assistantMessageUpdated(m_session.messages.last().content);
    }
    setRetryAvailable(false);
    saveCurrentSession();
    m_activeRequestKind = ActiveRequestKind::None;
    emit statusMessage(QStringLiteral("Generation stopped."),
                       QStringLiteral("已停止生成。"),
                       2500);
}

// V12.5: 自动执行计划步骤并将结果追加到聊天消息中。
void ApplicationController::executePlanAndReportToChat(const AgentPlan &plan)
{
    const QString &workspace = m_config.agentProjectDirectory;
    QString execSummary;
    int successCount = 0;
    int failCount = 0;

    execSummary += QStringLiteral("\n\n---\n");
    execSummary += text(
        QStringLiteral("**Executing plan (%1 steps)...**\n").arg(plan.steps.size()),
        QStringLiteral("**正在执行计划 (%1 步)...**\n").arg(plan.steps.size()));

    for (int i = 0; i < plan.steps.size(); ++i) {
        const AgentPlanStep &step = plan.steps[i];
        const ToolResult result = AgentPlanExecutor::executeStep(
            step, workspace, workspace);

        const QString status = result.ok
            ? QStringLiteral("\u2705")   // ✅
            : QStringLiteral("\u274C");  // ❌

        const QString desc = step.title.isEmpty()
            ? step.toolId
            : step.title;

        QString resultPreview = result.output;
        // 截断过长输出
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

    // V12.3: 追加流式工具执行结果（来自 handleToolUseBlockComplete）
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
    execSummary += text(
        QStringLiteral("\n**Result**: %1/%2 steps succeeded").arg(successCount).arg(plan.steps.size()),
        QStringLiteral("\n**结果**: %1/%2 步成功").arg(successCount).arg(plan.steps.size()));

    // 追加到当前助手消息
    m_currentAssistantContent += execSummary;
    if (!m_session.messages.isEmpty()) {
        m_session.messages.last().content = m_currentAssistantContent;
    }
    emit assistantMessageUpdated(m_currentAssistantContent);
    saveCurrentSession();

    // 状态栏提示
    emit statusMessage(
        QStringLiteral("Executed %1/%2 steps").arg(successCount).arg(plan.steps.size()),
        QStringLiteral("已执行 %1/%2 步").arg(successCount).arg(plan.steps.size()),
        5000);

    // V13.1: Agent 执行完成后自动追加每日日志
    if (!m_config.agentProjectDirectory.isEmpty()) {
        ProjectMemoryManager memoryMgr(m_config.agentProjectDirectory);
        QString logEntry = QStringLiteral("执行计划: %1/%2 步成功")
            .arg(successCount).arg(plan.steps.size());
        memoryMgr.appendDailyLog(QStringLiteral("log"), logEntry);
    }
}

void ApplicationController::retryLastRequest()
{
    if (m_isGenerating || !m_retryAvailable || m_retryUserContent.trimmed().isEmpty()) {
        return;
    }

    if (!m_config.isComplete()) {
        emit configurationMissing();
        return;
    }

    const QString retryContent = m_retryUserContent;
    setRetryAvailable(false);

    if (!m_session.messages.isEmpty() && m_session.messages.last().role == MessageRole::Assistant) {
        m_session.messages.removeLast();
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
    if (!m_session.messages.isEmpty()) {
        m_session.messages.last().content = m_currentAssistantContent;
    }

    emit assistantMessageUpdated(m_currentAssistantContent);
}

void ApplicationController::handleToolCallsReceived(const ToolCallList &toolCalls)
{
    // V12.4: 允许 Chat 模式（当 m_chatAutoExecute 为 true 时）
    if (m_activeRequestKind != ActiveRequestKind::AgentPlan &&
        m_activeRequestKind != ActiveRequestKind::UnifiedAgent &&
        !(m_activeRequestKind == ActiveRequestKind::ChatMessage && m_chatAutoExecute)) {
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
    // V12.4: 允许 Chat 模式（当 m_chatAutoExecute 为 true 时）
    const bool isToolMode = (m_activeRequestKind == ActiveRequestKind::AgentPlan ||
                             m_activeRequestKind == ActiveRequestKind::UnifiedAgent ||
                             (m_activeRequestKind == ActiveRequestKind::ChatMessage && m_chatAutoExecute));
    if (!isToolMode) {
        return;
    }

    // V12.4: 权限检查 — Chat 模式下需确认的工具需跳过（除非高权限模式）
    if (m_activeRequestKind == ActiveRequestKind::ChatMessage && !m_highPermissionMode) {
        const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
        const AgentToolDefinition *def = registry.findByFunctionName(toolName);
        if (def != nullptr && def->descriptor.requiresUserConfirmation) {
            PendingToolResult skipped;
            skipped.toolName = toolName;
            skipped.arguments = arguments;
            skipped.result = text(
                QStringLiteral("Skipped: This tool requires user confirmation. Enable \"High Permission\" to auto-execute."),
                QStringLiteral("已跳过：此工具需要用户确认。勾选「高权限」可自动执行。"));
            skipped.success = false;
            m_pendingToolResults.append(skipped);

            AppLogger::info(QStringLiteral("StreamingTool"),
                            QStringLiteral("Tool skipped in Chat mode (requires confirmation): %1")
                                .arg(toolName));
            return;
        }
    }

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
    const AgentToolExecutionContext context{
        m_config.agentProjectDirectory,    // workspaceDirectory
        m_config.agentProjectDirectory};   // projectDirectory

    PendingToolResult pending;
    pending.toolName = toolName;
    pending.arguments = arguments;

    // 通过 functionName 查找工具定义并执行
    const AgentToolDefinition *def = registry.findByFunctionName(toolName);
    if (def != nullptr) {
        const ToolResult toolResult = registry.execute(
            def->descriptor.id, arguments, context);
        pending.result = toolResult.output;
        pending.success = toolResult.ok;
    } else {
        pending.result = QStringLiteral("Tool not found: %1").arg(toolName);
        pending.success = false;
    }

    m_pendingToolResults.append(pending);

    AppLogger::info(QStringLiteral("StreamingTool"),
                    QStringLiteral("Tool executed during streaming: %1 (ok=%2)")
                        .arg(toolName)
                        .arg(pending.success ? QStringLiteral("true") : QStringLiteral("false")));
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
                  m_config.language,
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

        // V12.3: 组合流式工具执行结果
        QString streamingResultsSummary;
        if (!m_pendingToolResults.isEmpty()) {
            streamingResultsSummary = QStringLiteral("\n\n[Streaming results (executed early)]");
            for (const PendingToolResult &pending : m_pendingToolResults) {
                streamingResultsSummary += QStringLiteral("\ntool=%1: %2")
                    .arg(pending.toolName, pending.result);
            }
        }
        // V12.5: 不在此处 clear，让 executePlanAndReportToChat 统一处理追加和清理

            AppLogger::info(QStringLiteral("AgentPlan"),
                            QStringLiteral("Agent plan parsed. Auto-executing. steps=%1")
                                .arg(plan.steps.size()));
            // V12.5: 自动执行计划，不再弹出计划窗口
            executePlanAndReportToChat(plan);
            // V12.6: 循环继续判断
            if (m_isAgentLoopActive) {
                QString observation;
                for (const AgentPlanStep &step : plan.steps) {
                    observation += QStringLiteral("[%1] %2\n").arg(step.toolId, step.title);
                }
                m_agentLoopObservations.append(observation);
                executeAgentLoopIteration();
                return;
            }
            // streamingResultsSummary 已在 executePlanAndReportToChat 中追加
            return;
    }

    if (m_activeRequestKind == ActiveRequestKind::UnifiedAgent) {
        setGenerating(false);
        if (!m_agentToolCalls.isEmpty()) {
            const AgentPlanParseResult parseResult = AgentToolCallPlanBuilder::buildPlanFromToolCalls(
                m_agentToolCalls,
                AgentToolRegistryFactory::defaultRegistry(),
                m_config.language,
                AgentPlanParser::DefaultMaxPlanSteps);
            m_agentPlanResponseBuffer.clear();
            m_agentToolCalls.clear();
            m_pendingAgentRequestSession = ChatSession();
            m_nativeToolRequestActive = false;
            m_nativeToolFallbackAttempted = false;
            m_activeRequestKind = ActiveRequestKind::None;

            // V12.3: 组合流式工具执行结果
            QString streamingResultsSummary;
            if (!m_pendingToolResults.isEmpty()) {
                streamingResultsSummary = QStringLiteral("\n\n[Streaming results (executed early)]");
                for (const PendingToolResult &pending : m_pendingToolResults) {
                    streamingResultsSummary += QStringLiteral("\ntool=%1: %2")
                        .arg(pending.toolName, pending.result);
                }
            }
            m_pendingToolResults.clear();

            if (!parseResult.ok) {
                m_currentAssistantContent = text(
                    QStringLiteral("Tool call parsing failed: %1").arg(parseResult.error),
                    QStringLiteral("工具调用解析失败：%1").arg(parseResult.error))
                    + streamingResultsSummary;
                if (!m_session.messages.isEmpty()) {
                    m_session.messages.last().content = m_currentAssistantContent;
                }
                emit assistantMessageUpdated(m_currentAssistantContent);
                saveCurrentSession();
                return;
            }

            m_currentAssistantContent = text(
                QStringLiteral("Agent plan generated. Review before executing steps."),
                QStringLiteral("Agent 计划已生成。请先检查再执行步骤。"))
                + streamingResultsSummary;
            if (!m_session.messages.isEmpty()) {
                m_session.messages.last().content = m_currentAssistantContent;
            }
            emit assistantMessageUpdated(m_currentAssistantContent);
            saveCurrentSession();
            // V12.5: 自动执行计划，不再弹出计划窗口
            executePlanAndReportToChat(parseResult.plan);
            // V12.6: 循环继续判断
            if (m_isAgentLoopActive) {
                QString observation;
                for (const AgentPlanStep &step : parseResult.plan.steps) {
                    observation += QStringLiteral("[%1] %2\n").arg(step.toolId, step.title);
                }
                m_agentLoopObservations.append(observation);
                executeAgentLoopIteration();
                return;
            }
            return;
        }

        const auto response = UnifiedResponseParser::parse(m_agentPlanResponseBuffer, m_config.language);
        m_agentPlanResponseBuffer.clear();
        m_agentToolCalls.clear();
        m_pendingAgentRequestSession = ChatSession();
        m_nativeToolRequestActive = false;
        m_nativeToolFallbackAttempted = false;
        m_activeRequestKind = ActiveRequestKind::None;

        if (response.kind == UnifiedResponseKind::Chat) {
            // V12.3: 组合流式工具执行结果
            QString streamingResultsSummary;
            if (!m_pendingToolResults.isEmpty()) {
                streamingResultsSummary = QStringLiteral("\n\n[Streaming results (executed early)]");
                for (const PendingToolResult &pending : m_pendingToolResults) {
                    streamingResultsSummary += QStringLiteral("\ntool=%1: %2")
                        .arg(pending.toolName, pending.result);
                }
            }
            m_pendingToolResults.clear();

            // V12.6: Chat 响应 = AI 判断任务完成
            if (m_isAgentLoopActive) {
                m_isAgentLoopActive = false;
                m_currentAssistantContent = response.chatMessage + streamingResultsSummary
                    + QStringLiteral("\n\n---\n")
                    + text("Task completed.", "任务完成。");
                if (!m_session.messages.isEmpty()) {
                    m_session.messages.last().content = m_currentAssistantContent;
                }
                emit assistantMessageUpdated(m_currentAssistantContent);
                saveCurrentSession();
                emit statusMessage("Agent loop finished", "Agent 循环完成", 3000);
                return;
            }

            // 聊天回复 → 显示在消息流中
            m_currentAssistantContent = response.chatMessage + streamingResultsSummary;
            if (!m_session.messages.isEmpty()) {
                m_session.messages.last().content = m_currentAssistantContent;
            }
            emit assistantMessageUpdated(m_currentAssistantContent);
            saveCurrentSession();
            AppLogger::info(QStringLiteral("UnifiedAgent"),
                            QStringLiteral("Unified agent returned chat reply."));
            return;
        }

        if (response.kind == UnifiedResponseKind::Plan) {
            // V12.3: 组合流式工具执行结果
            QString streamingResultsSummary;
            if (!m_pendingToolResults.isEmpty()) {
                streamingResultsSummary = QStringLiteral("\n\n[Streaming results (executed early)]");
                for (const PendingToolResult &pending : m_pendingToolResults) {
                    streamingResultsSummary += QStringLiteral("\ntool=%1: %2")
                        .arg(pending.toolName, pending.result);
                }
            }
            m_pendingToolResults.clear();

            // 任务计划 → 解析并执行
            const AgentPlanParseResult parseResult = AgentPlanParser::parseJsonPlan(
                response.planJson, defaultAgentToolCatalog());

            if (!parseResult.ok) {
                // 解析失败 → 显示原始响应 + 错误提示
                m_currentAssistantContent = text(
                    QStringLiteral("Plan parsing failed: %1").arg(parseResult.error),
                    QStringLiteral("计划解析失败：%1").arg(parseResult.error))
                    + streamingResultsSummary;
                if (!m_session.messages.isEmpty()) {
                    m_session.messages.last().content = m_currentAssistantContent;
                }
                emit assistantMessageUpdated(m_currentAssistantContent);
                saveCurrentSession();
                return;
            }

            AppLogger::info(QStringLiteral("UnifiedAgent"),
                            QStringLiteral("Unified agent plan parsed and ready. steps=%1")
                                .arg(parseResult.plan.steps.size()));
            m_currentAssistantContent = text(
                QStringLiteral("Agent plan generated. Review before executing steps."),
                QStringLiteral("Agent 计划已生成。请先检查再执行步骤。"))
                + streamingResultsSummary;
            if (!m_session.messages.isEmpty()) {
                m_session.messages.last().content = m_currentAssistantContent;
            }
            emit assistantMessageUpdated(m_currentAssistantContent);
            saveCurrentSession();
            // V12.5: 自动执行计划，不再弹出计划窗口
            executePlanAndReportToChat(parseResult.plan);
            // V12.6: 循环继续判断
            if (m_isAgentLoopActive) {
                QString observation;
                for (const AgentPlanStep &step : parseResult.plan.steps) {
                    observation += QStringLiteral("[%1] %2\n").arg(step.toolId, step.title);
                }
                m_agentLoopObservations.append(observation);
                executeAgentLoopIteration();
                return;
            }
            return;
        }

        // V12.3: 组合流式工具执行结果（兜底路径也需要）
        QString fallbackStreamingResults;
        if (!m_pendingToolResults.isEmpty()) {
            fallbackStreamingResults = QStringLiteral("\n\n[Streaming results (executed early)]");
            for (const PendingToolResult &pending : m_pendingToolResults) {
                fallbackStreamingResults += QStringLiteral("\ntool=%1: %2")
                    .arg(pending.toolName, pending.result);
            }
        }
        m_pendingToolResults.clear();

        // 解析失败 → 当作聊天回复兜底
        m_currentAssistantContent = response.rawResponse + fallbackStreamingResults;
        if (!m_session.messages.isEmpty()) {
            m_session.messages.last().content = m_currentAssistantContent;
        }
        emit assistantMessageUpdated(m_currentAssistantContent);
        saveCurrentSession();
        return;
    }

    setGenerating(false);
    setRetryAvailable(false);

    // V12.4: Chat 模式 — 追加流式工具执行结果
    if (m_activeRequestKind == ActiveRequestKind::ChatMessage && !m_pendingToolResults.isEmpty()) {
        QString toolResultsBlock;
        toolResultsBlock += QStringLiteral("\n\n---\n");
        toolResultsBlock += text(
            QStringLiteral("**[Auto-Execute Results]**\n"),
            QStringLiteral("**[自动执行结果]**\n"));
        for (const PendingToolResult &pr : m_pendingToolResults) {
            const QString status = pr.success
                ? QStringLiteral("✅")
                : QStringLiteral("⚠️");
            toolResultsBlock += QStringLiteral("%1 **%2**: %3\n")
                .arg(status, pr.toolName, pr.result.left(500));
        }
        m_currentAssistantContent += toolResultsBlock;
        if (!m_session.messages.isEmpty()) {
            m_session.messages.last().content = m_currentAssistantContent;
        }
        emit assistantMessageUpdated(m_currentAssistantContent);
    }
    m_pendingToolResults.clear();
    m_chatAutoExecute = false;  // V12.4: 请求结束重置

    if (!m_session.messages.isEmpty() && m_session.messages.last().content.isEmpty()) {
        m_session.messages.last().content = text(QStringLiteral("(Empty response)"), QStringLiteral("（空回复）"));
        emit assistantMessageUpdated(m_session.messages.last().content);
    }

    saveCurrentSession();
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
        m_currentAssistantContent = text(
            QStringLiteral("Agent request failed: %1").arg(message),
            QStringLiteral("Agent 请求失败：%1").arg(message));
        if (!m_session.messages.isEmpty()) {
            m_session.messages.last().content = m_currentAssistantContent;
        }
        emit assistantMessageUpdated(m_currentAssistantContent);
        saveCurrentSession();
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

    if (!m_session.messages.isEmpty()) {
        m_session.messages.last().content = displayMessage;
    }

    emit assistantMessageUpdated(displayMessage);
    setRetryAvailable(true, m_lastRequestUserContent);
    emit statusMessage(QStringLiteral("Request failed. You can retry the last message."),
                       QStringLiteral("请求失败，可以重试上一条消息。"),
                       5000);
    saveCurrentSession();
    m_agentToolCalls.clear();
    m_pendingAgentRequestSession = ChatSession();
    m_nativeToolRequestActive = false;
    m_nativeToolFallbackAttempted = false;
    m_chatAutoExecute = false;  // V12.4: 失败重置
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
    m_aiClient.sendChat(m_config, m_pendingAgentRequestSession);
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

    const QString summary = text(summaryEnglish, summaryChinese);
    const QString trimmedDetail = detail.trimmed();
    if (trimmedDetail.isEmpty()) {
        return summary;
    }

    return text(QStringLiteral("%1\n\nDetails: %2"), QStringLiteral("%1\n\n详细信息：%2")).arg(summary, trimmedDetail);
}

bool ApplicationController::saveCurrentSession(bool moveToTop)
{
    if (!m_historyAvailable) {
        return false;
    }

    QString error;
    if (!m_chatHistoryStorage.saveSession(m_session, &error)) {
        emit statusMessage(QStringLiteral("Failed to save chat history: %1").arg(error),
                           QStringLiteral("保存聊天记录失败：%1").arg(error),
                           6000);
        return false;
    }

    if (!m_chatHistoryStorage.replaceSessionMessages(m_session, &error)) {
        emit statusMessage(QStringLiteral("Failed to save chat history: %1").arg(error),
                           QStringLiteral("保存聊天记录失败：%1").arg(error),
                           6000);
        return false;
    }

    if (m_sessionSearchQuery.isEmpty()) {
        upsertCurrentSessionSummary(moveToTop);
    } else {
        reloadSessionSummaries(&error);
    }
    emit sessionListChanged();
    return true;
}

bool ApplicationController::reloadSessionSummaries(QString *error)
{
    if (!m_historyAvailable) {
        m_sessionSummaries.clear();
        return false;
    }

    m_sessionSummaries = m_sessionSearchQuery.isEmpty()
                             ? m_chatHistoryStorage.loadSessionSummaries(m_sessionListFilter, error)
                             : m_chatHistoryStorage.searchSessionSummaries(m_sessionSearchQuery, m_sessionListFilter, error);
    return error == nullptr || error->isEmpty();
}

void ApplicationController::upsertCurrentSessionSummary(bool moveToTop)
{
    if (!sessionMatchesCurrentFilter(m_session)) {
        QString error;
        reloadSessionSummaries(&error);
        return;
    }

    SessionSummaryList::upsert(&m_sessionSummaries, m_session, moveToTop);
}

bool ApplicationController::sessionMatchesCurrentFilter(const ChatSession &session) const
{
    return sessionMatchesFilter(session, m_sessionListFilter);
}

bool ApplicationController::hasPersistableCurrentSession() const
{
    const QString title = m_session.title.trimmed();
    const bool hasCustomTitle = !title.isEmpty() && title != QStringLiteral("New Chat");
    return !m_session.messages.isEmpty() || m_session.hasSystemPrompt() || hasCustomTitle;
}

QString ApplicationController::text(const QString &english, const QString &chinese) const
{
    return m_config.language == AppLanguage::English ? english : chinese;
}
