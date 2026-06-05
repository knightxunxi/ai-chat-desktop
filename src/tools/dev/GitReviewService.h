#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace GitReviewService {

// 功能：获取 git diff 摘要（--stat + 文本差异），输出限制行数。
// 仅执行只读 git 命令，不做 add/commit/push。
ToolResult reviewDiff(bool stagedOnly = false, int maxLines = 200);

// 功能：获取最近 git 提交记录（--oneline 或完整格式）。
// 仅执行只读 git 命令。
ToolResult reviewLog(int maxCount = 20, bool oneline = true);

} // namespace GitReviewService
