#pragma once

#include "core/AppConfig.h"
#include "core/PromptTemplate.h"
#include "storage/ConfigStorage.h"
#include "storage/PromptTemplateStorage.h"

#include <QObject>
#include <QVector>

// 学习注释：配置协调器，集中管理应用配置和提示词模板的加载、保存和变更通知。
// 从 ApplicationController 分离，遵循单一职责原则。
class ConfigCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit ConfigCoordinator(QObject *parent = nullptr);

    // 功能：加载配置和提示词模板，发出变更信号；使用模块：ApplicationController::initialize。
    void initialize();

    // 功能：读取当前配置；使用模块：ApplicationController 及需要配置的各组件。
    const AppConfig &config() const;
    // 功能：读取提示词模板；使用模块：ApplicationController::promptTemplates。
    const QVector<PromptTemplate> &promptTemplates() const;
    // 功能：根据当前语言选择文案；使用模块：ApplicationController 各错误提示和状态消息。
    QString text(const QString &english, const QString &chinese) const;

public slots:
    // 功能：保存 API、模型和语言配置；使用模块：SettingsDialog 确认后由 ApplicationController 调用。
    void saveConfig(const AppConfig &config);
    // 功能：保存角色提示词模板；使用模块：RolePromptDialog 确认后由 ApplicationController 调用。
    void savePromptTemplates(const QVector<PromptTemplate> &templates);

signals:
    void configChanged();          // 功能：配置变化通知；使用模块：ApplicationController 转发到 MainWindow。
    void promptTemplatesChanged(); // 功能：模板变化通知；使用模块：ApplicationController 转发到 MainWindow。
    void startupWarning(const QString &english, const QString &chinese); // 功能：启动期警告；使用模块：ApplicationController 转发到 MainWindow。

private:
    AppConfig m_config;                         // 功能：当前应用配置；使用模块：请求、语言和设置保存。
    QVector<PromptTemplate> m_promptTemplates;  // 功能：角色提示词模板缓存；使用模块：RolePromptDialog。
    ConfigStorage m_configStorage;              // 功能：配置读写；使用模块：initialize/saveConfig。
    PromptTemplateStorage m_promptTemplateStorage; // 功能：模板读写；使用模块：initialize/savePromptTemplates。
};
