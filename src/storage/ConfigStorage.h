#pragma once

#include "core/AppConfig.h"

#include <memory>
#include <QString>

class CredentialStorage;

// 学习注释：应用配置持久化入口，普通配置写 QSettings，API Key 委托给 CredentialStorage。
// 使用模块：ApplicationController 初始化和保存设置时调用，SettingsDialog 只负责编辑数据。
class ConfigStorage
{
public:
    // 功能：使用默认 WindowsCredentialStorage；使用模块：ApplicationController 正常运行时使用。
    ConfigStorage();
    // 功能：支持注入凭据存储和 QSettings 名称；使用模块：ConfigStorageTest 做隔离测试。
    ConfigStorage(std::shared_ptr<CredentialStorage> credentialStorage,
                  const QString &organizationName,
                  const QString &applicationName);

    // 功能：读取服务商、Base URL、模型、语言等配置；使用模块：ApplicationController::initialize。
    AppConfig load() const;
    // 功能：保存配置并安全写入 API Key；使用模块：ApplicationController::saveConfig。
    bool save(const AppConfig &config, QString *error = nullptr) const;

private:
    std::shared_ptr<CredentialStorage> m_credentialStorage; // 功能：API Key 安全存储策略；使用模块：save/load。
    QString m_organizationName; // 功能：QSettings 组织名；使用模块：定位本地配置命名空间。
    QString m_applicationName;  // 功能：QSettings 应用名；使用模块：定位本地配置命名空间。
};
