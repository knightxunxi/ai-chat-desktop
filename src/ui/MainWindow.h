#pragma once

#include <QMainWindow>

class QLabel;
class QListWidget;
class QPushButton;
class QTextEdit;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void updateSendButtonState();

    QListWidget *m_sessionList = nullptr;
    QWidget *m_chatContainer = nullptr;
    QTextEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QLabel *m_modelLabel = nullptr;
    QLabel *m_personaLabel = nullptr;
};
