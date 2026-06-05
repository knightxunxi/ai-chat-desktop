#include "tools/perception/OcrService.h"

#include "tools/core/WorkspacePolicy.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

namespace {

// 功能：使用 PowerShell 调用 Windows.Media.Ocr API 对图片执行 OCR；
//       返回提取的文本，失败时返回空字符串。
QString runPowerShellOcr(const QString &resolvedPath)
{
    // PowerShell 脚本：使用 WinRT Windows.Media.Ocr API 执行 OCR
    // 流程：StorageFile → IRandomAccessStream → BitmapDecoder → SoftwareBitmap → OcrEngine → 逐行文本
    const QString script = QStringLiteral(
        "$ErrorActionPreference = 'Stop';"
        "Add-Type -AssemblyName System.Runtime.WindowsRuntime;"
        "$null = [Windows.Media.Ocr.OcrEngine,            Windows.Foundation, ContentType=WindowsRuntime];"
        "$null = [Windows.Graphics.Imaging.BitmapDecoder,  Windows.Foundation, ContentType=WindowsRuntime];"
        "$null = [Windows.Storage.StorageFile,             Windows.Foundation, ContentType=WindowsRuntime];"
        "$null = [Windows.Storage.FileAccessMode,          Windows.Foundation, ContentType=WindowsRuntime];"
        "$file   = [Windows.Storage.StorageFile]::GetFileFromPathAsync('%1').GetAwaiter().GetResult();"
        "$stream = $file.OpenAsync([Windows.Storage.FileAccessMode]::Read).GetAwaiter().GetResult();"
        "$dec    = [Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream).GetAwaiter().GetResult();"
        "$bmp    = $dec.GetSoftwareBitmapAsync().GetAwaiter().GetResult();"
        "$engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages();"
        "if (-not $engine) {"
        "    $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage('en');"
        "}"
        "if (-not $engine) {"
        "    [Console]::Error.WriteLine('OCR engine not available for any language');"
        "    exit 1;"
        "}"
        "$result = $engine.RecognizeAsync($bmp).GetAwaiter().GetResult();"
        "if (-not $result -or $result.Lines().Count -eq 0) {"
        "    Write-Output '';"
        "} else {"
        "    foreach ($line in $result.Lines()) {"
        "        $lineText = $line.Text;"
        "        Write-Output $lineText;"
        "    }"
        "}");

    // 将路径中的反斜杠替换为正斜杠，避免 PowerShell 转义问题
    const QString escapedPath = QString(resolvedPath).replace(QLatin1Char('\\'), QLatin1Char('/'));
    const QString finalScript = script.arg(escapedPath);

    QProcess process;
    process.start(QStringLiteral("powershell"),
                  QStringList() << QStringLiteral("-NoProfile")
                                << QStringLiteral("-NonInteractive")
                                << QStringLiteral("-ExecutionPolicy")
                                << QStringLiteral("Bypass")
                                << QStringLiteral("-Command")
                                << finalScript);

    if (!process.waitForStarted(5000)) {
        return QString();
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(3000);
        return QString();
    }

    if (process.exitCode() != 0) {
        return QString();
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return output;
}

// 功能：清理 OCR 输出，合并连续空白行、去除首尾空行。
QString cleanOcrOutput(const QString &rawText)
{
    if (rawText.trimmed().isEmpty()) {
        return QString();
    }

    QStringList lines = rawText.split(QLatin1Char('\n'));
    QStringList cleaned;
    bool lastWasEmpty = false;

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            if (!lastWasEmpty && !cleaned.isEmpty()) {
                cleaned.append(QString());
                lastWasEmpty = true;
            }
        } else {
            cleaned.append(trimmed);
            lastWasEmpty = false;
        }
    }

    // 去尾空行
    while (!cleaned.isEmpty() && cleaned.last().isEmpty()) {
        cleaned.removeLast();
    }

    return cleaned.join(QLatin1Char('\n'));
}

} // namespace

namespace OcrService {

ToolResult extractText(const QString &workspaceDirectory, const QString &imagePath)
{
    // 1. 空路径校验
    if (imagePath.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Image path must not be empty."));
    }

    // 2. 解析完整路径
    const QString resolvedPath = QDir(workspaceDirectory).filePath(imagePath);

    // 3. 工作区边界校验（防止路径穿越）
    if (!WorkspacePolicy::isPathInsideWorkspace(workspaceDirectory, resolvedPath)) {
        return ToolResult::failure(QStringLiteral("Image path is outside workspace directory."));
    }

    // 4. 文件存在性校验
    QFileInfo fileInfo(resolvedPath);
    if (!fileInfo.exists()) {
        return ToolResult::failure(QStringLiteral("Image file does not exist: %1").arg(imagePath));
    }

    // 5. 图片格式基本校验（扩展名）
    const QString suffix = fileInfo.suffix().toLower();
    static const QStringList supportedFormats = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("bmp"), QStringLiteral("tiff"), QStringLiteral("tif")
    };
    if (!supportedFormats.contains(suffix)) {
        // 非图片格式：仍然尝试 OCR，但给出提示
        (void)suffix;
    }

    const qint64 sizeKb = fileInfo.size() / 1024;

    // 7. 执行 OCR (PowerShell + Windows.Media.Ocr)
    const QString rawOcrText = runPowerShellOcr(resolvedPath);

    // 8. 处理 OCR 结果
    if (rawOcrText.isEmpty()) {
        return ToolResult::success(
            QStringLiteral("Image: %1 (%2 KB)\n"
                           "OCR: No text recognized (image may not contain recognizable text, "
                           "or OCR engine is unavailable)")
                .arg(imagePath)
                .arg(sizeKb));
    }

    const QString cleanedText = cleanOcrOutput(rawOcrText);

    if (cleanedText.isEmpty()) {
        return ToolResult::success(
            QStringLiteral("Image: %1 (%2 KB)\nOCR: No readable text found after cleanup")
                .arg(imagePath)
                .arg(sizeKb));
    }

    // 9. 返回成功结果
    return ToolResult::success(
        QStringLiteral("Image: %1 (%2 KB)\n\n%3")
            .arg(imagePath)
            .arg(sizeKb)
            .arg(cleanedText));
}

} // namespace OcrService
