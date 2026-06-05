#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class QMouseEvent;

// V16.1: Agent 思考步骤卡片 — 显示 AI 推理、工具选择和执行结果，默认折叠
class AgentStepWidget : public QFrame
{
    Q_OBJECT

public:
    explicit AgentStepWidget(int iteration, const QString &reasoning,
                             const QString &toolId, const QString &title,
                             QWidget *parent = nullptr);

    // 功能：返回当前轮次；使用模块：MainWindow 查找对应步骤设置结果。
    int iteration() const;
    QString toolId() const;
    bool hasResult() const;

    // 功能：设置工具执行结果；使用模块：agentLoopToolFinished 信号。
    void setResult(bool ok, const QString &outputPreview);

    // 功能：标记为思考步骤（无工具调用）；使用模块：纯思考步骤。
    void markAsThinking();

    // 功能：设置折叠/展开状态；使用模块：外部控制展开。
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void applyCollapsedState();

    int m_iteration = 0;
    QString m_toolId;
    QLabel *m_headerLabel = nullptr;
    QLabel *m_reasoningLabel = nullptr;
    QLabel *m_resultLabel = nullptr;
    bool m_collapsed = true;
    bool m_hasResult = false;
};
