#pragma once

#include "scheduler/ScheduledTask.h"

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVector>

class TaskScheduler : public QObject {
    Q_OBJECT
public:
    explicit TaskScheduler(QObject *parent = nullptr);

    void addTask(const ScheduledTask &task);
    void removeTask(const QString &taskId);
    void updateTask(const ScheduledTask &task);
    QVector<ScheduledTask> allTasks() const;
    void start();
    void stop();
    bool isRunning() const;

signals:
    void taskTriggered(const ScheduledTask &task);

private:
    void tick();
    QDateTime nextRunTime(const QString &cron, const QDateTime &from);

    QTimer *m_timer = nullptr;
    QVector<ScheduledTask> m_tasks;
};
