#pragma once

#include <QDialog>

class SessionCoordinator;

// V19: 使用统计面板 — 显示会话数、消息数、Agent 循环次数等统计
class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsDialog(const SessionCoordinator *sessionCoordinator, QWidget *parent = nullptr);

private:
    void setupUi();

    const SessionCoordinator *m_sessionCoordinator = nullptr; // 功能：读取真实会话历史统计。
};
