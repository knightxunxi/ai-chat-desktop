#include "ui/ChatView.h"

#include "ui/MessageWidget.h"

#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

ChatView::ChatView(QWidget *parent)
    : QWidget(parent)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName(QStringLiteral("chatScrollArea"));
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentWidget->setObjectName(QStringLiteral("chatContainer"));

    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(24, 20, 24, 20);
    m_contentLayout->setSpacing(15);
    m_contentLayout->addStretch(1);

    m_scrollArea->setWidget(m_contentWidget);
    rootLayout->addWidget(m_scrollArea);
}

MessageWidget *ChatView::addMessage(MessageRole role, const QString &content)
{
    auto *message = new MessageWidget(role, content, m_contentWidget);
    m_contentLayout->insertWidget(m_contentLayout->count() - 1, message);

    if (role == MessageRole::Assistant) {
        m_lastAssistantMessage = message;
    }

    scrollToBottom();
    return message;
}

void ChatView::updateLastAssistantMessage(const QString &content)
{
    if (m_lastAssistantMessage == nullptr) {
        m_lastAssistantMessage = addMessage(MessageRole::Assistant, content);
        return;
    }

    m_lastAssistantMessage->setContent(content);
    scrollToBottom();
}

void ChatView::clearMessages()
{
    while (m_contentLayout->count() > 1) {
        QLayoutItem *item = m_contentLayout->takeAt(0);
        delete item->widget();
        delete item;
    }

    m_lastAssistantMessage = nullptr;
}

int ChatView::messageCount() const
{
    return qMax(0, m_contentLayout->count() - 1);
}

void ChatView::scrollToBottom()
{
    const auto scroll = [this]() {
        m_contentWidget->updateGeometry();
        m_contentLayout->activate();
        QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    };

    QTimer::singleShot(0, this, scroll);
    QTimer::singleShot(30, this, [this, scroll]() {
        if (m_scrollArea == nullptr) {
            return;
        }

        scroll();
    });
}
