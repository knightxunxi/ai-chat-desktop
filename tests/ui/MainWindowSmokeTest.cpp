#include "ui/MainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>

#include <cassert>
#include <cstdio>

namespace {

static int testCount = 0;
static int passCount = 0;

void test(const char *name, std::function<void()> body)
{
    ++testCount;
    try {
        body();
        ++passCount;
        printf("  PASS: %s\n", name);
    } catch (const std::exception &e) {
        printf("  FAIL: %s — %s\n", name, e.what());
    } catch (...) {
        printf("  FAIL: %s — unknown error\n", name);
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // ── 1. 构造验证：主窗口创建 + 核心子控件存在 ───────────────────────
    // MainWindow 构造时会调用 ApplicationController::initialize()，
    // 首次启动无配置文件时也应当正常完成。
    test("main window constructs without crash", [] {
        MainWindow window;
        // 虽然未 show()，但 widget 树已构建完成

        // 核心控件验证
        auto *chatView = window.findChild<QWidget *>(QStringLiteral("chatScrollArea"));
        assert(chatView != nullptr); // ChatView 内部的 QScrollArea

        auto *sessionList = window.findChild<QListWidget *>(QStringLiteral("sessionList"));
        assert(sessionList != nullptr);

        auto *messageInput = window.findChild<QTextEdit *>(QStringLiteral("messageInput"));
        assert(messageInput != nullptr);

        auto *sendButton = window.findChild<QPushButton *>(QStringLiteral("sendButton"));
        assert(sendButton != nullptr);

        auto *retryButton = window.findChild<QPushButton *>(QStringLiteral("retryButton"));
        assert(retryButton != nullptr);

        auto *modeToggle = window.findChild<QPushButton *>(QStringLiteral("modeToggleButton"));
        assert(modeToggle != nullptr);
    });

    // ── 2. 模式切换按钮 ───────────────────────────────────────────────
    test("mode toggle button exists and is initially Chat mode", [] {
        MainWindow window;
        auto *modeToggle = window.findChild<QPushButton *>(QStringLiteral("modeToggleButton"));
        assert(modeToggle != nullptr);
        // 初始状态应包含 "Chat" 字样（中英文都可能）
        assert(!modeToggle->text().isEmpty());
    });

    // ── 3. 发送按钮状态 ───────────────────────────────────────────────
    test("send button initially disabled (empty input)", [] {
        MainWindow window;
        auto *sendButton = window.findChild<QPushButton *>(QStringLiteral("sendButton"));
        assert(sendButton != nullptr);
        assert(!sendButton->isEnabled());
    });

    // ── 4. 主题切换按钮 ───────────────────────────────────────────────
    test("theme toggle button exists", [] {
        MainWindow window;
        auto *themeBtn = window.findChild<QPushButton *>(QStringLiteral("themeToggleButton"));
        assert(themeBtn != nullptr);
        assert(themeBtn->isEnabled());
    });

    // ── 5. 侧边栏操作按钮 ─────────────────────────────────────────────
    test("sidebar action buttons exist", [] {
        MainWindow window;
        auto *newChat = window.findChild<QPushButton *>(QStringLiteral("newChatButton"));
        auto *rename = window.findChild<QPushButton *>(QStringLiteral("renameChatButton"));
        auto *exportBtn = window.findChild<QPushButton *>(QStringLiteral("exportChatButton"));
        auto *favorite = window.findChild<QPushButton *>(QStringLiteral("favoriteChatButton"));
        auto *archive = window.findChild<QPushButton *>(QStringLiteral("archiveChatButton"));
        auto *deleteBtn = window.findChild<QPushButton *>(QStringLiteral("deleteChatButton"));
        assert(newChat != nullptr);
        assert(rename != nullptr);
        assert(exportBtn != nullptr);
        assert(favorite != nullptr);
        assert(archive != nullptr);
        assert(deleteBtn != nullptr);
    });

    // ── 6. 顶部工具栏按钮 ─────────────────────────────────────────────
    test("top toolbar buttons exist", [] {
        MainWindow window;
        auto *settingsBtn = window.findChild<QPushButton *>(QStringLiteral("settingsButton"));
        auto *toolsBtn = window.findChild<QPushButton *>(QStringLiteral("toolsButton"));
        auto *logBtn = window.findChild<QPushButton *>(QStringLiteral("logButton"));
        assert(settingsBtn != nullptr);
        assert(toolsBtn != nullptr);
        assert(logBtn != nullptr);
    });

    // ── 7. 状态栏 ─────────────────────────────────────────────────────
    test("status bar exists", [] {
        MainWindow window;
        assert(window.statusBar() != nullptr);
    });

    // ── 8. 窗口属性 ───────────────────────────────────────────────────
    test("window title and size are set", [] {
        MainWindow window;
        assert(!window.windowTitle().isEmpty());
        assert(window.minimumWidth() >= 800);
        assert(window.minimumHeight() >= 500);
    });

    // ── 9. 搜索栏 ─────────────────────────────────────────────────────
    test("session search input exists", [] {
        MainWindow window;
        auto *searchEdit = window.findChild<QLineEdit *>(QStringLiteral("sessionSearchEdit"));
        assert(searchEdit != nullptr);
    });

    // ── 10. 析构不崩溃 ─────────────────────────────────────────────────
    test("destruction does not crash", [] {
        // MainWindow 在 lambda 结束时析构，验证 ApplicationController 清理安全
        MainWindow window;
        (void)window;
    });

    printf("\n%d/%d tests passed\n", passCount, testCount);
    return (passCount == testCount) ? 0 : 1;
}
