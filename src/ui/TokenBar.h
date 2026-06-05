#pragma once

#include <QFrame>

class QLabel;
class QProgressBar;

// V17.3: Token 用量可视化条，显示 "2,847 / 8,192 tokens" 和进度条。
// 使用模块：ChatView 底部嵌入，MainWindow 通过 updateTokenUsage 驱动。
class TokenBar : public QFrame
{
    Q_OBJECT

public:
    explicit TokenBar(QWidget *parent = nullptr);

    // 功能：更新 token 用量显示；参数 used 为已用 token 数，limit 为上下文窗口上限。
    void updateTokens(int used, int limit);

private:
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_countLabel = nullptr;
};
