#include "app/SummaryAPIClient.h"
#include "core/ChatMessage.h"

#include <cassert>
#include <iostream>

static int g_failed = 0;
static int g_passed = 0;

static void checkBool(const char *name, bool expected, bool actual)
{
    if (expected == actual) {
        ++g_passed;
        std::cout << "PASS: " << name << std::endl;
    } else {
        ++g_failed;
        std::cerr << "FAIL: " << name << " — expected "
                  << (expected ? "true" : "false") << ", got "
                  << (actual ? "true" : "false") << std::endl;
    }
}

int main()
{
    // ── T11: buildSummaryPrompt with User + Assistant messages ─
    {
        QVector<ChatMessage> messages;
        messages.append(ChatMessage::create(
            QStringLiteral("s1"), MessageRole::User,
            QStringLiteral("今天天气怎么样？")));
        messages.append(ChatMessage::create(
            QStringLiteral("s1"), MessageRole::Assistant,
            QStringLiteral("今天天气晴朗，适合出行。")));

        const QString prompt = SummaryAPIClient::buildSummaryPrompt(messages);

        // Prompt should contain the instruction
        const bool hasInstruction = prompt.contains(QStringLiteral("总结"));
        checkBool("testBuildSummaryPrompt-hasInstruction", true, hasInstruction);

        // Prompt should contain the user message content
        const bool hasUserContent = prompt.contains(QStringLiteral("今天天气怎么样"));
        checkBool("testBuildSummaryPrompt-hasUserContent", true, hasUserContent);

        // Prompt should contain the assistant message content
        const bool hasAssistantContent =
            prompt.contains(QStringLiteral("今天天气晴朗"));
        checkBool("testBuildSummaryPrompt-hasAssistantContent", true,
                   hasAssistantContent);

        // Prompt should have role labels
        const bool hasUserLabel = prompt.contains(QStringLiteral("用户"));
        const bool hasAssistantLabel = prompt.contains(QStringLiteral("助手"));
        checkBool("testBuildSummaryPrompt-hasUserLabel", true, hasUserLabel);
        checkBool("testBuildSummaryPrompt-hasAssistantLabel", true,
                   hasAssistantLabel);

        // Prompt should not be empty
        assert(!prompt.isEmpty());
        std::cout << "  -> prompt length: " << prompt.size() << " chars"
                  << std::endl;
    }

    // ── T12: generateSummary with empty messages ──────────────
    {
        SummaryAPIClient client;
        const QVector<ChatMessage> emptyMessages;
        const QString result = client.generateSummary(emptyMessages);

        // Empty input should produce empty output (no network call)
        checkBool("testEmptyMessages", true, result.isEmpty());
    }

    // ── Extra: buildSummaryPrompt with system message ─────────
    {
        QVector<ChatMessage> messages;
        messages.append(ChatMessage::create(
            QStringLiteral("s1"), MessageRole::System,
            QStringLiteral("你是一个助手")));

        const QString prompt = SummaryAPIClient::buildSummaryPrompt(messages);
        const bool hasSystemLabel = prompt.contains(QStringLiteral("系统"));
        checkBool("testBuildSummaryPrompt-systemLabel", true, hasSystemLabel);
    }

    // ── Extra: setTimeout ─────────────────────────────────────
    {
        SummaryAPIClient client;
        client.setTimeout(10000);
        // No assertion possible on private member; smoke test: no crash.
        std::cout << "PASS: testSetTimeout (smoke)" << std::endl;
        ++g_passed;
    }

    std::cout << "\n=== SummaryAPIClient Results: " << g_passed
              << " passed, " << g_failed << " failed ===\n";

    return g_failed > 0 ? 1 : 0;
}
