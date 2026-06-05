#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace InputSimulator {

// 功能：模拟键盘输入文本（受控，仅测试窗口）。
ToolResult sendText(const QString &text);

// 功能：模拟按键组合（如 Ctrl+C）。
ToolResult sendKeyCombo(const QString &keyCombo);

} // namespace InputSimulator
