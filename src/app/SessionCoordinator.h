#pragma once

#include "core/ChatSession.h"
#include "core/SessionListFilter.h"
#include "storage/ChatHistoryStorage.h"

#include <QObject>
#include <QVector>

// 学习注释：会话协调器，集中管理聊天会话的生命周期、持久化和列表展示。
// 从 ApplicationController 分离，遵循单一职责原则。
class SessionCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit SessionCoordinator(QObject *parent = nullptr);

    // 功能：初始化聊天历史存储、加载会话列表和最近会话；使用模块：ApplicationController::initialize。
    void initialize();

    // 功能：读取当前会话（const，公开 API）；使用模块：MainWindow 刷新聊天区和角色显示。
    const ChatSession &currentSession() const;
    // 功能：读取当前会话（非 const，内部使用）；使用模块：ApplicationController 内部修改会话。
    ChatSession &currentSession();
    // 功能：读取会话摘要列表；使用模块：MainWindow 侧边栏列表。
    const QVector<ChatSession> &sessionSummaries() const;
    // 功能：读取当前会话列表筛选条件；使用模块：MainWindow 同步筛选控件。
    SessionListFilter sessionListFilter() const;
    // 功能：获取聊天历史存储指针；使用模块：AgentOrchestrator 等需要直接访问存储的组件。
    ChatHistoryStorage *chatHistoryStorage();
    const ChatHistoryStorage *chatHistoryStorage() const;
    // 功能：读取历史库是否可用；使用模块：ApplicationController 内部判断。
    bool isHistoryAvailable() const;
    // 功能：创建新会话；使用模块：ApplicationController::startNewChat。
    void createNewChat();
    // 功能：切换当前会话；使用模块：ApplicationController::switchToSession。
    bool switchToSession(const QString &sessionId);
    // 功能：删除当前会话；使用模块：ApplicationController::deleteCurrentSession。
    void deleteCurrentSession();
    // 功能：按关键字搜索会话；使用模块：MainWindow 会话搜索框。
    void searchSessions(const QString &query);
    // 功能：切换会话列表筛选条件；使用模块：MainWindow 筛选控件。
    void setSessionListFilter(SessionListFilter filter);
    // 功能：切换当前会话收藏状态；使用模块：MainWindow 收藏按钮。
    void toggleCurrentSessionFavorite();
    // 功能：切换当前会话归档状态；使用模块：MainWindow 归档按钮。
    void toggleCurrentSessionArchived();
    // 功能：重命名当前会话；使用模块：MainWindow 重命名按钮。
    bool renameCurrentSession(const QString &title);
    // 功能：设置当前会话的 system prompt；使用模块：角色提示词窗口应用模板或自定义内容。
    bool setSystemPrompt(const QString &prompt);
    // 功能：导出当前会话为 Markdown；使用模块：MainWindow::exportCurrentChat。
    bool exportCurrentSessionMarkdown(const QString &filePath, QString *error = nullptr) const;
    // 功能：保存当前会话；使用模块：ApplicationController 多处调用。
    bool saveCurrentSession(bool moveToTop = true);
    // 功能：编辑当前会话中指定消息的内容；使用模块：MainWindow::onMessageEditConfirmed。
    bool editCurrentMessage(const QString &messageId, const QString &newContent);
    // 功能：截断当前会话指定消息之后的所有消息；使用模块：MainWindow::onMessageEditConfirmed。
    void truncateCurrentSessionFrom(const QString &messageId);
    // 功能：为当前会话创建对话分支；使用模块：MainWindow。
    void createMessageBranch(const QString &parentMessageId);
    // 功能：把当前会话插入或更新到摘要列表（暴露给 AC 用于首条消息时更新侧边栏）。
    void upsertCurrentSessionSummary(bool moveToTop);

signals:
    void sessionListChanged();       // 功能：会话列表变化通知；使用模块：ApplicationController 转发到 MainWindow。
    void sessionListFilterChanged(); // 功能：会话筛选变化通知；使用模块：ApplicationController 转发到 MainWindow。
    void currentSessionChanged();    // 功能：当前会话变化通知；使用模块：ApplicationController 转发到 MainWindow。
    void currentChatCleared();       // 功能：清空起始提示；使用模块：用户发送首条消息时清空默认提示。
    void statusMessage(const QString &english, const QString &chinese, int timeoutMs); // 功能：状态栏消息；使用模块：ApplicationController 转发到 MainWindow。
    void startupWarning(const QString &english, const QString &chinese); // 功能：启动期警告；使用模块：ApplicationController 转发到 MainWindow。

private:
    // 功能：重新加载会话摘要；使用模块：初始化、搜索、保存后刷新列表。
    bool reloadSessionSummaries(QString *error = nullptr);
    // 功能：判断会话是否应出现在当前筛选中；使用模块：摘要列表刷新和组织状态切换。
    bool sessionMatchesCurrentFilter(const ChatSession &session) const;
    // 功能：判断当前空会话是否需要保存；使用模块：新建/切换会话前避免保存无意义空白会话。
    bool hasPersistableCurrentSession() const;

    ChatSession m_session;                      // 功能：当前会话完整数据；使用模块：聊天区展示和请求上下文。
    QVector<ChatSession> m_sessionSummaries;    // 功能：侧边栏会话摘要；使用模块：MainWindow::populateSessionList。
    SessionListFilter m_sessionListFilter = SessionListFilter::Active; // 功能：会话列表筛选；使用模块：搜索、收藏、归档。
    QString m_sessionSearchQuery;               // 功能：当前搜索关键字；使用模块：reloadSessionSummaries。
    ChatHistoryStorage m_chatHistoryStorage;    // 功能：聊天历史读写；使用模块：会话保存、搜索、切换、删除。
    bool m_historyAvailable = false;            // 功能：历史库是否可用；使用模块：保存/搜索时降级处理。
};
