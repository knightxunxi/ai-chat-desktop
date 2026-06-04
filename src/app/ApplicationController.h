#pragma once

#include "app/AgentPlan.h"
#include "app/ContextWindowManager.h"
#include "app/SummaryAPIClient.h"
#include "core/AppConfig.h"
#include "core/ChatSession.h"
#include "core/PromptTemplate.h"
#include "core/SessionListFilter.h"
#include "services/RequestErrorCategory.h"
#include "services/OpenAICompatibleClient.h"
#include "services/ToolCall.h"
#include "storage/ChatHistoryStorage.h"
#include "storage/ConfigStorage.h"
#include "storage/PromptTemplateStorage.h"

#include <QObject>
#include <QVector>

#include <memory>

// Forward declarations for V13.3
class HookManager;
class SkillManager;
struct SkillDefinition;

// V15.4: MCP 注册表前向声明
class McpRegistry;

// V15.1 调度器
#include "scheduler/ScheduledTask.h"
class TaskScheduler;
class TaskStorage;

// V12.3 流式工具执行 — 待处理的结果缓存
struct PendingToolResult {
    QString toolName;
    QJsonObject arguments;
    QString result;       // 工具执行输出
    bool success = false;
};

// 学习注释：应用控制层，连接 UI、配置存储、聊天历史、提示词模板和 AI 客户端。
// 使用模块：MainWindow 通过信号/槽调用它，避免把业务逻辑直接写在界面类里。
class ApplicationController : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;

    // 功能：加载配置、提示词模板和聊天历史；使用模块：MainWindow 构造时调用。
    void initialize();

    // 功能：读取当前配置；使用模块：MainWindow/SettingsDialog 展示当前模型和语言。
    const AppConfig &config() const;
    // 功能：获取 AI 客户端指针；使用模块：AgentPlanDialog 无限循环模式。
    AIClient *aiClient() { return &m_aiClient; }
    const AIClient *aiClient() const { return &m_aiClient; }
    // 功能：读取当前会话；使用模块：MainWindow 刷新聊天区和角色显示。
    const ChatSession &currentSession() const;
    // 功能：读取会话摘要列表；使用模块：MainWindow 侧边栏列表。
    const QVector<ChatSession> &sessionSummaries() const;
    // 功能：读取当前会话列表筛选条件；使用模块：MainWindow 同步筛选控件。
    SessionListFilter sessionListFilter() const;
    // 功能：读取提示词模板；使用模块：RolePromptDialog 打开时初始化模板列表。
    const QVector<PromptTemplate> &promptTemplates() const;
    // 功能：判断是否正在生成回复；使用模块：MainWindow 控制按钮启用状态。
    bool isGenerating() const;
    // 功能：导出当前会话为 Markdown；使用模块：MainWindow::exportCurrentChat。
    bool exportCurrentSessionMarkdown(const QString &filePath, QString *error = nullptr) const;

