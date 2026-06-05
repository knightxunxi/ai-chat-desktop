#pragma once

#include "scheduler/ScheduledTask.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

class TaskStorage {
public:
    explicit TaskStorage(const QString &filePath);

    bool save(const QVector<ScheduledTask> &tasks);
    QVector<ScheduledTask> load();

private:
    QJsonObject taskToJson(const ScheduledTask &t) const;
    ScheduledTask jsonToTask(const QJsonObject &j) const;

    QString m_filePath;
};
