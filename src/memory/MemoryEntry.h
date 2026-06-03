#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

// V13.1: 三层记忆系统中的记忆条目数据结构。
// 使用模块：DailyMemoryWriter 写入日志，ProjectMemoryManager 管理记忆。
struct MemoryEntry {
    QDateTime timestamp;   // 时间戳
    QString category;      // "log" | "decision" | "convention" | "preference"
    QString content;       // 记忆内容
    QString source;        // "agent_auto" | "user_explicit" | "hook"

    // 功能：工厂方法，创建带当前时间戳的 MemoryEntry。
    static MemoryEntry create(const QString &category,
                              const QString &content,
                              const QString &source);

    // 功能：格式化为日志行 "HH:mm:ss [category] (source) content..."
    QString formatLine() const;
};

using MemoryEntryList = QVector<MemoryEntry>;
