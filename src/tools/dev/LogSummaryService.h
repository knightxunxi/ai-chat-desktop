#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace LogSummaryService {

// 功能：从应用日志文件中按关键词/级别过滤最近内容。
// 自动脱敏 API Key、Token、Bearer 等敏感字段。
ToolResult summarize(const QString &logFilePath,
                     const QString &keyword = QString(),
                     int maxLines = 50,
                     const QString &level = QStringLiteral("all"));

} // namespace LogSummaryService
