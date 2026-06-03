#include "services/StreamParser.h"

#include <cassert>
#include <cstdio>
#include <functional>
#include <stdexcept>

// V12.3 流式工具执行测试 — 验证 StreamParser 在工具调用参数完整时立即发出 ToolUseComplete 事件。

static int testCount = 0;
static int passCount = 0;

static void test(const char *name, std::function<void()> body)
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

static void assertNoBlockEvents(const StreamParseResult &result, const char *context)
{
    if (!result.blockEvents.isEmpty()) {
        throw std::runtime_error(
            QStringLiteral("%1: expected no blockEvents, got %2")
                .arg(context)
                .arg(result.blockEvents.size())
                .toStdString());
    }
}

static void assertBlockEventCount(const StreamParseResult &result, int expected, const char *context)
{
    if (result.blockEvents.size() != expected) {
        throw std::runtime_error(
            QStringLiteral("%1: expected %2 blockEvents, got %3")
                .arg(context)
                .arg(expected)
                .arg(result.blockEvents.size())
                .toStdString());
    }
}

static void assertToolUseComplete(const ContentBlockEvent &event,
                                  const QString &expectedName,
                                  const char *context)
{
    if (event.type != ContentBlockEventType::ToolUseComplete) {
        throw std::runtime_error(
            QStringLiteral("%1: expected ToolUseComplete, got different type").arg(context).toStdString());
    }
    if (event.toolName != expectedName) {
        throw std::runtime_error(
            QStringLiteral("%1: expected toolName=%2, got %3")
                .arg(context, expectedName, event.toolName)
                .toStdString());
    }
    if (event.arguments.isEmpty()) {
        throw std::runtime_error(
            QStringLiteral("%1: arguments should not be empty").arg(context).toStdString());
    }
}

