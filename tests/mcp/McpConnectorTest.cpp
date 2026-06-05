#include "mcp/McpConnector.h"

#include "tools/ToolResult.h"

#include <QCoreApplication>
#include <QTemporaryDir>

#include <cassert>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    // 1. connect-empty-command → connectToServer("") 返回 false
    {
        McpConnector connector;
        bool result = connector.connectToServer(QString(), QStringList());
        assert(!result);
        assert(!connector.isConnected());
    }

    // 2. connect-nonexistent → connectToServer("nonexistent_cmd", {}) 返回 false
    {
        McpConnector connector;
        bool result = connector.connectToServer(QStringLiteral("nonexistent_cmd_xyz_123"), QStringList());
        assert(!result);
        assert(!connector.isConnected());
    }

    // 3. isConnected-before → 未连接时 isConnected() 返回 false
    {
        McpConnector connector;
        assert(!connector.isConnected());
    }

    // 4. listTools-disconnected → 未连接时 listTools() 返回空
    {
        McpConnector connector;
        QVector<McpToolDefinition> tools = connector.listTools();
        assert(tools.isEmpty());
    }

    // 5. disconnect-safe → disconnect() 不连接时安全返回
    {
        McpConnector connector;
        connector.disconnect(); // 不应该崩溃
        assert(!connector.isConnected());
    }

    // 6. cleanup-on-destroy → 析构时正确清理进程
    {
        McpConnector connector;
        // 尝试连接不存在的命令（会失败），然后析构
        connector.connectToServer(QStringLiteral("nonexistent_cmd_xyz_456"), QStringList());
        // 析构时不应崩溃
    }

    // 7. listResources-disconnected → 未连接时 listResources() 返回空
    {
        McpConnector connector;
        QVector<McpResource> resources = connector.listResources();
        assert(resources.isEmpty());
    }

    // 8. callTool-disconnected → 未连接时 callTool() 返回失败
    {
        McpConnector connector;
        ToolResult result = connector.callTool(QStringLiteral("test_tool"), QJsonObject());
        assert(!result.ok);
        assert(!result.error.isEmpty());
    }

    return 0;
}
