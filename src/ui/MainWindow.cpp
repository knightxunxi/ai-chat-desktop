#include "ui/MainWindow.h"

#include "ui/ChatView.h"
#include "ui/SettingsDialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    loadConfig();
    loadSession();
    setupUi();
    populateChatView();
    applyConfig();
    applyLanguage();
    updateSendButtonState();

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
        return;
    }

    const std::optional<ChatSession> latestSession = m_chatHistoryStorage.loadLatestSession(&error);
    m_session = latestSession.value_or(ChatSession::createDefault());
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
    m_sessionList->addItem(QStringLiteral("Getting Started"));

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

    m_settingsButton = new QPushButton(header);
    m_settingsButton->setObjectName(QStringLiteral("settingsButton"));

    headerLayout->addWidget(titleGroup, 1);
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
    m_settingsButton->setText(text(QStringLiteral("Settings"), QStringLiteral("设置")));
    m_personaLabel->setText(text(QStringLiteral("Role: Default assistant"), QStringLiteral("角色：默认助手")));
    m_messageInput->setPlaceholderText(text(QStringLiteral("Type a message..."), QStringLiteral("输入消息...")));
    m_sendButton->setText(text(QStringLiteral("Send"), QStringLiteral("发送")));

    if (m_chatView != nullptr && m_session.messages.isEmpty()) {
        populateChatView();
    }

    if (m_sessionList->count() > 0) {
        m_sessionList->item(0)->setText(text(QStringLiteral("Getting Started"), QStringLiteral("开始使用")));
    }
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
    const QString displayMessage = text(QStringLiteral("Request failed: %1"), QStringLiteral("请求失败：%1")).arg(message);
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
    m_sendButton->setText(generating ? text(QStringLiteral("Sending"), QStringLiteral("发送中")) : text(QStringLiteral("Send"), QStringLiteral("发送")));
    updateSendButtonState();
}

void MainWindow::saveCurrentSession()
{
    QString error;
    if (!m_chatHistoryStorage.saveSession(m_session, &error)) {
        return;
    }

    for (const ChatMessage &message : m_session.messages) {
        m_chatHistoryStorage.saveMessage(message, &error);
    }
}
