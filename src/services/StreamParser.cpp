#include "services/StreamParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

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

void StreamParser::reset()
{
    m_buffer.clear();
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

    const QJsonObject delta = choices.first().toObject().value(QStringLiteral("delta")).toObject();
    const QString content = delta.value(QStringLiteral("content")).toString();
    if (!content.isEmpty()) {
        result.textDeltas.append(content);
    }
}
