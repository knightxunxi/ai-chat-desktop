#pragma once

#include "app/ApplicationController.h"
#include "core/ChatSession.h"

#include <QMainWindow>

class ChatView;
class QCloseEvent;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLineEdit;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

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
    void openLogViewerDialog();
    void editSystemPrompt();
    void startNewChat();
    void renameCurrentChat();
    void exportCurrentChat();
    void deleteCurrentChat();
    void switchToSession(QListWidgetItem *item);
    void sendCurrentMessage();
    void addUserMessage(const QString &content);
    void addAssistantPlaceholder();
    void setGenerating(bool generating);
    void setRetryAvailable(bool available);
    void showConfigurationMissingWarning();
    void showStatusMessage(const QString &english, const QString &chinese, int timeoutMs);
    void showStartupWarning(const QString &english, const QString &chinese);

    ApplicationController m_controller;
    QListWidget *m_sessionList = nullptr;
    QLineEdit *m_sessionSearchEdit = nullptr;
    QPushButton *m_newChatButton = nullptr;
    QPushButton *m_renameChatButton = nullptr;
    QPushButton *m_exportChatButton = nullptr;
    QPushButton *m_deleteChatButton = nullptr;
    QPushButton *m_systemPromptButton = nullptr;
    QPushButton *m_logButton = nullptr;
    ChatView *m_chatView = nullptr;
    QTextEdit *m_messageInput = nullptr;
    QPushButton *m_retryButton = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QLabel *m_modelLabel = nullptr;
    QLabel *m_personaLabel = nullptr;
};
