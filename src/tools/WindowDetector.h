#pragma once

#include "tools/ToolResult.h"

#include <QString>

namespace WindowDetector {

// 功能：枚举所有可见窗口，返回标题列表，限 maxCount 个。
ToolResult listWindows(int maxCount = 50);

// 功能：获取当前前台窗口标题。
ToolResult foregroundWindowTitle();

} // namespace WindowDetector
