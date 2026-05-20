#pragma once

#include "core/MessageRole.h"

#include <QWidget>

class MessageWidget;
class QScrollArea;
class QVBoxLayout;

class ChatView : public QWidget
{
    Q_OBJECT

public:
    explicit ChatView(QWidget *parent = nullptr);

    MessageWidget *addMessage(MessageRole role, const QString &content);
    void updateLastAssistantMessage(const QString &content);
    void clearMessages();

private:
    void scrollToBottom();

    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_contentWidget = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;
    MessageWidget *m_lastAssistantMessage = nullptr;
};
