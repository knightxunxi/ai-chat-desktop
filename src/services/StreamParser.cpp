#include "services/StreamParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>

StreamParseResult StreamParser::consume(const QByteArray &data)
{
    StreamParseResult result;
    m_buffer.append(data);

    while (true) {
        const qsizetype newlineIndex = m_buffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }

        QByteArray line = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex + 1);

        if (line.endsWith('\r')) {
            line.chop(1);
        }

        parseLine(line, result);
    }

    return result;
}

StreamParseResult StreamParser::finish()
{
    StreamParseResult result;
    if (m_buffer.isEmpty()) {
        return result;
    }

    QByteArray line = m_buffer;
    m_buffer.clear();

    if (line.endsWith('\r')) {
        line.chop(1);
    }

    parseLine(line, result);
    return result;
}

void StreamParser::reset()
{
    m_buffer.clear();
    m_toolCalls.clear();
    m_emittedToolCalls.clear();
}

void StreamParser::parseLine(const QByteArray &line, StreamParseResult &result)
{
    if (!line.startsWith("data:")) {
        return;
    }

    const QByteArray payload = line.mid(5).trimmed();
    if (payload.isEmpty()) {
        return;
    }

    if (payload == "[DONE]") {
        result.done = true;
        result.toolCalls = m_toolCalls;
        // V12.3: 对尚未 emit 的 tool call 进行 best-effort parse 并补发事件
        emitPendingToolCalls(result);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return;
    }

    const QJsonArray choices = document.object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        return;
    }

    // V17.6 P2-2: 检测 finish_reason=="length" 表示输出被截断
    const QJsonObject firstChoice = choices.first().toObject();
    const QString finishReason = firstChoice.value(QStringLiteral("finish_reason")).toString();
    if (finishReason == QStringLiteral("length")) {
        result.truncated = true;
    }

    const QJsonObject delta = firstChoice.value(QStringLiteral("delta")).toObject();
    const QString content = delta.value(QStringLiteral("content")).toString();
    if (!content.isEmpty()) {
        result.textDeltas.append(content);
    }

    // V12.3: 传入 result 以便在工具调用参数完整时直接填充 blockEvents
    appendToolCallDeltas(delta.value(QStringLiteral("tool_calls")).toArray(), result);
}

void StreamParser::appendToolCallDeltas(const QJsonArray &toolCallDeltas, StreamParseResult &result)
{
    for (const QJsonValue &toolCallValue : toolCallDeltas) {
        if (!toolCallValue.isObject()) {
            continue;
        }

        const QJsonObject toolCallObject = toolCallValue.toObject();
        const int index = toolCallObject.value(QStringLiteral("index")).toInt(-1);
        if (index < 0) {
            continue;
        }

        while (m_toolCalls.size() <= index) {
            m_toolCalls.append(ToolCall());
        }

        ToolCall &toolCall = m_toolCalls[index];
        const QString id = toolCallObject.value(QStringLiteral("id")).toString();
        if (!id.isEmpty()) {
            toolCall.id = id;
        }

        const QJsonObject function = toolCallObject.value(QStringLiteral("function")).toObject();
        const QString functionName = function.value(QStringLiteral("name")).toString();
        if (!functionName.isEmpty()) {
            toolCall.functionName = functionName;
        }

        const QString argumentsDelta = function.value(QStringLiteral("arguments")).toString();
        if (!argumentsDelta.isEmpty()) {
            toolCall.arguments += argumentsDelta;
        }

        // V12.3: 检测工具调用参数是否已完整（JSON parse 成功 = 完成信号）
        if (!toolCall.functionName.isEmpty()
            && !toolCall.arguments.isEmpty()
            && !m_emittedToolCalls.contains(index)) {

            QJsonParseError parseError;
            const QJsonDocument parsed = QJsonDocument::fromJson(
                toolCall.arguments.toUtf8(), &parseError);

            if (parseError.error == QJsonParseError::NoError && parsed.isObject()) {
                m_emittedToolCalls.insert(index);

                ContentBlockEvent event;
                event.type = ContentBlockEventType::ToolUseComplete;
                event.toolName = toolCall.functionName;
                event.arguments = parsed.object();
                result.blockEvents.append(event);
            }
        }
    }
}

void StreamParser::emitPendingToolCalls(StreamParseResult &result)
{
    // V12.3: 遍历所有 tool calls，对尚未 emit 的尝试 best-effort JSON parse
    for (int index = 0; index < m_toolCalls.size(); ++index) {
        if (m_emittedToolCalls.contains(index)) {
            continue;
        }

        const ToolCall &toolCall = m_toolCalls[index];
        if (toolCall.functionName.isEmpty() || toolCall.arguments.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument parsed = QJsonDocument::fromJson(
            toolCall.arguments.toUtf8(), &parseError);

        if (parseError.error == QJsonParseError::NoError && parsed.isObject()) {
            m_emittedToolCalls.insert(index);

            ContentBlockEvent event;
            event.type = ContentBlockEventType::ToolUseComplete;
            event.toolName = toolCall.functionName;
            event.arguments = parsed.object();
            result.blockEvents.append(event);
        }
    }
}
