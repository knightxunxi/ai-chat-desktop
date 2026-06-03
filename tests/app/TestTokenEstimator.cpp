#include "app/TokenEstimator.h"
#include "core/ChatMessage.h"

#include <cassert>
#include <iostream>
#include <string>

static int g_failed = 0;
static int g_passed = 0;

static void check(const char *name, size_t expected, size_t actual)
{
    if (expected == actual) {
        ++g_passed;
        std::cout << "PASS: " << name << std::endl;
    } else {
        ++g_failed;
        std::cerr << "FAIL: " << name << " — expected " << expected
                  << ", got " << actual << std::endl;
    }
}

int main()
{
    // ── T1: Chinese only ──────────────────────────────────────
    {
        const size_t tokens = TokenEstimator::estimateTokens(QStringLiteral("你好世界"));
        // 4 Chinese chars × 1.5 = 6 tokens
        check("testChineseOnly", 6, tokens);
    }

    // ── T2: English only ──────────────────────────────────────
    {
        const size_t tokens = TokenEstimator::estimateTokens(QStringLiteral("Hello World"));
        // 10 countable chars / 4 = 2.5 → +0.5 → 3 tokens
        check("testEnglishOnly", 3, tokens);
    }

    // ── T3: Mixed Chinese + English ───────────────────────────
    {
        const size_t tokens = TokenEstimator::estimateTokens(QStringLiteral("你好 World 你好"));
        // 4 Chinese × 1.5 = 6.0
        // 5 English / 4 = 1.25
        // total = 7.25 → +0.5 → 7 tokens
        check("testMixed", 7, tokens);
    }

    // ── T4: Code snippet ──────────────────────────────────────
    {
        const size_t tokens = TokenEstimator::estimateTokens(
            QStringLiteral("for(int i=0;i<10;i++){}"));
        // 22 countable chars / 4 = 5.5 → +0.5 → 6 tokens
        check("testCode", 6, tokens);
    }

    // ── T5: Empty string ──────────────────────────────────────
    {
        const size_t tokens = TokenEstimator::estimateTokens(QStringLiteral(""));
        check("testEmptyString", 0, tokens);
    }

    // ── T6: Message with role overhead ────────────────────────
    {
        ChatMessage msg = ChatMessage::create(
            QStringLiteral("session-id"), MessageRole::User, QStringLiteral("Hi"));
        // "Hi" → 2 / 4 = 0.5 → +0.5 → 1 token content + 4 role = 5
        const size_t tokens = TokenEstimator::estimateMessageTokens(msg);
        check("testMessageWithRoleOverhead", 5, tokens);
    }

    // ── T7: System prompt ─────────────────────────────────────
    {
        // 6 Chinese chars × 1.5 = 9 + 4 = 13 tokens
        const size_t tokens =
            TokenEstimator::estimateSystemPromptTokens(QStringLiteral("你是一个助手"));
        check("testSystemPrompt", 13, tokens);
    }

    // ── Extra: Whitespace only ────────────────────────────────
    {
        const size_t tokens = TokenEstimator::estimateTokens(QStringLiteral("   \n\t  "));
        check("testWhitespaceOnly", 0, tokens);
    }

    // ── Extra: Chinese with punctuation ───────────────────────
    {
        // "你好。" = 3 Chinese chars (including CJK punctuation '。')
        // 3 × 1.5 = 4.5 → +0.5 → 5 tokens
        const size_t tokens = TokenEstimator::estimateTokens(QStringLiteral("你好。"));
        check("testChineseWithPunctuation", 5, tokens);
    }

    // ── Extra: Total tokens across message list ───────────────
    {
        QVector<ChatMessage> messages;
        messages.append(ChatMessage::create(
            QStringLiteral("s1"), MessageRole::User, QStringLiteral("Hi")));
        messages.append(ChatMessage::create(
            QStringLiteral("s1"), MessageRole::Assistant, QStringLiteral("Hello")));
        // "Hi"   → 2/4=0.5→1 + 4 = 5
        // "Hello"→ 5/4=1.25→1 + 4 = 5
        // total = 10
        const size_t total = TokenEstimator::estimateTotalTokens(messages);
        check("testTotalTokens", 10, total);
    }

    std::cout << "\n=== TokenEstimator Results: "
              << g_passed << " passed, " << g_failed << " failed ===\n";

    return g_failed > 0 ? 1 : 0;
}
