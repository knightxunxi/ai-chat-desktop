#pragma once

// 功能：MCP 服务器注册表 — 管理多个 MCP 连接器（本地进程 + 网络 TLS）。
// V15.4: 初始版本，仅支持本地 QProcess。
// V19 #23: 新增 addNetworkServer() + config 序列化支持 network 类型条目。

#include "mcp/McpConnector.h"

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

// V15.4: MCP 服务器注册表，管理多个 MCP 连接器并路由工具调用
// V19 #23: 新增 NetworkTls 模式支持，可配置 host/port/apiKey
class McpRegistry : public QObject {
    Q_OBJECT
public:
    explicit McpRegistry(QObject *parent = nullptr);

    // 功能：添加基于进程的 MCP 服务器配置；使用模块：配置加载。
    void addServer(const QString &name, const QString &command,
                   const QStringList &args, bool enabled = true);

    // V19 #23: 添加基于网络 TLS 的远程 MCP 服务器配置
    void addNetworkServer(const QString &name, const McpNetworkConfig &networkConfig,
                          bool enabled = true);

    // 功能：连接所有已启用的服务器；使用模块：ApplicationController::initialize。
    void connectAll();
    // 功能：获取所有已连接 MCP 服务器的工具定义；使用模块：AgentToolRegistry。
    QVector<McpToolDefinition> allTools() const;
    // 功能：调用指定 MCP 工具（自动路由到正确连接器）；使用模块：AgentToolDefinition execute。
    ToolResult callTool(const QString &toolName, const QJsonObject &args);
    // 功能：从 JSON 文件加载配置；使用模块：ApplicationController::initialize。
    bool loadConfig(const QString &configPath);
    // 功能：保存配置到 JSON 文件；使用模块：设置界面。
    bool saveConfig(const QString &configPath);
    // 功能：获取所有连接器指针；使用模块：工具注册。
    QVector<McpConnector *> connectors() const;
    // 功能：获取服务器条目数量；使用模块：测试。
    int serverCount() const;

private:
    struct ServerEntry {
        QString name;
        QString command;
        QStringList args;
        McpNetworkConfig networkConfig;
        bool isNetwork = false;
        bool enabled = true;
        McpConnector *connector = nullptr;
    };
    QVector<ServerEntry> m_servers;
};
