#include "ui/MainWindow.h"

#include "support/AppLogger.h"
#include "ui/ChatView.h"
#include "ui/FileToolsDialog.h"
#include "ui/LogViewerDialog.h"
#include "ui/RolePromptDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/ToolsDialog.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTextEdit>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    connectController();
    m_controller.initialize();
    updateSendButtonState();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_controller.isGenerating()) {
        m_controller.cancelCurrentRequest();
    }

    event->accept();
    QCoreApplication::quit();
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("AI Chat Desktop"));
    resize(1120, 760);
    setMinimumSize(860, 560);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralRoot"));
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

    m_renameChatButton = new QPushButton(sidebar);
    m_renameChatButton->setObjectName(QStringLiteral("renameChatButton"));

    m_exportChatButton = new QPushButton(sidebar);
    m_exportChatButton->setObjectName(QStringLiteral("exportChatButton"));

    m_favoriteChatButton = new QPushButton(sidebar);
    m_favoriteChatButton->setObjectName(QStringLiteral("favoriteChatButton"));

    m_archiveChatButton = new QPushButton(sidebar);
    m_archiveChatButton->setObjectName(QStringLiteral("archiveChatButton"));

    m_deleteChatButton = new QPushButton(sidebar);
    m_deleteChatButton->setObjectName(QStringLiteral("deleteChatButton"));

    m_sessionSearchEdit = new QLineEdit(sidebar);
    m_sessionSearchEdit->setObjectName(QStringLiteral("sessionSearchEdit"));
    m_sessionSearchEdit->setClearButtonEnabled(true);

    auto *sessionFilterBar = new QFrame(sidebar);
    sessionFilterBar->setObjectName(QStringLiteral("sessionFilterBar"));
    auto *sessionFilterLayout = new QHBoxLayout(sessionFilterBar);
    sessionFilterLayout->setContentsMargins(0, 0, 0, 0);
    sessionFilterLayout->setSpacing(0);

    m_activeFilterButton = new QPushButton(sessionFilterBar);
    m_activeFilterButton->setObjectName(QStringLiteral("activeFilterButton"));
    m_activeFilterButton->setCheckable(true);

    m_favoriteFilterButton = new QPushButton(sessionFilterBar);
    m_favoriteFilterButton->setObjectName(QStringLiteral("favoriteFilterButton"));
    m_favoriteFilterButton->setCheckable(true);

    m_archivedFilterButton = new QPushButton(sessionFilterBar);
    m_archivedFilterButton->setObjectName(QStringLiteral("archivedFilterButton"));
    m_archivedFilterButton->setCheckable(true);

    sessionFilterLayout->addWidget(m_activeFilterButton);
    sessionFilterLayout->addWidget(m_favoriteFilterButton);
    sessionFilterLayout->addWidget(m_archivedFilterButton);

    m_sessionList = new QListWidget(sidebar);
    m_sessionList->setObjectName(QStringLiteral("sessionList"));

    sidebarLayout->addWidget(m_newChatButton);
    sidebarLayout->addWidget(m_renameChatButton);
    sidebarLayout->addWidget(m_exportChatButton);
    sidebarLayout->addWidget(m_favoriteChatButton);
    sidebarLayout->addWidget(m_archiveChatButton);
    sidebarLayout->addWidget(m_deleteChatButton);
    sidebarLayout->addWidget(m_sessionSearchEdit);
    sidebarLayout->addWidget(sessionFilterBar);
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

    m_toolsButton = new QPushButton(header);
    m_toolsButton->setObjectName(QStringLiteral("toolsButton"));

    m_fileToolsButton = new QPushButton(header);
    m_fileToolsButton->setObjectName(QStringLiteral("fileToolsButton"));

    m_logButton = new QPushButton(header);
    m_logButton->setObjectName(QStringLiteral("logButton"));

    m_settingsButton = new QPushButton(header);
    m_settingsButton->setObjectName(QStringLiteral("settingsButton"));

    headerLayout->addWidget(titleGroup, 1);
    headerLayout->addWidget(m_systemPromptButton);
    headerLayout->addWidget(m_toolsButton);
    headerLayout->addWidget(m_fileToolsButton);
    headerLayout->addWidget(m_logButton);
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

    m_retryButton = new QPushButton(composer);
    m_retryButton->setObjectName(QStringLiteral("retryButton"));
    m_retryButton->setFixedSize(88, 44);
    m_retryButton->setCursor(Qt::PointingHandCursor);
    m_retryButton->setVisible(false);

    m_sendButton = new QPushButton(composer);
    m_sendButton->setObjectName(QStringLiteral("sendButton"));
    m_sendButton->setFixedSize(96, 44);

    composerLayout->addWidget(m_messageInput, 1);
    composerLayout->addWidget(m_retryButton, 0, Qt::AlignBottom);
    composerLayout->addWidget(m_sendButton, 0, Qt::AlignBottom);

    mainLayout->addWidget(header);
    mainLayout->addWidget(m_chatView, 1);
    mainLayout->addWidget(composer);

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(mainPanel, 1);

    setCentralWidget(central);

    connect(m_messageInput, &QTextEdit::textChanged, this, &MainWindow::updateSendButtonState);
    connect(m_retryButton, &QPushButton::clicked, &m_controller, &ApplicationController::retryLastRequest);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendCurrentMessage);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::openSettingsDialog);
    connect(m_toolsButton, &QPushButton::clicked, this, &MainWindow::openToolsDialog);
    connect(m_fileToolsButton, &QPushButton::clicked, this, &MainWindow::openFileToolsDialog);
    connect(m_logButton, &QPushButton::clicked, this, &MainWindow::openLogViewerDialog);
    connect(m_systemPromptButton, &QPushButton::clicked, this, &MainWindow::editSystemPrompt);
    connect(m_newChatButton, &QPushButton::clicked, this, &MainWindow::startNewChat);
    connect(m_renameChatButton, &QPushButton::clicked, this, &MainWindow::renameCurrentChat);
    connect(m_exportChatButton, &QPushButton::clicked, this, &MainWindow::exportCurrentChat);
    connect(m_favoriteChatButton, &QPushButton::clicked, this, &MainWindow::toggleCurrentChatFavorite);
    connect(m_archiveChatButton, &QPushButton::clicked, this, &MainWindow::toggleCurrentChatArchived);
    connect(m_deleteChatButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentChat);
    connect(m_sessionList, &QListWidget::itemClicked, this, &MainWindow::switchToSession);
    connect(m_sessionSearchEdit, &QLineEdit::textChanged, &m_controller, &ApplicationController::searchSessions);
    connect(m_activeFilterButton, &QPushButton::clicked, this, [this]() {
        changeSessionFilter(SessionListFilter::Active);
    });
    connect(m_favoriteFilterButton, &QPushButton::clicked, this, [this]() {
        changeSessionFilter(SessionListFilter::Favorite);
    });
    connect(m_archivedFilterButton, &QPushButton::clicked, this, [this]() {
        changeSessionFilter(SessionListFilter::Archived);
    });

    auto *returnShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")), m_messageInput);
    connect(returnShortcut, &QShortcut::activated, this, &MainWindow::sendCurrentMessage);

    auto *enterShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Enter")), m_messageInput);
    connect(enterShortcut, &QShortcut::activated, this, &MainWindow::sendCurrentMessage);
}

