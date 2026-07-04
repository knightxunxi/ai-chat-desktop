#include "mcp/McpConnector.h"

#include "tools/ToolResult.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSslConfiguration>
#include <QTimer>
#include <QCoreApplication>

const QString McpConnector::kJsonRpcVersion = QStringLiteral("2.0");

McpConnector::McpConnector(QObject *parent)
    : QObject(parent)
{
}

McpConnector::~McpConnector()
{
    cleanup();
}

// ── 本地进程模式 ──────────────────────────────────────────────────────────

bool McpConnector::connectToServer(const QString &command, const QStringList &args)
{
    if (command.isEmpty()) {
        return false;
    }

    cleanup();
    m_mode = McpConnectionMode::LocalProcess;
    m_process = new QProcess(this);

    QObject::connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (m_connected) {
            m_connected = false;
            emit serverDisconnected();
        }
    });

    QObject::connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     this, [this](int, QProcess::ExitStatus) {
        if (m_connected) {
            m_connected = false;
            emit serverDisconnected();
        }
    });

    m_process->start(command, args);
    if (!m_process->waitForStarted(10000)) {
        const QString errorMsg = m_process->errorString();
        delete m_process;
        m_process = nullptr;
        emit serverError(QStringLiteral("Failed to start MCP server: %1").arg(errorMsg));
        return false;
    }

    m_connected = true;
    return performHandshake();
}

// ── 网络 TLS 模式 ─────────────────────────────────────────────────────────

bool McpConnector::connectToServer(const McpNetworkConfig &networkConfig)
{
    if (networkConfig.host.isEmpty()) {
        return false;
    }

    cleanup();
    m_mode = McpConnectionMode::NetworkTls;
    m_apiKey = networkConfig.apiKey;
    m_socket = new QSslSocket(this);

    QObject::connect(m_socket, &QSslSocket::disconnected, this, [this]() {
        if (m_connected) {
            m_connected = false;
            emit serverDisconnected();
        }
    });
    QObject::connect(m_socket, &QSslSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit serverError(QStringLiteral("MCP TLS socket error: %1").arg(m_socket->errorString()));
    });

    // 连接到远程主机
    m_socket->connectToHost(networkConfig.host, networkConfig.port);
    if (!m_socket->waitForConnected(networkConfig.timeoutMs)) {
        const QString err = m_socket->errorString();
        delete m_socket;
        m_socket = nullptr;
        emit serverError(QStringLiteral("Failed to connect MCP host %1:%2: %3")
            .arg(networkConfig.host).arg(networkConfig.port).arg(err));
        return false;
    }

    // 启动 TLS 加密（如适用）—— connectToHostEncrypted 自动降级
    // QSslSocket::connectToHostEncrypted 在非 SSL 端口会失败，
    // 所以我们先连接再协商
    if (networkConfig.port == 443 || networkConfig.port == 8443) {
        m_socket->startClientEncryption();
        if (!m_socket->waitForEncrypted(networkConfig.timeoutMs)) {
            const QString err = m_socket->errorString();
            emit serverError(QStringLiteral("TLS handshake failed: %1").arg(err));
            cleanup();
            return false;
        }
    }

    m_connected = true;
    m_socketBuffer.clear();
    return performHandshake();
}

// ── JSON-RPC 握手 ─────────────────────────────────────────────────────────

bool McpConnector::performHandshake()
{
    // 按 MCP 协议：initialize 请求
    QJsonObject initParams;
    initParams[QStringLiteral("protocolVersion")] = QStringLiteral("2024-11-05");
    initParams[QStringLiteral("clientInfo")] = QJsonObject{
        {QStringLiteral("name"), QStringLiteral("CodeXX")},
        {QStringLiteral("version"), QStringLiteral("1.0")},
    };
    if (m_mode == McpConnectionMode::NetworkTls && !m_apiKey.trimmed().isEmpty()) {
        initParams[QStringLiteral("authorization")] = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("bearer")},
            {QStringLiteral("token"), m_apiKey.trimmed()},
        };
    }

    QJsonObject response = sendRequest(QStringLiteral("initialize"), initParams);
    if (response.isEmpty()) {
        // 某些老版本 MCP 不需要握手，忽略
        return true;
    }

    // 检查 initialize 响应中的 error
    if (response.contains(QStringLiteral("error"))) {
        QJsonObject errObj = response.value(QStringLiteral("error")).toObject();
        QString msg = errObj.value(QStringLiteral("message")).toString();
        emit serverError(QStringLiteral("MCP initialize failed: %1").arg(msg));
        disconnect();
        return false;
    }

    return true;
}

