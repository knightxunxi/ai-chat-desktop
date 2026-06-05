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

// V18.2 P1-1: 鼠标操作
ToolResult mouseClick(int x, int y, const QString &button)
{
    if (x < 0 || y < 0)
        return ToolResult::failure(QStringLiteral("Coordinates must be non-negative."));

#ifdef Q_OS_WIN
    // 移动鼠标
    SetCursorPos(x, y);
    Sleep(10);

    DWORD downFlag = 0, upFlag = 0;
    const QString b = button.trimmed().toLower();
    if (b == QStringLiteral("left")) { downFlag = MOUSEEVENTF_LEFTDOWN; upFlag = MOUSEEVENTF_LEFTUP; }
    else if (b == QStringLiteral("right")) { downFlag = MOUSEEVENTF_RIGHTDOWN; upFlag = MOUSEEVENTF_RIGHTUP; }
    else if (b == QStringLiteral("middle")) { downFlag = MOUSEEVENTF_MIDDLEDOWN; upFlag = MOUSEEVENTF_MIDDLEUP; }
    else return ToolResult::failure(QStringLiteral("Button must be left, right, or middle."));

    INPUT down = {}; down.type = INPUT_MOUSE; down.mi.dwFlags = downFlag;
    INPUT up   = {}; up.type   = INPUT_MOUSE; up.mi.dwFlags   = upFlag;
    SendInput(1, &down, sizeof(INPUT));
    SendInput(1, &up,   sizeof(INPUT));

    return ToolResult::success(
        QStringLiteral("Mouse %1 click at (%2, %3)").arg(button).arg(x).arg(y));
#else
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(button);
    return ToolResult::failure(QStringLiteral("Mouse input is only supported on Windows."));
#endif
}

ToolResult mouseScroll(int x, int y, int delta)
{
#ifdef Q_OS_WIN
    SetCursorPos(x, y);
    Sleep(10);

    INPUT scroll = {};
    scroll.type = INPUT_MOUSE;
    scroll.mi.dwFlags = MOUSEEVENTF_WHEEL;
    scroll.mi.mouseData = static_cast<DWORD>(delta * WHEEL_DELTA);
    SendInput(1, &scroll, sizeof(INPUT));

    return ToolResult::success(
        QStringLiteral("Mouse scroll %1 at (%2, %3)").arg(QString::number(delta), QString::number(x), QString::number(y)));
#else
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(delta);
    return ToolResult::failure(QStringLiteral("Mouse input is only supported on Windows."));
#endif
}

ToolResult moveMouse(int x, int y)
{
    if (x < 0 || y < 0)
        return ToolResult::failure(QStringLiteral("Coordinates must be non-negative."));

#ifdef Q_OS_WIN
    SetCursorPos(x, y);
    return ToolResult::success(
        QStringLiteral("Mouse moved to (%1, %2)").arg(x).arg(y));
#else
    Q_UNUSED(x); Q_UNUSED(y);
    return ToolResult::failure(QStringLiteral("Mouse input is only supported on Windows."));
#endif
}

// V18.3: 鼠标拖拽
ToolResult mouseDrag(int fromX, int fromY, int toX, int toY, const QString &button)
{
#ifdef Q_OS_WIN
    const QString b = button.trimmed().toLower();
    DWORD downFlag = MOUSEEVENTF_LEFTDOWN, upFlag = MOUSEEVENTF_LEFTUP;
    if (b == QStringLiteral("right")) { downFlag = MOUSEEVENTF_RIGHTDOWN; upFlag = MOUSEEVENTF_RIGHTUP; }
    else if (b == QStringLiteral("middle")) { downFlag = MOUSEEVENTF_MIDDLEDOWN; upFlag = MOUSEEVENTF_MIDDLEUP; }

    SetCursorPos(fromX, fromY); Sleep(10);
    INPUT d = {}; d.type = INPUT_MOUSE; d.mi.dwFlags = downFlag; SendInput(1, &d, sizeof(INPUT));
    Sleep(20);
    SetCursorPos(toX, toY); Sleep(10);
    INPUT u = {}; u.type = INPUT_MOUSE; u.mi.dwFlags = upFlag; SendInput(1, &u, sizeof(INPUT));

    return ToolResult::success(QStringLiteral("Dragged from (%1,%2) to (%3,%4)").arg(fromX).arg(fromY).arg(toX).arg(toY));
#else
    Q_UNUSED(fromX); Q_UNUSED(fromY); Q_UNUSED(toX); Q_UNUSED(toY); Q_UNUSED(button);
    return ToolResult::failure(QStringLiteral("Only supported on Windows."));
#endif
}

// V18.3: 获取当前鼠标坐标
ToolResult mousePosition()
{
#ifdef Q_OS_WIN
    POINT pt;
    GetCursorPos(&pt);
    return ToolResult::success(QStringLiteral("Mouse at (%1, %2)").arg(pt.x).arg(pt.y));
#else
    return ToolResult::failure(QStringLiteral("Only supported on Windows."));
#endif
}

// V18.3: 单键按下/释放
ToolResult keyPress(const QString &key, bool hold)
{
#ifdef Q_OS_WIN
    const QString k = key.trimmed().toUpper();
    WORD vk = 0;
    if (k == QStringLiteral("ENTER") || k == QStringLiteral("RETURN")) vk = VK_RETURN;
    else if (k == QStringLiteral("TAB")) vk = VK_TAB;
    else if (k == QStringLiteral("ESC") || k == QStringLiteral("ESCAPE")) vk = VK_ESCAPE;
    else if (k == QStringLiteral("SPACE")) vk = VK_SPACE;
    else if (k == QStringLiteral("BACKSPACE") || k == QStringLiteral("BACK")) vk = VK_BACK;
    else if (k == QStringLiteral("DELETE") || k == QStringLiteral("DEL")) vk = VK_DELETE;
    else if (k == QStringLiteral("UP")) vk = VK_UP;
    else if (k == QStringLiteral("DOWN")) vk = VK_DOWN;
    else if (k == QStringLiteral("LEFT")) vk = VK_LEFT;
    else if (k == QStringLiteral("RIGHT")) vk = VK_RIGHT;
    else if (k == QStringLiteral("HOME")) vk = VK_HOME;
    else if (k == QStringLiteral("END")) vk = VK_END;
    else if (k == QStringLiteral("ALT")) vk = VK_MENU;
    else if (k == QStringLiteral("WIN") || k == QStringLiteral("LWIN")) vk = VK_LWIN;
    else if (k.length() == 1) vk = VkKeyScanA(k.at(0).toLatin1()) & 0xFF;
    else return ToolResult::failure(QStringLiteral("Unknown key: %1").arg(key));

    INPUT in = {}; in.type = INPUT_KEYBOARD; in.ki.wVk = vk;
    if (hold) SendInput(1, &in, sizeof(INPUT));
    else { SendInput(1, &in, sizeof(INPUT)); in.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &in, sizeof(INPUT)); }

    return ToolResult::success(QStringLiteral("Key %1: %2").arg(key, hold ? QStringLiteral("hold") : QStringLiteral("press")));
#else
    Q_UNUSED(key); Q_UNUSED(hold);
    return ToolResult::failure(QStringLiteral("Only supported on Windows."));
#endif
}

} // namespace InputSimulator
