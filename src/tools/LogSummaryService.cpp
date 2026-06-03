#include "tools/LogSummaryService.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

// 学习注释：LogSummaryService 读取应用日志文件并按关键词/级别过滤。
// 输出自动脱敏（API Key/Token/Bearer/secret 等敏感字段替换为 [REDACTED]）。

namespace {

// 功能：脱敏日志行中的敏感字段；使用模块：summarize 输出构建。
QString sanitizeLine(const QString &line)
{
    static const QRegularExpression sensitivePatterns[] = {
        QRegularExpression(QStringLiteral("(api[_-]?key|apikey|api_key)\\s*[=:]\\s*\\S+"),
                           QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("(token|bearer|secret)\\s*[=:]\\s*\\S+"),
                           QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("(password|passwd)\\s*[=:]\\s*\\S+"),
                           QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("sk-[A-Za-z0-9]{16,}")),
        QRegularExpression(QStringLiteral("(ghp|gho|ghu|ghs|ghr)_[A-Za-z0-9]{36,}")),
    };

    QString result = line;
    for (const auto &pattern : sensitivePatterns) {
        result.replace(pattern, QStringLiteral("[REDACTED]"));
    }
    return result;
}

// 功能：检查日志行是否匹配指定级别；使用模块：summarize 过滤。
bool matchesLevel(const QString &line, const QString &level)
{
    if (level.isEmpty() || level == QStringLiteral("all")) {
        return true;
    }
    return line.contains(level, Qt::CaseInsensitive);
}

// 功能：检查日志行是否包含关键词；使用模块：summarize 过滤。
bool matchesKeyword(const QString &line, const QString &keyword)
{
    if (keyword.isEmpty()) {
        return true;
    }
    return line.contains(keyword, Qt::CaseInsensitive);
}

} // namespace

namespace LogSummaryService {

ToolResult summarize(const QString &logFilePath,
                     const QString &keyword,
                     int maxLines,
                     const QString &level)
{
    if (logFilePath.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Log file path must not be empty."));
    }

    const QFileInfo fileInfo(logFilePath);
    if (!fileInfo.exists()) {
        return ToolResult::failure(QStringLiteral("Log file does not exist: %1").arg(logFilePath));
    }
    if (!fileInfo.isFile()) {
        return ToolResult::failure(QStringLiteral("Path is not a file: %1").arg(logFilePath));
    }
    if (fileInfo.size() > 10 * 1024 * 1024) {
        return ToolResult::failure(QStringLiteral("Log file too large (>10 MB): %1").arg(logFilePath));
    }

    QFile file(logFilePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return ToolResult::failure(QStringLiteral("Cannot open log file: %1").arg(file.errorString()));
    }

    QTextStream stream(&file);

    QStringList matchingLines;
    while (!stream.atEnd() && matchingLines.size() < maxLines) {
        const QString line = stream.readLine();
        if (matchesKeyword(line, keyword) && matchesLevel(line, level)) {
            matchingLines.append(sanitizeLine(line));
        }
    }
    file.close();

    if (matchingLines.isEmpty()) {
        const QString filterDesc = keyword.isEmpty()
            ? QString()
            : QStringLiteral(" matching \"%1\"").arg(keyword);
        return ToolResult::success(QStringLiteral("(No log lines found%1 with level filter \"%2\")")
                                       .arg(filterDesc, level));
    }

    const int maxLineLength = 1024;
    QStringList outputLines;
    outputLines.append(QStringLiteral("=== Log Summary (keyword=\"%1\", level=\"%2\", max=%3) ===")
                           .arg(keyword.isEmpty() ? QStringLiteral("*") : keyword, level)
                           .arg(maxLines));

    for (const QString &line : matchingLines) {
        if (line.size() > maxLineLength) {
            outputLines.append(line.left(maxLineLength) + QStringLiteral("..."));
        } else {
            outputLines.append(line);
        }
    }

    outputLines.append(QStringLiteral("--- Total matching lines: %1 ---").arg(matchingLines.size()));
    return ToolResult::success(outputLines.join(QLatin1Char('\n')));
}

} // namespace LogSummaryService
