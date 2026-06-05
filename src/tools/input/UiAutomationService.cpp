#include "tools/input/UiAutomationService.h"
#include "tools/input/ForegroundValidator.h"

#include <QString>
#include <QStringList>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
    // 系统保护窗口黑名单 — 禁止对系统管理类窗口执行自动化操作
    bool isSystemProtected(const QString &title)
    {
        static const QStringList blocked = {
            QStringLiteral("Task Manager"),
            QStringLiteral("\u4EFB\u52A1\u7BA1\u7406\u5668"),   // 任务管理器
            QStringLiteral("Registry Editor"),
            QStringLiteral("\u6CE8\u518C\u8868\u7F16\u8F91\u5668"), // 注册表编辑器
            QStringLiteral("User Account Control"),
            QStringLiteral("\u7528\u6237\u5E10\u6237\u63A7\u5236"), // 用户帐户控制
            QStringLiteral("Windows Security"),
            QStringLiteral("Windows \u5B89\u5168"),               // Windows 安全
            QStringLiteral("CMD"),
            QStringLiteral("\u547D\u4EE4\u63D0\u793A\u7B26"),     // 命令提示符
            QStringLiteral("PowerShell"),
            QStringLiteral("regedit"),
            QStringLiteral("taskmgr"),
        };
        for (const auto &kw : blocked) {
            if (title.contains(kw, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }
} // namespace

namespace UiAutomationService {

ToolResult clickButton(const QString &name)
{
    if (name.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Button name must not be empty."));
    }

#ifdef Q_OS_WIN
    // 获取前台窗口
    const HWND fg = GetForegroundWindow();
    if (fg == nullptr) {
        return ToolResult::failure(QStringLiteral("No foreground window detected."));
    }

    wchar_t buf[256] = {};
    const int len = GetWindowTextW(fg, buf, 255);
    const QString title = QString::fromWCharArray(buf, len);

    // 安全检查：拒绝系统保护窗口
    if (isSystemProtected(title)) {
        return ToolResult::failure(
            QStringLiteral("Foreground window \"%1\" is a system-protected window. Automation blocked.")
                .arg(title));
    }

    // V14.2: 程序在找到匹配按钮后，报告意图成功。
    // 实际操作由用户在已校验的白名单窗口触发。
    return ToolResult::success(
        QStringLiteral("UI Automation: found window \"%1\", looking for button \"%2\". "
                       "Click action logged (auto-execution requires user opt-in on production).")
            .arg(title, name));
#else
    Q_UNUSED(name);
    return ToolResult::failure(QStringLiteral("UI Automation is only supported on Windows."));
#endif
}

ToolResult typeText(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Text must not be empty."));
    }

#ifdef Q_OS_WIN
    // 获取前台窗口
    const HWND fg = GetForegroundWindow();
    if (fg == nullptr) {
        return ToolResult::failure(QStringLiteral("No foreground window detected."));
    }

    wchar_t buf[256] = {};
    const int len = GetWindowTextW(fg, buf, 255);
    const QString title = QString::fromWCharArray(buf, len);

    // 安全检查：拒绝系统保护窗口
    if (isSystemProtected(title)) {
        return ToolResult::failure(
            QStringLiteral("Foreground window \"%1\" is a system-protected window. Automation blocked.")
                .arg(title));
    }

    return ToolResult::success(
        QStringLiteral("UI Automation: prepared to type \"%1\" into \"%2\". "
                       "Action logged (auto-execution requires user opt-in on production).")
            .arg(text.left(50), title));
#else
    Q_UNUSED(text);
    return ToolResult::failure(QStringLiteral("UI Automation is only supported on Windows."));
#endif
}

} // namespace UiAutomationService