void MainWindow::connectController()
{
    connect(&m_controller, &ApplicationController::configChanged, this, [this]() {
        applyConfig();
        applyLanguage();
    });
    connect(&m_controller, &ApplicationController::promptTemplatesChanged, this, &MainWindow::applyLanguage);
    connect(&m_controller, &ApplicationController::sessionListChanged, this, &MainWindow::populateSessionList);
    connect(&m_controller, &ApplicationController::sessionListFilterChanged, this, [this]() {
        updateSessionFilterButtons();
        updateSessionOrganizationControls();
    });
    connect(&m_controller, &ApplicationController::currentSessionChanged, this, [this]() {
        populateChatView();
        applyLanguage();
        updateCurrentSessionListItem();
        updateSendButtonState();
    });
    connect(&m_controller, &ApplicationController::currentChatCleared, m_chatView, &ChatView::clearMessages);
    connect(&m_controller, &ApplicationController::userMessageAdded, this, &MainWindow::addUserMessage);
    connect(&m_controller, &ApplicationController::assistantMessageStarted, this, &MainWindow::addAssistantPlaceholder);
    connect(&m_controller, &ApplicationController::assistantMessageUpdated, m_chatView, &ChatView::updateLastAssistantMessage);
    connect(&m_controller, &ApplicationController::generatingChanged, this, &MainWindow::setGenerating);
    connect(&m_controller, &ApplicationController::retryAvailableChanged, this, &MainWindow::setRetryAvailable);
    connect(&m_controller, &ApplicationController::configurationMissing, this, &MainWindow::showConfigurationMissingWarning);
    connect(&m_controller, &ApplicationController::statusMessage, this, &MainWindow::showStatusMessage);
    connect(&m_controller, &ApplicationController::startupWarning, this, &MainWindow::showStartupWarning);
}

