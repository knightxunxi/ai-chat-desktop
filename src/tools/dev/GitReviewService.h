#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace GitReviewService {

// 功能：获取 git diff 摘要（--stat + 文本差异），输出限制行数。
// 仅执行只读 git 命令，不做 add/commit/push。
ToolResult reviewDiff(bool stagedOnly = false, int maxLines = 200);

// 功能：获取最近 git 提交记录（--oneline 或完整格式）。
// 仅执行只读 git 命令。
ToolResult reviewLog(int maxCount = 20, bool oneline = true);

// #27: 结构化代码审查 — 分析 diff 并返回分类问题列表
// 返回格式示例：
// Issues:
// - severity=warning | file=src/foo.cpp:42 | 变量未初始化
// - severity=error   | file=src/bar.cpp:15 | 缺少空指针检查
struct ReviewIssue {
    QString severity;   // error | warning | info
    QString file;       // 相对文件路径
    int line = 0;       // 行号
    QString message;    // 问题描述
    QString suggestion; // 修改建议
};
ToolResult structuredReview(const QString &diffText);

// #27: 记录审查结果到历史文件
void recordReviewToHistory(const QString &projectDir, const QVector<ReviewIssue> &issues);

// #27: 读取历史审查记录，去重返回新问题
QVector<ReviewIssue> filterNewIssues(const QString &projectDir, const QVector<ReviewIssue> &issues);

} // namespace GitReviewService
