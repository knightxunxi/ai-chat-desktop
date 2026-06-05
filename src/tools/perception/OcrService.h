#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace OcrService {

// 功能：从工作目录内的图片文件提取文字。
// 使用 Windows.Media.Ocr API（通过 PowerShell 调用）进行 OCR 识别。
// 支持 Windows 10+，识别引擎根据用户语言配置自动选择。
ToolResult extractText(const QString &workspaceDirectory, const QString &imagePath);

} // namespace OcrService
