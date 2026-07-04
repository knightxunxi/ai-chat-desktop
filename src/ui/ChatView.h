#pragma once

#include "core/MessageRole.h"

#include <QHash>
#include <QWidget>

class AgentStepWidget;
class MessageWidget;
class QLabel;
class QLineEdit;
class QScrollArea;
class QVBoxLayout;
class QPushButton;
class TokenBar;
class TypingIndicator;

// 学习注释：聊天消息容器，负责按顺序排列 MessageWidget 并在新增/更新时滚动到底部。
// 使用模块：MainWindow 持有它，ApplicationController 的消息信号最终会驱动它刷新。
class ChatView : public QWidget
{
    Q_OBJECT

public:
    explicit ChatView(QWidget *parent = nullptr);

    // 功能：添加一条消息气泡；使用模块：MainWindow::addUserMessage/populateChatView。
    MessageWidget *addMessage(MessageRole role, const QString &content);
    // CH-8: 带 messageId 的重载；使用模块：MainWindow 建立 messageId 映射。
    MessageWidget *addMessage(MessageRole role, const QString &content, const QString &messageId);
    // 功能：更新最后一条助手消息；使用模块：流式回复刷新。
    void updateLastAssistantMessage(const QString &content);
    // 功能：清空全部消息；使用模块：切换会话、开始首条消息前清默认提示。
    void clearMessages();
    // 功能：返回当前消息数量；使用模块：测试和状态判断。
    int messageCount() const;
    // V16.1: 在消息列表中插入 Agent 步骤卡片
    void addAgentStepWidget(AgentStepWidget *widget);
    // V16.3: 添加调试信息卡片；使用模块：MainWindow::onAgentDebugPrompt。
    void addDebugCard(const QString &title, const QString &content);
    // CH-8: 按 messageId 查找 MessageWidget；使用模块：MainWindow 编辑确认后查找。
    MessageWidget *widgetForMessageId(const QString &messageId);
    // CH-8: 移除指定消息及之后的所有消息；使用模块：MainWindow::onMessageEditConfirmed。
    void removeMessagesFrom(const QString &messageId);
    // V16.1: 获取内容 widget（供 MainWindow 添加自定义 widget）
    QWidget *getContentWidget() const { return m_contentWidget; }

    // V17.3: Token 用量更新
    void updateTokenUsage(int used, int limit);
    // V17.4: 搜索栏
    void showSearchBar();
    void hideSearchBar();
    // V17.4: 打字指示器
    void showTyping();
    void hideTyping();

    // V26: 在指定消息下方添加分支指示器按钮
    QPushButton *addBranchIndicator(const QString &messageId, int branchCount, int currentIndex);
    // V26: 添加分支消息预览
    MessageWidget *addBranchMessage(MessageRole role, const QString &content, const QString &branchId);

private:
    // 功能：滚动到最新消息；使用模块：addMessage/updateLastAssistantMessage 内部调用。
    void scrollToBottom();
    // V17.4: 执行搜索并高亮匹配项
    void performSearch(const QString &query);
    void navigateMatch(int direction); // +1 下一项，-1 上一项

    QScrollArea *m_scrollArea = nullptr;             // 功能：提供滚动容器；使用模块：聊天主视图。
    QWidget *m_contentWidget = nullptr;              // 功能：承载消息布局；使用模块：QScrollArea 内部 widget。
    QVBoxLayout *m_contentLayout = nullptr;          // 功能：纵向排列消息；使用模块：addMessage。
    MessageWidget *m_lastAssistantMessage = nullptr; // 功能：缓存最后一条助手消息；使用模块：流式更新。
    // CH-8: messageId → MessageWidget 映射；使用模块：removeMessagesFrom/widgetForMessageId。
    QHash<QString, MessageWidget *> m_messageWidgets;

    // V17.3: Token 用量条
    TokenBar *m_tokenBar = nullptr;
    // V17.4: 搜索栏
    QLineEdit *m_searchEdit = nullptr;
    QString m_searchQuery;
    int m_currentMatchIndex = -1;
    QVector<QLabel *> m_matchedLabels;
    // V17.4: 打字指示器
    TypingIndicator *m_typingIndicator = nullptr;
};
