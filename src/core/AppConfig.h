#pragma once

#include "core/AppLanguage.h"

#include <QDir>
#include <QStandardPaths>
#include <optional>
#include <QString>

// 学习注释：应用配置模型，集中描述一次 API 请求和界面语言所需的设置。
// 使用模块：SettingsDialog 负责编辑，ConfigStorage 负责保存，ApplicationController 负责读取并传给 AIClient。
struct AppConfig {
    QString providerName;               // 功能：服务商显示名；使用模块：设置窗口、主窗口模型信息展示。
    QString baseUrl;                    // 功能：OpenAI-compatible API 基础地址；使用模块：OpenAICompatibleClient 拼接请求地址。
    QString modelName;                  // 功能：当前调用的模型名；使用模块：请求体构造、主窗口标题区域展示。
    QString apiKey;                     // 功能：API 访问凭据；使用模块：ConfigStorage 通过 CredentialStorage 安全读写。
    std::optional<double> temperature;  // 功能：可选采样温度；使用模块：请求体构造时决定是否写入 JSON。
    std::optional<int> maxTokens;       // 功能：可选最大输出 token 数；使用模块：请求体构造时决定是否写入 JSON。
    AppLanguage language = AppLanguage::Chinese; // 功能：界面语言；使用模块：各 UI 组件选择中文或英文文案。
    QString agentWorkspaceDirectory;    // 功能：Agent 默认工作目录；使用模块：后续自动文件生成和工作目录策略。
    QString agentProjectDirectory;      // 功能：Agent 命令项目目录；使用模块：V9 command.* 命令执行。

    // 功能：返回默认 Agent 工作目录；使用模块：默认配置和工作目录策略。
    static QString defaultAgentWorkspaceDirectory()
    {
        QString baseDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (baseDirectory.trimmed().isEmpty()) {
            baseDirectory = QDir::homePath();
        }

        return QDir(baseDirectory).filePath(QStringLiteral("AIChatDesktop/workspace"));
    }

    // 功能：返回默认 Agent 项目目录；使用模块：默认配置和 V9 命令执行。
    static QString defaultAgentProjectDirectory()
    {
        return QDir::currentPath();
    }

    // 功能：生成默认配置；使用模块：ConfigStorage 在本地无配置时提供初始值。
    static AppConfig defaultConfig()
    {
        AppConfig config;
        config.providerName = QStringLiteral("DeepSeek");
        config.baseUrl = QStringLiteral("https://api.deepseek.com");
        config.modelName = QStringLiteral("deepseek-v4-flash");
        config.language = AppLanguage::Chinese;
        config.agentWorkspaceDirectory = defaultAgentWorkspaceDirectory();
        config.agentProjectDirectory = defaultAgentProjectDirectory();
        return config;
    }

    // 功能：判断发送请求前的必要配置是否齐全；使用模块：ApplicationController::sendMessage。
    bool isComplete() const
    {
        return !baseUrl.trimmed().isEmpty() &&
               !modelName.trimmed().isEmpty() &&
               !apiKey.trimmed().isEmpty();
    }
};
