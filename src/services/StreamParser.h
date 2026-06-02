#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

// 学习注释：一次 SSE 解析结果，可能包含多段文本增量和完成标记。
// 使用模块：OpenAICompatibleClient 在 readyRead 时消费网络数据。
struct StreamParseResult {
    QVector<QString> textDeltas; // 功能：解析出的文本片段；使用模块：逐段发给 ApplicationController。
    bool done = false;           // 功能：是否收到 [DONE]；使用模块：判断流式响应是否结束。
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

    QByteArray m_buffer; // 功能：保存尚未形成完整行的半截数据；使用模块：跨 readyRead 调用继续解析。
};
