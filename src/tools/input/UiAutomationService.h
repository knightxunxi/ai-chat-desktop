#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace UiAutomationService {

// 功能：在前台窗口中查找控件并点击。
// name 是控件的名称或 AutomationId。
ToolResult clickButton(const QString &name);

// 功能：在前台窗口中输入文本。
ToolResult typeText(const QString &text);

} // namespace UiAutomationService
