#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace ScreenCaptureService {

// 功能：截取当前屏幕并保存为 PNG 到工作目录。
// outputPath 是相对于 workspaceDirectory 的路径。
ToolResult captureToFile(const QString &workspaceDirectory, const QString &outputPath);

} // namespace ScreenCaptureService
