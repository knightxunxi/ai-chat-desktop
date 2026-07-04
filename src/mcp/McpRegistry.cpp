#include "mcp/McpRegistry.h"

#include "tools/ToolResult.h"

#include <QFile>
#include <QFileInfo>
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
    entry.isNetwork = false;
    m_servers.append(entry);
}

// V19 #23: 网络 TLS 模式
void McpRegistry::addNetworkServer(const QString &name, const McpNetworkConfig &networkConfig,
                                   bool enabled)
{
    ServerEntry entry;
    entry.name = name;
    entry.isNetwork = true;
    entry.networkConfig = networkConfig;
    entry.enabled = enabled;
    m_servers.append(entry);
}

void McpRegistry::connectAll()
{
    for (auto &entry : m_servers) {
        if (!entry.enabled) continue;

        if (entry.connector && entry.connector->isConnected()) continue;

        if (entry.connector) {
            entry.connector->disconnect();
            delete entry.connector;
        }

        entry.connector = new McpConnector(this);
        bool ok = false;

        if (entry.isNetwork) {
            ok = entry.connector->connectToServer(entry.networkConfig);
        } else {
            ok = entry.connector->connectToServer(entry.command, entry.args);
        }

        if (!ok) {
            delete entry.connector;
            entry.connector = nullptr;
        }
    }
}

QVector<McpToolDefinition> McpRegistry::allTools() const
{
    QVector<McpToolDefinition> tools;
    for (const auto &entry : m_servers) {
        if (!entry.connector || !entry.connector->isConnected()) continue;
        tools += entry.connector->listTools();
    }
    return tools;
}

ToolResult McpRegistry::callTool(const QString &toolName, const QJsonObject &args)
{
    for (const auto &entry : m_servers) {
        if (!entry.connector || !entry.connector->isConnected()) continue;

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
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) return false;

    QJsonObject root = doc.object();
    if (!root.contains(QStringLiteral("servers"))) return false;

    QJsonArray serversArray = root.value(QStringLiteral("servers")).toArray();
    m_servers.clear();

    for (const QJsonValue &val : serversArray) {
        QJsonObject serverObj = val.toObject();
        const QString type = serverObj.value(QStringLiteral("type")).toString();

        if (type == QStringLiteral("network")) {
            // 网络 TLS 模式
            ServerEntry entry;
            entry.name = serverObj.value(QStringLiteral("name")).toString();
            entry.isNetwork = true;
            entry.enabled = serverObj.value(QStringLiteral("enabled")).toBool(true);
            entry.networkConfig.host = serverObj.value(QStringLiteral("host")).toString();
            entry.networkConfig.port = static_cast<quint16>(
                serverObj.value(QStringLiteral("port")).toInt(8443));
            entry.networkConfig.apiKey = serverObj.value(QStringLiteral("apiKey")).toString();
            entry.networkConfig.timeoutMs = serverObj.value(QStringLiteral("timeoutMs")).toInt(30000);
            if (!entry.name.isEmpty() && !entry.networkConfig.host.isEmpty()) {
                m_servers.append(entry);
            }
        } else {
            // 本地进程模式（默认）
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
        serverObj[QStringLiteral("enabled")] = entry.enabled;

        if (entry.isNetwork) {
            serverObj[QStringLiteral("type")] = QStringLiteral("network");
            serverObj[QStringLiteral("host")] = entry.networkConfig.host;
            serverObj[QStringLiteral("port")] = entry.networkConfig.port;
            serverObj[QStringLiteral("apiKey")] = entry.networkConfig.apiKey;
            serverObj[QStringLiteral("timeoutMs")] = entry.networkConfig.timeoutMs;
        } else {
            serverObj[QStringLiteral("type")] = QStringLiteral("local");
            serverObj[QStringLiteral("command")] = entry.command;
            QJsonArray argsArray;
            for (const QString &arg : entry.args) {
                argsArray.append(arg);
            }
            serverObj[QStringLiteral("args")] = argsArray;
        }

        serversArray.append(serverObj);
    }

    root[QStringLiteral("servers")] = serversArray;

    QJsonDocument doc(root);
    QDir parentDir = QFileInfo(configPath).absoluteDir();
    if (!parentDir.exists()) {
        parentDir.mkpath(QStringLiteral("."));
    }

    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

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
