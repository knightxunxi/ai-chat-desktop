#pragma once

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>

struct ToolResult;

// V15.4: MCP 工具定义结构体
struct McpToolDefinition {
    QString name;
    QString description;
    QJsonObject inputSchema; // JSON Schema for parameters
};

// V15.4: MCP 资源结构体
struct McpResource {
    QString uri;
    QString name;
    QString description;
    QString mimeType;
};

// V15.4: JSON-RPC over QProcess MCP 连接器
// 使用模块：McpRegistry 通过外部进程与 MCP 服务器通信。
class McpConnector : public QObject {
    Q_OBJECT
public:
    explicit McpConnector(QObject *parent = nullptr);
    ~McpConnector() override;

    // 功能：启动 MCP 服务器进程并建立 JSON-RPC 通信；使用模块：McpRegistry::connectAll。
    bool connectToServer(const QString &command, const QStringList &args);
    // 功能：断开 MCP 服务器连接并清理进程；使用模块：McpRegistry 析构和解注册。
    void disconnect();
    // 功能：判断是否已连接；使用模块：McpRegistry::connectAll 跳过已连接的。
    bool isConnected() const;

    // 功能：获取 MCP 服务器工具列表；使用模块：McpRegistry::allTools。
    QVector<McpToolDefinition> listTools();
    // 功能：调用指定 MCP 工具并返回结果；使用模块：AgentToolDefinition::execute 回调。
    ToolResult callTool(const QString &name, const QJsonObject &args);
    // 功能：获取 MCP 服务器资源列表；使用模块：McpRegistry 资源发现。
    QVector<McpResource> listResources();

signals:
    // 功能：通知服务器进程异常退出；使用模块：McpRegistry 重连逻辑。
    void serverDisconnected();
    // 功能：通知服务器错误；使用模块：日志和用户提示。
    void serverError(const QString &error);

private:
    // 功能：发送 JSON-RPC 请求并等待响应；使用模块：listTools/callTool/listResources。
    QJsonObject sendRequest(const QString &method, const QJsonObject &params);
    // 功能：从 stdout 读取一行 JSON 响应；使用模块：sendRequest。
    QJsonObject readResponse();
    // 功能：清理进程和资源；使用模块：disconnect 和析构。
    void cleanup();

    QProcess *m_process = nullptr;
    int m_nextRequestId = 1;
    bool m_connected = false;
    // 功能：JSON-RPC 2.0 版本常量（仅使用 id/method/params/result/error 字段）
    static const QString kJsonRpcVersion;
};