int main()
{
    printf("StreamParserV12Test\n");

    // 1. blockStart_text_delta — 纯文本 delta 不产生 ToolUseComplete 事件
    test("blockStart_text_delta", []() {
        StreamParser parser;
        const StreamParseResult result = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"content\":\"I will\"}}]}\n");

        assert(result.textDeltas.size() == 1);
        assert(result.textDeltas[0] == QStringLiteral("I will"));
        assertNoBlockEvents(result, "text delta only");
    });

    // 2. toolUseComplete_single_tool — 单个工具调用参数完整时发出 ToolUseComplete
    test("toolUseComplete_single_tool", []() {
        StreamParser parser;
        const StreamParseResult result = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_1\","
            "\"function\":{\"name\":\"read_file\",\"arguments\":\"{\\\"path\\\":\\\"/tmp/test.txt\\\"}\"}}]}}]}\n");

        assertBlockEventCount(result, 1, "single tool call");
        assertToolUseComplete(result.blockEvents[0], QStringLiteral("read_file"), "single tool call");
        assert(result.blockEvents[0].arguments.value(QStringLiteral("path")).toString() == QStringLiteral("/tmp/test.txt"));
    });

    // 3. toolUseComplete_multi_tool_calls — 多个工具调用按完成顺序发出事件
    test("toolUseComplete_multi_tool_calls", []() {
        StreamParser parser;

        // First SSE: tool 0 gets name, no arguments yet
        StreamParseResult r1 = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_a\","
            "\"function\":{\"name\":\"read_file\",\"arguments\":\"\"}}]}}]}\n");

        assertNoBlockEvents(r1, "multi: tool 0 name only");

        // Second SSE: tool 0 gets arguments (should complete now)
        StreamParseResult r2 = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,"
            "\"function\":{\"arguments\":\"{\\\"path\\\":\\\"/a.txt\\\"}\"}}]}}]}\n");

        assertBlockEventCount(r2, 1, "multi: tool 0 complete");
        assertToolUseComplete(r2.blockEvents[0], QStringLiteral("read_file"), "multi: tool 0");

        // Third SSE: tool 1 arrives fully
        StreamParseResult r3 = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":1,\"id\":\"call_b\","
            "\"function\":{\"name\":\"write_file\",\"arguments\":\"{\\\"path\\\":\\\"/b.txt\\\"}\"}}]}}]}\n");

        assertBlockEventCount(r3, 1, "multi: tool 1 complete");
        assertToolUseComplete(r3.blockEvents[0], QStringLiteral("write_file"), "multi: tool 1");
    });

    // 4. toolUseComplete_arguments_partial — 参数未完整时不发出事件
    test("toolUseComplete_arguments_partial", []() {
        StreamParser parser;

        // First: set up tool call with name but incomplete JSON
        StreamParseResult r1 = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_x\","
            "\"function\":{\"name\":\"search\",\"arguments\":\"{\\\"query\\\":\\\"hel\"}}]}}]}\n");

        assertNoBlockEvents(r1, "partial: incomplete JSON arguments");

        // Complete the JSON
        StreamParseResult r2 = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,"
            "\"function\":{\"arguments\":\"lo\\\"}\"}}]}}]}\n");

        assertBlockEventCount(r2, 1, "partial: JSON now complete");
        assertToolUseComplete(r2.blockEvents[0], QStringLiteral("search"), "partial: complete");
    });

    // 5. noToolCalls_plainText — 纯文本消息不产生工具事件
    test("noToolCalls_plainText", []() {
        StreamParser parser;
        const StreamParseResult result = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"content\":\"Hello world\"}}]}\n"
            "data: [DONE]\n");

        assert(result.done);
        assert(result.textDeltas.size() == 1);
        assert(result.textDeltas[0] == QStringLiteral("Hello world"));
        assertNoBlockEvents(result, "plain text only");
        assert(result.toolCalls.isEmpty());
    });

    // 6. toolUseComplete_at_done — 在 [DONE] 时补发未完成的工具调用
    test("toolUseComplete_at_done", []() {
        StreamParser parser;

        // Send a tool call that is complete but wasn't emitted during streaming
        // (simulating a case where the final chunk arrives with [DONE])
        const StreamParseResult result = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_z\","
            "\"function\":{\"name\":\"run\",\"arguments\":\"{\\\"cmd\\\":\\\"ls\\\"}\"}}]}}]}\n"
            "data: [DONE]\n");

        assert(result.done);
        // Should have both the streaming blockEvent AND the [DONE] gives it a second chance
        // But emitPendingToolCalls won't re-emit because it was already emitted during streaming
        assertBlockEventCount(result, 1, "at_done: tool should be emitted");
        assertToolUseComplete(result.blockEvents[0], QStringLiteral("run"), "at_done");
    });

    // 7. toolUseComplete_reset — reset() 后状态正确清空
    test("toolUseComplete_reset", []() {
        StreamParser parser;

        // Produce a tool call
        StreamParseResult r1 = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_r\","
            "\"function\":{\"name\":\"tool_a\",\"arguments\":\"{\\\"x\\\":1}\"}}]}}]}\n");

        assertBlockEventCount(r1, 1, "reset: first tool");

        // Reset parser state
        parser.reset();

        // Same tool call again — should be emitted again (emit state is cleared)
        StreamParseResult r2 = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_s\","
            "\"function\":{\"name\":\"tool_b\",\"arguments\":\"{\\\"y\\\":2}\"}}]}}]}\n");

        assertBlockEventCount(r2, 1, "reset: second tool after reset");
        assertToolUseComplete(r2.blockEvents[0], QStringLiteral("tool_b"), "reset: after reset");
    });

    // 8. content_delta_text — content delta 不触发 ToolUseComplete
    test("content_delta_text", []() {
        StreamParser parser;

        const StreamParseResult result = parser.consume(
            "data: {\"choices\":[{\"delta\":{\"content\":\"Let me think about this...\"}}]}\n"
            "data: {\"choices\":[{\"delta\":{\"content\":\"I will now search.\"}}]}\n");

        assert(result.textDeltas.size() == 2);
        assert(result.textDeltas[0] == QStringLiteral("Let me think about this..."));
        assert(result.textDeltas[1] == QStringLiteral("I will now search."));
        assertNoBlockEvents(result, "content only");
        assert(result.toolCalls.isEmpty());
    });

    printf("\nResults: %d/%d passed\n", passCount, testCount);
    return passCount == testCount ? 0 : 1;
}
