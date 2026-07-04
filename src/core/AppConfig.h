#pragma once

#include "core/AppLanguage.h"

#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <optional>

// V19: AI 后端类型
enum class AIBackendType {
    Direct,   // C++ OpenAICompatibleClient 直连
    Sidecar   // Python sidecar 能力层
};

// 学习注释：应用配置模型，集中描述一次 API 请求和界面语言所需的设置。
// 使用模块：SettingsDialog 负责编辑，ConfigStorage 负责保存，ApplicationController 负责读取并传给 AIClient。
struct AppConfig {
    QString providerName;               // 功能：服务商显示名；使用模块：设置窗口、主窗口模型信息展示。
    QString baseUrl;                    // 功能：OpenAI-compatible API 基础地址；使用模块：OpenAICompatibleClient 拼接请求地址。
    QString modelName;                  // 功能：当前调用的模型名；使用模块：请求体构造、主窗口标题区域展示。
    QString apiKey;                     // 功能：API 访问凭据；使用模块：ConfigStorage 通过 CredentialStorage 安全读写。
    std::optional<double> temperature;  // 功能：可选采样温度；使用模块：请求体构造时决定是否写入 JSON。
    std::optional<int> maxTokens;       // 功能：可选最大输出 token 数；使用模块：请求体构造时决定是否写入 JSON。
    AppLanguage language = AppLanguage::Chinese;
    QString agentWorkspaceDirectory;
    QString agentProjectDirectory;
    AIBackendType backendType = AIBackendType::Direct; // V19
    QString pythonExecutable;       // 功能：Python sidecar 启动命令；使用模块：ApplicationController::initAIClient。
    QString pythonSidecarDirectory; // 功能：Python sidecar 包目录；使用模块：ApplicationController::initAIClient。

    static QString defaultAgentWorkspaceDirectory()
    {
        QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (base.trimmed().isEmpty()) base = QDir::homePath();
        return QDir(base).filePath(QStringLiteral("AIChatDesktop/workspace"));
    }

    static QString defaultAgentProjectDirectory()
    {
        return QDir::currentPath();
    }

    static QString defaultPythonExecutable()
    {
        return QStringLiteral("python");
    }

    static QString defaultPythonSidecarDirectory()
    {
        return QDir(QDir::currentPath()).filePath(QStringLiteral("python/agent_sidecar"));
    }

    static AppConfig defaultConfig()
    {
        AppConfig config;
        config.providerName = QStringLiteral("DeepSeek");
        config.baseUrl = QStringLiteral("https://api.deepseek.com");
        config.modelName = QStringLiteral("deepseek-v4-flash");
        config.language = AppLanguage::Chinese;
        config.agentWorkspaceDirectory = defaultAgentWorkspaceDirectory();
        config.agentProjectDirectory = defaultAgentProjectDirectory();
        config.backendType = AIBackendType::Direct;
        config.pythonExecutable = defaultPythonExecutable();
        config.pythonSidecarDirectory = defaultPythonSidecarDirectory();
        return config;
    }

    bool isComplete() const
    {
        return !baseUrl.trimmed().isEmpty() &&
               !modelName.trimmed().isEmpty() &&
               !apiKey.trimmed().isEmpty();
    }
};
