#include "tools/MarkdownCleanupTool.h"
#include "tools/TextCleanupTool.h"

#include <cassert>

int main()
{
    TextCleanupTool textTool;
    assert(textTool.id() == QStringLiteral("text.cleanup"));
    assert(textTool.displayName(AppLanguage::Chinese) == QStringLiteral("文本清理"));

    ToolResult result = textTool.run(QStringLiteral("  A  \r\n\r\n\r\n B \r C  "));
    assert(result.ok);
    assert(result.output == QStringLiteral("A\n\nB\nC"));

    MarkdownCleanupTool markdownTool;
    assert(markdownTool.id() == QStringLiteral("markdown.cleanup"));
    assert(markdownTool.displayName(AppLanguage::English) == QStringLiteral("Markdown Cleanup"));

    const QString markdownInput = QStringLiteral(
        "# Title   \n\n\nParagraph   \n```cpp\nint main() {   \n    return 0;   \n}\n```\n\n\nNext   ");
    result = markdownTool.run(markdownInput);
    assert(result.ok);
    assert(result.output.startsWith(QStringLiteral("# Title\n\nParagraph")));
    assert(result.output.contains(QStringLiteral("int main() {   ")));
    assert(result.output.contains(QStringLiteral("    return 0;   ")));
    assert(result.output.endsWith(QStringLiteral("Next")));
    assert(!result.output.contains(QStringLiteral("# Title   ")));
    assert(!result.output.contains(QStringLiteral("\n\n\n")));

    result = markdownTool.run(QString());
    assert(!result.ok);
    assert(result.output.isEmpty());
    assert(!result.error.isEmpty());

    return 0;
}
