#pragma once

#include "memory/MemoryEntry.h"

#include <QDate>
#include <QString>
#include <QStringList>

// V13.1: 每日日志写入器，管理 .workbuddy/memory/YYYY-MM-DD.md 文件的读写。
// 使用模块：ProjectMemoryManager 委托日志追加和检索。
class DailyMemoryWriter {
public:
    explicit DailyMemoryWriter(const QString &projectDir);

    // 功能：追加一条日志到当日 YYYY-MM-DD.md 文件。
    bool append(const MemoryEntry &entry);

    // 功能：批量追加多条日志。
    int appendAll(const MemoryEntryList &entries);

    // 功能：读取最近 N 天的日志条目。
    MemoryEntryList recentLogs(int days) const;

    // 功能：在当日日志中按关键词检索。
    MemoryEntryList search(const QString &keyword) const;

    // 功能：返回当日日志文件的完整路径。
    QString todayLogPath() const;

    // V13.2: 列出记忆目录下所有 YYYY-MM-DD.md 文件路径，不含 MEMORY.md。
    QStringList logFiles() const;

    // V13.2: 解析指定日期的日志文件，返回条目列表。
    MemoryEntryList entriesForDate(const QDate &date) const;

    // V13.2: 用新内容替换指定文件（用于写入压缩摘要）。
    bool writeFile(const QString &filePath, const QString &content);

    // V13.2: 删除指定日志文件。
    bool removeFile(const QString &filePath);

private:
    QString m_projectDir;

    // 功能：返回记忆目录路径 .workbuddy/memory/。
    QString memoryDir() const;

    // 功能：返回当日日志文件路径 .workbuddy/memory/YYYY-MM-DD.md。
    QString todayFilePath() const;

    // 功能：解析日志文件，返回 MemoryEntry 列表。
    static MemoryEntryList parseFile(const QString &filePath);
};
