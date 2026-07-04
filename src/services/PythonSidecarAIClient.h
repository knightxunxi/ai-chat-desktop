#pragma once

// V19 Phase C: Python sidecar AI 客户端 — 实现 AIClient 接口。
// 通过 PythonSidecarClient 发送 model.chat 请求并处理响应。
// 配置切换：AppConfig.backendType == Sidecar 时使用。

#include "services/AIClient.h"
#include "services/PythonSidecarClient.h"
#include "services/PythonSidecarProtocol.h"

#include <QJsonArray>
#include <QProcessEnvironment>
#include <QObject>
#include <QString>

// V19: Python sidecar 实现的 AIClient 后端。
// 通过 QProcess 启动 Python sidecar，通过 JSONL 协议转发模型请求。
class PythonSidecarAIClient : public AIClient
{
    Q_OBJECT

public:
    explicit PythonSidecarAIClient(QObject *parent = nullptr);
    ~PythonSidecarAIClient() override;

    // AIClient 接口
    void sendChat(const AppConfig &config, const ChatSession &session) override;
    void sendChatWithTools(const AppConfig &config, const ChatSession &session, const QJsonArray &tools) override;
    void sendChatWithImages(const AppConfig &config, const ChatSession &session,
                            const QJsonArray &tools, const QJsonArray &images) override;
    void cancel() override;

    // 生命周期
    bool startSidecar(const QString &pythonExecutable, const QString &sidecarDir, int timeoutMs = 3000);
    bool isSidecarRunning() const;
    void stopSidecar();
    QString lastError() const;
    // 功能：暴露底层 sidecar 客户端给只读工具；使用模块：system.list_providers。
    PythonSidecarClient *sidecarClient();
    const PythonSidecarClient *sidecarClient() const;

private:
    QJsonArray buildMessageArray(const ChatSession &session) const;
    void doChatRequest(const AppConfig &config, const QJsonArray &messages,
                       const QJsonArray &tools, const QJsonArray &images);

    PythonSidecarClient m_client;
    bool m_cancelled = false;
};
