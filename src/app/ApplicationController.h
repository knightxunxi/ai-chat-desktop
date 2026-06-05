#pragma once

#include "app/AgentOrchestrator.h"
#include "app/AgentPlan.h"
#include "app/ConfigCoordinator.h"
#include "app/SessionCoordinator.h"
#include "core/AppConfig.h"
#include "core/ChatSession.h"
#include "core/PromptTemplate.h"
#include "core/SessionListFilter.h"
#include "services/RequestErrorCategory.h"
#include "services/OpenAICompatibleClient.h"
#include "services/ToolCall.h"

#include <QObject>
#include <QVector>

#include <memory>

// V15.1 调度器
#include "scheduler/ScheduledTask.h"
class TaskScheduler;
class TaskStorage;

// 学习注释：应用控制层，连接 UI、配置存储、聊天历史、提示词模板和 AI 客户端。
// 使用模块：MainWindow 通过信号/槽调用它，避免把业务逻辑直接写在界面类里。
// R1 重构后：ApplicationController 专注于 Chat 消息处理 + 协调各子协调器。
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
    // 功能：获取配置协调器指针；使用模块：外部组件直接访问配置和 text() 方法。
    const ConfigCoordinator *configCoordinator() const { return &m_configCoordinator; }
    ConfigCoordinator *configCoordinator() { return &m_configCoordinator; }
    // 功能：获取会话协调器指针；使用模块：外部组件访问会话管理功能。
    const SessionCoordinator *sessionCoordinator() const { return &m_sessionCoordinator; }
    SessionCoordinator *sessionCoordinator() { return &m_sessionCoordinator; }
    // 功能：获取 Agent 编排器指针；使用模块：外部组件访问 Agent 循环控制。
    const AgentOrchestrator *agentOrchestrator() const { return &m_agentOrchestrator; }
    AgentOrchestrator *agentOrchestrator() { return &m_agentOrchestrator; }
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
    // V17.1: 发送带图片附件的用户消息；使用模块：MainWindow 粘贴图片后发送。
    void sendMessageWithImages(const QString &content, const QStringList &imageBase64List);
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
    // V16.3: Agent 调试模式开关；使用模块：MainWindow 调试复选框（已移至设置界面）。
    void setAgentDebugMode(bool enabled);
    bool agentDebugMode() const;
    // V17.2: Agent 会话恢复
    bool hasPendingAgentState() const;
    AgentLoopState pendingAgentState() const;
    void resumeAgentLoop();
    void clearAgentLoopState();
    // V17.4: 为当前会话创建对话分支
    void createMessageBranch(const QString &parentMessageId);
    // CH-8: 公开保存会话接口；使用模块：MainWindow::onMessageEditConfirmed。
    bool saveCurrentSession(bool moveToTop = true);
    // CH-8: 编辑当前会话中指定消息的内容；使用模块：MainWindow::onMessageEditConfirmed。
    bool editCurrentMessage(const QString &messageId, const QString &newContent);
    // CH-8: 截断当前会话指定消息之后的所有消息；使用模块：MainWindow::onMessageEditConfirmed。
    void truncateCurrentSessionFrom(const QString &messageId);

    // V15.1: 定时任务调度
    void addScheduledTask(const QString &name, const QString &cron, const QString &prompt);
    void removeScheduledTask(const QString &taskId);
    void updateScheduledTask(const ScheduledTask &task);
    QVector<ScheduledTask> scheduledTasks() const;

