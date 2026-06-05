#pragma once

#include <QFrame>

class QLabel;
class QTimer;

// V17.4: 打字指示器，在 AI 思考时显示动画点。
// 使用模块：ChatView 底部嵌入，MainWindow 在 assistantMessageStarted/Finished 时控制。
class TypingIndicator : public QFrame
{
    Q_OBJECT

public:
    explicit TypingIndicator(QWidget *parent = nullptr);

    // 功能：开始动画循环；使用模块：assistantMessageStarted 信号。
    void startAnimation();
    // 功能：停止动画并隐藏；使用模块：assistantMessageFinished 信号。
    void stopAnimation();

private:
    QLabel *m_dotLabel = nullptr;
    QTimer *m_animTimer = nullptr;
    int m_dotCount = 0;
};
