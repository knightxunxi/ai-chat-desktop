#include "scheduler/TaskStorage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

TaskStorage::TaskStorage(const QString &filePath)
    : m_filePath(filePath)
{
}

bool TaskStorage::save(const QVector<ScheduledTask> &tasks)
{
    // 确保目录存在
    const QFileInfo fi(m_filePath);
    QDir().mkpath(fi.absolutePath());

    QJsonArray arr;
    for (const auto &t : tasks) {
        if (t.isValid())
            arr.append(taskToJson(t));
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QJsonDocument doc(arr);
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

QVector<ScheduledTask> TaskStorage::load()
{
    QVector<ScheduledTask> result;

    QFile file(m_filePath);
    if (!file.exists())
        return result;
    if (!file.open(QIODevice::ReadOnly))
        return result;

    QByteArray data = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError)
        return result;
    if (!doc.isArray())
        return result;

    const QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        if (!val.isObject())
            continue;
        ScheduledTask t = jsonToTask(val.toObject());
        if (t.isValid())
            result.append(t);
    }

    return result;
}

QJsonObject TaskStorage::taskToJson(const ScheduledTask &t) const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = t.id;
    obj[QStringLiteral("name")] = t.name;
    obj[QStringLiteral("cronExpression")] = t.cronExpression;
    obj[QStringLiteral("agentPrompt")] = t.agentPrompt;
    obj[QStringLiteral("enabled")] = t.enabled;
    if (t.lastRun.isValid())
        obj[QStringLiteral("lastRun")] = t.lastRun.toUTC().toString(Qt::ISODate);
    if (t.nextRun.isValid())
        obj[QStringLiteral("nextRun")] = t.nextRun.toUTC().toString(Qt::ISODate);
    obj[QStringLiteral("maxRetries")] = t.maxRetries;
    return obj;
}

ScheduledTask TaskStorage::jsonToTask(const QJsonObject &j) const
{
    ScheduledTask t;
    t.id = j.value(QStringLiteral("id")).toString();
    t.name = j.value(QStringLiteral("name")).toString();
    t.cronExpression = j.value(QStringLiteral("cronExpression")).toString();
    t.agentPrompt = j.value(QStringLiteral("agentPrompt")).toString();
    t.enabled = j.value(QStringLiteral("enabled")).toBool(true);
    t.lastRun = QDateTime::fromString(j.value(QStringLiteral("lastRun")).toString(), Qt::ISODate);
    if (t.lastRun.isValid())
        t.lastRun = t.lastRun.toUTC();
    t.nextRun = QDateTime::fromString(j.value(QStringLiteral("nextRun")).toString(), Qt::ISODate);
    if (t.nextRun.isValid())
        t.nextRun = t.nextRun.toUTC();
    t.maxRetries = j.value(QStringLiteral("maxRetries")).toInt(3);
    return t;
}