// ── 断开连接 ──────────────────────────────────────────────────────────────

void McpConnector::disconnect()
{
    m_connected = false;
    cleanup();
}

bool McpConnector::isConnected() const
{
    if (!m_connected) return false;
    if (m_mode == McpConnectionMode::LocalProcess) {
        return m_process != nullptr && m_process->state() == QProcess::Running;
    }
    return m_socket != nullptr && m_socket->state() == QAbstractSocket::ConnectedState;
}

// ── 工具调用 ──────────────────────────────────────────────────────────────

QVector<McpToolDefinition> McpConnector::listTools()
{
    if (!isConnected()) return {};

    QJsonObject response = sendRequest(QStringLiteral("tools/list"), QJsonObject());
    if (response.isEmpty()) return {};

    QJsonArray toolsArray;
    if (response.contains(QStringLiteral("result"))) {
        QJsonObject resultObj = response.value(QStringLiteral("result")).toObject();
        if (resultObj.contains(QStringLiteral("tools"))) {
            toolsArray = resultObj.value(QStringLiteral("tools")).toArray();
        }
    }

    QVector<McpToolDefinition> tools;
    for (const QJsonValue &val : toolsArray) {
        QJsonObject obj = val.toObject();
        if (obj.isEmpty()) continue;
        McpToolDefinition tool;
        tool.name = obj.value(QStringLiteral("name")).toString();
        tool.description = obj.value(QStringLiteral("description")).toString();
        tool.inputSchema = obj.value(QStringLiteral("inputSchema")).toObject();
        if (!tool.name.isEmpty()) tools.append(tool);
    }
    return tools;
}

ToolResult McpConnector::callTool(const QString &name, const QJsonObject &args)
{
    if (!isConnected()) {
        return ToolResult::failure(QStringLiteral("MCP server not connected"));
    }

    QJsonObject params;
    params[QStringLiteral("name")] = name;
    params[QStringLiteral("arguments")] = args;

    QJsonObject response = sendRequest(QStringLiteral("tools/call"), params);
    if (response.isEmpty()) {
        return ToolResult::failure(QStringLiteral("MCP tools/call: empty response"));
    }

    if (response.contains(QStringLiteral("error"))) {
        QJsonObject errorObj = response.value(QStringLiteral("error")).toObject();
        return ToolResult::failure(QStringLiteral("MCP error: %1").arg(
            errorObj.value(QStringLiteral("message")).toString()));
    }

    if (response.contains(QStringLiteral("result"))) {
        QJsonObject resultObj = response.value(QStringLiteral("result")).toObject();
        if (resultObj.contains(QStringLiteral("content"))) {
            QJsonArray contentArray = resultObj.value(QStringLiteral("content")).toArray();
            QStringList texts;
            for (const QJsonValue &val : contentArray) {
                QJsonObject contentItem = val.toObject();
                if (contentItem.contains(QStringLiteral("text"))) {
                    texts.append(contentItem.value(QStringLiteral("text")).toString());
                }
            }
            return ToolResult::success(texts.join(QStringLiteral("\n")));
        }
    }

    return ToolResult::success(QString());
}

QVector<McpResource> McpConnector::listResources()
{
    if (!isConnected()) return {};

    QJsonObject response = sendRequest(QStringLiteral("resources/list"), QJsonObject());
    if (response.isEmpty()) return {};

    QJsonArray resourcesArray;
    if (response.contains(QStringLiteral("result"))) {
        QJsonObject resultObj = response.value(QStringLiteral("result")).toObject();
        if (resultObj.contains(QStringLiteral("resources"))) {
            resourcesArray = resultObj.value(QStringLiteral("resources")).toArray();
        }
    }

    QVector<McpResource> resources;
    for (const QJsonValue &val : resourcesArray) {
        QJsonObject obj = val.toObject();
        if (obj.isEmpty()) continue;
        McpResource res;
        res.uri = obj.value(QStringLiteral("uri")).toString();
        res.name = obj.value(QStringLiteral("name")).toString();
        res.description = obj.value(QStringLiteral("description")).toString();
        res.mimeType = obj.value(QStringLiteral("mimeType")).toString();
        if (!res.uri.isEmpty()) resources.append(res);
    }
    return resources;
}

