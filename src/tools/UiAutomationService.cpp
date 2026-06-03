#include "tools/UiAutomationService.h"

#include <QString>

// 学习注释：UiAutomationService 是 Windows UI Automation 的受控封装。
// V13 第一版为占位实现，记录操作意图但不执行真实点击/输入。
// 真实 UIA 集成需要：
//   1. 引入 UIAutomationClient.h / UIAutomationCoreApi.h
//   2. 获取前台窗口的 IUIAutomationElement
//   3. 按 Name/AutomationId 查找目标控件
//   4. 调用 InvokePattern 或 ValuePattern
// 先在逻辑上跑通，交互 bug 最后修。

namespace UiAutomationService {

ToolResult clickButton(const QString &name)
{
    if (name.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Button name must not be empty."));
    }

    // V13 占位：记录操作意图
    return ToolResult::success(
        QStringLiteral("UI Automation placeholder: would click \"%1\".\n"
                       "Full UIA implementation requires Windows UIAutomationClient API integration (planned V13.1).")
            .arg(name));
}

ToolResult typeText(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Text must not be empty."));
    }

    // V13 占位：记录操作意图
    return ToolResult::success(
        QStringLiteral("UI Automation placeholder: would type \"%1\".\n"
                       "Full UIA implementation requires Windows UIAutomationClient API integration (planned V13.1).")
            .arg(text));
}

} // namespace UiAutomationService
