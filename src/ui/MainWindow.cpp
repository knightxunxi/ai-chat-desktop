#include "ui/MainWindow.h"

#include "support/AppLogger.h"
#include "ui/ChatView.h"
#include "ui/LogViewerDialog.h"
#include "ui/MessageWidget.h"
#include "ui/RolePromptDialog.h"
#include "ui/ScheduledTaskDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/ToolsDialog.h"

#include <QApplication>
#include <QBuffer>
#include <QCloseEvent>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTextEdit>
#include <QTimer>
#include <QUuid>
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

    m_agentPlanButton = new QPushButton(header);
    m_agentPlanButton->setObjectName(QStringLiteral("agentPlanButton"));
    m_agentPlanButton->setVisible(false);  // 已由统一模式替代，保留接口供内部使用

    m_logButton = new QPushButton(header);
    m_logButton->setObjectName(QStringLiteral("logButton"));

    m_settingsButton = new QPushButton(header);
    m_settingsButton->setObjectName(QStringLiteral("settingsButton"));

    m_scheduledTaskButton = new QPushButton(header);
    m_scheduledTaskButton->setObjectName(QStringLiteral("scheduledTaskButton"));

    // V16.3: 主题切换按钮（日月图标）
    m_themeToggleButton = new QPushButton(header);
    m_themeToggleButton->setObjectName(QStringLiteral("themeToggleButton"));
    m_themeToggleButton->setToolTip(text(
        QStringLiteral("Toggle dark/light theme"),
        QStringLiteral("切换深色/浅色主题")));
    m_themeToggleButton->setFixedSize(32, 32);
    m_themeToggleButton->setCursor(Qt::PointingHandCursor);
    m_themeToggleButton->setFont(QFont(m_themeToggleButton->font().family(), 14));
    m_themeToggleButton->setText(QStringLiteral("\xF0\x9F\x8C\x99"));  // 🌙 默认亮色 → 点切暗色
    connect(m_themeToggleButton, &QPushButton::clicked, this, &MainWindow::toggleDarkMode);

    headerLayout->addWidget(titleGroup, 1);
    headerLayout->addWidget(m_systemPromptButton);
    headerLayout->addWidget(m_toolsButton);
    headerLayout->addWidget(m_agentPlanButton);
    headerLayout->addWidget(m_logButton);
    headerLayout->addWidget(m_settingsButton);
    headerLayout->addWidget(m_scheduledTaskButton);
    headerLayout->addWidget(m_themeToggleButton);

    m_chatView = new ChatView(mainPanel);

    auto *composer = new QFrame(mainPanel);
    composer->setObjectName(QStringLiteral("composer"));
    auto *composerOuterLayout = new QVBoxLayout(composer);
    composerOuterLayout->setContentsMargins(0, 0, 0, 0);
    composerOuterLayout->setSpacing(0);

    auto *composerRow = new QFrame(composer);
    auto *composerLayout = new QHBoxLayout(composerRow);
    composerLayout->setContentsMargins(24, 18, 24, 24);
    composerLayout->setSpacing(12);

    m_messageInput = new QTextEdit(composerRow);
    m_messageInput->setObjectName(QStringLiteral("messageInput"));
    // V17.2: 改用最小高度，允许内容增长以适应内联图片
    m_messageInput->setMinimumHeight(88);
    m_messageInput->setMaximumHeight(200);

    // V17.1: 安装事件过滤器以支持图片粘贴
    m_messageInput->installEventFilter(this);

    m_retryButton = new QPushButton(composerRow);
    m_retryButton->setObjectName(QStringLiteral("retryButton"));
    m_retryButton->setFixedSize(88, 44);
    m_retryButton->setCursor(Qt::PointingHandCursor);
    m_retryButton->setVisible(false);

    m_sendButton = new QPushButton(composerRow);
    m_sendButton->setObjectName(QStringLiteral("sendButton"));
    m_sendButton->setFixedSize(96, 44);

    m_modeToggleButton = new QPushButton(composerRow);
    m_modeToggleButton->setObjectName(QStringLiteral("modeToggleButton"));
    m_modeToggleButton->setFixedSize(64, 44);
    m_modeToggleButton->setCursor(Qt::PointingHandCursor);
    m_modeToggleButton->setCheckable(true);
    m_modeToggleButton->setChecked(false);

    composerLayout->addWidget(m_messageInput, 1);
    composerLayout->addWidget(m_modeToggleButton, 0, Qt::AlignBottom);
    composerLayout->addWidget(m_retryButton, 0, Qt::AlignBottom);
    composerLayout->addWidget(m_sendButton, 0, Qt::AlignBottom);

    composerOuterLayout->addWidget(composerRow);

    mainLayout->addWidget(header);
    mainLayout->addWidget(m_chatView, 1);
    mainLayout->addWidget(composer);

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(mainPanel, 1);

    setCentralWidget(central);

    connect(m_messageInput, &QTextEdit::textChanged, this, &MainWindow::updateSendButtonState);
    connect(m_retryButton, &QPushButton::clicked, &m_controller, &ApplicationController::retryLastRequest);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendCurrentMessage);
    connect(m_modeToggleButton, &QPushButton::clicked, this, &MainWindow::toggleAgentMode);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::openSettingsDialog);
    connect(m_scheduledTaskButton, &QPushButton::clicked, this, &MainWindow::openScheduledTaskDialog);
    connect(m_toolsButton, &QPushButton::clicked, this, &MainWindow::openToolsDialog);
    connect(m_agentPlanButton, &QPushButton::clicked, this, &MainWindow::generateAgentPlan);
    connect(m_logButton, &QPushButton::clicked, this, &MainWindow::openLogViewerDialog);    connect(m_systemPromptButton, &QPushButton::clicked, this, &MainWindow::editSystemPrompt);
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
    connect(&m_controller, &ApplicationController::currentChatCleared, this, [this]() {
        m_currentStepGroup = nullptr;
        m_messageWidgets.clear();
        m_chatView->clearMessages();
    });
    connect(&m_controller, &ApplicationController::userMessageAdded, this, &MainWindow::addUserMessage);
    connect(&m_controller, &ApplicationController::assistantMessageStarted, this, &MainWindow::addAssistantPlaceholder);
    connect(&m_controller, &ApplicationController::assistantMessageUpdated, m_chatView, &ChatView::updateLastAssistantMessage);
    connect(&m_controller, &ApplicationController::generatingChanged, this, &MainWindow::setGenerating);
    connect(&m_controller, &ApplicationController::retryAvailableChanged, this, &MainWindow::setRetryAvailable);
    connect(&m_controller, &ApplicationController::configurationMissing, this, &MainWindow::showConfigurationMissingWarning);
    connect(&m_controller, &ApplicationController::statusMessage, this, &MainWindow::showStatusMessage);
    connect(&m_controller, &ApplicationController::startupWarning, this, &MainWindow::showStartupWarning);
    // V12.5: agentPlanReady 信号已移除，计划自动执行不再通过弹窗
    // V12.6: Agent 循环迭代状态更新
    connect(&m_controller, &ApplicationController::agentLoopIterationUpdated,
        this, [this](int iteration, int maxIterations) {
            showStatusMessage(
                QString("Agent loop %1/%2").arg(iteration).arg(maxIterations),
                QString("Agent 循环 %1/%2").arg(iteration).arg(maxIterations),
                3000);
        });
    // V16.1: Agent 完成后在聊天区显示技能使用摘要
    connect(&m_controller, &ApplicationController::agentLoopSkillSummary,
        this, [this](const QString &summary) {
            m_chatView->addMessage(MessageRole::System, summary);
        });

    // V16.1: Agent 思考步骤可视化 — V18.4 改为分组折叠
    connect(&m_controller, &ApplicationController::agentLoopThought,
        this, [this](int iter, const QString &reason, const QString &toolId, const QString &title) {
            auto *step = new AgentStepWidget(iter, reason, toolId, title, m_chatView);
            if (!m_currentStepGroup) {
                m_currentStepGroup = new AgentStepGroupWidget(m_chatView->getContentWidget());
                m_chatView->addAgentStepWidget(nullptr); // 触发占位（由 group 替换）
                // 将 group 插入到最后一个 message 之后
                auto *layout = m_chatView->getContentWidget()->findChild<QVBoxLayout *>();
                if (layout) layout->addWidget(m_currentStepGroup);
            }
            m_currentStepGroup->addStep(step);
        });

    connect(&m_controller, &ApplicationController::agentLoopToolFinished,
        this, [this](int iter, const QString &toolId, bool ok, const QString &preview) {
            if (m_currentStepGroup) {
                m_currentStepGroup->setStepResult(iter, toolId, ok, preview);
            }
        });

    // V18.4: Agent 循环完成 → 折叠步骤组
    connect(&m_controller, &ApplicationController::agentLoopCompleted,
        this, [this]() {
            if (m_currentStepGroup) {
                m_currentStepGroup->finish();
                m_currentStepGroup = nullptr;
            }
        });

    // V16.3: Agent 调试信号
    connect(&m_controller, &ApplicationController::agentLoopPromptDebug,
        this, &MainWindow::onAgentDebugPrompt);

    // V17.3: Token 用量更新
    connect(&m_controller, &ApplicationController::tokenUsageUpdated, this,
        [this](int used, int limit) {
            if (m_chatView != nullptr) {
                m_chatView->updateTokenUsage(used, limit);
            }
        });

    // V17.4: Ctrl+F 搜索快捷键
    auto *searchShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+F")), this);
    connect(searchShortcut, &QShortcut::activated, this, [this]() {
        if (m_chatView != nullptr) {
            m_chatView->showSearchBar();
        }
    });
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

    m_currentStepGroup = nullptr;
    m_chatView->clearMessages();
    m_messageWidgets.clear();
    if (m_controller.currentSession().messages.isEmpty()) {
        m_chatView->addMessage(MessageRole::Assistant,
                               text(QStringLiteral("Start a conversation by configuring your API settings, then send a message."),
                                    QStringLiteral("请先配置 API 设置，然后发送消息开始对话。")));
        return;
    }

    const auto &session = m_controller.currentSession();
    for (const ChatMessage &message : session.messages) {
        auto *widget = m_chatView->addMessage(message.role, message.content, message.id);
        m_messageWidgets.insert(message.id, widget);

        // CH-8: 连接编辑信号
        connect(widget, &MessageWidget::editConfirmed, this, [this, widget](const QString &newContent) {
            QString mid = widget->property("messageId").toString();
            onMessageEditConfirmed(mid, newContent);
        });

        // V16.3: 连接右键菜单信号
        connect(widget, &MessageWidget::deleteRequested, this, [this, widget]() {
            onMessageDeleteRequested(widget);
        });
        if (message.role == MessageRole::Assistant) {
            connect(widget, &MessageWidget::regenerateRequested, this, &MainWindow::onMessageRegenerateRequested);
        }
        if (message.role == MessageRole::User) {
            connect(widget, &MessageWidget::quoteReplyRequested, this, &MainWindow::onQuoteReplyRequested);
        }
    }

    // AG-4: 渲染 Agent 执行步骤回放
    if (!session.agentSteps.isEmpty()) {
        for (const auto &step : session.agentSteps) {
            auto *stepWidget = new AgentStepWidget(
                step.stepNumber,
                step.reasoning,
                step.toolName,
                step.toolName,
                m_chatView->getContentWidget());
            if (step.status == QStringLiteral("success")) {
                stepWidget->setResult(true, step.toolResult.left(120));
            } else if (step.status == QStringLiteral("error")) {
                stepWidget->setResult(false, step.toolResult.left(120));
            } else {
                stepWidget->setResult(true, step.toolResult.left(120));
            }
            m_chatView->addAgentStepWidget(stepWidget);
        }
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
    m_agentPlanButton->setText(text(QStringLiteral("Agent Plan"), QStringLiteral("Agent 计划")));
    m_logButton->setText(text(QStringLiteral("Logs"), QStringLiteral("日志")));
    m_settingsButton->setText(text(QStringLiteral("Settings"), QStringLiteral("设置")));
    m_scheduledTaskButton->setText(text(QStringLiteral("Scheduled Tasks"), QStringLiteral("调度任务")));
    m_retryButton->setText(text(QStringLiteral("Retry"), QStringLiteral("重试")));
    m_personaLabel->setText(text(QStringLiteral("Role: %1"), QStringLiteral("角色：%1")).arg(currentRoleDisplayName()));
    m_messageInput->setPlaceholderText(text(QStringLiteral("Type a message..."), QStringLiteral("输入消息...")));
    updateSessionFilterButtons();
    updateSessionOrganizationControls();
    updateSendButtonAppearance();  // 也会更新模式切换按钮文案

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

    // 模式切换按钮样式：Agent 模式高亮
    m_modeToggleButton->setText(m_isAgentMode
        ? text(QStringLiteral("Agent"), QStringLiteral("Agent"))
        : text(QStringLiteral("Chat"), QStringLiteral("Chat")));
    if (m_isAgentMode) {
        m_modeToggleButton->setStyleSheet(QStringLiteral(
            "QPushButton#modeToggleButton {"
            "  background: #4f46e5; border: 1px solid #4338ca; color: #ffffff;"
            "  font-weight: 600; font-size: 12px; border-radius: 6px;"
            "}"
            "QPushButton#modeToggleButton:hover {"
            "  background: #4338ca; border-color: #3730a3;"
            "}"));
    } else {
        m_modeToggleButton->setStyleSheet(QStringLiteral(
            "QPushButton#modeToggleButton {"
            "  background: transparent; border: 1px solid #d1d5db; color: #6b7280;"
            "  font-weight: 500; font-size: 12px; border-radius: 6px;"
            "}"
            "QPushButton#modeToggleButton:hover {"
            "  background: #f3f4f6; border-color: #9ca3af; color: #374151;"
            "}"));
    }
    m_modeToggleButton->style()->unpolish(m_modeToggleButton);
    m_modeToggleButton->style()->polish(m_modeToggleButton);
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
    dialog.setDebugMode(m_controller.agentDebugMode());

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_controller.saveConfig(dialog.config());
    m_controller.setAgentDebugMode(dialog.debugMode());
}

