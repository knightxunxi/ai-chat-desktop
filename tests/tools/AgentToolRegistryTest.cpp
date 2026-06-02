#include "tools/AgentToolRegistry.h"

#include "support/AppLogger.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>

#include <cassert>

namespace {

QString readFile(const QString &path)
{
    QFile file(path);
    assert(file.open(QFile::ReadOnly | QFile::Text));
    return QString::fromUtf8(file.readAll());
}

} // namespace

int main()
{
    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());
    AppLogger::setLogFilePathForTests(temporaryDirectory.filePath(QStringLiteral("agent-tool-registry.log")));
    QString loggerError;
    assert(AppLogger::initialize(&loggerError));

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();
    assert(registry.definitions().size() >= 13);
    assert(registry.descriptors().size() == registry.definitions().size());

    QSet<QString> ids;
    QSet<QString> functionNames;
    for (const AgentToolDefinition &definition : registry.definitions()) {
        assert(!definition.descriptor.id.isEmpty());
        assert(!definition.functionName.isEmpty());
        assert(!definition.functionName.contains(QLatin1Char('.')));
        assert(!definition.parameterSchema.isEmpty());
        assert(!ids.contains(definition.descriptor.id));
        assert(!functionNames.contains(definition.functionName));
        ids.insert(definition.descriptor.id);
        functionNames.insert(definition.functionName);
    }

    assert(AgentToolRegistryFactory::functionNameForToolId(QStringLiteral("workspace.write_text")) == QStringLiteral("workspace_write_text"));
    assert(registry.findById(QStringLiteral("json.format")) != nullptr);
    assert(registry.findByFunctionName(QStringLiteral("json_format")) != nullptr);
    assert(registry.findByFunctionName(QStringLiteral("json_format"))->descriptor.id == QStringLiteral("json.format"));
    assert(registry.canExecuteDirectly(QStringLiteral("json.format")));
    assert(registry.canExecuteDirectly(QStringLiteral("workspace.write_text")));
    assert(!registry.canExecuteDirectly(QStringLiteral("file.read_text")));

    const QJsonArray schemas = registry.functionToolSchemas(AppLanguage::English);
    assert(!schemas.isEmpty());
    bool foundWorkspaceWrite = false;
    bool foundFilePickerTool = false;
    for (const QJsonValue &schemaValue : schemas) {
        assert(schemaValue.isObject());
        const QJsonObject schema = schemaValue.toObject();
        assert(schema.value(QStringLiteral("type")).toString() == QStringLiteral("function"));
        const QJsonObject function = schema.value(QStringLiteral("function")).toObject();
        const QString functionName = function.value(QStringLiteral("name")).toString();
        assert(!functionName.contains(QLatin1Char('.')));
        assert(function.value(QStringLiteral("parameters")).isObject());
        if (functionName == QStringLiteral("workspace_write_text")) {
            foundWorkspaceWrite = true;
            const QJsonObject parameters = function.value(QStringLiteral("parameters")).toObject();
            assert(parameters.value(QStringLiteral("type")).toString() == QStringLiteral("object"));
            assert(parameters.value(QStringLiteral("required")).toArray().contains(QStringLiteral("path")));
            assert(parameters.value(QStringLiteral("required")).toArray().contains(QStringLiteral("content")));
        }
        if (functionName == QStringLiteral("file_read_text")) {
            foundFilePickerTool = true;
        }
    }
    assert(foundWorkspaceWrite);
    assert(!foundFilePickerTool);

    AgentToolExecutionContext context;
    context.workspaceDirectory = temporaryDirectory.filePath(QStringLiteral("workspace"));

    QJsonObject textParameters;
    textParameters.insert(QStringLiteral("input"), QStringLiteral(" A \n\n\n B "));
    ToolResult result = registry.execute(QStringLiteral("text.cleanup"), textParameters, context);
    assert(result.ok);
    assert(result.output == QStringLiteral("A\n\nB"));

    QJsonObject writeParameters;
    writeParameters.insert(QStringLiteral("path"), QStringLiteral("notes/hello.txt"));
    writeParameters.insert(QStringLiteral("content"), QStringLiteral("hello"));
    result = registry.execute(QStringLiteral("workspace.write_text"), writeParameters, context);
    assert(result.ok);
    assert(readFile(QDir(context.workspaceDirectory).filePath(QStringLiteral("notes/hello.txt"))) == QStringLiteral("hello"));

    result = registry.execute(QStringLiteral("file.read_text"), QJsonObject(), context);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("file picker"), Qt::CaseInsensitive));

    result = registry.execute(QStringLiteral("missing.tool"), QJsonObject(), context);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("registered"), Qt::CaseInsensitive));

    return 0;
}
