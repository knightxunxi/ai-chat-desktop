#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace InputSimulator {

// 功能：模拟键盘输入文本（受控，仅测试窗口）。
ToolResult sendText(const QString &text);

// 功能：模拟按键组合（如 Ctrl+C）。
ToolResult sendKeyCombo(const QString &keyCombo);

// V18.2 P1-1: 鼠标操作
ToolResult mouseClick(int x, int y, const QString &button);  // "left" / "right" / "middle"
ToolResult mouseScroll(int x, int y, int delta);              // positive=up, negative=down
ToolResult moveMouse(int x, int y);                           // 绝对坐标

// V18.3: 鼠标扩展操作
ToolResult mouseDrag(int fromX, int fromY, int toX, int toY, const QString &button); // 拖拽
ToolResult mousePosition();  // 获取当前鼠标坐标

// V18.3: 单键按下/释放
ToolResult keyPress(const QString &key, bool hold);  // hold=true 按下一个键（持续）→ 用于修饰键

} // namespace InputSimulator