signals:
    void configChanged();          // 功能：配置变化通知；使用模块：MainWindow 刷新模型和语言。
    void promptTemplatesChanged(); // 功能：模板变化通知；使用模块：MainWindow 刷新角色显示文案。
    void sessionListChanged();     // 功能：会话列表变化通知；使用模块：MainWindow 重新填充侧边栏。
    void sessionListFilterChanged(); // 功能：会话筛选变化通知；使用模块：MainWindow 同步筛选控件。
    void currentSessionChanged();  // 功能：当前会话变化通知；使用模块：MainWindow 重新渲染聊天区。
    void currentChatCleared();     // 功能：清空起始提示；使用模块：用户发送首条消息时清空默认提示。
    void userMessageAdded(const QString &content);       // 功能：新增用户消息；使用模块：MainWindow 增量添加消息气泡。
    void assistantMessageStarted();                      // 功能：助手回复开始；使用模块：MainWindow 添加"思考中"占位。
    void assistantMessageUpdated(const QString &content); // 功能：助手回复内容变化；使用模块：ChatView 更新最后一条助手消息。
    void generatingChanged(bool generating);             // 功能：生成状态变化；使用模块：MainWindow 切换发送/停止按钮。
    // V12.6: Agent 循环迭代更新。参数: 当前轮次, 总轮次上限。
    void agentLoopIterationUpdated(int iteration, int maxIterations);
    // V13.3: Agent 循环完成后发送技能摘要；使用模块：MainWindow/ChatView 显示技能使用情况。
    void agentLoopSkillSummary(const QString &summary);
    // V16.1: Agent 每步思考过程通知
    void agentLoopThought(int iteration, const QString &reasoning, const QString &toolId, const QString &title);
    // V16.1: 工具执行完成通知
    void agentLoopToolFinished(int iteration, const QString &toolId, bool ok, const QString &outputPreview);
    // V16.3: Agent 调试 — 暴露完整的循环提示词
    void agentLoopPromptDebug(const QString &prompt);
    void retryAvailableChanged(bool available);          // 功能：重试可用状态变化；使用模块：MainWindow 显示或隐藏重试按钮。
    void configurationMissing();                         // 功能：缺少必要配置；使用模块：MainWindow 弹出设置提示。
    void statusMessage(const QString &english, const QString &chinese, int timeoutMs); // 功能：状态栏消息；使用模块：MainWindow。
    void startupWarning(const QString &english, const QString &chinese); // 功能：启动期警告；使用模块：MainWindow 延迟弹窗。
    // V17.3: Token 用量更新通知；使用模块：MainWindow 更新 TokenBar。
    void tokenUsageUpdated(int used, int limit);
    // V12.5: agentPlanReady 信号已移除。计划不再通过弹窗预览，改为自动执行并追加到聊天。
    // V12.6: Agent 循环完成通知
    void agentLoopCompleted();

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
    // 功能：启动下一轮 Agent 循环 AI 请求（构建提示词 + 发送）。
    void continueAgentLoop();

    ConfigCoordinator m_configCoordinator;        // 功能：配置管理协调器。
    SessionCoordinator m_sessionCoordinator;      // 功能：会话管理协调器。
    AgentOrchestrator m_agentOrchestrator;        // 功能：Agent 编排器。
    OpenAICompatibleClient m_aiClient;            // 功能：实际网络请求客户端。
    QString m_currentAssistantContent;            // 功能：流式回复累积内容。
    QString m_agentPlanResponseBuffer;            // 功能：Agent 计划流式响应缓存。
    ToolCallList m_agentToolCalls;                // 功能：Agent 原生工具调用缓存。
    ChatSession m_pendingAgentRequestSession;     // 功能：缓存 Agent 请求会话。
    int m_pendingAgentPlanContinuationDepth = 0;  // 功能：当前计划请求的继续轮次。
    QString m_lastRequestUserContent;             // 功能：记录最近请求的用户文本。
    QString m_retryUserContent;                   // 功能：当前可重试的用户文本。
    bool m_isGenerating = false;                  // 功能：是否正在请求 AI。
    bool m_retryAvailable = false;                // 功能：是否允许重试。
    bool m_nativeToolRequestActive = false;       // 功能：当前 Agent 请求是否携带 tools。
    bool m_nativeToolFallbackAttempted = false;   // 功能：是否已经降级重试过。
    ActiveRequestKind m_activeRequestKind = ActiveRequestKind::None;

    // V17.1: 待发送图片列表
    QJsonArray m_pendingImages;

    // V15.1: 调度器
    std::unique_ptr<TaskScheduler> m_taskScheduler;
    TaskStorage *m_taskStorage = nullptr;
    // V15.2: 任务触发后持久化状态
    void onScheduledTaskTriggered(const ScheduledTask &task);

    // V12.4: 测试友元 — ChatToolExecutionTest 需要访问私有成员和 slot
    friend struct ChatToolExecutionTestAccessor;
    // V12.6: 测试友元 — AgentLoopExecutionTest 需要访问私有成员
    friend struct AgentLoopExecutionTestAccessor;
};
