#pragma once

// 功能：插件系统核心接口 — 插件必须实现的基类。
// N4: 最小闭环 — 插件通过 PluginInterface 暴露工具定义列表，
//      由 PluginManager 加载后桥接到 AgentToolRegistry。
// 参考: Qt6 QPluginLoader

#include "tools/AgentToolCatalog.h"
#include "tools/ToolResult.h"

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

struct AgentToolExecutionContext;

// 插件工具定义（轻量版，避免依赖完整 AgentToolDefinition）
struct PluginToolInfo {
    QString id;           // e.g. "plugin.hello_world"
    QString name;         // e.g. "Hello World"
    QString description;  // e.g. "A sample plugin tool"
    AgentToolRisk risk = AgentToolRisk::Low;
    QJsonObject schema;   // JSON Schema for parameters
};

// 插件界面 — 每个插件 .dll 必须实现此接口并导出一个 PluginInterface 实例
class PluginInterface : public QObject {
    Q_OBJECT
public:
    explicit PluginInterface(QObject *parent = nullptr) : QObject(parent) {}

    // 元数据
    virtual QString pluginId() const = 0;
    virtual QString pluginName() const = 0;
    virtual QString pluginVersion() const = 0;

    // 工具列表
    virtual QVector<PluginToolInfo> tools() const = 0;

    // 工具执行
    virtual ToolResult execute(const QString &toolId,
                                const QJsonObject &parameters,
                                const AgentToolExecutionContext &context) = 0;
};

// Qt plugin 接口标识符
#define PluginInterface_iid "com.codexx.PluginInterface/1.0"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)
