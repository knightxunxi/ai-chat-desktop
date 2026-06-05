#pragma once

#include "hooks/HookDefinition.h"

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QVector>

// ============================================================================
// TimestampHook — 在 PreSend 阶段注入当前 UTC 时间戳
// ============================================================================

class TimestampHook : public HookBase {
public:
    HookResult execute(const HookContext &ctx) override;
    QString name() const override { return QStringLiteral("builtin.timestamp"); }
    HookPoint hookPoint() const override { return HookPoint::PreSend; }
    QString hookType() const override { return QStringLiteral("builtin"); }
};

// ============================================================================
// RateLimitHook — 在 PreSend 阶段按 sessionId 限速（60s 内最多 20 次）
// ============================================================================

class RateLimitHook : public HookBase {
public:
    explicit RateLimitHook(int maxRequestsPerMinute = 20);

    HookResult execute(const HookContext &ctx) override;
    QString name() const override { return QStringLiteral("builtin.rate_limit"); }
    HookPoint hookPoint() const override { return HookPoint::PreSend; }
    QString hookType() const override { return QStringLiteral("builtin"); }

private:
    int m_maxRequestsPerMinute;
    QMap<QString, QVector<qint64>> m_requestTimestamps;
};

// ============================================================================
// SensitiveFilterHook — 在 PostReceive 阶段过滤疑似 API Key/Token
// ============================================================================

class SensitiveFilterHook : public HookBase {
public:
    HookResult execute(const HookContext &ctx) override;
    QString name() const override { return QStringLiteral("builtin.sensitive_filter"); }
    HookPoint hookPoint() const override { return HookPoint::PostReceive; }
    QString hookType() const override { return QStringLiteral("builtin"); }

private:
    QString filterSensitive(const QString &text) const;
};
