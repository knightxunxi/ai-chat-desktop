#include "tools/JsonCompactTool.h"
#include "tools/JsonFormatTool.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cassert>

namespace {

QJsonDocument parseOutput(const QString &output)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(output.toUtf8(), &parseError);
    assert(parseError.error == QJsonParseError::NoError);
    return document;
}

} // namespace

int main()
{
    JsonFormatTool formatTool;
    JsonCompactTool compactTool;

    assert(formatTool.id() == QStringLiteral("json.format"));
    assert(formatTool.displayName(AppLanguage::Chinese) == QStringLiteral("JSON 格式化"));
    assert(formatTool.displayName(AppLanguage::English) == QStringLiteral("JSON Format"));

    ToolResult result = formatTool.run(QStringLiteral("{\"name\":\"test\",\"items\":[1,2]}"));
    assert(result.ok);
    assert(result.error.isEmpty());
    assert(result.output.contains('\n'));
    assert(result.output.contains(QStringLiteral("\"name\": \"test\"")));
    assert(parseOutput(result.output).object().value(QStringLiteral("items")).toArray().size() == 2);

    result = compactTool.run(QStringLiteral("{\n  \"name\": \"test\",\n  \"items\": [1, 2]\n}"));
    assert(result.ok);
    assert(!result.output.contains('\n'));
    assert(parseOutput(result.output).object().value(QStringLiteral("name")).toString() == QStringLiteral("test"));

    result = formatTool.run(QStringLiteral("[{\"id\":1},{\"id\":2}]"));
    assert(result.ok);
    assert(parseOutput(result.output).array().size() == 2);

    result = formatTool.run(QStringLiteral("{\"name\":"));
    assert(!result.ok);
    assert(result.output.isEmpty());
    assert(result.error.contains(QStringLiteral("JSON parse error")));
    assert(result.error.contains(QStringLiteral("offset")));

    result = compactTool.run(QString());
    assert(!result.ok);
    assert(result.output.isEmpty());
    assert(!result.error.isEmpty());

    return 0;
}
