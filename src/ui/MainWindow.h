#pragma once

#include "core/AppConfig.h"
#include "core/ChatSession.h"
#include "services/OpenAICompatibleClient.h"
#include "storage/ChatHistoryStorage.h"
#include "storage/ConfigStorage.h"

#include <QMainWindow>

class ChatView;
class QLabel;
class QListWidget;
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
    void populateChatView();
    void applyConfig();
    void applyLanguage();
    QString text(const QString &english, const QString &chinese) const;
    void updateSendButtonState();
    void openSettingsDialog();
    void sendCurrentMessage();
    void handleTextDelta(const QString &delta);
    void handleRequestFinished();
    void handleRequestFailed(const QString &message);
    void setGenerating(bool generating);
    void saveCurrentSession();

    AppConfig m_config;
    ChatSession m_session;
    ConfigStorage m_configStorage;
    ChatHistoryStorage m_chatHistoryStorage;
    OpenAICompatibleClient m_aiClient;
    QString m_currentAssistantContent;
    bool m_isGenerating = false;
    QListWidget *m_sessionList = nullptr;
    QPushButton *m_newChatButton = nullptr;
    ChatView *m_chatView = nullptr;
    QTextEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QLabel *m_modelLabel = nullptr;
    QLabel *m_personaLabel = nullptr;
};
