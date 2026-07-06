#include "platform/PlatformServices.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#include <shellscalingapi.h>

// ── Windows 平台实现 ──────────────────────────────────────────────────

class WindowsPlatformServices : public PlatformServices {
public:
    QString platformName() const override { return QStringLiteral("Windows"); }
    bool isSupported() const override { return true; }

    // ── 凭据存储（Win32 Credential Manager） ──
    bool saveCredential(const QString &key, const QString &value) override
    {
        const QString target = QStringLiteral("CodeXX_") + key;
        const std::wstring targetW = target.toStdWString();
        const std::wstring valueW = value.toStdWString();

        CREDENTIALW cred = {};
        cred.Type = CRED_TYPE_GENERIC;
        cred.TargetName = const_cast<LPWSTR>(targetW.c_str());
        cred.CredentialBlobSize = static_cast<DWORD>(valueW.size() * sizeof(wchar_t));
        cred.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<wchar_t *>(valueW.c_str()));
        cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

        // 先删除旧凭据再写入
        CredDeleteW(targetW.c_str(), CRED_TYPE_GENERIC, 0);
        return CredWriteW(&cred, 0) != FALSE;
    }

    QString loadCredential(const QString &key) override
    {
        const QString target = QStringLiteral("CodeXX_") + key;
        const std::wstring targetW = target.toStdWString();

        PCREDENTIALW cred = nullptr;
        if (!CredReadW(targetW.c_str(), CRED_TYPE_GENERIC, 0, &cred)) {
            return {};
        }

        QString result = QString::fromWCharArray(
            reinterpret_cast<const wchar_t *>(cred->CredentialBlob),
            cred->CredentialBlobSize / sizeof(wchar_t));
        CredFree(cred);
        return result;
    }

    bool deleteCredential(const QString &key) override
    {
        const QString target = QStringLiteral("CodeXX_") + key;
        const std::wstring targetW = target.toStdWString();
        return CredDeleteW(targetW.c_str(), CRED_TYPE_GENERIC, 0) != FALSE;
    }

    // ── 屏幕截图 ──
    ToolResult captureScreen(const QString &outputPath) override
    {
        // 委托给已有实现 — 调用 ScreenCaptureService
        // 此处仅做占位，实际可通过 QScreen::grabWindow 实现
        Q_UNUSED(outputPath);
        return ToolResult::failure(QStringLiteral("captureScreen: not implemented via PlatformServices yet"));
    }

    ToolResult recognizeText(const QByteArray &imageData) override
    {
        Q_UNUSED(imageData);
        return ToolResult::failure(QStringLiteral("recognizeText: not implemented via PlatformServices yet"));
    }

    // ── 窗口检测 ──
    QVector<WindowInfo> enumWindows() override
    {
        QVector<WindowInfo> results;
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto *vec = reinterpret_cast<QVector<WindowInfo> *>(lParam);
            if (!IsWindowVisible(hwnd)) return TRUE;

            wchar_t title[256] = {};
            wchar_t className[256] = {};
            GetWindowTextW(hwnd, title, 256);
            GetClassNameW(hwnd, className, 256);

            if (wcslen(title) == 0) return TRUE;

            RECT rect = {};
            GetWindowRect(hwnd, &rect);

            WindowInfo info;
            info.hwnd = reinterpret_cast<qint64>(hwnd);
            info.title = QString::fromWCharArray(title);
            info.className = QString::fromWCharArray(className);
            info.visible = true;
            info.x = rect.left;
            info.y = rect.top;
            info.width = rect.right - rect.left;
            info.height = rect.bottom - rect.top;
            vec->append(info);
            return TRUE;
        }, reinterpret_cast<LPARAM>(&results));
        return results;
    }

    WindowInfo foregroundWindow() override
    {
        HWND hwnd = GetForegroundWindow();
        WindowInfo info;
        info.hwnd = reinterpret_cast<qint64>(hwnd);

        wchar_t title[256] = {};
        wchar_t className[256] = {};
        GetWindowTextW(hwnd, title, 256);
        GetClassNameW(hwnd, className, 256);
        info.title = QString::fromWCharArray(title);
        info.className = QString::fromWCharArray(className);
        info.visible = IsWindowVisible(hwnd) != FALSE;

        RECT rect = {};
        GetWindowRect(hwnd, &rect);
        info.x = rect.left;
        info.y = rect.top;
        info.width = rect.right - rect.left;
        info.height = rect.bottom - rect.top;
        return info;
    }

    bool waitWindow(const QString &titleSubstring, int timeoutMs) override
    {
        QElapsedTimer timer;
        timer.start();
        while (!timer.hasExpired(timeoutMs)) {
            if (findWindowByTitle(titleSubstring).hwnd != 0) return true;
            QThread::msleep(200);
            QCoreApplication::processEvents();
        }
        return false;
    }

    // ── 输入模拟 ──
    ToolResult mouseClick(int x, int y) override
    {
        SetCursorPos(x, y);
        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, inputs, sizeof(INPUT));
        return ToolResult::success(QStringLiteral("Clicked at (%1, %2)").arg(x).arg(y));
    }

    ToolResult mouseScroll(int delta) override
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = static_cast<DWORD>(delta);
        SendInput(1, &input, sizeof(INPUT));
        return ToolResult::success(QStringLiteral("Scrolled by %1").arg(delta));
    }

    ToolResult keyPress(const QString &key) override
    {
        // 简单文本键输入（完整虚拟键映射留待 InputSimulator 专用实现）
        const auto data = key.toStdWString();
        for (wchar_t ch : data) {
            SHORT vk = VkKeyScanW(ch);
            if (vk == -1) continue;

            INPUT inputs[2] = {};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = vk & 0xFF;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = vk & 0xFF;
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, inputs, sizeof(INPUT));
        }
        return ToolResult::success(QStringLiteral("Sent key: %1").arg(key));
    }

    ToolResult typeText(const QString &text) override
    {
        return keyPress(text);
    }

    // ── 前台保护 ──
    bool isForegroundAllowed() override
    {
        // 默认允许（ForegroundValidator 已有更精细的规则）
        return true;
    }

    QString foregroundGuardReason() override
    {
        return {};
    }