void MainWindow::openLogViewerDialog()
{
    LogViewerDialog dialog(AppLogger::logFilePath(), m_controller.config().language, this);
    dialog.exec();
}

void MainWindow::openToolsDialog()
{
    ToolsDialog dialog(m_controller.config().language, this);
    // V12.5: insertToolOutputIntoInput 已移除，工具输出不再插入聊天输入框
    dialog.exec();
}

void MainWindow::openScheduledTaskDialog()
{
    ScheduledTaskDialog dialog(&m_controller, this);
    dialog.exec();
}

// V15.5: openFileToolsDialog 已移除，FileInteractionService API 保留不变。
// 文件工具功能现通过 Agent Skill（.workbuddy/skills/file-*/）调用，见 docs/FileInteractionService-API.md

void MainWindow::generateAgentPlan()
{
    m_controller.generateAgentPlan(m_messageInput->toPlainText());
}

// V12.5: openAgentPlanDialog 和 insertToolOutputIntoInput 已移除。
// 计划不再弹出窗口，改为自动执行；工具输出插入功能已淘汰。

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
    // V17.1: 即使文本为空，如果有图片也可以发送
    if (content.isEmpty() && m_pendingImages.isEmpty()) {
        return;
    }

    // V17.3: DeepSeek API 不支持图片输入（识图模式仅网页端可用）
    // 自动剥离图片，只发文字。保留 sendMessageWithImages 用于未来支持视觉的模型。
    if (!m_pendingImages.isEmpty()) {
        const int imageCount = m_pendingImages.size();
        clearPendingImages();
        if (content.isEmpty()) {
            showStatusMessage(
                QStringLiteral("Image not sent — current model does not support image input"),
                QStringLiteral("图片未发送 — 当前模型不支持图片输入"),
                4000);
            m_messageInput->clear();
            return;
        }
        showStatusMessage(
            QStringLiteral("%1 image(s) stripped — current model does not support image input")
                .arg(imageCount),
            QStringLiteral("已剥离 %1 张图片 — 当前模型不支持图片输入").arg(imageCount),
            3000);
    }

    if (m_isAgentMode) {
        m_controller.sendAgentLoopMessage(content);  // V12.6: Agent 循环执行
    } else {
        m_controller.sendMessage(content);            // 纯 Chat
    }
}