void MainWindow::populateSessionList()
{
    if (m_sessionList == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_sessionList);
    m_sessionList->clear();

    for (const ChatSession &session : m_controller.sessionSummaries()) {
        auto *item = new QListWidgetItem(sessionListTitle(session));
        item->setData(Qt::UserRole, session.id);
        m_sessionList->addItem(item);
    }

    if (findSessionItem(m_controller.currentSession().id) == nullptr) {
        auto *item = new QListWidgetItem(sessionListTitle(m_controller.currentSession()));
        item->setData(Qt::UserRole, m_controller.currentSession().id);
        m_sessionList->insertItem(0, item);
    }

    updateCurrentSessionListItem();
}

void MainWindow::updateCurrentSessionListItem()
{
    if (m_sessionList == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_sessionList);
    QListWidgetItem *item = findSessionItem(m_controller.currentSession().id);
    if (item == nullptr) {
        item = new QListWidgetItem();
        m_sessionList->insertItem(0, item);
    }

    item->setText(sessionListTitle(m_controller.currentSession()));
    item->setData(Qt::UserRole, m_controller.currentSession().id);
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
    const QString prefix = session.isFavorite ? QStringLiteral("[*] ") : QString();
    if (title.isEmpty() || (title == QStringLiteral("New Chat") && session.messages.isEmpty())) {
        return prefix + text(QStringLiteral("Getting Started"), QStringLiteral("开始使用"));
    }

    return prefix + title;
}

void MainWindow::populateChatView()
{
    if (m_chatView == nullptr) {
        return;
    }

    m_chatView->clearMessages();
    if (m_controller.currentSession().messages.isEmpty()) {
        m_chatView->addMessage(MessageRole::Assistant,
                               text(QStringLiteral("Start a conversation by configuring your API settings, then send a message."),
                                    QStringLiteral("请先配置 API 设置，然后发送消息开始对话。")));
        return;
    }

    for (const ChatMessage &message : m_controller.currentSession().messages) {
        m_chatView->addMessage(message.role, message.content);
    }
}

void MainWindow::applyConfig()
{
    m_modelLabel->setText(text(QStringLiteral("Model: %1"), QStringLiteral("当前模型：%1")).arg(m_controller.config().modelName));
}

void MainWindow::applyLanguage()
{
    setWindowTitle(text(QStringLiteral("AI Chat Desktop"), QStringLiteral("AI 聊天桌面应用")));
    m_newChatButton->setText(text(QStringLiteral("New Chat"), QStringLiteral("新建会话")));
    m_renameChatButton->setText(text(QStringLiteral("Rename"), QStringLiteral("重命名")));
    m_exportChatButton->setText(text(QStringLiteral("Export"), QStringLiteral("导出")));
    m_deleteChatButton->setText(text(QStringLiteral("Delete Chat"), QStringLiteral("删除会话")));
    m_sessionSearchEdit->setPlaceholderText(text(QStringLiteral("Search chats"), QStringLiteral("搜索会话")));
    m_systemPromptButton->setText(text(QStringLiteral("Role Prompt"), QStringLiteral("角色提示词")));
    m_toolsButton->setText(text(QStringLiteral("Tools"), QStringLiteral("工具")));
    m_fileToolsButton->setText(text(QStringLiteral("File Tools"), QStringLiteral("文件工具")));
    m_logButton->setText(text(QStringLiteral("Logs"), QStringLiteral("日志")));
    m_settingsButton->setText(text(QStringLiteral("Settings"), QStringLiteral("设置")));
    m_retryButton->setText(text(QStringLiteral("Retry"), QStringLiteral("重试")));
    m_personaLabel->setText(text(QStringLiteral("Role: %1"), QStringLiteral("角色：%1")).arg(currentRoleDisplayName()));
    m_messageInput->setPlaceholderText(text(QStringLiteral("Type a message..."), QStringLiteral("输入消息...")));
    updateSessionFilterButtons();
    updateSessionOrganizationControls();
    updateSendButtonAppearance();

    if (m_chatView != nullptr && m_controller.currentSession().messages.isEmpty()) {
        populateChatView();
    }

    updateCurrentSessionListItem();
}