private:
    WindowInfo findWindowByTitle(const QString &substring)
    {
        const auto infos = enumWindows();
        for (const auto &info : infos) {
            if (info.title.contains(substring, Qt::CaseInsensitive)) {
                return info;
            }
        }
        return {};
    }
};

#else // Q_OS_WIN

// ── 非 Windows 平台实现 ──────────────────────────────────────────────

class UnsupportedPlatformServices : public PlatformServices {
public:
    QString platformName() const override
    {
#ifdef Q_OS_MACOS
        return QStringLiteral("macOS");
#elif defined(Q_OS_LINUX)
        return QStringLiteral("Linux");
#else
        return QStringLiteral("Unknown");
#endif
    }
    bool isSupported() const override { return false; }

    bool saveCredential(const QString &, const QString &) override { return false; }
    QString loadCredential(const QString &) override { return {}; }
    bool deleteCredential(const QString &) override { return false; }

    ToolResult captureScreen(const QString &) override
    { return ToolResult::failure(QStringLiteral("Screen capture not supported on this platform.")); }
    ToolResult recognizeText(const QByteArray &) override
    { return ToolResult::failure(QStringLiteral("OCR not supported on this platform.")); }

    QVector<WindowInfo> enumWindows() override { return {}; }
    WindowInfo foregroundWindow() override { return {}; }
    bool waitWindow(const QString &, int) override { return false; }

    ToolResult mouseClick(int, int) override
    { return ToolResult::failure(QStringLiteral("Input simulation not supported on this platform.")); }
    ToolResult mouseScroll(int) override
    { return ToolResult::failure(QStringLiteral("Mouse scroll not supported on this platform.")); }
    ToolResult keyPress(const QString &) override
    { return ToolResult::failure(QStringLiteral("Key press not supported on this platform.")); }
    ToolResult typeText(const QString &) override
    { return ToolResult::failure(QStringLiteral("Text input not supported on this platform.")); }

    bool isForegroundAllowed() override { return false; }
    QString foregroundGuardReason() override { return QStringLiteral("Platform not supported."); }
};

#endif

// ── 工厂函数 ─────────────────────────────────────────────────────────

std::unique_ptr<PlatformServices> createPlatformServices()
{
#ifdef Q_OS_WIN
    return std::make_unique<WindowsPlatformServices>();
#else
    return std::make_unique<UnsupportedPlatformServices>();
#endif
}
