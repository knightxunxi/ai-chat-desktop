#include "tools/OcrService.h"

#include "tools/WorkspacePolicy.h"

#include <QDir>
#include <QFileInfo>

namespace OcrService {

ToolResult extractText(const QString &workspaceDirectory, const QString &imagePath)
{
    if (imagePath.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Image path must not be empty."));
    }

    const QString resolvedPath = QDir(workspaceDirectory).filePath(imagePath);
    if (!WorkspacePolicy::isPathInsideWorkspace(workspaceDirectory, resolvedPath)) {
        return ToolResult::failure(QStringLiteral("Image path is outside workspace directory."));
    }

    QFileInfo fileInfo(resolvedPath);
    if (!fileInfo.exists()) {
        return ToolResult::failure(QStringLiteral("Image file does not exist: %1").arg(imagePath));
    }

    // V12 占位实现：验证文件存在后返回元信息
    // 完整 OCR 需要后续集成 Tesseract 或 Windows OCR API
    const qint64 sizeKb = fileInfo.size() / 1024;
    return ToolResult::success(
        QStringLiteral("Image found: %1 (%2 KB)\n"
                       "OCR is placeholder — full text extraction requires OCR integration (planned V12.1).")
            .arg(imagePath)
            .arg(sizeKb));
}

} // namespace OcrService
