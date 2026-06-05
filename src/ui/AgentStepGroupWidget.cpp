#include "ui/AgentStepGroupWidget.h"
#include "ui/AgentStepWidget.h"

#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

AgentStepGroupWidget::AgentStepGroupWidget(QWidget *parent)
    : QFrame(parent)
    , m_collapsed(true)
{
    setObjectName(QStringLiteral("agentStepGroup"));
    setCursor(Qt::PointingHandCursor);
    setupUi();
    applyCollapsedState();
}

void AgentStepGroupWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 6, 8, 6);
    root->setSpacing(4);

    m_headerLabel = new QLabel(this);
    m_headerLabel->setObjectName(QStringLiteral("agentStepGroupHeader"));
    m_headerLabel->setCursor(Qt::PointingHandCursor);
    m_headerLabel->setText(QStringLiteral("\u25B6 Executing..."));
    root->addWidget(m_headerLabel);

    m_detailContainer = new QWidget(this);
    m_detailContainer->setObjectName(QStringLiteral("agentStepGroupDetail"));
    m_detailLayout = new QVBoxLayout(m_detailContainer);
    m_detailLayout->setContentsMargins(12, 2, 0, 2);
    m_detailLayout->setSpacing(2);
    root->addWidget(m_detailContainer);
}

void AgentStepGroupWidget::addStep(AgentStepWidget *step)
{
    if (!step) return;
    m_steps.append(step);

    // 放到 detail 容器中，不再单独加入 ChatView
    step->setParent(m_detailContainer);
    m_detailLayout->addWidget(step);

    // 更新实时标题
    QStringList toolNames;
    for (int i = 0; i < m_steps.size() && i < 5; ++i) {
        // 从已有的 step header 中提取工具名
        if (auto *hdr = m_steps[i]->findChild<QLabel *>(QStringLiteral("agentStepHeader"))) {
            QString txt = hdr->text();
            txt.replace(QStringLiteral("\u25B6 "), QString()).replace(QStringLiteral("\u25BC "), QString());
            // 提取工具名（"Step N — Name" 的 Name 部分）
            int dash = txt.indexOf(QStringLiteral(" — "));
            if (dash > 0) txt = txt.mid(dash + 3);
            toolNames.append(txt.left(20));
        }
    }
    QString summary = toolNames.join(QStringLiteral(", "));
    if (m_steps.size() > 5) summary += QStringLiteral(", ...");
    m_headerLabel->setText(QStringLiteral("\u25B6 [%1/%1] %2").arg(m_steps.size()).arg(summary));
}

void AgentStepGroupWidget::setStepResult(int iteration, const QString &toolId, bool ok, const QString &preview)
{
    AgentStepWidget *fallback = nullptr;
    for (auto *step : m_steps) {
        if (!step || step->iteration() != iteration) {
            continue;
        }
        if (fallback == nullptr && !step->hasResult()) {
            fallback = step;
        }
        if (step->toolId() == toolId && !step->hasResult()) {
            step->setResult(ok, preview);
            return;
        }
    }

    for (auto *step : m_steps) {
        if (step && step->iteration() == iteration && step->toolId() == toolId) {
            step->setResult(ok, preview);
            return;
        }
    }

    if (fallback != nullptr) {
        fallback->setResult(ok, preview);
    }
}

void AgentStepGroupWidget::finish()
{
    m_finished = true;
    int succeeded = 0;
    int failed = 0;
    QStringList toolNames;
    for (auto *step : m_steps) {
        if (step) {
            // 从 header 提取状态
            if (auto *hdr = step->findChild<QLabel *>(QStringLiteral("agentStepHeader"))) {
                QString txt = hdr->text();
                if (txt.contains(QStringLiteral("\u2705"))) succeeded++;
                else if (txt.contains(QStringLiteral("\u274C"))) failed++;
                else succeeded++; // 没有失败标记就视为成功

                txt.replace(QStringLiteral("\u25B6 "), QString()).replace(QStringLiteral("\u25BC "), QString());
                int dash = txt.indexOf(QStringLiteral(" — "));
                if (dash > 0) txt = txt.mid(dash + 3);
                if (toolNames.size() < 5) toolNames.append(txt.left(20));
            }
        }
    }
    if (toolNames.size() < m_steps.size()) toolNames.append(QStringLiteral("..."));

    const QString statusIco = failed > 0 ? QStringLiteral("\u26A0\uFE0F") : QStringLiteral("\u2705");
    m_headerLabel->setText(QStringLiteral("%1 Completed: %2 steps — %3")
        .arg(statusIco).arg(m_steps.size()).arg(toolNames.join(QStringLiteral(", "))));
    m_collapsed = true;
    applyCollapsedState();
}

void AgentStepGroupWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_collapsed = !m_collapsed;
    applyCollapsedState();
}

void AgentStepGroupWidget::applyCollapsedState()
{
    m_detailContainer->setVisible(!m_collapsed);

    QString header = m_headerLabel->text();
    if (m_collapsed) {
        if (header.contains(QStringLiteral("\u25BC ")))
            header.replace(QStringLiteral("\u25BC "), QStringLiteral("\u25B6 "));
        else if (!header.startsWith(QStringLiteral("\u25B6 ")) && !header.startsWith(QStringLiteral("\u2705")) && !header.startsWith(QStringLiteral("\u26A0")))
            header = QStringLiteral("\u25B6 ") + header;
    } else {
        // 替换 ▶ 为 ▼
        if (header.contains(QStringLiteral("\u25B6 ")))
            header.replace(QStringLiteral("\u25B6 "), QStringLiteral("\u25BC "));
        for (auto *step : m_steps) {
            if (step) step->setCollapsed(false); // 展开时展开所有子步骤
        }
    }
    m_headerLabel->setText(header);
}