void MainWindow::updateSessionFilterButtons()
{
    const QSignalBlocker activeBlocker(m_activeFilterButton);
    const QSignalBlocker favoriteBlocker(m_favoriteFilterButton);
    const QSignalBlocker archivedBlocker(m_archivedFilterButton);

    m_activeFilterButton->setText(text(QStringLiteral("Active"), QStringLiteral("全部")));
    m_favoriteFilterButton->setText(text(QStringLiteral("Favorites"), QStringLiteral("收藏")));
    m_archivedFilterButton->setText(text(QStringLiteral("Archived"), QStringLiteral("归档")));

    const SessionListFilter currentFilter = m_controller.sessionListFilter();
    m_activeFilterButton->setChecked(currentFilter == SessionListFilter::Active);
    m_favoriteFilterButton->setChecked(currentFilter == SessionListFilter::Favorite);
    m_archivedFilterButton->setChecked(currentFilter == SessionListFilter::Archived);
}

void MainWindow::updateSessionOrganizationControls()
{
    const ChatSession &session = m_controller.currentSession();
    m_favoriteChatButton->setText(session.isFavorite
                                      ? text(QStringLiteral("Unfavorite"), QStringLiteral("取消收藏"))
                                      : text(QStringLiteral("Favorite"), QStringLiteral("收藏")));
    m_archiveChatButton->setText(session.isArchived
                                     ? text(QStringLiteral("Unarchive"), QStringLiteral("取消归档"))
                                     : text(QStringLiteral("Archive"), QStringLiteral("归档")));
    const bool enabled = !m_controller.isGenerating();
    m_favoriteChatButton->setEnabled(enabled);
    m_archiveChatButton->setEnabled(enabled);
}

QString MainWindow::text(const QString &english, const QString &chinese) const
{
    return m_controller.config().language == AppLanguage::English ? english : chinese;
}

QString MainWindow::currentRoleDisplayName() const
{
    const QString systemPrompt = m_controller.currentSession().systemPrompt.trimmed();
    if (systemPrompt.isEmpty()) {
        return text(QStringLiteral("Default assistant"), QStringLiteral("默认助手"));
    }

    for (const PromptTemplate &promptTemplate : m_controller.promptTemplates()) {
        if (promptTemplate.content.trimmed() == systemPrompt) {
            return promptTemplate.name;
        }
    }

    return text(QStringLiteral("Custom prompt"), QStringLiteral("自定义提示词"));
}

void MainWindow::updateSendButtonAppearance()
{
    const bool generating = m_controller.isGenerating();
    m_sendButton->setText(generating ? text(QStringLiteral("Stop"), QStringLiteral("停止")) : text(QStringLiteral("Send"), QStringLiteral("发送")));
    m_sendButton->setProperty("stopMode", generating);
    m_sendButton->setStyleSheet(generating
                                    ? QStringLiteral("QPushButton#sendButton { background: #dc2626; border-color: #dc2626; color: #ffffff; font-weight: 600; }"
                                                     "QPushButton#sendButton:hover { background: #b91c1c; border-color: #b91c1c; }")
                                    : QString());
    m_sendButton->style()->unpolish(m_sendButton);
    m_sendButton->style()->polish(m_sendButton);
}

