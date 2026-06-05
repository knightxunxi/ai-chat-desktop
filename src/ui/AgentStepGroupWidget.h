#pragma once

#include <QFrame>
#include <QString>
#include <QVector>

class AgentStepWidget;
class QLabel;
class QVBoxLayout;

// V18.4: Agent 步骤分组卡片 — 将多个步骤折叠成一个可展开的摘要
class AgentStepGroupWidget : public QFrame
{
    Q_OBJECT

public:
    explicit AgentStepGroupWidget(QWidget *parent = nullptr);
    void addStep(AgentStepWidget *step);
    void setStepResult(int iteration, const QString &toolId, bool ok, const QString &preview);
    void finish();
    int stepCount() const { return m_steps.size(); }

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void applyCollapsedState();
    void setupUi();

    QLabel *m_headerLabel = nullptr;
    QWidget *m_detailContainer = nullptr;
    QVBoxLayout *m_detailLayout = nullptr;
    QVector<AgentStepWidget *> m_steps;
    bool m_collapsed = true;
    bool m_finished = false;
};
