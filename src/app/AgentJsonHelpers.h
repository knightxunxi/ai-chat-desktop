#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

// 功能：共享 JSON 解析辅助函数，消除 AgentPlanParser 与 AgentLoopActionParser 间重复；使用模块：两处 Parser。

inline QString requiredString(const QJsonObject &object, const QString &fieldName, bool *ok)
{
    const QJsonValue value = object.value(fieldName);
    if (!value.isString()) {
        *ok = false;
        return QString();
    }

    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        *ok = false;
        return QString();
    }

    return text;
}

inline QString optionalString(const QJsonObject &object, const QString &fieldName)
{
    const QJsonValue value = object.value(fieldName);
    return value.isString() ? value.toString().trimmed() : QString();
}

inline QString stripMarkdownFence(QString text)
{
    text = text.trimmed();
    if (!text.startsWith(QStringLiteral("```"))) {
        return text;
    }

    const int firstLineEnd = text.indexOf(QLatin1Char('\n'));
    if (firstLineEnd < 0) {
        return text;
    }

    text = text.mid(firstLineEnd + 1).trimmed();
    if (text.endsWith(QStringLiteral("```"))) {
        text.chop(3);
    }

    return text.trimmed();
}

inline QString extractJsonObjectCandidate(const QString &text)
{
    const QString unfenced = stripMarkdownFence(text);
    const int firstBrace = unfenced.indexOf(QLatin1Char('{'));
    const int lastBrace = unfenced.lastIndexOf(QLatin1Char('}'));
    if (firstBrace < 0 || lastBrace < firstBrace) {
        return unfenced;
    }

    return unfenced.mid(firstBrace, lastBrace - firstBrace + 1).trimmed();
}
