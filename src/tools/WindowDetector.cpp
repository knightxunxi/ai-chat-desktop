#include "tools/WindowDetector.h"

#include <QStringList>

#ifdef Q_OS_WIN
#include <windows.h>

namespace {

struct EnumContext {
    QStringList *titles;
    int maxCount;
};

BOOL CALLBACK enumWindowProc(HWND hwnd, LPARAM lParam)
{
    auto *ctx = reinterpret_cast<EnumContext *>(lParam);
    if (ctx->titles->size() >= ctx->maxCount) {
        return FALSE; // Stop enumeration
    }

    if (!IsWindowVisible(hwnd)) {
        return TRUE; // Skip invisible
    }

    wchar_t buffer[256] = {};
    const int len = GetWindowTextW(hwnd, buffer, 255);
    if (len <= 0) {
        return TRUE; // Skip untitled
    }

    ctx->titles->append(QString::fromWCharArray(buffer, len));
    return TRUE;
}

} // namespace

#endif // Q_OS_WIN

namespace WindowDetector {

ToolResult listWindows(int maxCount)
{
#ifdef Q_OS_WIN
    if (maxCount <= 0) {
        maxCount = 50;
    }

    QStringList titles;
    EnumContext ctx = {&titles, maxCount};
    EnumWindows(enumWindowProc, reinterpret_cast<LPARAM>(&ctx));

    if (titles.isEmpty()) {
        return ToolResult::success(QStringLiteral("(No visible windows found)"));
    }

    QString output;
    output += QStringLiteral("=== Visible Windows (%1) ===\n").arg(titles.size());
    for (int i = 0; i < titles.size(); ++i) {
        output += QStringLiteral("[%1] %2\n").arg(i + 1).arg(titles.at(i));
    }
    return ToolResult::success(output);
#else
    return ToolResult::failure(QStringLiteral("Window enumeration is only supported on Windows."));
#endif
}

ToolResult foregroundWindowTitle()
{
#ifdef Q_OS_WIN
    const HWND fg = GetForegroundWindow();
    if (fg == nullptr) {
        return ToolResult::failure(QStringLiteral("Cannot detect foreground window."));
    }

    wchar_t buffer[256] = {};
    const int len = GetWindowTextW(fg, buffer, 255);
    if (len <= 0) {
        return ToolResult::success(QStringLiteral("Foreground window: (untitled)"));
    }

    const QString title = QString::fromWCharArray(buffer, len);
    return ToolResult::success(QStringLiteral("Foreground window: \"%1\"").arg(title));
#else
    return ToolResult::failure(QStringLiteral("Window detection is only supported on Windows."));
#endif
}

} // namespace WindowDetector