public slots:
    // 功能：保存 API、模型和语言配置；使用模块：SettingsDialog 确认后由 MainWindow 调用。
    void saveConfig(const AppConfig &config);
    // 功能：保存角色提示词模板；使用模块：RolePromptDialog 确认后由 MainWindow 调用。
    void savePromptTemplates(const QVector<PromptTemplate> &templates);
    // 功能：设置当前会话的 system prompt；使用模块：角色提示词窗口应用模板或自定义内容。
    void setSystemPrompt(const QString &prompt);
    // 功能：重命名当前会话；使用模块：MainWindow 重命名按钮。
    void renameCurrentSession(const QString &title);
    // 功能：按关键字搜索会话；使用模块：MainWindow 会话搜索框。
    void searchSessions(const QString &query);
    // 功能：切换会话列表筛选条件；使用模块：MainWindow 筛选控件。
    void setSessionListFilter(SessionListFilter filter);
    // 功能：切换当前会话收藏状态；使用模块：MainWindow 收藏按钮。
    void toggleCurrentSessionFavorite();
    // 功能：切换当前会话归档状态；使用模块：MainWindow 归档按钮。
    void toggleCurrentSessionArchived();
    // 功能：创建新会话；使用模块：MainWindow 新建会话按钮。
    void startNewChat();
    // 功能：切换当前会话；使用模块：MainWindow 侧边栏点击会话。
    void switchToSession(const QString &sessionId);
    // 功能：删除当前会话；使用模块：MainWindow 删除按钮。
    void deleteCurrentSession();
    // 功能：发送用户消息并启动 AI 请求；使用模块：MainWindow 发送按钮和快捷键。
    void sendMessage(const QString &content);
    // 功能：取消当前生成；使用模块：停止按钮和窗口关闭流程。
    void cancelCurrentRequest();
    // 功能：重新发送上一条失败请求；使用模块：MainWindow 重试按钮。
    void retryLastRequest();
    // 功能：根据用户目标生成 Agent 结构化计划；使用模块：MainWindow 计划按钮。
    void generateAgentPlan(const QString &goal, int continuationDepth = 0);
    // 功能：发送统一模式消息（AI 自动判断聊天/执行）；使用模块：MainWindow 统一模式切换。
    void sendUnifiedMessage(const QString &content);
    // V12.6: Agent 连续循环执行入口。替代 sendUnifiedMessage 的 Agent 模式单次执行。
    void sendAgentLoopMessage(const QString &content);
    // V12.4: 发送 Chat 模式带工具的消息；使用模块：MainWindow 自动执行按钮。
    void sendMessageWithTools(const QString &content);
    // V12.4: 设置 Chat 模式自动执行工具开关；使用模块：MainWindow 自动执行按钮。
    void setChatAutoExecute(bool enabled);
    // V12.4: 设置高权限模式（允许执行需确认的工具）；使用模块：MainWindow 高权限复选框。
    void setHighPermissionMode(bool enabled);

    // V15.1: 定时任务调度
    void addScheduledTask(const QString &name, const QString &cron, const QString &prompt);
    void removeScheduledTask(const QString &taskId);
    QVector<ScheduledTask> scheduledTasks() const;

signals:
    void configChanged();          // 功能：配置变化通知；使用模块：MainWindow 刷新模型和语言。
    void promptTemplatesChanged(); // 功能：模板变化通知；使用模块：MainWindow 刷新角色显示文案。
    void sessionListChanged();     // 功能：会话列表变化通知；使用模块：MainWindow 重新填充侧边栏。
    void sessionListFilterChanged(); // 功能：会话筛选变化通知；使用模块：MainWindow 同步筛选控件。
    void currentSessionChanged();  // 功能：当前会话变化通知；使用模块：MainWindow 重新渲染聊天区。
    void currentChatCleared();     // 功能：清空起始提示；使用模块：用户发送首条消息时清空默认提示。
    void userMessageAdded(const QString &content);       // 功能：新增用户消息；使用模块：MainWindow 增量添加消息气泡。
    void assistantMessageStarted();                      // 功能：助手回复开始；使用模块：MainWindow 添加“思考中”占位。
    void assistantMessageUpdated(const QString &content); // 功能：助手回复内容变化；使用模块：ChatView 更新最后一条助手消息。
    void generatingChanged(bool generating);             // 功能：生成状态变化；使用模块：MainWindow 切换发送/停止按钮。
    // V12.6: Agent 循环迭代更新。参数: 当前轮次, 总轮次上限。
    void agentLoopIterationUpdated(int iteration, int maxIterations);
    // V13.3: Agent 循环完成后发送技能摘要；使用模块：MainWindow/ChatView 显示技能使用情况。
    void agentLoopSkillSummary(const QString &summary);
    void retryAvailableChanged(bool available);          // 功能：重试可用状态变化；使用模块：MainWindow 显示或隐藏重试按钮。
    void configurationMissing();                         // 功能：缺少必要配置；使用模块：MainWindow 弹出设置提示。
    void statusMessage(const QString &english, const QString &chinese, int timeoutMs); // 功能：状态栏消息；使用模块：MainWindow。
    void startupWarning(const QString &english, const QString &chinese); // 功能：启动期警告；使用模块：MainWindow 延迟弹窗。
    // V12.5: agentPlanReady 信号已移除。计划不再通过弹窗预览，改为自动执行并追加到聊天。

