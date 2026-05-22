#pragma once

#include "app/ApplicationController.h"
#include "core/ChatSession.h"

#include <QMainWindow>

class ChatView;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void connectController();
    void populateSessionList();
    void updateCurrentSessionListItem();
    QListWidgetItem *findSessionItem(const QString &sessionId) const;
    QString sessionListTitle(const ChatSession &session) const;
    void populateChatView();
    void applyConfig();
    void applyLanguage();
    QString text(const QString &english, const QString &chinese) const;
    QString currentRoleDisplayName() const;
    void updateSendButtonAppearance();
    void updateSendButtonState();
    void openSettingsDialog();
    void editSystemPrompt();
    void startNewChat();
    void deleteCurrentChat();
    void switchToSession(QListWidgetItem *item);
    void sendCurrentMessage();
    void addUserMessage(const QString &content);
    void addAssistantPlaceholder();
    void setGenerating(bool generating);
    void showConfigurationMissingWarning();
    void showStatusMessage(const QString &english, const QString &chinese, int timeoutMs);
    void showStartupWarning(const QString &english, const QString &chinese);

    ApplicationController m_controller;
    QListWidget *m_sessionList = nullptr;
    QPushButton *m_newChatButton = nullptr;
    QPushButton *m_deleteChatButton = nullptr;
    QPushButton *m_systemPromptButton = nullptr;
    ChatView *m_chatView = nullptr;
    QTextEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QLabel *m_modelLabel = nullptr;
    QLabel *m_personaLabel = nullptr;
};
