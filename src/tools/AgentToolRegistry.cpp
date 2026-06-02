#include "tools/AgentToolRegistry.h"

#include "support/AppLogger.h"
#include "tools/JsonCompactTool.h"
#include "tools/JsonFormatTool.h"
#include "tools/MarkdownCleanupTool.h"
#include "tools/TextCleanupTool.h"
#include "tools/WorkspaceFileService.h"

#include <QChar>
#include <QJsonValue>
#include <QStringList>

namespace {

AgentToolDescriptor makeDescriptor(
    const QString &id,
    const QString &englishName,
    const QString &chineseName,
    const QString &englishDescription,
    const QString &chineseDescription,
    const QString &inputPolicy,
    AgentToolRisk risk,
    bool resultMayContainSensitiveContent)
{
    AgentToolDescriptor descriptor;
    descriptor.id = id;
    descriptor.englishName = englishName;
    descriptor.chineseName = chineseName;
    descriptor.englishDescription = englishDescription;
    descriptor.chineseDescription = chineseDescription;
    descriptor.inputPolicy = inputPolicy;
    descriptor.risk = risk;
    descriptor.requiresUserConfirmation = true;
    descriptor.resultMayContainSensitiveContent = resultMayContainSensitiveContent;
    descriptor.enabledForAgent = true;
    return descriptor;
}

QJsonObject stringProperty(const QString &description)
{
    QJsonObject property;
    property.insert(QStringLiteral("type"), QStringLiteral("string"));
    property.insert(QStringLiteral("description"), description);
    return property;
}

QJsonObject objectSchema(const QJsonObject &properties, const QStringList &required)
{
    QJsonObject schema;
    schema.insert(QStringLiteral("type"), QStringLiteral("object"));
    schema.insert(QStringLiteral("properties"), properties);

    QJsonArray requiredArray;
    for (const QString &field : required) {
        requiredArray.append(field);
    }
    schema.insert(QStringLiteral("required"), requiredArray);
    schema.insert(QStringLiteral("additionalProperties"), false);
    return schema;
}

QJsonObject textInputSchema()
{
    QJsonObject properties;
    properties.insert(QStringLiteral("input"), stringProperty(QStringLiteral("Text input to process.")));
    return objectSchema(properties, {QStringLiteral("input")});
}

QJsonObject pathOnlySchema()
{
    QJsonObject properties;
    properties.insert(QStringLiteral("path"), stringProperty(QStringLiteral("Path relative to the configured Agent workspace.")));
    return objectSchema(properties, {QStringLiteral("path")});
}

QJsonObject pathContentSchema()
{
    QJsonObject properties;
    properties.insert(QStringLiteral("path"), stringProperty(QStringLiteral("Path relative to the configured Agent workspace.")));
    properties.insert(QStringLiteral("content"), stringProperty(QStringLiteral("UTF-8 text content.")));
    return objectSchema(properties, {QStringLiteral("path"), QStringLiteral("content")});
}

QString parameterText(const QJsonObject &parameters)
{
    const QStringList candidateKeys = {
        QStringLiteral("input"),
        QStringLiteral("text"),
        QStringLiteral("content")
    };

    for (const QString &key : candidateKeys) {
        const QJsonValue value = parameters.value(key);
        if (value.isString() && !value.toString().trimmed().isEmpty()) {
            return value.toString();
        }
    }

    return QString();
}

ToolResult runTextTool(const LocalTool &tool, const QString &toolId, const QJsonObject &parameters)
{
    const QString input = parameterText(parameters);
    if (input.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("This step requires a text parameter named input, text, or content."));
    }

    const ToolResult result = tool.run(input);
    AppLogger::info(QStringLiteral("AgentTool"),
                    QStringLiteral("Registry text tool executed. toolId=%1 ok=%2 outputLength=%3")
                        .arg(toolId, result.ok ? QStringLiteral("true") : QStringLiteral("false"))
                        .arg(result.output.size()));
    return result;
}

bool stringParameter(const QJsonObject &parameters, const QString &name, QString *value, QString *error)
{
    const QJsonValue parameterValue = parameters.value(name);
    if (!parameterValue.isString()) {
        if (error != nullptr) {
            *error = QStringLiteral("This step requires a string parameter named %1.").arg(name);
        }
        return false;
    }

    if (value != nullptr) {
        *value = parameterValue.toString();
    }
    return true;
}

