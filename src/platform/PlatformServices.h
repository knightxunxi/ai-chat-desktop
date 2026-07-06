#pragma once

// 功能：跨平台服务抽象接口 — 封装平台特有能力。
// N5: 定义纯虚接口 + 工厂函数，Windows 和 Unsupported 各有一个实现。
// 所有接口使用 Qt 跨平台类型（QString, QByteArray），不暴露 Win32 类型。
// 使用模块：ApplicationController 初始化时创建，下层工具通过此接口访问平台能力。

#include "tools/ToolResult.h"

#include <QByteArray>
#include <QString>
#include <QVector>

struct WindowInfo {
    qint64 hwnd = 0;    // 平台无关的窗口句柄
    QString title;
    QString className;
    bool visible = false;
    int x = 0, y = 0, width = 0, height = 0;
};

// 功能：平台服务接口 — 所有平台特有操作均通过此接口访问。
class PlatformServices {
public:
    virtual ~PlatformServices() = default;

    // ── 凭据存储 ──
    virtual bool saveCredential(const QString &key, const QString &value) = 0;
    virtual QString loadCredential(const QString &key) = 0;
    virtual bool deleteCredential(const QString &key) = 0;

    // ── 屏幕 / OCR ──
    virtual ToolResult captureScreen(const QString &outputPath) = 0;
    virtual ToolResult recognizeText(const QByteArray &imageData) = 0;

    // ── 窗口检测 ──
    virtual QVector<WindowInfo> enumWindows() = 0;
    virtual WindowInfo foregroundWindow() = 0;
    virtual bool waitWindow(const QString &titleSubstring, int timeoutMs) = 0;

    // ── 输入模拟 ──
    virtual ToolResult mouseClick(int x, int y) = 0;
    virtual ToolResult mouseScroll(int delta) = 0;
    virtual ToolResult keyPress(const QString &key) = 0;
    virtual ToolResult typeText(const QString &text) = 0;

    // ── 前台保护 ──
    virtual bool isForegroundAllowed() = 0;
    virtual QString foregroundGuardReason() = 0;

    // ── 平台标识 ──
    virtual QString platformName() const = 0;
    virtual bool isSupported() const = 0;
};

// N5: 平台服务工厂 — 根据编译平台创建对应实现
#include <memory>
std::unique_ptr<PlatformServices> createPlatformServices();
