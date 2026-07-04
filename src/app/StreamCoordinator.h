#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

class AIClient;
class ChatSession;
class ToolCallList;
struct AppConfig;

// V19: 流式响应协调器 — 处理 AI 回复的增量接收、工具调用、完成与失败
// 从 ApplicationController 拆分出的职责：网络响应 → 文本增量/工具调用/完成/失败
// 输入：AIClient 信号    输出：处理后的信号给 ApplicationController
class StreamCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit StreamCoordinator(QObject *parent = nullptr);

    void setAIClient(AIClient *client); // 绑定 AI 客户端信号

signals:
    // 转发给 UI 的信号
    void textDeltaProcessed(const QString &delta);
    void toolCallsProcessed(const ToolCallList &toolCalls);
    void requestFinishedProcessed();
    void requestFailedProcessed(const QString &message, int category);
    void responseTruncatedProcessed();

private:
    void onTextDelta(const QString &delta);
    void onToolCalls(const ToolCallList &toolCalls);
    void onRequestFinished();
    void onRequestFailed(const QString &message, int category);
    void onResponseTruncated();

    AIClient *m_aiClient = nullptr;
    bool m_connected = false;
};
