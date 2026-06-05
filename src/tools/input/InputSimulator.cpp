#include "tools/input/InputSimulator.h"

#include <QString>
#include <QStringList>
#include <QVector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace InputSimulator {

ToolResult sendText(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Text must not be empty."));
    }

#ifdef Q_OS_WIN
    for (const QChar &ch : text) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wScan = ch.unicode();
        in.ki.dwFlags = KEYEVENTF_UNICODE;
        SendInput(1, &in, sizeof(INPUT));

        in.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        SendInput(1, &in, sizeof(INPUT));
    }
    return ToolResult::success(
        QStringLiteral("SendInput: text sent (%1 chars)").arg(text.size()));
#else
    Q_UNUSED(text);
    return ToolResult::failure(QStringLiteral("SendInput is only supported on Windows."));
#endif
}

ToolResult sendKeyCombo(const QString &keyCombo)
{
    if (keyCombo.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Key combo must not be empty."));
    }

#ifdef Q_OS_WIN
    const QStringList parts = keyCombo.split(QLatin1Char('+'));
    QVector<WORD> keys;

    for (const auto &p : parts) {
        const QString k = p.trimmed().toUpper();
        if (k == QStringLiteral("CTRL")) {
            keys.append(VK_CONTROL);
        } else if (k == QStringLiteral("ALT")) {
            keys.append(VK_MENU);
        } else if (k == QStringLiteral("SHIFT")) {
            keys.append(VK_SHIFT);
        } else if (k == QStringLiteral("WIN")) {
            keys.append(VK_LWIN);
        } else if (k.size() == 1) {
            keys.append(static_cast<WORD>(VkKeyScanW(k[0].unicode())));
        }
    }

    if (keys.isEmpty()) {
        return ToolResult::failure(
            QStringLiteral("No valid keys found in combo \"%1\".").arg(keyCombo));
    }

    // 按下所有键（修饰键 + 字符键）
    for (WORD vk : keys) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = vk;
        SendInput(1, &in, sizeof(INPUT));
    }

    // 释放（倒序）
    for (int i = keys.size() - 1; i >= 0; --i) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = keys[i];
        in.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &in, sizeof(INPUT));
    }

    return ToolResult::success(
        QStringLiteral("SendInput: key combo \"%1\" sent").arg(keyCombo));
#else
    Q_UNUSED(keyCombo);
    return ToolResult::failure(QStringLiteral("SendInput is only supported on Windows."));
#endif
}

} // namespace InputSimulator