private:
    enum class ActiveRequestKind {
        None,
        ChatMessage,
        AgentPlan,
        UnifiedAgent          // 统一模式：AI 自动判断聊天还是执行
    };

    // 功能：处理流式文本增量；使用模块：OpenAICompatibleClient::textDeltaReceived 信号。
    void handleTextDelta(const QString &delta);
    // 功能：处理原生 Function Calling 工具调用；使用模块：OpenAICompatibleClient::toolCallsReceived 信号。
    void handleToolCallsReceived(const ToolCallList &toolCalls);
    // 功能：处理请求正常结束；使用模块：OpenAICompatibleClient::requestFinished 信号。
    void handleRequestFinished();
    // 功能：处理请求失败；使用模块：OpenAICompatibleClient::requestFailed 信号。
    void handleRequestFailed(const QString &message, RequestErrorCategory category);
    // V12.3: 处理流式工具调用参数完整事件；使用模块：OpenAICompatibleClient::toolUseBlockComplete 信号。
    void handleToolUseBlockComplete(const QString &toolName, const QJsonObject &arguments);
    // V12.5: 自动执行计划步骤并将结果追加到聊天消息；替换原 agentPlanReady 弹窗逻辑。
    void executePlanAndReportToChat(const AgentPlan &plan);
    // 功能：创建助手占位消息并调用 AI 客户端；使用模块：sendMessage/retryLastRequest。
    void startAssistantRequest(const QString &userContentForRetry);
    // 功能：统一切换生成状态并发信号；使用模块：请求开始、完成、失败、取消。
    void setGenerating(bool generating);
    // 功能：统一维护重试按钮状态；使用模块：失败后启用，取消或成功后关闭。
    void setRetryAvailable(bool available, const QString &userContent = QString());
    // 功能：把错误分类转换为用户可读文本；使用模块：handleRequestFailed。
    QString requestFailureMessage(RequestErrorCategory category, const QString &detail) const;
    // 功能：tools 请求不兼容时退回 JSON plan；使用模块：handleRequestFailed。
    bool retryAgentRequestWithoutNativeTools(RequestErrorCategory category);
    // 功能：保存当前会话和消息；使用模块：发送、停止、完成、切换会话前。
    bool saveCurrentSession(bool moveToTop = true);
    // 功能：重新加载会话摘要；使用模块：初始化、搜索、保存后刷新列表。
    bool reloadSessionSummaries(QString *error = nullptr);
    // 功能：把当前会话插入或更新到摘要列表；使用模块：保存会话后维护侧边栏顺序。
    void upsertCurrentSessionSummary(bool moveToTop);
    // 功能：判断会话是否应出现在当前筛选中；使用模块：摘要列表刷新和组织状态切换。
    bool sessionMatchesCurrentFilter(const ChatSession &session) const;
    // 功能：判断当前空会话是否需要保存；使用模块：新建/切换会话前避免保存无意义空白会话。
    bool hasPersistableCurrentSession() const;
    // 功能：根据当前语言选择文案；使用模块：状态消息和错误提示。
    QString text(const QString &english, const QString &chinese) const;

    // V12.1 上下文窗口管理
    // 功能：获取当前模型的上下文窗口 token 上限；使用模块：ContextWindowManager 初始化。
    size_t getContextWindowTokens() const;

    AppConfig m_config;                         // 功能：当前应用配置；使用模块：请求、语言和设置保存。
    ChatSession m_session;                      // 功能：当前会话完整数据；使用模块：聊天区展示和请求上下文。
    QVector<ChatSession> m_sessionSummaries;    // 功能：侧边栏会话摘要；使用模块：MainWindow::populateSessionList。
    SessionListFilter m_sessionListFilter = SessionListFilter::Active; // 功能：会话列表筛选；使用模块：搜索、收藏、归档。
    QVector<PromptTemplate> m_promptTemplates;  // 功能：角色提示词模板缓存；使用模块：RolePromptDialog。
    ConfigStorage m_configStorage;              // 功能：配置读写；使用模块：initialize/saveConfig。
    ChatHistoryStorage m_chatHistoryStorage;    // 功能：聊天历史读写；使用模块：会话保存、搜索、切换、删除。
    PromptTemplateStorage m_promptTemplateStorage; // 功能：模板读写；使用模块：initialize/savePromptTemplates。
    OpenAICompatibleClient m_aiClient;          // 功能：实际网络请求客户端；使用模块：startAssistantRequest/cancelCurrentRequest。
    QString m_currentAssistantContent;          // 功能：流式回复累积内容；使用模块：handleTextDelta。
    QString m_agentPlanResponseBuffer;          // 功能：Agent 计划流式响应缓存；使用模块：计划生成完成后解析 JSON。
    ToolCallList m_agentToolCalls;              // 功能：Agent 原生工具调用缓存；使用模块：V9.2 Function Calling 计划转换。
    ChatSession m_pendingAgentRequestSession;   // 功能：缓存 Agent 请求会话；使用模块：tools 不兼容时降级重试。
    int m_pendingAgentPlanContinuationDepth = 0; // 功能：当前计划请求的继续轮次；使用模块：解析成功后写入 AgentPlan。
    QString m_lastRequestUserContent;           // 功能：记录最近请求的用户文本；使用模块：失败后重试。
    QString m_retryUserContent;                 // 功能：当前可重试的用户文本；使用模块：retryLastRequest。
    QString m_sessionSearchQuery;               // 功能：当前搜索关键字；使用模块：reloadSessionSummaries。
    bool m_historyAvailable = false;            // 功能：历史库是否可用；使用模块：保存/搜索时降级处理。
    bool m_isGenerating = false;                // 功能：是否正在请求 AI；使用模块：按钮状态和操作保护。
    bool m_retryAvailable = false;              // 功能：是否允许重试；使用模块：MainWindow 重试按钮。
    bool m_nativeToolRequestActive = false;      // 功能：当前 Agent 请求是否携带 tools；使用模块：失败时决定是否降级。
    bool m_nativeToolFallbackAttempted = false;  // 功能：是否已经降级重试过；使用模块：防止无限重试。
    ActiveRequestKind m_activeRequestKind = ActiveRequestKind::None; // 功能：区分普通聊天和 Agent 计划请求。
    // V12.1 上下文窗口管理
    ContextWindowManager m_contextWindowManager; // 功能：上下文窗口压缩器；使用模块：请求前检查并压缩上下文。
    SummaryAPIClient m_summaryClient;            // 功能：摘要生成 API 客户端；使用模块：上下文压缩时生成摘要。
    // V12.3 流式工具执行 — 待处理结果缓存
    QVector<PendingToolResult> m_pendingToolResults; // 功能：流式期间执行的工具结果；使用模块：请求结束时组合到响应。
    // V12.4: Chat 模式自动执行工具标记（每次请求重置）
    bool m_chatAutoExecute = false;  // 功能：当前 Chat 请求是否携带工具；使用模块：handleToolCallsReceived/handleToolUseBlockComplete 路由判断。
    bool m_highPermissionMode = false; // 功能：是否允许执行需确认的高权限工具；使用模块：handleToolUseBlockComplete 权限检查。

    // V12.6: Agent 连续循环执行状态
    bool m_isAgentLoopActive = false;
    QString m_agentLoopGoal;
    QStringList m_agentLoopObservations;
    int m_agentLoopIteration = 0;
    static constexpr int kMaxAgentLoopIterations = 50;

    // V13.3: Skills + Hooks 系统
    std::unique_ptr<SkillManager> m_skillManager;
    std::unique_ptr<HookManager> m_hookManager;
    QVector<SkillDefinition> m_matchedSkills;

    // V15.4: MCP 注册表
    std::unique_ptr<McpRegistry> m_mcpRegistry;

    // V15.1: 调度器
    std::unique_ptr<TaskScheduler> m_taskScheduler;
    TaskStorage *m_taskStorage = nullptr;
    // V15.2: 任务触发后持久化状态
    void onScheduledTaskTriggered(const ScheduledTask &task);

    // V12.6: 执行一次循环迭代（被 handleRequestFinished 调用）
    void executeAgentLoopIteration();
    // V12.6: 启动下一轮 AI 请求（构建提示词 + 发送）
    void continueAgentLoop();

    // V12.4: 测试友元 — ChatToolExecutionTest 需要访问私有成员和 slot
    friend struct ChatToolExecutionTestAccessor;
    // V12.6: 测试友元 — AgentLoopExecutionTest 需要访问私有成员
    friend struct AgentLoopExecutionTestAccessor;
};
