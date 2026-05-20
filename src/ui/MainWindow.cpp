#include "ui/MainWindow.h"

#include "ui/ChatView.h"
#include "ui/SettingsDialog.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    loadConfig();
    loadSession();
    setupUi();
    populateSessionList();
    populateChatView();
    applyConfig();
    applyLanguage();
    updateSendButtonState();
    showStartupWarningIfNeeded();

    connect(&m_aiClient, &OpenAICompatibleClient::textDeltaReceived, this, &MainWindow::handleTextDelta);
    connect(&m_aiClient, &OpenAICompatibleClient::requestFinished, this, &MainWindow::handleRequestFinished);
    connect(&m_aiClient, &OpenAICompatibleClient::requestFailed, this, &MainWindow::handleRequestFailed);
}

void MainWindow::loadConfig()
{
    m_config = m_configStorage.load();
}

void MainWindow::loadSession()
{
    QString error;
    if (!m_chatHistoryStorage.initialize(&error)) {
        m_session = ChatSession::createDefault();
        m_sessionSummaries.clear();
        m_historyAvailable = false;
        m_startupWarningMessage = text(QStringLiteral("Chat history is unavailable. New messages will not be restored after restart.\n\n%1"),
                                       QStringLiteral("聊天记录不可用。新消息在重启后将无法恢复。\n\n%1"))
                                      .arg(error);
        return;
    }

    m_historyAvailable = true;
    m_sessionSummaries = m_chatHistoryStorage.loadSessionSummaries(&error);
    if (m_sessionSummaries.isEmpty()) {
        m_session = ChatSession::createDefault();
    } else {
        const std::optional<ChatSession> latestSession = m_chatHistoryStorage.loadSession(m_sessionSummaries.first().id, &error);
        m_session = latestSession.value_or(ChatSession::createDefault());
        if (!latestSession.has_value() && !error.isEmpty()) {
            m_startupWarningMessage = text(QStringLiteral("Failed to load the latest chat history.\n\n%1"),
                                           QStringLiteral("加载最近聊天记录失败。\n\n%1"))
                                          .arg(error);
        }
    }

    if (m_sessionSummaries.isEmpty() && !error.isEmpty()) {
        m_startupWarningMessage = text(QStringLiteral("Failed to load the latest chat history.\n\n%1"),
                                       QStringLiteral("加载最近聊天记录失败。\n\n%1"))
                                      .arg(error);
    }
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("AI Chat Desktop"));
    resize(1120, 760);
    setMinimumSize(860, 560);

    auto *central = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *sidebar = new QFrame(central);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(248);

    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 16, 16, 16);
    sidebarLayout->setSpacing(12);

    m_newChatButton = new QPushButton(sidebar);
    m_newChatButton->setObjectName(QStringLiteral("newChatButton"));

    m_sessionList = new QListWidget(sidebar);
    m_sessionList->setObjectName(QStringLiteral("sessionList"));

    sidebarLayout->addWidget(m_newChatButton);
    sidebarLayout->addWidget(m_sessionList, 1);

    auto *mainPanel = new QWidget(central);
    mainPanel->setObjectName(QStringLiteral("mainPanel"));
    auto *mainLayout = new QVBoxLayout(mainPanel);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *header = new QFrame(mainPanel);
    header->setObjectName(QStringLiteral("chatHeader"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 16, 24, 16);
    headerLayout->setSpacing(16);

    auto *titleGroup = new QWidget(header);
    auto *titleLayout = new QVBoxLayout(titleGroup);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(4);

    m_modelLabel = new QLabel(titleGroup);
    m_modelLabel->setObjectName(QStringLiteral("modelLabel"));

    m_personaLabel = new QLabel(titleGroup);
    m_personaLabel->setObjectName(QStringLiteral("personaLabel"));

    titleLayout->addWidget(m_modelLabel);
    titleLayout->addWidget(m_personaLabel);

    m_systemPromptButton = new QPushButton(header);
    m_systemPromptButton->setObjectName(QStringLiteral("systemPromptButton"));

    m_settingsButton = new QPushButton(header);
    m_settingsButton->setObjectName(QStringLiteral("settingsButton"));

    headerLayout->addWidget(titleGroup, 1);
    headerLayout->addWidget(m_systemPromptButton);
    headerLayout->addWidget(m_settingsButton);

    m_chatView = new ChatView(mainPanel);

    auto *composer = new QFrame(mainPanel);
    composer->setObjectName(QStringLiteral("composer"));
    auto *composerLayout = new QHBoxLayout(composer);
    composerLayout->setContentsMargins(24, 18, 24, 24);
    composerLayout->setSpacing(12);

    m_messageInput = new QTextEdit(composer);
    m_messageInput->setObjectName(QStringLiteral("messageInput"));
    m_messageInput->setFixedHeight(88);

    m_sendButton = new QPushButton(composer);
    m_sendButton->setObjectName(QStringLiteral("sendButton"));
    m_sendButton->setFixedSize(96, 44);

    composerLayout->addWidget(m_messageInput, 1);
    composerLayout->addWidget(m_sendButton, 0, Qt::AlignBottom);

    mainLayout->addWidget(header);
    mainLayout->addWidget(m_chatView, 1);
    mainLayout->addWidget(composer);

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(mainPanel, 1);

    setCentralWidget(central);

    connect(m_messageInput, &QTextEdit::textChanged, this, &MainWindow::updateSendButtonState);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendCurrentMessage);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::openSettingsDialog);
    connect(m_systemPromptButton, &QPushButton::clicked, this, &MainWindow::editSystemPrompt);
    connect(m_newChatButton, &QPushButton::clicked, this, &MainWindow::startNewChat);
    connect(m_sessionList, &QListWidget::itemClicked, this, &MainWindow::switchToSession);

    auto *returnShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")), m_messageInput);
    connect(returnShortcut, &QShortcut::activated, this, &MainWindow::sendCurrentMessage);

    auto *enterShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Enter")), m_messageInput);
    connect(enterShortcut, &QShortcut::activated, this, &MainWindow::sendCurrentMessage);
}

