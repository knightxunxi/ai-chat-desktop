#include "tools/text/MarkdownCleanupTool.h"
#include "tools/text/TextCleanupTool.h"

#include <QStringList>

namespace {

QString normalizeLineEndings(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

QString trimTrailingWhitespace(QString line)
{
    while (!line.isEmpty() && line.back().isSpace()) {
        line.chop(1);
    }

    return line;
}

void removeOuterEmptyLines(QStringList &lines)
{
    while (!lines.isEmpty() && lines.first().trimmed().isEmpty()) {
        lines.removeFirst();
    }

    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
        lines.removeLast();
    }
}

ToolResult emptyInputFailure(const QString &input)
{
    if (input.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Input is empty."));
    }

    return ToolResult::success(QString());
}

} // namespace

QString MarkdownCleanupTool::id() const
{
    return QStringLiteral("markdown.cleanup");
}

QString MarkdownCleanupTool::displayName(AppLanguage language) const
{
    return language == AppLanguage::Chinese ? QStringLiteral("Markdown 整理") : QStringLiteral("Markdown Cleanup");
}

QString MarkdownCleanupTool::description(AppLanguage language) const
{
    return language == AppLanguage::Chinese
               ? QStringLiteral("清理 Markdown 多余空行和行尾空白，并保留代码块内容。")
               : QStringLiteral("Removes extra blank lines and trailing spaces while preserving code blocks.");
}

ToolResult MarkdownCleanupTool::run(const QString &input) const
{
    const ToolResult emptyCheck = emptyInputFailure(input);
    if (!emptyCheck.ok) {
        return emptyCheck;
    }

    const QString normalized = normalizeLineEndings(input);
    const QStringList sourceLines = normalized.split(QLatin1Char('\n'));
    QStringList outputLines;
    bool inCodeBlock = false;
    int consecutiveBlankLines = 0;

    for (const QString &sourceLine : sourceLines) {
        const bool isCodeFence = sourceLine.trimmed().startsWith(QStringLiteral("```"));
        if (isCodeFence) {
            outputLines.append(sourceLine);
            inCodeBlock = !inCodeBlock;
            consecutiveBlankLines = 0;
            continue;
        }

        if (inCodeBlock) {
            outputLines.append(sourceLine);
            continue;
        }

        const QString cleanedLine = trimTrailingWhitespace(sourceLine);
        if (cleanedLine.trimmed().isEmpty()) {
            if (consecutiveBlankLines == 0) {
                outputLines.append(QString());
            }
            ++consecutiveBlankLines;
            continue;
        }

        outputLines.append(cleanedLine);
        consecutiveBlankLines = 0;
    }

    removeOuterEmptyLines(outputLines);
    return ToolResult::success(outputLines.join(QLatin1Char('\n')));
}

QString TextCleanupTool::id() const
{
    return QStringLiteral("text.cleanup");
}

QString TextCleanupTool::displayName(AppLanguage language) const
{
    return language == AppLanguage::Chinese ? QStringLiteral("文本清理") : QStringLiteral("Text Cleanup");
}

QString TextCleanupTool::description(AppLanguage language) const
{
    return language == AppLanguage::Chinese
               ? QStringLiteral("统一换行、去除首尾空白，并压缩连续空行。")
               : QStringLiteral("Normalizes line endings, trims whitespace, and collapses repeated blank lines.");
}

ToolResult TextCleanupTool::run(const QString &input) const
{
    const ToolResult emptyCheck = emptyInputFailure(input);
    if (!emptyCheck.ok) {
        return emptyCheck;
    }

    const QString normalized = normalizeLineEndings(input);
    const QStringList sourceLines = normalized.split(QLatin1Char('\n'));
    QStringList outputLines;
    int consecutiveBlankLines = 0;

    for (const QString &sourceLine : sourceLines) {
        const QString cleanedLine = sourceLine.trimmed();
        if (cleanedLine.isEmpty()) {
            if (consecutiveBlankLines == 0) {
                outputLines.append(QString());
            }
            ++consecutiveBlankLines;
            continue;
        }

        outputLines.append(cleanedLine);
        consecutiveBlankLines = 0;
    }

    removeOuterEmptyLines(outputLines);
    return ToolResult::success(outputLines.join(QLatin1Char('\n')));
}
