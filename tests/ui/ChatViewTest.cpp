#include "ui/ChatView.h"
#include "ui/AgentStepGroupWidget.h"
#include "ui/AgentStepWidget.h"
#include "ui/MessageWidget.h"
#include "ui/TokenBar.h"
#include "ui/TypingIndicator.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>

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

    // ── 1. 构造验证：子控件创建 ──────────────────────────────────────
    test("construct creates scroll area, container, token bar, typing indicator", [] {
        ChatView view;
        auto *scrollArea = view.findChild<QScrollArea *>(QStringLiteral("chatScrollArea"));
        auto *container = view.findChild<QWidget *>(QStringLiteral("chatContainer"));
        auto *tokenBar = view.findChild<TokenBar *>();
        auto *typing = view.findChild<TypingIndicator *>();
        assert(scrollArea != nullptr);
        assert(container != nullptr);
        assert(view.getContentWidget() == container);
        assert(tokenBar != nullptr);
        assert(typing != nullptr);
        assert(view.messageCount() == 0);
    });

    // ── 2. addMessage — 基础添加 ────────────────────────────────────
    test("addMessage adds user/assistant/system messages", [] {
        ChatView view;
        auto *userMsg = view.addMessage(MessageRole::User, QStringLiteral("hello"));
        assert(userMsg != nullptr);
        assert(userMsg->role() == MessageRole::User);
        assert(view.messageCount() == 1);

        auto *asstMsg = view.addMessage(MessageRole::Assistant, QStringLiteral("hi"));
        assert(asstMsg != nullptr);
        assert(asstMsg->role() == MessageRole::Assistant);
        assert(view.messageCount() == 2);

        auto *sysMsg = view.addMessage(MessageRole::System, QStringLiteral("info"));
        assert(sysMsg != nullptr);
        assert(sysMsg->role() == MessageRole::System);
        assert(view.messageCount() == 3);
    });

    // ── 3. addMessage with messageId ─────────────────────────────────
    test("addMessage with messageId stores mapping", [] {
        ChatView view;
        auto *msg = view.addMessage(MessageRole::User, QStringLiteral("test"),
                                    QStringLiteral("msg-001"));
        assert(msg != nullptr);
        assert(view.widgetForMessageId(QStringLiteral("msg-001")) == msg);
    });

    test("widgetForMessageId returns nullptr for unknown id", [] {
        ChatView view;
        assert(view.widgetForMessageId(QStringLiteral("nonexistent")) == nullptr);
    });

    // ── 4. updateLastAssistantMessage — 流式更新 ─────────────────────
    test("updateLastAssistantMessage updates last assistant content", [] {
        ChatView view;
        auto *asst = view.addMessage(MessageRole::Assistant, QStringLiteral("old"));
        view.updateLastAssistantMessage(QStringLiteral("new content"));
        assert(asst->content() == QStringLiteral("new content"));
    });

    test("updateLastAssistantMessage auto-creates when none exists", [] {
        ChatView view;
        assert(view.messageCount() == 0);
        view.updateLastAssistantMessage(QStringLiteral("auto created"));
        assert(view.messageCount() == 1);
    });

    // ── 5. clearMessages ─────────────────────────────────────────────
    test("clearMessages removes all messages", [] {
        ChatView view;
        view.addMessage(MessageRole::User, QStringLiteral("a"));
        view.addMessage(MessageRole::Assistant, QStringLiteral("b"));
        view.addMessage(MessageRole::System, QStringLiteral("c"));
        assert(view.messageCount() == 3);
        view.clearMessages();
        assert(view.messageCount() == 0);
    });

    test("clearMessages clears messageId mapping", [] {
        ChatView view;
        view.addMessage(MessageRole::User, QStringLiteral("x"), QStringLiteral("id-x"));
        assert(view.widgetForMessageId(QStringLiteral("id-x")) != nullptr);
        view.clearMessages();
        assert(view.widgetForMessageId(QStringLiteral("id-x")) == nullptr);
    });

    // ── 6. removeMessagesFrom — 从指定消息截断 ────────────────────────
    test("removeMessagesFrom truncates from given message onward", [] {
        ChatView view;
        view.addMessage(MessageRole::User, QStringLiteral("m1"), QStringLiteral("a"));
        view.addMessage(MessageRole::Assistant, QStringLiteral("m2"), QStringLiteral("b"));
        view.addMessage(MessageRole::User, QStringLiteral("m3"), QStringLiteral("c"));
        assert(view.messageCount() == 3);

        view.removeMessagesFrom(QStringLiteral("b"));
        // "a" should remain, "b" and "c" removed
        assert(view.messageCount() == 1);
        assert(view.widgetForMessageId(QStringLiteral("a")) != nullptr);
        assert(view.widgetForMessageId(QStringLiteral("b")) == nullptr);
        assert(view.widgetForMessageId(QStringLiteral("c")) == nullptr);
    });

    test("removeMessagesFrom last message clears all", [] {
        ChatView view;
        view.addMessage(MessageRole::User, QStringLiteral("only"), QStringLiteral("last"));
        view.removeMessagesFrom(QStringLiteral("last"));
        assert(view.messageCount() == 0);
    });

    // ── 7. updateTokenUsage ──────────────────────────────────────────
    test("updateTokenUsage does not crash", [] {
        ChatView view;
        view.updateTokenUsage(0, 0);
        view.updateTokenUsage(500, 4096);
        view.updateTokenUsage(4096, 4096); // at limit
        // TokenBar is internal; just verify no crash
    });

    // ── 8. search bar show/hide ──────────────────────────────────────
    test("showSearchBar creates visible search edit, hideSearchBar hides it", [] {
        ChatView view;
        view.showSearchBar();
        auto *searchEdit = view.findChild<QLineEdit *>();
        assert(searchEdit != nullptr);
        assert(!searchEdit->isHidden());

        view.hideSearchBar();
        assert(searchEdit->isHidden());
    });

    // ── 9. typing indicator show/hide ────────────────────────────────
    test("showTyping and hideTyping do not crash", [] {
        ChatView view;
        view.showTyping();
        view.hideTyping();
        view.showTyping();
        view.showTyping(); // double show should be safe
        view.hideTyping();
    });

    // ── 10. addDebugCard ─────────────────────────────────────────────
    // 注: messageCount() = m_contentLayout->count() - 1，计入所有布局条目
    test("addDebugCard increments messageCount", [] {
        ChatView view;
        assert(view.messageCount() == 0);
        view.addDebugCard(QStringLiteral("Debug Title"), QStringLiteral("debug body"));
        assert(view.messageCount() == 1); // debug card 是布局子条目
    });

    // ── 11. addAgentStepWidget ───────────────────────────────────────
    test("addAgentStepWidget increments messageCount", [] {
        ChatView view;
        auto *step = new AgentStepWidget(1, QStringLiteral("reasoning"),
                                         QStringLiteral("tool-1"),
                                         QStringLiteral("Test Tool"),
                                         view.getContentWidget());
        assert(view.messageCount() == 0);
        view.addAgentStepWidget(step);
        assert(view.messageCount() == 1); // step widget 是布局子条目
    });

    test("agent step group routes same-iteration results by tool id", [] {
        AgentStepGroupWidget group;
        auto *first = new AgentStepWidget(1, QStringLiteral("first"),
                                          QStringLiteral("tool.first"),
                                          QStringLiteral("First"),
                                          &group);
        auto *second = new AgentStepWidget(1, QStringLiteral("second"),
                                           QStringLiteral("tool.second"),
                                           QStringLiteral("Second"),
                                           &group);
        group.addStep(first);
        group.addStep(second);

        group.setStepResult(1, QStringLiteral("tool.second"), false, QStringLiteral("second failed"));

        auto *firstResult = first->findChild<QLabel *>(QStringLiteral("agentStepResult"));
        auto *secondResult = second->findChild<QLabel *>(QStringLiteral("agentStepResult"));
        assert(firstResult != nullptr);
        assert(secondResult != nullptr);
        assert(firstResult->text().isEmpty());
        assert(secondResult->text().contains(QStringLiteral("second failed")));

        group.setStepResult(1, QStringLiteral("tool.first"), true, QStringLiteral("first ok"));
        assert(firstResult->text().contains(QStringLiteral("first ok")));
    });

    // ── 12. 混合添加后 messageCount 统计所有布局条目 ───────────────────
    test("messageCount accounts for all layout children", [] {
        ChatView view;
        view.addMessage(MessageRole::User, QStringLiteral("a"));         // +1 = 1
        view.addDebugCard(QStringLiteral("dbg"), QStringLiteral("x"));   // +1 = 2
        view.addMessage(MessageRole::Assistant, QStringLiteral("b"));    // +1 = 3
        auto *step = new AgentStepWidget(1, QStringLiteral("r"),
                                         QStringLiteral("t"), QStringLiteral("T"),
                                         view.getContentWidget());
        view.addAgentStepWidget(step);                                   // +1 = 4
        view.addMessage(MessageRole::User, QStringLiteral("c"));        // +1 = 5
        assert(view.messageCount() == 5);
    });

    printf("\n%d/%d tests passed\n", passCount, testCount);
    return (passCount == testCount) ? 0 : 1;
}
