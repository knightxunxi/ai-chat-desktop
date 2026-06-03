#include "app/ContextWindowManager.h"
#include "app/TokenEstimator.h"
#include "core/ChatSession.h"

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

static ChatSession buildSession(int roundCount, int fillCharCount = 500)
{
    ChatSession session = ChatSession::createDefault();
    for (int i = 0; i < roundCount; ++i) {
        session.addMessage(MessageRole::User,
                           QString("Question %1: ").arg(i)
                               + QString(fillCharCount, 'a'));
        session.addMessage(MessageRole::Assistant,
                           QString("Answer %1: ").arg(i)
                               + QString(fillCharCount, 'a'));
    }
    return session;
}

int main()
{
    // ── T8: Short conversation — no trim needed ───────────────
    {
        const ChatSession session = buildSession(5);
        ContextWindowManager mgr(128000);
        const ContextWindowResult result =
            mgr.processMessages(session.messages, QStringLiteral("Test"));

        checkBool("testShortConversation", false, result.wasTrimmed);
        assert(result.processedMessages.size() == session.messages.size());
        std::cout << "  -> messages preserved: "
                  << result.processedMessages.size() << " / "
                  << session.messages.size() << std::endl;
    }

    // ── T9: Long conversation — trim triggered ────────────────
    {
        // 60 rounds × 3000 chars/msg = enough to exceed 64K × 0.85 threshold
        const ChatSession session = buildSession(60, 3000);
        ContextWindowManager mgr(64000);

        // Log token estimate for diagnostics
        const size_t estimatedTotal =
            TokenEstimator::estimateTotalTokens(session.messages)
            + TokenEstimator::estimateSystemPromptTokens(QStringLiteral("Test"));
        std::cout << "  -> estimated total tokens: " << estimatedTotal
                  << " (threshold: "
                  << static_cast<size_t>(64000.0 * 0.85) << ")" << std::endl;

        const ContextWindowResult result =
            mgr.processMessages(session.messages, QStringLiteral("Test"));

        checkBool("testLongConversation", true, result.wasTrimmed);
        std::cout << "  -> trimmedRounds=" << result.trimmedRoundCount
                  << ", keptMessages=" << result.processedMessages.size()
                  << " / " << session.messages.size() << std::endl;

        // Verify some messages were actually trimmed
        assert(result.processedMessages.size() < session.messages.size());
    }

    // ── T10: Role ordering after trim ─────────────────────────
    {
        const ChatSession session = buildSession(60, 3000);
        ContextWindowManager mgr(64000);
        const ContextWindowResult result =
            mgr.processMessages(session.messages, QStringLiteral("Test"));

        assert(result.wasTrimmed);
        assert(!result.processedMessages.isEmpty());

        const bool firstIsUser =
            (result.processedMessages.first().role == MessageRole::User);
        const bool lastIsUser =
            (result.processedMessages.last().role == MessageRole::User);

        checkBool("testRoleOrderingAfterTrim-firstIsUser", true, firstIsUser);
        checkBool("testRoleOrderingAfterTrim-lastIsUser", true, lastIsUser);
    }

    // ── Extra: buildCompressionHint ───────────────────────────
    {
        const QString hintNoSummary =
            ContextWindowManager::buildCompressionHint(3, QString());
        assert(!hintNoSummary.isEmpty());
        assert(hintNoSummary.contains(QStringLiteral("3")));
        std::cout << "PASS: testBuildCompressionHint-noSummary ("
                  << hintNoSummary.toStdString() << ")" << std::endl;
        ++g_passed;

        const QString hintWithSummary =
            ContextWindowManager::buildCompressionHint(5, QStringLiteral("摘要内容"));
        assert(hintWithSummary.contains(QStringLiteral("5")));
        assert(hintWithSummary.contains(QStringLiteral("摘要内容")));
        std::cout << "PASS: testBuildCompressionHint-withSummary" << std::endl;
        ++g_passed;
    }

    // ── Extra: contextWindowForModel ──────────────────────────
    {
        assert(ContextWindowManager::contextWindowForModel(
                   QStringLiteral("deepseek-v4-flash")) == 128000);
        assert(ContextWindowManager::contextWindowForModel(
                   QStringLiteral("deepseek-chat")) == 64000);
        assert(ContextWindowManager::contextWindowForModel(
                   QStringLiteral("gpt-4")) == 8192);
        assert(ContextWindowManager::contextWindowForModel(
                   QStringLiteral("unknown-model")) == 64000); // default
        std::cout << "PASS: testContextWindowForModel" << std::endl;
        ++g_passed;
    }

    std::cout << "\n=== ContextWindowManager Results: " << g_passed
              << " passed, " << g_failed << " failed ===\n";

    return g_failed > 0 ? 1 : 0;
}
