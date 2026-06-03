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
    assert(registry.definitions().size() >= 19);
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
    assert(registry.canExecuteDirectly(QStringLiteral("file.read_text")));
    assert(registry.canExecuteDirectly(QStringLiteral("command.git_status")));
    assert(registry.canExecuteDirectly(QStringLiteral("command.list_project_files")));
    assert(registry.canExecuteDirectly(QStringLiteral("memory.append_project_note")));

    const QJsonArray schemas = registry.functionToolSchemas(AppLanguage::English);
    assert(!schemas.isEmpty());
    bool foundWorkspaceWrite = false;
    bool foundFilePickerTool = false;
    bool foundCommandTool = false;
    bool foundMemoryTool = false;
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
        if (functionName == QStringLiteral("command_git_status")) {
            foundCommandTool = true;
            const QJsonObject parameters = function.value(QStringLiteral("parameters")).toObject();
            assert(parameters.value(QStringLiteral("type")).toString() == QStringLiteral("object"));
            assert(parameters.value(QStringLiteral("required")).toArray().isEmpty());
        }
        if (functionName == QStringLiteral("memory_append_project_note")) {
            foundMemoryTool = true;
            const QJsonObject parameters = function.value(QStringLiteral("parameters")).toObject();
            assert(parameters.value(QStringLiteral("required")).toArray().contains(QStringLiteral("content")));
        }
    }
    assert(foundWorkspaceWrite);
    assert(foundFilePickerTool);
    assert(foundCommandTool);
    assert(foundMemoryTool);

    AgentToolExecutionContext context;
    context.workspaceDirectory = temporaryDirectory.filePath(QStringLiteral("workspace"));
    context.projectDirectory = temporaryDirectory.filePath(QStringLiteral("project"));
    assert(QDir().mkpath(context.projectDirectory));
    QFile projectFile(QDir(context.projectDirectory).filePath(QStringLiteral("README.md")));
    assert(projectFile.open(QFile::WriteOnly | QFile::Text));
    assert(projectFile.write("hello") == 5);
    projectFile.close();

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

    result = registry.execute(QStringLiteral("command.list_project_files"), QJsonObject(), context);
    assert(result.ok);
    assert(result.output.contains(QStringLiteral("command.list_project_files")));
    assert(result.output.contains(QStringLiteral("[FILE] README.md")));

    QJsonObject memoryParameters;
    memoryParameters.insert(QStringLiteral("content"), QStringLiteral("Prefer ctest before commit."));
    result = registry.execute(QStringLiteral("memory.append_project_note"), memoryParameters, context);
    assert(result.ok);
    assert(result.output.contains(QStringLiteral("AGENT_MEMORY.md")));
    assert(readFile(QDir(context.projectDirectory).filePath(QStringLiteral("AGENT_MEMORY.md"))).contains(QStringLiteral("Prefer ctest before commit.")));

    QJsonObject unexpectedCommandParameters;
    unexpectedCommandParameters.insert(QStringLiteral("args"), QStringLiteral("status"));
    result = registry.execute(QStringLiteral("command.git_status"), unexpectedCommandParameters, context);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("parameters"), Qt::CaseInsensitive));

    result = registry.execute(QStringLiteral("file.read_text"), QJsonObject(), context);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("path"), Qt::CaseInsensitive));

    result = registry.execute(QStringLiteral("missing.tool"), QJsonObject(), context);
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("registered"), Qt::CaseInsensitive));

    return 0;
}
