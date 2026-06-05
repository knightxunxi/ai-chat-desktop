#include "tools/perception/ScreenCaptureService.h"

#include "tools/core/WorkspacePolicy.h"

#include <QDir>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace ScreenCaptureService {

ToolResult captureToFile(const QString &workspaceDirectory, const QString &outputPath)
{
    if (outputPath.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Output path must not be empty."));
    }

    const QString resolvedPath = QDir(workspaceDirectory).filePath(outputPath);
    if (!WorkspacePolicy::isPathInsideWorkspace(workspaceDirectory, resolvedPath)) {
        return ToolResult::failure(QStringLiteral("Output path is outside workspace directory."));
    }

    const QFileInfo fileInfo(resolvedPath);
    QDir parentDir = fileInfo.absoluteDir();
    if (!parentDir.exists() && !parentDir.mkpath(QStringLiteral("."))) {
        return ToolResult::failure(QStringLiteral("Cannot create parent directory: %1").arg(parentDir.absolutePath()));
    }

#ifdef Q_OS_WIN
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HDC hdcScreen = GetDC(nullptr);
    if (hdcScreen == nullptr) {
        return ToolResult::failure(QStringLiteral("Cannot get screen device context."));
    }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY);

    // 保存为 BMP（GDI 原生支持，无 Qt Gui 依赖）
    const std::wstring wPath = resolvedPath.toStdWString();

    BITMAPFILEHEADER bmfHeader = {};
    bmfHeader.bfType = 0x4D42; // "BM"
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = screenWidth;
    bi.biHeight = screenHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    const DWORD imageSize = screenWidth * screenHeight * 4;
    bmfHeader.bfSize = bmfHeader.bfOffBits + imageSize;

    std::vector<BYTE> pixels(imageSize);
    GetDIBits(hdcMem, hBitmap, 0, screenHeight, pixels.data(), (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    HANDLE hFile = CreateFileW(wPath.c_str(), GENERIC_WRITE, 0, nullptr,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return ToolResult::failure(QStringLiteral("Cannot create file: %1").arg(outputPath));
    }

    DWORD bytesWritten = 0;
    WriteFile(hFile, &bmfHeader, sizeof(bmfHeader), &bytesWritten, nullptr);
    WriteFile(hFile, &bi, sizeof(bi), &bytesWritten, nullptr);
    WriteFile(hFile, pixels.data(), imageSize, &bytesWritten, nullptr);
    CloseHandle(hFile);

    const QFileInfo savedInfo(resolvedPath);
    const qint64 sizeKb = savedInfo.size() / 1024;
    return ToolResult::success(
        QStringLiteral("Screenshot saved: %1 (%2x%3, %4 KB)")
            .arg(outputPath)
            .arg(screenWidth)
            .arg(screenHeight)
            .arg(sizeKb));
#else
    return ToolResult::failure(QStringLiteral("Screen capture is only supported on Windows."));
#endif
}

} // namespace ScreenCaptureService
