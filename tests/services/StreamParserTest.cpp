#include "services/StreamParser.h"

#include <cassert>

int main()
{
    StreamParser parser;

    StreamParseResult result = parser.consume(
        "data: {\"choices\":[{\"delta\":{\"content\":\"Hel\"}}]}\n"
        "data: {\"choices\":[{\"delta\":{\"content\":\"lo\"}}]}\n");

    assert(result.textDeltas.size() == 2);
    assert(result.textDeltas[0] == QStringLiteral("Hel"));
    assert(result.textDeltas[1] == QStringLiteral("lo"));
    assert(!result.done);

    result = parser.consume("data: {\"choices\":[{\"delta\":{\"content\":\" wor");
    assert(result.textDeltas.isEmpty());

    result = parser.consume("ld\"}}]}\r\n");
    assert(result.textDeltas.size() == 1);
    assert(result.textDeltas[0] == QStringLiteral(" world"));

    result = parser.consume("data: [DONE]\n");
    assert(result.done);

    parser.reset();
    result = parser.consume("data: {\"choices\":[{\"delta\":{\"content\":\"tail\"}}]}");
    assert(result.textDeltas.isEmpty());

    result = parser.finish();
    assert(result.textDeltas.size() == 1);
    assert(result.textDeltas[0] == QStringLiteral("tail"));

    parser.reset();
    result = parser.consume(
        "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call-1\",\"type\":\"function\",\"function\":{\"name\":\"json_format\",\"arguments\":\"{\\\"input\\\":\"}}]}}]}\n"
        "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\"\\\"hello\\\"}\"}}]}}]}\n"
        "data: [DONE]\n");
    assert(result.done);
    assert(result.toolCalls.size() == 1);
    assert(result.toolCalls.first().id == QStringLiteral("call-1"));
    assert(result.toolCalls.first().functionName == QStringLiteral("json_format"));
    assert(result.toolCalls.first().arguments == QStringLiteral("{\"input\":\"hello\"}"));

    return 0;
}