void MainWindow::toggleAgentMode()
{
    m_isAgentMode = !m_isAgentMode;
    updateSendButtonAppearance();

    // Agent 模式下输入框 placeholder 提示可执行循环任务
    m_messageInput->setPlaceholderText(
        m_isAgentMode
            ? text(QStringLiteral("Type a task — AI will loop and auto-execute..."),
                   QStringLiteral("...AI 自动判断并循环执行..."))
            : text(QStringLiteral("Type a message..."), QStringLiteral("输入消息...")));
}

void MainWindow::addUserMessage(const QString &content)
{
    const auto &session = m_controller.currentSession();
    QString msgId = session.messages.isEmpty() ? QString() : session.messages.last().id;
    auto *msg = m_chatView->addMessage(MessageRole::User, content, msgId);
    if (!msgId.isEmpty()) {
        m_messageWidgets.insert(msgId, msg);
    }
    // CH-8: 连接编辑确认信号
    connect(msg, &MessageWidget::editConfirmed, this, [this, msg](const QString &newContent) {
        QString mid = msg->property("messageId").toString();
        onMessageEditConfirmed(mid, newContent);
    });
    // V16.3: 连接右键菜单信号
    connect(msg, &MessageWidget::deleteRequested, this, [this, msg]() {
        onMessageDeleteRequested(msg);
    });
    connect(msg, &MessageWidget::quoteReplyRequested, this, &MainWindow::onQuoteReplyRequested);
    m_messageInput->clear();
}

