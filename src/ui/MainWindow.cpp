#include "ui/MainWindow.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    updateSendButtonState();
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

    auto *newChatButton = new QPushButton(QStringLiteral("New Chat"), sidebar);
    newChatButton->setObjectName(QStringLiteral("newChatButton"));

    m_sessionList = new QListWidget(sidebar);
    m_sessionList->setObjectName(QStringLiteral("sessionList"));
    m_sessionList->addItem(QStringLiteral("Getting Started"));

    sidebarLayout->addWidget(newChatButton);
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

    m_modelLabel = new QLabel(QStringLiteral("Model: deepseek-v4-flash"), titleGroup);
    m_modelLabel->setObjectName(QStringLiteral("modelLabel"));

    m_personaLabel = new QLabel(QStringLiteral("Role: Default assistant"), titleGroup);
    m_personaLabel->setObjectName(QStringLiteral("personaLabel"));

    titleLayout->addWidget(m_modelLabel);
    titleLayout->addWidget(m_personaLabel);

    m_settingsButton = new QPushButton(QStringLiteral("Settings"), header);
    m_settingsButton->setObjectName(QStringLiteral("settingsButton"));

    headerLayout->addWidget(titleGroup, 1);
    headerLayout->addWidget(m_settingsButton);

    auto *scrollArea = new QScrollArea(mainPanel);
    scrollArea->setObjectName(QStringLiteral("chatScrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_chatContainer = new QWidget(scrollArea);
    m_chatContainer->setObjectName(QStringLiteral("chatContainer"));
    auto *chatLayout = new QVBoxLayout(m_chatContainer);
    chatLayout->setContentsMargins(28, 28, 28, 28);
    chatLayout->setSpacing(14);
    chatLayout->addStretch(1);

    auto *welcome = new QLabel(QStringLiteral("Start a conversation by configuring your API settings, then send a message."), m_chatContainer);
    welcome->setObjectName(QStringLiteral("welcomeMessage"));
    welcome->setWordWrap(true);
    chatLayout->insertWidget(0, welcome);

    scrollArea->setWidget(m_chatContainer);

    auto *composer = new QFrame(mainPanel);
    composer->setObjectName(QStringLiteral("composer"));
    auto *composerLayout = new QHBoxLayout(composer);
    composerLayout->setContentsMargins(24, 18, 24, 24);
    composerLayout->setSpacing(12);

    m_messageInput = new QTextEdit(composer);
    m_messageInput->setObjectName(QStringLiteral("messageInput"));
    m_messageInput->setPlaceholderText(QStringLiteral("Type a message..."));
    m_messageInput->setFixedHeight(88);

    m_sendButton = new QPushButton(QStringLiteral("Send"), composer);
    m_sendButton->setObjectName(QStringLiteral("sendButton"));
    m_sendButton->setFixedSize(96, 44);

    composerLayout->addWidget(m_messageInput, 1);
    composerLayout->addWidget(m_sendButton, 0, Qt::AlignBottom);

    mainLayout->addWidget(header);
    mainLayout->addWidget(scrollArea, 1);
    mainLayout->addWidget(composer);

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(mainPanel, 1);

    setCentralWidget(central);

    connect(m_messageInput, &QTextEdit::textChanged, this, &MainWindow::updateSendButtonState);
}

void MainWindow::updateSendButtonState()
{
    const bool hasText = !m_messageInput->toPlainText().trimmed().isEmpty();
    m_sendButton->setEnabled(hasText);
}
