#include "plugins/examples/ExamplePlugin.h"

#include <QDateTime>

ExamplePlugin::ExamplePlugin(QObject *parent)
    : PluginInterface(parent)
{
}

QString ExamplePlugin::pluginId() const
{
    return QStringLiteral("example_plugin");
}

QString ExamplePlugin::pluginName() const
{
    return QStringLiteral("Example Plugin");
}

QString ExamplePlugin::pluginVersion() const
{
    return QStringLiteral("1.0.0");
}

QVector<PluginToolInfo> ExamplePlugin::tools() const
{
    PluginToolInfo tool;
    tool.id = QStringLiteral("plugin.hello_world");
    tool.name = QStringLiteral("Hello World");
    tool.description = QStringLiteral("Returns a greeting message with current time.");
    tool.risk = AgentToolRisk::Low;
    // 无参数 schema（空对象）
    tool.schema = QJsonObject();
    return {tool};
}

ToolResult ExamplePlugin::execute(const QString &toolId,
                                    const QJsonObject &parameters,
                                    const AgentToolExecutionContext &context)
{
    Q_UNUSED(parameters);
    Q_UNUSED(context);

    if (toolId == QStringLiteral("plugin.hello_world")) {
        const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        return ToolResult::success(
            QStringLiteral("Hello from ExamplePlugin! Current UTC time: %1").arg(now));
    }

    return ToolResult::failure(QStringLiteral("Unknown tool: %1").arg(toolId));
}