void MainWindow::addAssistantPlaceholder()
{
    const auto &session = m_controller.currentSession();
    QString msgId = session.messages.isEmpty() ? QString() : session.messages.last().id;
    auto *msg = m_chatView->addMessage(MessageRole::Assistant,
        text(QStringLiteral("Thinking..."), QStringLiteral("思考中...")), msgId);
    if (!msgId.isEmpty()) {
        m_messageWidgets.insert(msgId, msg);
    }
    // V16.3: 连接右键菜单信号
    connect(msg, &MessageWidget::deleteRequested, this, [this, msg]() {
        onMessageDeleteRequested(msg);
    });
    connect(msg, &MessageWidget::regenerateRequested, this, &MainWindow::onMessageRegenerateRequested);

    // V17.4: 显示打字指示器
    if (m_chatView != nullptr) {
        m_chatView->showTyping();
    }
}

void MainWindow::setGenerating(bool generating)
{
    m_messageInput->setEnabled(!generating);
    m_modeToggleButton->setEnabled(!generating);
    m_newChatButton->setEnabled(!generating);
    m_renameChatButton->setEnabled(!generating);
    m_exportChatButton->setEnabled(!generating);
    m_favoriteChatButton->setEnabled(!generating);
    m_archiveChatButton->setEnabled(!generating);
    m_deleteChatButton->setEnabled(!generating);
    m_systemPromptButton->setEnabled(!generating);
    m_toolsButton->setEnabled(true);
    m_agentPlanButton->setEnabled(!generating);
    m_settingsButton->setEnabled(!generating);
    m_scheduledTaskButton->setEnabled(!generating);
    m_logButton->setEnabled(true);
    m_retryButton->setEnabled(!generating && m_retryButton->isVisible());
    m_themeToggleButton->setEnabled(!generating);
    updateSendButtonAppearance();
    updateSendButtonState();

    // V17.4: 生成结束时隐藏打字指示器
    if (!generating && m_chatView != nullptr) {
        m_chatView->hideTyping();
    }
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

// ─── V16.3: 右键菜单处理 ────────────────────────────────────────────

void MainWindow::onMessageDeleteRequested(MessageWidget *msg)
{
    // 从布局中移除并删除 widget
    msg->setVisible(false);
    msg->deleteLater();
    // 注意：删除后信息会丢失，这里只做 UI 移除。消息数据层面的删除留给后续版本。
}

void MainWindow::onMessageRegenerateRequested()
{
    // V17.4: 对话分支 — 保存当前分支后再重新生成
    const auto &session = m_controller.currentSession();
    if (!session.messages.isEmpty()) {
        m_controller.createMessageBranch(session.messages.last().id);
    }

    m_controller.retryLastRequest();
}

void MainWindow::onQuoteReplyRequested(const QString &content)
{
    QString quoted = QStringLiteral("> ") + content.trimmed() + QStringLiteral("\n\n");
    m_messageInput->insertPlainText(quoted);
    m_messageInput->setFocus();
}

// ─── CH-8: onMessageEditConfirmed ────────────────────────────────────

void MainWindow::onMessageEditConfirmed(const QString &messageId, const QString &newContent)
{
    // 1. 编辑消息内容（保留在会话中）
    m_controller.editCurrentMessage(messageId, newContent);

    // 2. 从 UI 中移除该消息及之后的所有消息
    m_chatView->removeMessagesFrom(messageId);

    // 3. 截断会话中该消息之后的所有消息
    m_controller.truncateCurrentSessionFrom(messageId);

    // 4. 更新 widget 显示
    auto *widget = m_messageWidgets.value(messageId);
    if (widget != nullptr) {
        widget->setContent(newContent);
    }

    // 5. 保存会话
    m_controller.saveCurrentSession();

    // 6. 设置输入框并重新发送（触发 AI 回复）
    m_messageInput->setPlainText(newContent);
    sendCurrentMessage();
}

// ─── V16.3: 主题切换 ────────────────────────────────────────────────

void MainWindow::toggleDarkMode()
{
    bool current = qApp->property("darkMode").toBool();
    bool dark = !current;
    qApp->setProperty("darkMode", dark);

    // 设置 window 级别的属性以便 QSS 选择器工作
    setProperty("darkMode", dark);
    centralWidget()->setProperty("darkMode", dark);
    style()->unpolish(this);
    style()->polish(this);
    centralWidget()->style()->unpolish(centralWidget());
    centralWidget()->style()->polish(centralWidget());

    // 强制刷新所有子 widget
    for (auto *child : findChildren<QWidget *>()) {
        child->style()->unpolish(child);
        child->style()->polish(child);
    }

    // 更新日月图标
    m_themeToggleButton->setText(dark ? QStringLiteral("\xE2\x98\x80")    // ☀ 暗色模式 → 可切回亮色
                                      : QStringLiteral("\xF0\x9F\x8C\x99"));  // 🌙 亮色模式 → 可切暗色

    // 重建消息 Markdown 渲染
    populateChatView();
}

// ─── V16.3: Agent 调试模式 ──────────────────────────────────────────

void MainWindow::onAgentDebugPrompt(const QString &prompt)
{
    m_chatView->addDebugCard(
        text(QStringLiteral("Agent Loop Prompt"), QStringLiteral("Agent 循环提示词")),
        prompt);
}

// ─── V17.1: 事件过滤器 — 拦截图片粘贴 ──────────────────────────────

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_messageInput && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Paste)) {
            QClipboard *cb = QApplication::clipboard();
            if (cb == nullptr) {
                return QMainWindow::eventFilter(obj, event);
            }
            const QImage image = cb->image();
            if (!image.isNull()) {
                onImagePasted(image);
                return true; // 消费事件，避免图片 base64 文本出现在输入框
            }
        }
        return QMainWindow::eventFilter(obj, event);
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onImagePasted(const QImage &image)
{
    if (m_pendingImages.size() >= kMaxPendingImages) {
        showStatusMessage(
            QStringLiteral("Maximum %1 images allowed").arg(kMaxPendingImages),
            QStringLiteral("最多支持 %1 张图片").arg(kMaxPendingImages),
            3000);
        return;
    }

    // 缩放图片用于 API（max 1024px 宽）
    QImage apiImage = image;
    if (apiImage.width() > 1024) {
        apiImage = apiImage.scaledToWidth(1024, Qt::SmoothTransformation);
    }

    // 转 base64 PNG（完整分辨率，用于 API 发送）
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    apiImage.save(&buf, "PNG");
    buf.close();

    QString base64 = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(ba.toBase64());
    m_pendingImages.append(base64);

    // 插入内联缩略图到 QTextEdit 光标位置（max 200px 宽显示）
    QImage displayImg = image.scaledToWidth(200, Qt::SmoothTransformation);

    QTextCursor cursor = m_messageInput->textCursor();
    // 如果光标前不是空白，先插入一个换行让图片独立成行
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    QString leadingText = cursor.selectedText().trimmed();
    cursor.movePosition(QTextCursor::End);
    if (!leadingText.isEmpty()) {
        cursor.insertText(QStringLiteral("\n"));
    }
    cursor.insertImage(displayImg);
    cursor.insertText(QStringLiteral(" ")); // 图片后留一个空格方便继续打字

    m_messageInput->setTextCursor(cursor);

    showStatusMessage(
        QStringLiteral("Image attached (%1 total)").arg(m_pendingImages.size()),
        QStringLiteral("已添加图片（共 %1 张）").arg(m_pendingImages.size()),
        2000);
}

void MainWindow::clearPendingImages()
{
    m_pendingImages.clear();
}