void MainWindow::populateSessionList()
{
    if (m_sessionList == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_sessionList);
    m_sessionList->clear();

    for (const ChatSession &session : m_sessionSummaries) {
        auto *item = new QListWidgetItem(sessionListTitle(session));
        item->setData(Qt::UserRole, session.id);
        m_sessionList->addItem(item);
    }

    if (findSessionItem(m_session.id) == nullptr) {
        auto *item = new QListWidgetItem(sessionListTitle(m_session));
        item->setData(Qt::UserRole, m_session.id);
        m_sessionList->insertItem(0, item);
    }

    updateCurrentSessionListItem();
}

void MainWindow::updateCurrentSessionListItem(bool moveToTop)
{
    if (m_sessionList == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_sessionList);
    QListWidgetItem *item = findSessionItem(m_session.id);
    if (item == nullptr) {
        item = new QListWidgetItem();
        m_sessionList->insertItem(0, item);
    } else if (moveToTop) {
        const int row = m_sessionList->row(item);
        if (row > 0) {
            item = m_sessionList->takeItem(row);
            m_sessionList->insertItem(0, item);
        }
    }

    item->setText(sessionListTitle(m_session));
    item->setData(Qt::UserRole, m_session.id);
    m_sessionList->setCurrentItem(item);
}

QListWidgetItem *MainWindow::findSessionItem(const QString &sessionId) const
{
    if (m_sessionList == nullptr) {
        return nullptr;
    }

    for (int row = 0; row < m_sessionList->count(); ++row) {
        QListWidgetItem *item = m_sessionList->item(row);
        if (item != nullptr && item->data(Qt::UserRole).toString() == sessionId) {
            return item;
        }
    }

    return nullptr;
}

QString MainWindow::sessionListTitle(const ChatSession &session) const
{
    const QString title = session.title.trimmed();
    if (title.isEmpty() || (title == QStringLiteral("New Chat") && session.messages.isEmpty())) {
        return text(QStringLiteral("Getting Started"), QStringLiteral("开始使用"));
    }

    return title;
}

void MainWindow::populateChatView()
{
    if (m_chatView == nullptr) {
        return;
    }

    m_chatView->clearMessages();
    if (m_session.messages.isEmpty()) {
        m_chatView->addMessage(MessageRole::Assistant,
                               text(QStringLiteral("Start a conversation by configuring your API settings, then send a message."),
                                    QStringLiteral("请先配置 API 设置，然后发送消息开始对话。")));
        return;
    }

    for (const ChatMessage &message : m_session.messages) {
        m_chatView->addMessage(message.role, message.content);
    }
}

