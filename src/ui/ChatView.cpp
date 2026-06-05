#include "ui/ChatView.h"

#include "ui/AgentStepWidget.h"
#include "ui/MessageWidget.h"
#include "ui/TokenBar.h"
#include "ui/TypingIndicator.h"

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
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

    // V17.4: 打字指示器（滚动区域和 token bar 之间）
    m_typingIndicator = new TypingIndicator(this);

    rootLayout->addWidget(m_scrollArea);
    rootLayout->addWidget(m_typingIndicator);

    // V17.3: Token 用量条
    m_tokenBar = new TokenBar(this);
    rootLayout->addWidget(m_tokenBar);
}

MessageWidget *ChatView::addMessage(MessageRole role, const QString &content)
{
    return addMessage(role, content, QString());
}

MessageWidget *ChatView::addMessage(MessageRole role, const QString &content, const QString &messageId)
{
    auto *message = new MessageWidget(role, content, m_contentWidget);
    if (!messageId.isEmpty()) {
        message->setProperty("messageId", messageId);
        m_messageWidgets.insert(messageId, message);
    }
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

    // CH-1: 使用增量更新替代全量重建
    m_lastAssistantMessage->updateContentIncremental(content);
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
    m_messageWidgets.clear();
}

int ChatView::messageCount() const
{
    return qMax(0, m_contentLayout->count() - 1);
}

void ChatView::addAgentStepWidget(AgentStepWidget *widget)
{
    m_contentLayout->insertWidget(m_contentLayout->count() - 1, widget);
    scrollToBottom();
}

// ─── V16.3: addDebugCard ────────────────────────────────────────────
void ChatView::addDebugCard(const QString &title, const QString &content)
{
    auto *card = new QFrame(m_contentWidget);
    card->setObjectName(QStringLiteral("debugCard"));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(6);

    auto *toggleBtn = new QPushButton(QStringLiteral("\u25BC ") + title, card);
    toggleBtn->setObjectName(QStringLiteral("debugCardToggle"));
    toggleBtn->setFlat(true);
    toggleBtn->setCursor(Qt::PointingHandCursor);

    auto *body = new QLabel(content, card);
    body->setObjectName(QStringLiteral("debugCardBody"));
    body->setWordWrap(true);
    body->setTextFormat(Qt::PlainText);
    body->setTextInteractionFlags(Qt::TextSelectableByMouse);
    body->setMaximumHeight(200);

    connect(toggleBtn, &QPushButton::clicked, card, [toggleBtn, body]() {
        bool visible = body->isVisible();
        body->setVisible(!visible);
        // 截取原标题并切换箭头
        QString t = toggleBtn->text().mid(2); // 去掉箭头
        toggleBtn->setText((visible ? QStringLiteral("\u25B6 ") : QStringLiteral("\u25BC ")) + t);
    });

    layout->addWidget(toggleBtn);
    layout->addWidget(body);

    m_contentLayout->insertWidget(m_contentLayout->count() - 1, card);
    scrollToBottom();
}

// ─── CH-8: widgetForMessageId ────────────────────────────────────────
MessageWidget *ChatView::widgetForMessageId(const QString &messageId)
{
    return m_messageWidgets.value(messageId, nullptr);
}