// ── JSON-RPC 发送 ─────────────────────────────────────────────────────────

QJsonObject McpConnector::sendRequest(const QString &method, const QJsonObject &params)
{
    const int requestId = m_nextRequestId++;

    QJsonObject request;
    request[QStringLiteral("jsonrpc")] = kJsonRpcVersion;
    request[QStringLiteral("id")] = requestId;
    request[QStringLiteral("method")] = method;
    request[QStringLiteral("params")] = params;

    QJsonDocument doc(request);
    QByteArray requestData = doc.toJson(QJsonDocument::Compact) + QByteArrayLiteral("\n");

    if (writeData(requestData) == -1) {
        m_connected = false;
        emit serverDisconnected();
        return {};
    }

    return readResponse();
}

// ── 底层读写 ───────────────────────────────────────────────────────────────

qint64 McpConnector::writeData(const QByteArray &data)
{
    if (m_mode == McpConnectionMode::LocalProcess && m_process) {
        return m_process->write(data);
    }
    if (m_mode == McpConnectionMode::NetworkTls && m_socket) {
        // 网络模式下，先发送 JSON-RPC 请求，在每行前可能带 API Key 认证头
        // 部分 MCP over SSE/WebSocket 协议需要在首条消息中携带认证
        // 标准 JSON-RPC MCP 直接发送请求行即可
        return m_socket->write(data);
    }
    return -1;
}

bool McpConnector::waitForReadyRead(int timeoutMs)
{
    if (m_mode == McpConnectionMode::LocalProcess && m_process) {
        return m_process->waitForReadyRead(timeoutMs);
    }
    if (m_mode == McpConnectionMode::NetworkTls && m_socket) {
        return m_socket->waitForReadyRead(timeoutMs);
    }
    return false;
}

QByteArray McpConnector::readLineData()
{
    if (m_mode == McpConnectionMode::LocalProcess && m_process) {
        return m_process->readLine();
    }
    if (m_mode == McpConnectionMode::NetworkTls && m_socket) {
        // 从 TLS socket 读取数据，按换行符分割
        m_socketBuffer.append(m_socket->readAll());
        int newlineIdx = m_socketBuffer.indexOf('\n');
        if (newlineIdx >= 0) {
            QByteArray line = m_socketBuffer.left(newlineIdx);
            m_socketBuffer.remove(0, newlineIdx + 1);
            return line;
        }
        // 没有完整行则等待更多数据
        return {};
    }
    return {};
}

// ── 响应解析 ──────────────────────────────────────────────────────────────

QJsonObject McpConnector::readResponse()
{
    if (m_mode == McpConnectionMode::LocalProcess && m_process) {
        if (!m_process->waitForReadyRead(30000)) {
            emit serverError(QStringLiteral("MCP response timeout"));
            return {};
        }
        QByteArray data = m_process->readLine();
        if (data.isEmpty()) return {};

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data.trimmed(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit serverError(QStringLiteral("MCP JSON parse error: %1").arg(parseError.errorString()));
            return {};
        }
        return doc.object();
    }

    if (m_mode == McpConnectionMode::NetworkTls && m_socket) {
        if (!m_socket->waitForReadyRead(30000)) {
            emit serverError(QStringLiteral("MCP network response timeout"));
            return {};
        }
        QByteArray data = readLineData();
        if (data.isEmpty()) return {};

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data.trimmed(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit serverError(QStringLiteral("MCP JSON parse error: %1").arg(parseError.errorString()));
            return {};
        }
        return doc.object();
    }

    return {};
}

// ── 清理 ──────────────────────────────────────────────────────────────────

void McpConnector::cleanup()
{
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (!m_process->waitForFinished(3000)) {
                m_process->kill();
                m_process->waitForFinished(2000);
            }
        }
        delete m_process;
        m_process = nullptr;
    }

    if (m_socket) {
        m_socket->disconnect();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->disconnectFromHost();
        }
        delete m_socket;
        m_socket = nullptr;
    }

    m_connected = false;
    m_nextRequestId = 1;
    m_socketBuffer.clear();
    m_apiKey.clear();
}
