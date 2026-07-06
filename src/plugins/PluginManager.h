#pragma once

// 功能：插件管理器 — 扫描插件目录、加载/卸载 DLL、桥接到 AgentToolRegistry。
// N4: 最小闭环 — 支持 plugin.json manifest + QPluginLoader 加载 + 工具注册。

#include "plugins/PluginInterface.h"
#include "tools/AgentToolCatalog.h"
#include "tools/ToolResult.h"

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

struct AgentToolExecutionContext;

struct PluginEntry {
    QString pluginId;
    QString name;
    QString version;
    bool enabled = true;
    bool loaded = false;
    QString loadError;
    PluginInterface *instance = nullptr;
    QVector<PluginToolInfo> tools;
    // manifest 文件路径
    QString manifestPath;
    // DLL 路径
    QString libraryPath;
};

class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager() override;

    // 功能：扫描 plugins/ 目录并加载所有插件；使用模块：ApplicationController 初始化。
    void scanAndLoadPlugins(const QString &pluginsDir);

    // 功能：获取所有已加载的插件条目；使用模块：ToolsDialog。
    QVector<PluginEntry> allPlugins() const;

    // 功能：获取所有插件工具定义（已启用+已加载）；使用模块：AgentToolRegistry。
    QVector<PluginToolInfo> allPluginTools() const;

    // 功能：执行插件工具；使用模块：AgentToolRegistry 回调。
    ToolResult executePluginTool(const QString &toolId,
                                  const QJsonObject &parameters,
                                  const AgentToolExecutionContext &context);

    // 功能：禁用/启用插件；使用模块：ToolsDialog UI。
    void setPluginEnabled(const QString &pluginId, bool enabled);

signals:
    void pluginLoaded(const QString &pluginId, const QString &name);
    void pluginLoadFailed(const QString &pluginId, const QString &error);
    void pluginUnloaded(const QString &pluginId);

private:
    // 功能：解析单个 plugin.json manifest；使用模块：scanAndLoadPlugins。
    bool loadManifest(const QString &manifestPath, PluginEntry &entry);
    // 功能：加载 plugin DLL；使用模块：scanAndLoadPlugins。
    bool loadLibrary(PluginEntry &entry);
    // 功能：卸载 plugin；使用模块：析构和 setPluginEnabled(false)。
    void unloadPlugin(PluginEntry &entry);

    QVector<PluginEntry> m_plugins;
};
