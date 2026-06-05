#include "hooks/BuiltinHooks.h"

#include <QDateTime>
#include <QJsonObject>
#include <QRegularExpression>

// ============================================================================
// TimestampHook 实现
// ============================================================================

HookResult TimestampHook::execute(const HookContext &ctx)
{
    HookResult result;
    result.action = HookAction::Modify;

    QJsonObject modified = ctx.context;

    const QString currentTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const QString timestampLine = QStringLiteral("\n[Current time: %1]").arg(currentTime);

    const QString prompt = ctx.context.value(QStringLiteral("prompt")).toString();
    modified.insert(QStringLiteral("prompt"), prompt + timestampLine);

    result.modifiedContext = modified;
    return result;
}

// ============================================================================
// RateLimitHook 实现
// ============================================================================

RateLimitHook::RateLimitHook(int maxRequestsPerMinute)
    : m_maxRequestsPerMinute(maxRequestsPerMinute)
{
}

HookResult RateLimitHook::execute(const HookContext &ctx)
{
    HookResult result;

    const QString sessionId = ctx.metadata.value(QStringLiteral("session_id")).toString(QStringLiteral("default"));
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    // 清理 60 秒前的时间戳
    QVector<qint64> &timestamps = m_requestTimestamps[sessionId];
    const qint64 cutoff = now - 60000; // 60 秒前
    timestamps.erase(
        std::remove_if(timestamps.begin(), timestamps.end(),
                       [cutoff](qint64 ts) { return ts < cutoff; }),
        timestamps.end());

    // 检查是否超频
    if (timestamps.size() >= m_maxRequestsPerMinute) {
        result.action = HookAction::Reject;
        result.error = QStringLiteral("Rate limit exceeded: %1 requests per minute (max %2)")
                           .arg(timestamps.size() + 1)
                           .arg(m_maxRequestsPerMinute);
        return result;
    }

    // 记录本次请求时间戳
    timestamps.append(now);

    result.action = HookAction::Pass;
    return result;
}

// ============================================================================
// SensitiveFilterHook 实现
// ============================================================================

HookResult SensitiveFilterHook::execute(const HookContext &ctx)
{
    HookResult result;

    const QString response = ctx.context.value(QStringLiteral("response")).toString();
    if (response.isEmpty()) {
        result.action = HookAction::Pass;
        return result;
    }

    const QString filtered = filterSensitive(response);

    if (filtered == response) {
        // 没有变化
        result.action = HookAction::Pass;
        return result;
    }

    result.action = HookAction::Modify;
    QJsonObject modified = ctx.context;
    modified.insert(QStringLiteral("response"), filtered);
    result.modifiedContext = modified;
    return result;
}

QString SensitiveFilterHook::filterSensitive(const QString &text) const
{
    QString result = text;

    // 匹配 sk-... (OpenAI API key 模式)
    static const QRegularExpression skPattern(
        QStringLiteral("sk-[a-zA-Z0-9]{20,}"),
        QRegularExpression::CaseInsensitiveOption);

    // 匹配 ghp_... (GitHub Personal Access Token)
    static const QRegularExpression ghpPattern(
        QStringLiteral("ghp_[a-zA-Z0-9]{20,}"),
        QRegularExpression::CaseInsensitiveOption);

    // 匹配 AIza... (Google API Key)
    static const QRegularExpression googleApiPattern(
        QStringLiteral("AIza[0-9A-Za-z\\-_]{20,}"),
        QRegularExpression::CaseInsensitiveOption);

    // 匹配通用 Bearer token 模式
    static const QRegularExpression bearerPattern(
        QStringLiteral("Bearer\\s+[a-zA-Z0-9\\-._~+/]+=*"),
        QRegularExpression::CaseInsensitiveOption);

    static const QRegularExpression authHeaderPattern(
        QStringLiteral("Authorization:\\s*Bearer\\s+[a-zA-Z0-9\\-._~+/]+=*"),
        QRegularExpression::CaseInsensitiveOption);

    result.replace(skPattern, QStringLiteral("[REDACTED]"));
    result.replace(ghpPattern, QStringLiteral("[REDACTED]"));
    result.replace(googleApiPattern, QStringLiteral("[REDACTED]"));
    result.replace(authHeaderPattern, QStringLiteral("Authorization: Bearer [REDACTED]"));
    result.replace(bearerPattern, QStringLiteral("Bearer [REDACTED]"));

    return result;
}
