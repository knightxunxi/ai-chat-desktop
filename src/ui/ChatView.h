#pragma once

#include "core/MessageRole.h"

#include <QWidget>

class MessageWidget;
class QScrollArea;
class QVBoxLayout;

// 学习注释：聊天消息容器，负责按顺序排列 MessageWidget 并在新增/更新时滚动到底部。
// 使用模块：MainWindow 持有它，ApplicationController 的消息信号最终会驱动它刷新。
class ChatView : public QWidget
{
    Q_OBJECT

public:
    explicit ChatView(QWidget *parent = nullptr);

    // 功能：添加一条消息气泡；使用模块：MainWindow::addUserMessage/populateChatView。
    MessageWidget *addMessage(MessageRole role, const QString &content);
    // 功能：更新最后一条助手消息；使用模块：流式回复刷新。
    void updateLastAssistantMessage(const QString &content);
    // 功能：清空全部消息；使用模块：切换会话、开始首条消息前清默认提示。
    void clearMessages();
    // 功能：返回当前消息数量；使用模块：测试和状态判断。
    int messageCount() const;

private:
    // 功能：滚动到最新消息；使用模块：addMessage/updateLastAssistantMessage 内部调用。
    void scrollToBottom();

    QScrollArea *m_scrollArea = nullptr;             // 功能：提供滚动容器；使用模块：聊天主视图。
    QWidget *m_contentWidget = nullptr;              // 功能：承载消息布局；使用模块：QScrollArea 内部 widget。
    QVBoxLayout *m_contentLayout = nullptr;          // 功能：纵向排列消息；使用模块：addMessage。
    MessageWidget *m_lastAssistantMessage = nullptr; // 功能：缓存最后一条助手消息；使用模块：流式更新。
};
