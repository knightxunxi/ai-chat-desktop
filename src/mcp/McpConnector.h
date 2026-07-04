#pragma once

// 功能：JSON-RPC over QProcess / QSslSocket MCP 连接器。
// V15.4: 初始 QProcess 本地进程模式。
// V19 #23: 新增 NetworkTls 网络模式 (QSslSocket) + API Key 认证 + MCP 初始化握手。

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QSslSocket>
#include <QString>
#include <QStringList>
#include <QVector>

struct ToolResult;

// V15.4: MCP 工具定义结构体
struct McpToolDefinition {
    QString name;
    QString description;
    QJsonObject inputSchema;
};

// V15.4: MCP 资源结构体
struct McpResource {
    QString uri;
    QString name;
    QString description;
    QString mimeType;
};

// V19 #23: MCP 连接模式
enum class McpConnectionMode {
    LocalProcess, // 本地 QProcess 启动 MCP 服务器（现有方式）
    NetworkTls,   // 远程 QSslSocket TLS 连接
};

// V19 #23: MCP 连接配置（网络模式用）
struct McpNetworkConfig {
    QString host;        // e.g. "mcp.example.com"
    quint16 port = 8443; // e.g. 8443
    QString apiKey;      // Bearer token 或 API Key
    int timeoutMs = 30000;
};

// V15.4: JSON-RPC over QProcess / QSslSocket MCP 连接器
// V19 #23: 新增 NetworkTls 模式，支持 TLS + API Key
class McpConnector : public QObject {
    Q_OBJECT
public:
    explicit McpConnector(QObject *parent = nullptr);
    ~McpConnector() override;

    // 功能：以本地进程模式连接 MCP 服务器；使用模块：McpRegistry。
    bool connectToServer(const QString &command, const QStringList &args);
    // V19 #23: 以网络 TLS 模式连接远程 MCP 服务器
    bool connectToServer(const McpNetworkConfig &networkConfig);
    // 功能：断开连接并清理资源；使用模块：析构和重建。
    void disconnect();
    // 功能：判断是否已连接；使用模块：工具注册。
    bool isConnected() const;

    // 功能：获取 MCP 服务器工具列表；使用模块：McpRegistry::allTools。
    QVector<McpToolDefinition> listTools();
    // 功能：调用指定 MCP 工具并返回结果；使用模块：AgentToolDefinition execute。
    ToolResult callTool(const QString &name, const QJsonObject &args);
    // 功能：获取 MCP 服务器资源列表；使用模块：McpRegistry 资源发现。
    QVector<McpResource> listResources();

signals:
    void serverDisconnected();
    void serverError(const QString &error);

private:
    QJsonObject sendRequest(const QString &method, const QJsonObject &params);
    QJsonObject readResponse();
    void cleanup();

    // V19 #23: 底层写入（QProcess 或 QSslSocket）
    qint64 writeData(const QByteArray &data);
    bool waitForReadyRead(int timeoutMs);
    QByteArray readLineData();

    // V19 #23: JSON-RPC 握手（按协议要求发送 initialize 请求）
    bool performHandshake();

    QProcess *m_process = nullptr;
    QSslSocket *m_socket = nullptr;
    int m_nextRequestId = 1;
    bool m_connected = false;
    McpConnectionMode m_mode = McpConnectionMode::LocalProcess;
    // 网络模式专用缓冲区
    QByteArray m_socketBuffer;
    // 网络 API Key
    QString m_apiKey;

    static const QString kJsonRpcVersion;
};