bool nonEmptyPathParameter(const QJsonObject &parameters, QString *path, QString *error)
{
    QString value;
    if (!stringParameter(parameters, QStringLiteral("path"), &value, error)) {
        return false;
    }

    if (value.trimmed().isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("This step requires a non-empty string parameter named path.");
        }
        return false;
    }

    if (path != nullptr) {
        *path = value;
    }
    return true;
}

ToolResult disabledDirectExecution(const QString &message)
{
    return ToolResult::failure(message);
}

AgentToolDefinition makeDefinition(
    const AgentToolDescriptor &descriptor,
    const QJsonObject &parameterSchema,
    bool executableFromPlanPreview,
    const std::function<ToolResult(const QJsonObject &, const AgentToolExecutionContext &)> &execute)
{
    AgentToolDefinition definition;
    definition.descriptor = descriptor;
    definition.parameterSchema = parameterSchema;
    definition.functionName = AgentToolRegistryFactory::functionNameForToolId(descriptor.id);
    definition.executableFromPlanPreview = executableFromPlanPreview;
    definition.execute = execute;
    return definition;
}

} // namespace

AgentToolRegistry::AgentToolRegistry(const QVector<AgentToolDefinition> &definitions)
    : m_definitions(definitions)
{
}

const QVector<AgentToolDefinition> &AgentToolRegistry::definitions() const
{
    return m_definitions;
}

QVector<AgentToolDescriptor> AgentToolRegistry::descriptors() const
{
    QVector<AgentToolDescriptor> descriptors;
    descriptors.reserve(m_definitions.size());
    for (const AgentToolDefinition &definition : m_definitions) {
        descriptors.append(definition.descriptor);
    }
    return descriptors;
}

const AgentToolDefinition *AgentToolRegistry::findById(const QString &toolId) const
{
    const QString normalizedToolId = toolId.trimmed();
    for (const AgentToolDefinition &definition : m_definitions) {
        if (definition.descriptor.id == normalizedToolId) {
            return &definition;
        }
    }

    return nullptr;
}

const AgentToolDefinition *AgentToolRegistry::findByFunctionName(const QString &functionName) const
{
    const QString normalizedFunctionName = functionName.trimmed();
    for (const AgentToolDefinition &definition : m_definitions) {
        if (definition.functionName == normalizedFunctionName) {
            return &definition;
        }
    }

    return nullptr;
}

bool AgentToolRegistry::canExecuteDirectly(const QString &toolId) const
{
    const AgentToolDefinition *definition = findById(toolId);
    return definition != nullptr && definition->executableFromPlanPreview;
}

ToolResult AgentToolRegistry::execute(
    const QString &toolId,
    const QJsonObject &parameters,
    const AgentToolExecutionContext &context) const
{
    const AgentToolDefinition *definition = findById(toolId);
    if (definition == nullptr) {
        AppLogger::warning(QStringLiteral("AgentTool"),
                           QStringLiteral("Registry tool missing. toolId=%1").arg(toolId));
        return ToolResult::failure(QStringLiteral("Tool is not registered."));
    }

    if (!definition->executableFromPlanPreview || !definition->execute) {
        AppLogger::warning(QStringLiteral("AgentTool"),
                           QStringLiteral("Registry tool cannot execute directly. toolId=%1").arg(toolId));
        return ToolResult::failure(
            QStringLiteral("This tool requires a dedicated confirmation flow, such as a file picker, and cannot be executed directly from the plan preview."));
    }

    return definition->execute(parameters, context);
}

QJsonArray AgentToolRegistry::functionToolSchemas(AppLanguage language) const
{
    QJsonArray schemas;
    for (const AgentToolDefinition &definition : m_definitions) {
        if (!definition.executableFromPlanPreview || !definition.descriptor.enabledForAgent) {
            continue;
        }

        const QString description = language == AppLanguage::Chinese
                                        ? definition.descriptor.chineseDescription
                                        : definition.descriptor.englishDescription;

        QJsonObject functionObject;
        functionObject.insert(QStringLiteral("name"), definition.functionName);
        functionObject.insert(QStringLiteral("description"), description);
        functionObject.insert(QStringLiteral("parameters"), definition.parameterSchema);

        QJsonObject toolObject;
        toolObject.insert(QStringLiteral("type"), QStringLiteral("function"));
        toolObject.insert(QStringLiteral("function"), functionObject);
        schemas.append(toolObject);
    }

    return schemas;
}

