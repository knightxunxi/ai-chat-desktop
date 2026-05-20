#pragma once

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
    void updateSendButtonState();
    void addLocalPreviewMessage();

    QListWidget *m_sessionList = nullptr;
    ChatView *m_chatView = nullptr;
    QTextEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QLabel *m_modelLabel = nullptr;
    QLabel *m_personaLabel = nullptr;
};