void MainWindow::updateSendButtonState()
{
    if (m_controller.isGenerating()) {
        m_sendButton->setEnabled(true);
        return;
    }

    const bool hasText = !m_messageInput->toPlainText().trimmed().isEmpty();
    m_sendButton->setEnabled(hasText);
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog dialog(m_controller.config(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_controller.saveConfig(dialog.config());
}

void MainWindow::openLogViewerDialog()
{
    LogViewerDialog dialog(AppLogger::logFilePath(), m_controller.config().language, this);
    dialog.exec();
}

void MainWindow::openToolsDialog()
{
    ToolsDialog dialog(m_controller.config().language, this);
    connect(&dialog, &ToolsDialog::outputInsertionRequested, this, &MainWindow::insertToolOutputIntoInput);
    dialog.exec();
}

void MainWindow::openFileToolsDialog()
{
    FileToolsDialog dialog(m_controller.config().language, this);
    connect(&dialog, &FileToolsDialog::outputInsertionRequested, this, &MainWindow::insertToolOutputIntoInput);
    dialog.exec();
}

void MainWindow::insertToolOutputIntoInput(const QString &output)
{
    if (output.isEmpty()) {
        return;
    }

    if (m_controller.isGenerating()) {
        showStatusMessage(QStringLiteral("Stop generation before inserting tool output."),
                          QStringLiteral("请先停止生成，再插入工具输出。"),
                          3000);
        return;
    }

    QTextCursor cursor = m_messageInput->textCursor();
    cursor.insertText(output);
    m_messageInput->setTextCursor(cursor);
    m_messageInput->setFocus();
    updateSendButtonState();
}

void MainWindow::editSystemPrompt()
{
    if (m_controller.isGenerating()) {
        return;
    }

    RolePromptDialog dialog(m_controller.currentSession().systemPrompt,
                            m_controller.promptTemplates(),
                            m_controller.config().language,
                            this);
    if (dialog.exec() == QDialog::Accepted) {
        m_controller.savePromptTemplates(dialog.templates());
        m_controller.setSystemPrompt(dialog.prompt());
    }
}

void MainWindow::startNewChat()
{
    m_messageInput->clear();
    m_controller.startNewChat();
}

void MainWindow::renameCurrentChat()
{
    if (m_controller.isGenerating()) {
        return;
    }

    const QString currentTitle = m_controller.currentSession().title == QStringLiteral("New Chat")
                                     ? QString()
                                     : m_controller.currentSession().title;
    QInputDialog dialog(this);
    dialog.setWindowTitle(text(QStringLiteral("Rename chat"), QStringLiteral("重命名会话")));
    dialog.setLabelText(text(QStringLiteral("Chat title"), QStringLiteral("会话标题")));
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setTextEchoMode(QLineEdit::Normal);
    dialog.setTextValue(currentTitle);
    dialog.setOkButtonText(QStringLiteral("确认"));
    dialog.setCancelButtonText(QStringLiteral("取消"));
    dialog.setFixedSize(500, 300);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_controller.renameCurrentSession(dialog.textValue());
}

void MainWindow::exportCurrentChat()
{
    if (m_controller.isGenerating()) {
        return;
    }

    if (m_controller.currentSession().messages.isEmpty()) {
        QMessageBox::information(this,
                                 text(QStringLiteral("Export chat"), QStringLiteral("导出会话")),
                                 text(QStringLiteral("There are no messages to export."),
                                      QStringLiteral("当前会话没有可导出的消息。")));
        return;
    }

    QString directoryPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (directoryPath.trimmed().isEmpty()) {
        directoryPath = QDir::homePath();
    }

    QString fileName = m_controller.currentSession().title.trimmed();
    if (fileName.isEmpty() || fileName == QStringLiteral("New Chat")) {
        fileName = QStringLiteral("chat-export");
    }

    const QString invalidCharacters = QStringLiteral("\\/:*?\"<>|");
    for (const QChar character : invalidCharacters) {
        fileName.replace(character, QLatin1Char('-'));
    }

    const QString defaultPath = QDir(directoryPath).filePath(fileName + QStringLiteral(".md"));
    QString filePath = QFileDialog::getSaveFileName(
        this,
        text(QStringLiteral("Export chat"), QStringLiteral("导出会话")),
        defaultPath,
        QStringLiteral("Markdown (*.md)"));

    if (filePath.trimmed().isEmpty()) {
        return;
    }

    if (QFileInfo(filePath).suffix().isEmpty()) {
        filePath += QStringLiteral(".md");
    }

    QString error;
    if (!m_controller.exportCurrentSessionMarkdown(filePath, &error)) {
        QMessageBox::warning(this,
                             text(QStringLiteral("Export failed"), QStringLiteral("导出失败")),
                             error);
        return;
    }

    showStatusMessage(QStringLiteral("Chat exported to %1").arg(QFileInfo(filePath).fileName()),
                      QStringLiteral("会话已导出到 %1").arg(QFileInfo(filePath).fileName()),
                      4000);
}

void MainWindow::toggleCurrentChatFavorite()
{
    m_controller.toggleCurrentSessionFavorite();
}

void MainWindow::toggleCurrentChatArchived()
{
    m_controller.toggleCurrentSessionArchived();
}

void MainWindow::deleteCurrentChat()
{
    if (m_controller.isGenerating()) {
        return;
    }

    const QMessageBox::StandardButton result = QMessageBox::question(
        this,
        text(QStringLiteral("Delete chat"), QStringLiteral("删除会话")),
        text(QStringLiteral("Delete the current chat session? This cannot be undone."),
             QStringLiteral("确定删除当前会话吗？此操作无法撤销。")),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result != QMessageBox::Yes) {
        return;
    }

    m_messageInput->clear();
    m_controller.deleteCurrentSession();
}

void MainWindow::changeSessionFilter(SessionListFilter filter)
{
    m_controller.setSessionListFilter(filter);
}

void MainWindow::switchToSession(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    m_messageInput->clear();
    m_controller.switchToSession(item->data(Qt::UserRole).toString());
}

void MainWindow::sendCurrentMessage()
{
    if (m_controller.isGenerating()) {
        m_controller.cancelCurrentRequest();
        return;
    }

    const QString content = m_messageInput->toPlainText().trimmed();
    if (content.isEmpty()) {
        return;
    }

    m_controller.sendMessage(content);
}

void MainWindow::addUserMessage(const QString &content)
{
    m_chatView->addMessage(MessageRole::User, content);
    m_messageInput->clear();
}

void MainWindow::addAssistantPlaceholder()
{
    m_chatView->addMessage(MessageRole::Assistant, text(QStringLiteral("Thinking..."), QStringLiteral("思考中...")));
}

void MainWindow::setGenerating(bool generating)
{
    m_messageInput->setEnabled(!generating);
    m_newChatButton->setEnabled(!generating);
    m_renameChatButton->setEnabled(!generating);
    m_exportChatButton->setEnabled(!generating);
    m_favoriteChatButton->setEnabled(!generating);
    m_archiveChatButton->setEnabled(!generating);
    m_deleteChatButton->setEnabled(!generating);
    m_systemPromptButton->setEnabled(!generating);
    m_toolsButton->setEnabled(true);
    m_fileToolsButton->setEnabled(true);
    m_settingsButton->setEnabled(!generating);
    m_logButton->setEnabled(true);
    m_retryButton->setEnabled(!generating && m_retryButton->isVisible());
    updateSendButtonAppearance();
    updateSendButtonState();
}

void MainWindow::setRetryAvailable(bool available)
{
    m_retryButton->setVisible(available);
    m_retryButton->setEnabled(available && !m_controller.isGenerating());
}

void MainWindow::showConfigurationMissingWarning()
{
    QMessageBox::warning(this,
                         text(QStringLiteral("Missing API settings"), QStringLiteral("API 配置不完整")),
                         text(QStringLiteral("Please configure Base URL, model, and API Key before sending a message."),
                              QStringLiteral("发送消息前，请先配置 Base URL、模型名称和 API Key。")));
}

void MainWindow::showStatusMessage(const QString &english, const QString &chinese, int timeoutMs)
{
    statusBar()->showMessage(text(english, chinese), timeoutMs);
}

void MainWindow::showStartupWarning(const QString &english, const QString &chinese)
{
    QTimer::singleShot(0, this, [this, english, chinese]() {
        QMessageBox::warning(this,
                             text(QStringLiteral("Chat history issue"), QStringLiteral("聊天记录问题")),
                             text(english, chinese));
    });
}