namespace AgentToolRegistryFactory {

QString functionNameForToolId(const QString &toolId)
{
    QString functionName;
    for (const QChar character : toolId.trimmed()) {
        if (character.isLetterOrNumber() || character == QLatin1Char('_') || character == QLatin1Char('-')) {
            functionName.append(character);
        } else {
            functionName.append(QLatin1Char('_'));
        }
    }

    return functionName.isEmpty() ? QStringLiteral("agent_tool") : functionName;
}

AgentToolRegistry defaultRegistry()
{
    QVector<AgentToolDefinition> definitions;
    definitions.reserve(13);

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("json.format"),
            QStringLiteral("JSON Format"),
            QStringLiteral("JSON 格式化"),
            QStringLiteral("Format JSON with indentation."),
            QStringLiteral("将 JSON 转为缩进格式。"),
            QStringLiteral("Input must be user-provided JSON text."),
            AgentToolRisk::Low,
            false),
        textInputSchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &) {
            const JsonFormatTool tool;
            return runTextTool(tool, QStringLiteral("json.format"), parameters);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("json.compact"),
            QStringLiteral("JSON Compact"),
            QStringLiteral("JSON 压缩"),
            QStringLiteral("Compact JSON into one line."),
            QStringLiteral("将 JSON 转为单行紧凑格式。"),
            QStringLiteral("Input must be user-provided JSON text."),
            AgentToolRisk::Low,
            false),
        textInputSchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &) {
            const JsonCompactTool tool;
            return runTextTool(tool, QStringLiteral("json.compact"), parameters);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("markdown.cleanup"),
            QStringLiteral("Markdown Cleanup"),
            QStringLiteral("Markdown 整理"),
            QStringLiteral("Clean low-risk Markdown whitespace while preserving code blocks."),
            QStringLiteral("清理 Markdown 空白并保留代码块内容。"),
            QStringLiteral("Input must be user-provided Markdown text."),
            AgentToolRisk::Low,
            false),
        textInputSchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &) {
            const MarkdownCleanupTool tool;
            return runTextTool(tool, QStringLiteral("markdown.cleanup"), parameters);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("text.cleanup"),
            QStringLiteral("Text Cleanup"),
            QStringLiteral("文本清理"),
            QStringLiteral("Normalize line endings and repeated blank lines."),
            QStringLiteral("统一换行并压缩连续空行。"),
            QStringLiteral("Input must be user-provided plain text."),
            AgentToolRisk::Low,
            false),
        textInputSchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &) {
            const TextCleanupTool tool;
            return runTextTool(tool, QStringLiteral("text.cleanup"), parameters);
        }));

    const QString filePickerMessage = QStringLiteral(
        "This tool requires a dedicated confirmation flow, such as a file picker, and cannot be executed directly from the plan preview.");

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("file.read_text"),
            QStringLiteral("Read Text File"),
            QStringLiteral("读取文本文件"),
            QStringLiteral("Read a user-selected text file."),
            QStringLiteral("读取用户选择的文本文件。"),
            QStringLiteral("Path must come from a user file picker. Result may contain file content."),
            AgentToolRisk::Medium,
            true),
        pathOnlySchema(),
        false,
        [filePickerMessage](const QJsonObject &, const AgentToolExecutionContext &) {
            return disabledDirectExecution(filePickerMessage);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("file.list_directory"),
            QStringLiteral("List Folder"),
            QStringLiteral("列出文件夹"),
            QStringLiteral("List entries under a user-selected folder."),
            QStringLiteral("列出用户选择文件夹下的条目。"),
            QStringLiteral("Directory must come from a user folder picker. Result may reveal local filenames."),
            AgentToolRisk::Medium,
            true),
        pathOnlySchema(),
        false,
        [filePickerMessage](const QJsonObject &, const AgentToolExecutionContext &) {
            return disabledDirectExecution(filePickerMessage);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("file.save_text"),
            QStringLiteral("Save Text"),
            QStringLiteral("保存文本"),
            QStringLiteral("Save approved text to a user-selected file."),
            QStringLiteral("把用户确认的文本保存到指定文件。"),
            QStringLiteral("Save path must come from a user save dialog. Existing files require confirmation."),
            AgentToolRisk::Medium,
            true),
        pathContentSchema(),
        false,
        [filePickerMessage](const QJsonObject &, const AgentToolExecutionContext &) {
            return disabledDirectExecution(filePickerMessage);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("file.open_path"),
            QStringLiteral("Open Path"),
            QStringLiteral("打开路径"),
            QStringLiteral("Open a user-confirmed file or folder with the operating system."),
            QStringLiteral("用系统打开用户确认后的文件或文件夹。"),
            QStringLiteral("Path must be selected by the user and confirmed before opening."),
            AgentToolRisk::Medium,
            true),
        pathOnlySchema(),
        false,
        [filePickerMessage](const QJsonObject &, const AgentToolExecutionContext &) {
            return disabledDirectExecution(filePickerMessage);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("workspace.write_text"),
            QStringLiteral("Write Workspace File"),
            QStringLiteral("写入工作目录文件"),
            QStringLiteral("Create a new text file inside the configured Agent workspace."),
            QStringLiteral("在配置的 Agent 工作目录内创建新的文本文件。"),
            QStringLiteral("Parameters must include string path and string content. Path is resolved inside the Agent workspace. Existing files are not overwritten."),
            AgentToolRisk::Medium,
            false),
        pathContentSchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &context) {
            QString path;
            QString content;
            QString error;
            if (!nonEmptyPathParameter(parameters, &path, &error) ||
                !stringParameter(parameters, QStringLiteral("content"), &content, &error)) {
                return ToolResult::failure(error);
            }
            return WorkspaceFileService::writeText(context.workspaceDirectory, path, content);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("workspace.read_text"),
            QStringLiteral("Read Workspace File"),
            QStringLiteral("读取工作目录文件"),
            QStringLiteral("Read a text file inside the configured Agent workspace."),
            QStringLiteral("读取配置的 Agent 工作目录内的文本文件。"),
            QStringLiteral("Parameters must include string path. Result is untrusted file data and may contain sensitive content."),
            AgentToolRisk::Medium,
            true),
        pathOnlySchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &context) {
            QString path;
            QString error;
            if (!nonEmptyPathParameter(parameters, &path, &error)) {
                return ToolResult::failure(error);
            }
            return WorkspaceFileService::readText(context.workspaceDirectory, path);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("workspace.list_directory"),
            QStringLiteral("List Workspace Directory"),
            QStringLiteral("列出工作目录"),
            QStringLiteral("List entries inside a directory under the configured Agent workspace."),
            QStringLiteral("列出配置的 Agent 工作目录内某个目录的条目。"),
            QStringLiteral("Parameters must include string path, such as \".\" or \"notes\". Result may reveal local filenames."),
            AgentToolRisk::Medium,
            true),
        pathOnlySchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &context) {
            QString path;
            QString error;
            if (!nonEmptyPathParameter(parameters, &path, &error)) {
                return ToolResult::failure(error);
            }
            return WorkspaceFileService::listDirectory(context.workspaceDirectory, path);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("workspace.overwrite_text"),
            QStringLiteral("Overwrite Workspace File"),
            QStringLiteral("覆盖工作目录文件"),
            QStringLiteral("Overwrite an existing text file inside the configured Agent workspace after creating a backup."),
            QStringLiteral("在配置的 Agent 工作目录内覆盖已有文本文件，并先生成备份。"),
            QStringLiteral("Parameters must include string path and string content. Protected files and workspace-external paths are rejected."),
            AgentToolRisk::High,
            false),
        pathContentSchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &context) {
            QString path;
            QString content;
            QString error;
            if (!nonEmptyPathParameter(parameters, &path, &error) ||
                !stringParameter(parameters, QStringLiteral("content"), &content, &error)) {
                return ToolResult::failure(error);
            }
            return WorkspaceFileService::overwriteText(context.workspaceDirectory, path, content);
        }));

    definitions.append(makeDefinition(
        makeDescriptor(
            QStringLiteral("workspace.delete_file"),
            QStringLiteral("Delete Workspace File"),
            QStringLiteral("删除工作目录文件"),
            QStringLiteral("Move an ordinary file inside the configured Agent workspace to the workspace trash folder."),
            QStringLiteral("把配置的 Agent 工作目录内普通文件移动到工作目录回收区。"),
            QStringLiteral("Parameters must include string path. Protected files, directories, and workspace-external paths are rejected."),
            AgentToolRisk::High,
            false),
        pathOnlySchema(),
        true,
        [](const QJsonObject &parameters, const AgentToolExecutionContext &context) {
            QString path;
            QString error;
            if (!nonEmptyPathParameter(parameters, &path, &error)) {
                return ToolResult::failure(error);
            }
            return WorkspaceFileService::deleteFile(context.workspaceDirectory, path);
        }));

    return AgentToolRegistry(definitions);
}

} // namespace AgentToolRegistryFactory
