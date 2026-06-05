#pragma once

#include "services/ToolCall.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QVector>

// V12.3: 流式内容块事件类型，用于通知上层工具调用参数已完整。
enum class ContentBlockEventType {
    ToolUseComplete   // 工具调用参数已完整
};

// V12.3: 流式内容块事件，在工具调用参数 JSON 凑齐时立即发出。
struct ContentBlockEvent {
    ContentBlockEventType type;
    QString toolName;
    QJsonObject arguments;
};

// 学习注释：一次 SSE 解析结果，可能包含多段文本增量和完成标记。
// 使用模块：OpenAICompatibleClient 在 readyRead 时消费网络数据。
struct StreamParseResult {
    QVector<QString> textDeltas;               // 功能：解析出的文本片段；使用模块：逐段发给 ApplicationController。
    ToolCallList toolCalls;                    // 功能：解析出的完整工具调用；使用模块：V9.2 Function Calling。
    bool done = false;                         // 功能：是否收到 [DONE]；使用模块：判断流式响应是否结束。
    QVector<ContentBlockEvent> blockEvents;    // V12.3: 流式内容块事件；使用模块：流式工具执行。
    bool truncated = false;                    // V17.6 P2-2: finish_reason=="length" 表示输出被截断。
};

// 学习注释：解析 OpenAI-compatible 的 server-sent events 响应。
// 使用模块：OpenAICompatibleClient 将 QNetworkReply 收到的原始字节交给它。
class StreamParser
{
public:
    // 功能：消费一段新增网络数据并返回解析结果；使用模块：OpenAICompatibleClient::handleReadyRead。
    StreamParseResult consume(const QByteArray &data);
    // 功能：请求结束时处理缓冲区剩余内容；使用模块：OpenAICompatibleClient::handleFinished。
    StreamParseResult finish();
    // 功能：清空解析状态；使用模块：每次新请求开始前重置。
    void reset();

private:
    // 功能：解析单行 SSE 数据；使用模块：consume/finish 内部调用。
    void parseLine(const QByteArray &line, StreamParseResult &result);
    // V12.3: 累积流式工具调用参数片段并检测完成；使用模块：parseLine 处理 delta.tool_calls。
    void appendToolCallDeltas(const QJsonArray &toolCallDeltas, StreamParseResult &result);
    // V12.3: 对尚未 emit 的 tool call 尝试 best-effort JSON parse 并发送事件；使用模块：[DONE] 处理。
    void emitPendingToolCalls(StreamParseResult &result);

    QByteArray m_buffer;               // 功能：保存尚未形成完整行的半截数据；使用模块：跨 readyRead 调用继续解析。
    ToolCallList m_toolCalls;          // 功能：缓存尚未完成的流式工具调用；使用模块：收到 [DONE] 后返回。
    QSet<int> m_emittedToolCalls;      // V12.3: 跟踪已 emit ToolUseComplete 的 tool call index；使用模块：避免重复 emit。
};
