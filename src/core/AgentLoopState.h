#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

// V17.2: Agent 循环状态快照，用于崩溃/中断后恢复。
// 使用模块：ApplicationController 在 Agent 循环迭代过程中保存状态，
//          MainWindow 在启动时检测并提示用户恢复。
struct AgentLoopState
{
    QString goal;
    int stepIndex = 0;
    int maxSteps = 50;
    QJsonArray accumulatedResults;
    QDateTime startedAt;
    QDateTime updatedAt;
    QString sessionId;

    bool isValid() const { return !goal.isEmpty() && stepIndex > 0; }

    QByteArray toJson() const
    {
        QJsonObject obj;
        obj["goal"] = goal;
        obj["stepIndex"] = stepIndex;
        obj["maxSteps"] = maxSteps;
        obj["results"] = accumulatedResults;
        obj["sessionId"] = sessionId;
        obj["startedAt"] = startedAt.toString(Qt::ISODate);
        obj["updatedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }

    static AgentLoopState fromJson(const QByteArray &data)
    {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        AgentLoopState state;
        if (!doc.isObject()) {
            return state;
        }
        const QJsonObject obj = doc.object();
        state.goal = obj.value("goal").toString();
        state.stepIndex = obj.value("stepIndex").toInt();
        state.maxSteps = obj.value("maxSteps").toInt(50);
        state.accumulatedResults = obj.value("results").toArray();
        state.sessionId = obj.value("sessionId").toString();
        state.startedAt = QDateTime::fromString(obj.value("startedAt").toString(), Qt::ISODate);
        state.updatedAt = QDateTime::fromString(obj.value("updatedAt").toString(), Qt::ISODate);
        return state;
    }
};
