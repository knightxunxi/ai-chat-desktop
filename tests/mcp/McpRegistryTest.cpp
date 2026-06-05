#include "mcp/McpRegistry.h"

#include "tools/ToolResult.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    // 1. add-server → addServer() then serverCount() == 1
    {
        McpRegistry registry;
        assert(registry.serverCount() == 0);

        registry.addServer(QStringLiteral("filesystem"),
                           QStringLiteral("npx"),
                           QStringList{QStringLiteral("-y"),
                                       QStringLiteral("@modelcontextprotocol/server-filesystem"),
                                       QStringLiteral(".")});
        assert(registry.serverCount() == 1);
    }

    // 2. disabled-server → enabled=false 时 connectAll 跳过
    {
        McpRegistry registry;
        registry.addServer(QStringLiteral("disabled_server"),
                           QStringLiteral("nonexistent_cmd_xyz"),
                           QStringList(),
                           false); // enabled = false

        registry.connectAll(); // 不应该尝试连接，不崩溃

        QVector<McpConnector *> connectors = registry.connectors();
        assert(connectors.isEmpty()); // disabled 的服务器没有连接器
    }

    // 3. load-config → loadConfig 正确解析 JSON
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());

        QString configPath = tempDir.filePath(QStringLiteral("mcp_servers.json"));

        // 写入配置文件
        {
            QFile file(configPath);
            assert(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            const QByteArray jsonContent = R"({
  "servers": [
    {
      "name": "filesystem",
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "."],
      "enabled": true
    },
    {
      "name": "search",
      "command": "python",
      "args": ["-m", "mcp_server_search"],
      "enabled": false
    }
  ]
})";
            file.write(jsonContent);
            file.close();
        }

        McpRegistry registry;
        assert(registry.loadConfig(configPath));
        assert(registry.serverCount() == 2);
    }

    // 4. save-config → saveConfig → loadConfig → 数据一致
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());

        QString configPath = tempDir.filePath(QStringLiteral("mcp_servers_save.json"));

        McpRegistry registryOut;
        registryOut.addServer(QStringLiteral("test_server"),
                              QStringLiteral("python"),
                              QStringList{QStringLiteral("-m"), QStringLiteral("mcp_module")},
                              true);

        assert(registryOut.saveConfig(configPath));

        // 重新加载并验证
        McpRegistry registryIn;
        assert(registryIn.loadConfig(configPath));
        assert(registryIn.serverCount() == 1);
    }

    // 5. allTools-empty → 未连接时 allTools() 为空
    {
        McpRegistry registry;
        QVector<McpToolDefinition> tools = registry.allTools();
        assert(tools.isEmpty());
    }

    // 6. callTool-unknown → callTool("unknown", {}) 返回失败
    {
        McpRegistry registry;
        ToolResult result = registry.callTool(QStringLiteral("unknown_tool"), QJsonObject());
        assert(!result.ok);
        assert(!result.error.isEmpty());
    }

    // 7. connectors-empty → 未连接时 connectors() 为空
    {
        McpRegistry registry;
        QVector<McpConnector *> connectors = registry.connectors();
        assert(connectors.isEmpty());
    }

    // 8. load-config-missing-file → 文件不存在时返回 false
    {
        McpRegistry registry;
        bool result = registry.loadConfig(QStringLiteral("/nonexistent/path/mcp_config.json"));
        assert(!result);
    }

    return 0;
}
