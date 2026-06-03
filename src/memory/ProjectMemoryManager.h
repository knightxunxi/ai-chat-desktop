#pragma once

#include "memory/MemoryEntry.h"

#include <QDate>
#include <QMap>
#include <QString>
#include <QStringList>

// V13.1: 三层记忆系统管理器。
// L1 (~/.codex/MEMORY.md) — 用户级跨项目偏好
// L2 (.workbuddy/memory/MEMORY.md) — 项目级技术决策、约定
// L3 (.workbuddy/memory/YYYY-MM-DD.md) — 每日日志
// 使用模块：ApplicationController 在执行完成后追加日志，在 Agent 循环中注入记忆。
class ProjectMemoryManager {
public:
    explicit ProjectMemoryManager(const QString &projectDir);

    // 功能：构建三层记忆拼接的 Prompt 片段，注入到 system prompt。
    QString buildMemorySection() const;

    // 功能：Agent 完成任务后自动追加每日日志。
    void appendDailyLog(const QString &category, const QString &entry);

    // 功能：用户显式说「记住」时写入项目 MEMORY.md (L2)。
    void remember(const QString &fact, const QString &source = QStringLiteral("user"));

    // 功能：读取最近 N 天的日志条目。
    MemoryEntryList recentLogs(int days) const;

    // 功能：按关键词在当日日志中检索。
    MemoryEntryList search(const QString &keyword) const;

    // 功能：静态方法，检查文本是否包含敏感信息。
    static bool containsSensitiveContent(const QString &text);

    // 功能：静态方法，检查文本是否像密钥/Token。
    static bool looksLikeSecret(const QString &text);

    // V13.2: 记忆压缩 — 阶段1：为 olderThanDays 天前的日志构建 LLM 压缩提示词。
    // 返回 Map<ISO周, 压缩提示词>。
    QMap<QString, QString> buildCompressionPrompts(int olderThanDays) const;

    // V13.2: 记忆压缩 — 阶段2：将从 LLM 获取的周摘要写入压缩文件，删除原始日志。
    bool applyCompression(int olderThanDays, const QMap<QString, QString> &weekSummaries);

private:
    QString m_projectDir;

    // 功能：返回 L1 路径 ~/.codex/MEMORY.md。
    static QString L1Path();

    // 功能：返回 L2 路径 .workbuddy/memory/MEMORY.md。
    QString L2Path() const;

    // 功能：安全读取文件内容，限制最大字节数。
    static QString readFile(const QString &path, int maxBytes = 8192);

    // V13.2: 列出所有压缩文件（*-compressed.md）的绝对路径。
    QStringList compressedFiles() const;
};
