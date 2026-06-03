#include "tools/InputSimulator.h"

#include <QString>

// V13 占位：SendInput 封装。
// 真实实现需要：
//   1. 校验前台窗口通过 ForegroundValidator
//   2. 构造 INPUT 结构体数组
//   3. 调用 SendInput()
//   4. 审计日志记录每次输入操作
// 先在逻辑上跑通。

namespace InputSimulator {

ToolResult sendText(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Text must not be empty."));
    }

    return ToolResult::success(
        QStringLiteral("SendInput placeholder: would send \"%1\".\n"
                       "Full implementation requires foreground validation and INPUT struct construction (planned V13.1).")
            .arg(text));
}

ToolResult sendKeyCombo(const QString &keyCombo)
{
    if (keyCombo.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Key combo must not be empty."));
    }

    return ToolResult::success(
        QStringLiteral("SendInput placeholder: would send key combo \"%1\".\n"
                       "Full implementation requires foreground validation and INPUT struct construction (planned V13.1).")
            .arg(keyCombo));
}

} // namespace InputSimulator
