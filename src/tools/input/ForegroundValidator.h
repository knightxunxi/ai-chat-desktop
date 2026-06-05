#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace ForegroundValidator {

// 功能：校验前台窗口标题是否匹配预期，返回确认后的窗口标题。
// 不匹配时拒绝后续输入操作。
ToolResult validateForeground(const QString &expectedTitle);

} // namespace ForegroundValidator