// ─── CH-8: removeMessagesFrom ────────────────────────────────────────
void ChatView::removeMessagesFrom(const QString &messageId)
{
    bool found = false;
    QVector<QWidget *> toRemove;

    // 从 contentLayout 中收集需要删除的 widget
    for (int i = 0; i < m_contentLayout->count(); ++i) {
        auto *item = m_contentLayout->itemAt(i);
        if (item == nullptr || item->widget() == nullptr) {
            continue;
        }

        auto *msg = qobject_cast<MessageWidget *>(item->widget());
        if (msg == nullptr) {
            if (found) {
                toRemove.append(item->widget());
            }
            continue;
        }

        QString mid = msg->property("messageId").toString();
        if (mid == messageId) {
            found = true;
            toRemove.append(msg);
        } else if (found) {
            toRemove.append(msg);
        }
    }

    for (auto *w : toRemove) {
        m_contentLayout->removeWidget(w);
        m_messageWidgets.remove(w->property("messageId").toString());
        // 如果当前删除的是缓存的最后一条助手消息，则清空缓存
        if (w == m_lastAssistantMessage) {
            m_lastAssistantMessage = nullptr;
        }
        w->deleteLater();
    }
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

// ─── V17.3: Token 用量更新 ──────────────────────────────────────────

void ChatView::updateTokenUsage(int used, int limit)
{
    if (m_tokenBar != nullptr) {
        m_tokenBar->updateTokens(used, limit);
    }
}

// ─── V17.4: 搜索栏 ──────────────────────────────────────────────────

void ChatView::showSearchBar()
{
    if (m_searchEdit != nullptr) {
        m_searchEdit->setVisible(true);
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
        return;
    }

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName(QStringLiteral("chatSearchBar"));
    m_searchEdit->setPlaceholderText(QStringLiteral("Search in conversation..."));
    m_searchEdit->setFixedHeight(32);

    // 插入到布局顶部（在 scrollArea 之前）
    auto *rootLayout = qobject_cast<QVBoxLayout *>(layout());
    if (rootLayout != nullptr) {
        rootLayout->insertWidget(0, m_searchEdit);
    }

    connect(m_searchEdit, &QLineEdit::textChanged, this, &ChatView::performSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() {
        navigateMatch(+1);
    });

    // Escape 关闭
    auto *escShortcut = new QShortcut(QKeySequence(QStringLiteral("Escape")), m_searchEdit);
    connect(escShortcut, &QShortcut::activated, this, &ChatView::hideSearchBar);

    // 关闭按钮
    m_searchEdit->setClearButtonEnabled(true);

    m_searchEdit->setFocus();
}

void ChatView::hideSearchBar()
{
    // 清除高亮
    for (QLabel *label : m_matchedLabels) {
        label->setStyleSheet(QString());
    }
    m_matchedLabels.clear();
    m_searchQuery.clear();
    m_currentMatchIndex = -1;

    if (m_searchEdit != nullptr) {
        m_searchEdit->setVisible(false);
        m_searchEdit->deleteLater();
        m_searchEdit = nullptr;
    }
}

void ChatView::performSearch(const QString &query)
{
    // 清除之前的高亮
    for (QLabel *label : m_matchedLabels) {
        label->setStyleSheet(QString());
    }
    m_matchedLabels.clear();
    m_searchQuery = query;
    m_currentMatchIndex = -1;

    if (query.isEmpty()) {
        return;
    }

    // 遍历所有 QLabel 查找匹配
    QList<QLabel *> allLabels = m_contentWidget->findChildren<QLabel *>();
    for (QLabel *label : allLabels) {
        if (label->text().contains(query, Qt::CaseInsensitive)) {
            m_matchedLabels.append(label);
        }
    }

    // 高亮第一个匹配项
    if (!m_matchedLabels.isEmpty()) {
        m_currentMatchIndex = 0;
        m_matchedLabels[0]->setStyleSheet(
            QStringLiteral("background-color: #fef08a; color: #000000;"));
        // 滚动到可见
        m_scrollArea->ensureWidgetVisible(m_matchedLabels[0], 100, 100);
    }
}

void ChatView::navigateMatch(int direction)
{
    if (m_matchedLabels.isEmpty()) {
        return;
    }

    // 清除当前高亮
    if (m_currentMatchIndex >= 0 && m_currentMatchIndex < m_matchedLabels.size()) {
        m_matchedLabels[m_currentMatchIndex]->setStyleSheet(QString());
    }

    // 计算新索引
    m_currentMatchIndex += direction;
    if (m_currentMatchIndex >= m_matchedLabels.size()) {
        m_currentMatchIndex = 0;
    } else if (m_currentMatchIndex < 0) {
        m_currentMatchIndex = m_matchedLabels.size() - 1;
    }

    // 高亮新项
    m_matchedLabels[m_currentMatchIndex]->setStyleSheet(
        QStringLiteral("background-color: #f59e0b; color: #000000;"));
    m_scrollArea->ensureWidgetVisible(m_matchedLabels[m_currentMatchIndex], 100, 100);
}

// ─── V17.4: 打字指示器 ──────────────────────────────────────────────

void ChatView::showTyping()
{
    if (m_typingIndicator != nullptr) {
        m_typingIndicator->startAnimation();
    }
}

void ChatView::hideTyping()
{
    if (m_typingIndicator != nullptr) {
        m_typingIndicator->stopAnimation();
    }
}
