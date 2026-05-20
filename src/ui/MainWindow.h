#pragma once

#include "core/AppConfig.h"
#include "core/ChatSession.h"
#include "services/OpenAICompatibleClient.h"
#include "storage/ChatHistoryStorage.h"
#include "storage/ConfigStorage.h"

#include <QMainWindow>
#include <QVector>

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
    void loadConfig();
    void loadSession();
    void populateSessionList();
    void updateCurrentSessionListItem(bool moveToTop = false);
    QListWidgetItem *findSessionItem(const QString &sessionId) const;
    QString sessionListTitle(const ChatSession &session) const;
    void populateChatView();
    void applyConfig();
    void applyLanguage();
    QString text(const QString &english, const QString &chinese) const;
    void updateSendButtonState();
    void openSettingsDialog();
    void editSystemPrompt();
    void startNewChat();
    void switchToSession(QListWidgetItem *item);
    void sendCurrentMessage();
    void handleTextDelta(const QString &delta);
    void handleRequestFinished();
    void handleRequestFailed(const QString &message);
    void setGenerating(bool generating);
    bool saveCurrentSession();
    void showStartupWarningIfNeeded();

    AppConfig m_config;
    ChatSession m_session;
    QVector<ChatSession> m_sessionSummaries;
    ConfigStorage m_configStorage;
    ChatHistoryStorage m_chatHistoryStorage;
    OpenAICompatibleClient m_aiClient;
    QString m_currentAssistantContent;
    QString m_startupWarningMessage;
    bool m_historyAvailable = false;
    bool m_isGenerating = false;
    QListWidget *m_sessionList = nullptr;
    QPushButton *m_newChatButton = nullptr;
    QPushButton *m_systemPromptButton = nullptr;
    ChatView *m_chatView = nullptr;
    QTextEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QLabel *m_modelLabel = nullptr;
    QLabel *m_personaLabel = nullptr;
};
