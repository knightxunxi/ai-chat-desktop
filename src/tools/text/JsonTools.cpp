#include "tools/text/JsonCompactTool.h"
#include "tools/text/JsonFormatTool.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace {

ToolResult transformJson(const QString &input, QJsonDocument::JsonFormat format)
{
    const QString trimmedInput = input.trimmed();
    if (trimmedInput.isEmpty()) {
        return ToolResult::failure(QStringLiteral("Input is empty."));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(trimmedInput.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return ToolResult::failure(
            QStringLiteral("JSON parse error at offset %1: %2")
                .arg(parseError.offset)
                .arg(parseError.errorString()));
    }

    if (document.isNull()) {
        return ToolResult::failure(QStringLiteral("Input is not a valid JSON document."));
    }

    return ToolResult::success(QString::fromUtf8(document.toJson(format)).trimmed());
}

} // namespace

QString JsonFormatTool::id() const
{
    return QStringLiteral("json.format");
}

QString JsonFormatTool::displayName(AppLanguage language) const
{
    return language == AppLanguage::Chinese ? QStringLiteral("JSON 格式化") : QStringLiteral("JSON Format");
}

QString JsonFormatTool::description(AppLanguage language) const
{
    return language == AppLanguage::Chinese
               ? QStringLiteral("将 JSON 转为缩进格式，便于阅读和检查。")
               : QStringLiteral("Formats JSON with indentation for easier reading.");
}

ToolResult JsonFormatTool::run(const QString &input) const
{
    return transformJson(input, QJsonDocument::Indented);
}

QString JsonCompactTool::id() const
{
    return QStringLiteral("json.compact");
}

QString JsonCompactTool::displayName(AppLanguage language) const
{
    return language == AppLanguage::Chinese ? QStringLiteral("JSON 压缩") : QStringLiteral("JSON Compact");
}

QString JsonCompactTool::description(AppLanguage language) const
{
    return language == AppLanguage::Chinese
               ? QStringLiteral("将 JSON 转为单行紧凑格式，便于复制和传输。")
               : QStringLiteral("Compacts JSON into a single line for copying and transport.");
}

ToolResult JsonCompactTool::run(const QString &input) const
{
    return transformJson(input, QJsonDocument::Compact);
}
