#include "ui/StatisticsDialog.h"

#include "app/SessionCoordinator.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

StatisticsDialog::StatisticsDialog(const SessionCoordinator *sessionCoordinator, QWidget *parent)
    : QDialog(parent)
    , m_sessionCoordinator(sessionCoordinator)
{
    setWindowTitle(QStringLiteral("Statistics"));
    resize(400, 300);
    setupUi();
}

void StatisticsDialog::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(8);

    // 统计项
    int sessionCount = 0, messageCount = 0, agentLoopCount = 0;

    if (m_sessionCoordinator != nullptr && m_sessionCoordinator->isHistoryAvailable()) {
        const ChatHistoryStorage *storage = m_sessionCoordinator->chatHistoryStorage();
        QSet<QString> sessionIds;
        QString error;
        for (const ChatSession &summary : storage->loadSessionSummaries(SessionListFilter::Active, &error)) {
            sessionIds.insert(summary.id);
        }
        for (const ChatSession &summary : storage->loadSessionSummaries(SessionListFilter::Archived, &error)) {
            sessionIds.insert(summary.id);
        }

        sessionCount = sessionIds.size();
        for (const QString &sessionId : sessionIds) {
            const std::optional<ChatSession> session = storage->loadSession(sessionId, &error);
            if (!session.has_value()) {
                continue;
            }
            messageCount += session->messages.size();
            agentLoopCount += session->agentSteps.isEmpty() ? 0 : 1;
        }
    } else if (m_sessionCoordinator != nullptr) {
        const ChatSession &session = m_sessionCoordinator->currentSession();
        sessionCount = 1;
        messageCount = session.messages.size();
        agentLoopCount = session.agentSteps.isEmpty() ? 0 : 1;
    }

    auto addStat = [&](const QString &label, int value) {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(label, this));
        row->addStretch();
        auto *val = new QLabel(QString::number(value), this);
        val->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 16px;"));
        row->addWidget(val);
        root->addLayout(row);
    };

    addStat(QStringLiteral("Sessions:"), sessionCount);
    addStat(QStringLiteral("Messages:"), messageCount);
    addStat(QStringLiteral("Agent Loops:"), agentLoopCount);

    root->addStretch();

    auto *btn = new QPushButton(QStringLiteral("Close"), this);
    connect(btn, &QPushButton::clicked, this, &QDialog::accept);
    root->addWidget(btn, 0, Qt::AlignCenter);
}
