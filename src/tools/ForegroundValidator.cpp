#include "tools/ForegroundValidator.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace ForegroundValidator {

ToolResult validateForeground(const QString &expectedTitle)
{
    if (expectedTitle.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Expected title must not be empty."));
    }

#ifdef Q_OS_WIN
    const HWND fg = GetForegroundWindow();
    if (fg == nullptr) {
        return ToolResult::failure(QStringLiteral("Cannot detect foreground window. No input will be sent."));
    }

    wchar_t buffer[512] = {};
    const int len = GetWindowTextW(fg, buffer, 511);
    const QString actualTitle = QString::fromWCharArray(buffer, len);

    if (actualTitle.isEmpty()) {
        return ToolResult::failure(
            QStringLiteral("Foreground window is untitled. Expected \"%1\". Input blocked.").arg(expectedTitle));
    }

    if (!actualTitle.contains(expectedTitle, Qt::CaseInsensitive)) {
        return ToolResult::failure(
            QStringLiteral("Foreground window mismatch: actual=\"%1\", expected=\"%2\". Input blocked.")
                .arg(actualTitle, expectedTitle));
    }

    return ToolResult::success(
        QStringLiteral("Foreground window validated: \"%1\" matches expected \"%2\".")
            .arg(actualTitle, expectedTitle));
#else
    return ToolResult::failure(QStringLiteral("Foreground validation is only supported on Windows."));
#endif
}

} // namespace ForegroundValidator
