#pragma once

#include <QDateTime>
#include <QString>

struct ScheduledTask {
    QString id;
    QString name;
    QString cronExpression;
    QString agentPrompt;
    bool enabled = true;
    QDateTime lastRun;
    QDateTime nextRun;
    int maxRetries = 3;

    bool isValid() const;
    static ScheduledTask create(const QString &name, const QString &cron,
                                 const QString &prompt);
};
