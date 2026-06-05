#include "ui/AgentStepWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

AgentStepWidget::AgentStepWidget(int iteration, const QString &reasoning,
                                 const QString &toolId, const QString &title,
                                 QWidget *parent)
    : QFrame(parent)
    , m_iteration(iteration)
    , m_collapsed(true)
{
    setObjectName(QStringLiteral("agentStepWidget"));
    setCursor(Qt::PointingHandCursor);

    setupUi();

    // 设置 header（初始带折叠箭头）
    const QString headerText = QStringLiteral("Step %1").arg(iteration);
    const QString toolDesc = title.isEmpty() ? toolId : title;
    const QString toolInfo = toolId.isEmpty()
        ? QString()
        : QStringLiteral(" — %1").arg(toolDesc);
    m_headerLabel->setText(QStringLiteral("\u25B6 ") + headerText + toolInfo);

    // 设置 reasoning
    if (!reasoning.isEmpty()) {
        m_reasoningLabel->setText(reasoning);
    }

    // 初始折叠
    applyCollapsedState();
}

int AgentStepWidget::iteration() const
{
    return m_iteration;
}

void AgentStepWidget::setResult(bool ok, const QString &outputPreview)
{
    const QString symbol = ok ? QStringLiteral("\u2705") : QStringLiteral("\u274C");
    const QString label = ok ? QStringLiteral("Output") : QStringLiteral("Failed");

    QString resultText = QStringLiteral("%1 %2: %3").arg(symbol, label, outputPreview);
    m_resultLabel->setText(resultText);
    m_resultLabel->setVisible(!m_collapsed);
}

void AgentStepWidget::markAsThinking()
{
    m_headerLabel->setText(QStringLiteral("\u25B6 Step %1 — Thinking").arg(m_iteration));
    m_reasoningLabel->hide();
    m_resultLabel->hide();
}

void AgentStepWidget::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    applyCollapsedState();
}

void AgentStepWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_collapsed = !m_collapsed;
    applyCollapsedState();
}

void AgentStepWidget::applyCollapsedState()
{
    // 更新箭头
    QString header = m_headerLabel->text();
    if (m_collapsed) {
        header.replace(QStringLiteral("\u25BC "), QStringLiteral("\u25B6 "));
        if (!header.startsWith(QStringLiteral("\u25B6 ")) && !header.startsWith(QStringLiteral("\u25BC "))) {
            header = QStringLiteral("\u25B6 ") + header;
        }
    } else {
        header.replace(QStringLiteral("\u25B6 "), QStringLiteral("\u25BC "));
    }
    m_headerLabel->setText(header);

    m_reasoningLabel->setVisible(!m_collapsed && !m_reasoningLabel->text().isEmpty());
    m_resultLabel->setVisible(!m_collapsed && !m_resultLabel->text().isEmpty());
}

void AgentStepWidget::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(10, 8, 10, 8);
    rootLayout->setSpacing(4);

    // Header — 粗体，显示轮次和工具
    m_headerLabel = new QLabel(this);
    m_headerLabel->setObjectName(QStringLiteral("agentStepHeader"));
    m_headerLabel->setWordWrap(true);
    m_headerLabel->setCursor(Qt::PointingHandCursor);
    rootLayout->addWidget(m_headerLabel);

    // Reasoning — 浅灰色，折叠时隐藏
    m_reasoningLabel = new QLabel(this);
    m_reasoningLabel->setObjectName(QStringLiteral("agentStepReasoning"));
    m_reasoningLabel->setWordWrap(true);
    rootLayout->addWidget(m_reasoningLabel);

    // Result — 执行结果，折叠时隐藏
    m_resultLabel = new QLabel(this);
    m_resultLabel->setObjectName(QStringLiteral("agentStepResult"));
    m_resultLabel->setWordWrap(true);
    rootLayout->addWidget(m_resultLabel);
}
