// V19: Agent E2E 循环测试 — 使用 MockApiClient 模拟 AI 回复
// 覆盖场景：
// 1. MockApiClient 工具调用预设 → 发射 toolCallsReceived
// 2. MockApiClient 纯文本预设 → 发射 textDeltaReceived + requestFinished
// 3. MockApiClient 错误模拟 → 发射 requestFailed
// 4. MockApiClient 无预设 → 发射 requestFailed("Mock: no more responses")

#include "../MockApiClient.h"

#include "app/ApplicationController.h"
#include "support/AppLogger.h"
#include <QTemporaryDir>
#include <cassert>
#include <cstdio>

static AppConfig makeConfig() {
    AppConfig cfg;
    cfg.apiKey = QStringLiteral("test-key");
    cfg.baseUrl = QStringLiteral("http://localhost:9999/v1");
    cfg.modelName = QStringLiteral("test-model");
    cfg.language = AppLanguage::English;
    return cfg;
}

int main()
{
    QTemporaryDir tmpDir;
    assert(tmpDir.isValid());
    AppLogger::setLogFilePathForTests(tmpDir.filePath(QStringLiteral("agent-e2e-test.log")));
    QString err;
    AppLogger::initialize(&err);

    // ================================================================
    // Test 1: 工具调用预设
    // ================================================================
    {
        ToolCallList tc;
        ToolCall t;
        t.functionName = QStringLiteral("file.read_text");
        t.arguments = QStringLiteral("{\"path\":\"/tmp/test.txt\"}");
        tc.append(t);

        MockApiClient mock;
        mock.setResponses({{QString(), tc}, {QStringLiteral("Done."), {}}});

        int deltaCount = 0, toolCount = 0, finishCount = 0;
        QObject::connect(&mock, &AIClient::textDeltaReceived, [&](const QString &) { deltaCount++; });
        QObject::connect(&mock, &AIClient::toolCallsReceived, [&](const ToolCallList &calls) {
            toolCount++; assert(calls.size() == 1); assert(calls[0].functionName == QStringLiteral("file.read_text"));
        });
        QObject::connect(&mock, &AIClient::requestFinished, [&]() { finishCount++; });

        mock.sendChatWithTools(makeConfig(), ChatSession(), QJsonArray());
        assert(deltaCount == 0);
        assert(toolCount == 1);
        assert(finishCount == 1);
        printf("  [PASS] mock-tool-call\n");
    }

    // ================================================================
    // Test 2: 纯文本回复序列
    // ================================================================
    {
        MockApiClient mock;
        mock.setResponses({{QStringLiteral("Hello"), {}}, {QStringLiteral("World"), {}}});

        int deltaSeq = 0, finishSeq = 0;
        QObject::connect(&mock, &AIClient::textDeltaReceived, [&](const QString &d) {
            deltaSeq++; assert(d == (deltaSeq == 1 ? QStringLiteral("Hello") : QStringLiteral("World")));
        });
        QObject::connect(&mock, &AIClient::requestFinished, [&]() { finishSeq++; });

        mock.sendChat(makeConfig(), ChatSession());
        assert(deltaSeq == 1);
        assert(finishSeq == 1);
        assert(mock.requestCount() == 1);

        mock.sendChat(makeConfig(), ChatSession());
        assert(deltaSeq == 2);
        assert(finishSeq == 2);
        assert(mock.requestCount() == 2);
        printf("  [PASS] mock-text-sequence\n");
    }

    // ================================================================
    // Test 3: 错误模拟
    // ================================================================
    {
        MockApiClient mock;
        MockResponseUnit errResp;
        errResp.isError = true;
        errResp.errorMessage = QStringLiteral("Auth failed");
        errResp.errorCategory = RequestErrorCategory::Authentication;
        mock.setResponses({errResp});

        int failSig = 0;
        QObject::connect(&mock, &AIClient::requestFailed, [&](const QString &msg, RequestErrorCategory cat) {
            failSig++; assert(msg == QStringLiteral("Auth failed")); assert(cat == RequestErrorCategory::Authentication);
        });

        mock.sendChat(makeConfig(), ChatSession());
        assert(failSig == 1);
        printf("  [PASS] mock-error-simulation\n");
    }

    // ================================================================
    // Test 4: 无预设回复（超出响应范围）
    // ================================================================
    {
        MockApiClient mock;
        int failEmpty = 0;
        QObject::connect(&mock, &AIClient::requestFailed, [&](const QString &, RequestErrorCategory) { failEmpty++; });

        mock.sendChat(makeConfig(), ChatSession());
        assert(failEmpty == 1);
        assert(mock.requestCount() == 1);
        printf("  [PASS] mock-empty-response\n");
    }

    // ================================================================
    // Test 5: 取消操作
    // ================================================================
    {
        MockApiClient mock;
        assert(!mock.wasCancelled());
        mock.cancel();
        assert(mock.wasCancelled());
        printf("  [PASS] mock-cancel\n");
    }

    // ================================================================
    // Summary
    // ================================================================
    printf("\n");
    printf("========================================\n");
    printf("All AgentE2ETest tests passed!\n");
    printf("  [PASS] mock-tool-call\n");
    printf("  [PASS] mock-text-sequence\n");
    printf("  [PASS] mock-error-simulation\n");
    printf("  [PASS] mock-empty-response\n");
    printf("  [PASS] mock-cancel\n");
    printf("========================================\n");
    return 0;
}
