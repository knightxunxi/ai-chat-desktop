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

    return 0;
}