void MainWindow::applyConfig()
{
    m_modelLabel->setText(text(QStringLiteral("Model: %1"), QStringLiteral("当前模型：%1")).arg(m_config.modelName));
}

void MainWindow::applyLanguage()
{
    setWindowTitle(text(QStringLiteral("AI Chat Desktop"), QStringLiteral("AI 聊天桌面应用")));
    m_newChatButton->setText(text(QStringLiteral("New Chat"), QStringLiteral("新建会话")));
    m_systemPromptButton->setText(text(QStringLiteral("Role Prompt"), QStringLiteral("角色提示词")));
    m_settingsButton->setText(text(QStringLiteral("Settings"), QStringLiteral("设置")));
    m_personaLabel->setText(m_session.hasSystemPrompt()
                                ? text(QStringLiteral("Role: Custom prompt"), QStringLiteral("角色：自定义提示词"))
                                : text(QStringLiteral("Role: Default assistant"), QStringLiteral("角色：默认助手")));
    m_messageInput->setPlaceholderText(text(QStringLiteral("Type a message..."), QStringLiteral("输入消息...")));
    m_sendButton->setText(text(QStringLiteral("Send"), QStringLiteral("发送")));

    if (m_chatView != nullptr && m_session.messages.isEmpty()) {
        populateChatView();
    }

    updateCurrentSessionListItem();
}

QString MainWindow::text(const QString &english, const QString &chinese) const
{
    return m_config.language == AppLanguage::English ? english : chinese;
}

