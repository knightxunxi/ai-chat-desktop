#include "mcp/McpRegistry.h"

#include "tools/ToolResult.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>

McpRegistry::McpRegistry(QObject *parent)
    : QObject(parent)
{
}

void McpRegistry::addServer(const QString &name, const QString &command,
                            const QStringList &args, bool enabled)
{
    ServerEntry entry;
    entry.name = name;
    entry.command = command;
    entry.args = args;
    entry.enabled = enabled;
    m_servers.append(entry);
}

void McpRegistry::connectAll()
{
    for (auto &entry : m_servers) {
        if (!entry.enabled) {
            continue;
        }

        // 如果已有连接器且已连接，跳过
        if (entry.connector && entry.connector->isConnected()) {
            continue;
        }

        // 清理旧连接器
        if (entry.connector) {
            entry.connector->disconnect();
            delete entry.connector;
        }

        entry.connector = new McpConnector(this);
        if (!entry.connector->connectToServer(entry.command, entry.args)) {
            delete entry.connector;
            entry.connector = nullptr;
        }
    }
}

QVector<McpToolDefinition> McpRegistry::allTools() const
{
    QVector<McpToolDefinition> tools;
    for (const auto &entry : m_servers) {
        if (!entry.connector || !entry.connector->isConnected()) {
            continue;
        }
        tools += entry.connector->listTools();
    }
    return tools;
}

ToolResult McpRegistry::callTool(const QString &toolName, const QJsonObject &args)
{
    for (const auto &entry : m_servers) {
        if (!entry.connector || !entry.connector->isConnected()) {
            continue;
        }

        // 先拉取工具列表，查找匹配的工具
        const QVector<McpToolDefinition> tools = entry.connector->listTools();
        for (const McpToolDefinition &tool : tools) {
            if (tool.name == toolName) {
                return entry.connector->callTool(toolName, args);
            }
        }
    }

    return ToolResult::failure(QStringLiteral("MCP tool not found: %1").arg(toolName));
}

bool McpRegistry::loadConfig(const QString &configPath)
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains(QStringLiteral("servers"))) {
        return false;
    }

    QJsonArray serversArray = root.value(QStringLiteral("servers")).toArray();
    m_servers.clear();

    for (const QJsonValue &val : serversArray) {
        QJsonObject serverObj = val.toObject();
        ServerEntry entry;
        entry.name = serverObj.value(QStringLiteral("name")).toString();
        entry.command = serverObj.value(QStringLiteral("command")).toString();
        entry.enabled = serverObj.value(QStringLiteral("enabled")).toBool(true);

        QJsonArray argsArray = serverObj.value(QStringLiteral("args")).toArray();
        for (const QJsonValue &argVal : argsArray) {
            entry.args.append(argVal.toString());
        }

        if (!entry.name.isEmpty() && !entry.command.isEmpty()) {
            m_servers.append(entry);
        }
    }

    return true;
}

bool McpRegistry::saveConfig(const QString &configPath)
{
    QJsonObject root;
    QJsonArray serversArray;

    for (const auto &entry : m_servers) {
        QJsonObject serverObj;
        serverObj[QStringLiteral("name")] = entry.name;
        serverObj[QStringLiteral("command")] = entry.command;
        serverObj[QStringLiteral("enabled")] = entry.enabled;

        QJsonArray argsArray;
        for (const QString &arg : entry.args) {
            argsArray.append(arg);
        }
        serverObj[QStringLiteral("args")] = argsArray;
        serversArray.append(serverObj);
    }

    root[QStringLiteral("servers")] = serversArray;

    QJsonDocument doc(root);

    // 确保父目录存在
    QDir parentDir = QFileInfo(configPath).absoluteDir();
    if (!parentDir.exists()) {
        parentDir.mkpath(QStringLiteral("."));
    }

    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QVector<McpConnector *> McpRegistry::connectors() const
{
    QVector<McpConnector *> result;
    for (const auto &entry : m_servers) {
        if (entry.connector) {
            result.append(entry.connector);
        }
    }
    return result;
}

int McpRegistry::serverCount() const
{
    return m_servers.size();
}
