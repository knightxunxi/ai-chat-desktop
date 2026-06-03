#pragma once

#include "tools/ToolResult.h"

#include <QString>

namespace OcrService {

// 功能：从工作目录内的图片文件提取文字（基础占位实现）。
// 当前返回图片文件信息，完整 OCR 需要后续集成 Windows OCR API 或 Tesseract。
ToolResult extractText(const QString &workspaceDirectory, const QString &imagePath);

} // namespace OcrService