void MainWindow::updateSendButtonState()
{
    const bool hasText = !m_messageInput->toPlainText().trimmed().isEmpty();
    m_sendButton->setEnabled(hasText && !m_isGenerating);
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog dialog(m_config, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_config = dialog.config();
    m_configStorage.save(m_config);
    applyConfig();
    applyLanguage();
}

void MainWindow::editSystemPrompt()
{
    if (m_isGenerating) {
        return;
    }

    bool accepted = false;
    const QString prompt = QInputDialog::getMultiLineText(
        this,
        text(QStringLiteral("Role Prompt"), QStringLiteral("角色提示词")),
        text(QStringLiteral("Set the system prompt for this chat:"), QStringLiteral("设置当前会话的系统提示词：")),
        m_session.systemPrompt,
        &accepted);

    if (!accepted) {
        return;
    }

    m_session.systemPrompt = prompt.trimmed();
    m_session.updatedAt = QDateTime::currentDateTimeUtc();
    if (!saveCurrentSession()) {
        statusBar()->showMessage(text(QStringLiteral("Failed to save the role prompt."),
                                      QStringLiteral("保存角色提示词失败。")),
                                 6000);
    }
    applyLanguage();
}

void MainWindow::startNewChat()
{
    if (m_isGenerating) {
        return;
    }

    if (m_session.messages.isEmpty() && !m_session.hasSystemPrompt()) {
        updateCurrentSessionListItem();
        populateChatView();
        return;
    }

    if (!m_session.messages.isEmpty() || m_session.hasSystemPrompt()) {
        saveCurrentSession();
    }

    m_session = ChatSession::createDefault();
    m_currentAssistantContent.clear();
    m_messageInput->clear();
    if (!saveCurrentSession()) {
        updateCurrentSessionListItem(true);
    }
    populateChatView();
    applyLanguage();
}

void MainWindow::switchToSession(QListWidgetItem *item)
{
    if (item == nullptr || m_isGenerating) {
        return;
    }

    const QString sessionId = item->data(Qt::UserRole).toString();
    if (sessionId.isEmpty() || sessionId == m_session.id) {
        updateCurrentSessionListItem();
        return;
    }

    if (!m_session.messages.isEmpty() || m_session.hasSystemPrompt()) {
        saveCurrentSession();
    }

    QString error;
    const std::optional<ChatSession> loaded = m_chatHistoryStorage.loadSession(sessionId, &error);
    if (!loaded.has_value()) {
        statusBar()->showMessage(text(QStringLiteral("Failed to load chat session: %1"),
                                      QStringLiteral("加载会话失败：%1"))
                                     .arg(error.isEmpty() ? sessionId : error),
                                 6000);
        updateCurrentSessionListItem();
        return;
    }

    m_session = loaded.value();
    m_currentAssistantContent.clear();
    m_messageInput->clear();
    populateChatView();
    applyLanguage();
    updateCurrentSessionListItem();
}

void MainWindow::sendCurrentMessage()
{
    const QString content = m_messageInput->toPlainText().trimmed();
    if (content.isEmpty()) {
        return;
    }

    if (!m_config.isComplete()) {
        QMessageBox::warning(this,
                             text(QStringLiteral("Missing API settings"), QStringLiteral("API 配置不完整")),
                             text(QStringLiteral("Please configure Base URL, model, and API Key before sending a message."),
                                  QStringLiteral("发送消息前，请先配置 Base URL、模型名称和 API Key。")));
        return;
    }

    if (m_session.messages.isEmpty()) {
        m_chatView->clearMessages();
        m_session.title = content.left(36);
        applyLanguage();
        updateCurrentSessionListItem(true);
    }

    m_session.addMessage(MessageRole::User, content);
    m_chatView->addMessage(MessageRole::User, content);
    m_messageInput->clear();

    m_session.addMessage(MessageRole::Assistant, QString());
    m_chatView->addMessage(MessageRole::Assistant, text(QStringLiteral("Thinking..."), QStringLiteral("思考中...")));
    m_currentAssistantContent.clear();

    setGenerating(true);
    m_aiClient.sendChat(m_config, m_session);
}

void MainWindow::handleTextDelta(const QString &delta)
{
    m_currentAssistantContent += delta;
    if (!m_session.messages.isEmpty()) {
        m_session.messages.last().content = m_currentAssistantContent;
    }
    m_chatView->updateLastAssistantMessage(m_currentAssistantContent);
}

void MainWindow::handleRequestFinished()
{
    setGenerating(false);
    if (!m_session.messages.isEmpty() && m_session.messages.last().content.isEmpty()) {
        m_session.messages.last().content = text(QStringLiteral("(Empty response)"), QStringLiteral("（空回复）"));
        m_chatView->updateLastAssistantMessage(m_session.messages.last().content);
    }
    saveCurrentSession();
}

void MainWindow::handleRequestFailed(const QString &message)
{
    setGenerating(false);
    const QString errorMessage = text(QStringLiteral("Request failed: %1"), QStringLiteral("请求失败：%1")).arg(message);
    const QString displayMessage = m_currentAssistantContent.isEmpty()
                                       ? errorMessage
                                       : QStringLiteral("%1\n\n%2").arg(m_currentAssistantContent, errorMessage);
    if (!m_session.messages.isEmpty()) {
        m_session.messages.last().content = displayMessage;
    }
    m_chatView->updateLastAssistantMessage(displayMessage);
    saveCurrentSession();
}

void MainWindow::setGenerating(bool generating)
{
    m_isGenerating = generating;
    m_messageInput->setEnabled(!generating);
    m_newChatButton->setEnabled(!generating);
    m_systemPromptButton->setEnabled(!generating);
    m_settingsButton->setEnabled(!generating);
    m_sendButton->setText(generating ? text(QStringLiteral("Sending"), QStringLiteral("发送中")) : text(QStringLiteral("Send"), QStringLiteral("发送")));
    updateSendButtonState();
}

bool MainWindow::saveCurrentSession()
{
    if (!m_historyAvailable) {
        return false;
    }

    QString error;
    if (!m_chatHistoryStorage.saveSession(m_session, &error)) {
        statusBar()->showMessage(text(QStringLiteral("Failed to save chat history: %1"),
                                      QStringLiteral("保存聊天记录失败：%1"))
                                     .arg(error),
                                 6000);
        return false;
    }

    for (const ChatMessage &message : m_session.messages) {
        if (!m_chatHistoryStorage.saveMessage(message, &error)) {
            statusBar()->showMessage(text(QStringLiteral("Failed to save chat history: %1"),
                                          QStringLiteral("保存聊天记录失败：%1"))
                                         .arg(error),
                                     6000);
            return false;
        }
    }

    updateCurrentSessionListItem(true);
    return true;
}

void MainWindow::showStartupWarningIfNeeded()
{
    if (m_startupWarningMessage.isEmpty()) {
        return;
    }

    QTimer::singleShot(0, this, [this]() {
        QMessageBox::warning(this,
                             text(QStringLiteral("Chat history issue"), QStringLiteral("聊天记录问题")),
                             m_startupWarningMessage);
    });
}
