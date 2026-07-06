#pragma once

// 功能：示例插件 — 演示 PluginInterface 实现，供后续插件开发复制。
// N4-5: 一个只读示例工具，编译为独立的 .dll。

#include "plugins/PluginInterface.h"

#include <QObject>

class ExamplePlugin : public PluginInterface {
    Q_OBJECT
    Q_INTERFACES(PluginInterface)
    Q_PLUGIN_METADATA(IID PluginInterface_iid)

public:
    explicit ExamplePlugin(QObject *parent = nullptr);

    QString pluginId() const override;
    QString pluginName() const override;
    QString pluginVersion() const override;
    QVector<PluginToolInfo> tools() const override;
    ToolResult execute(const QString &toolId,
                        const QJsonObject &parameters,
                        const AgentToolExecutionContext &context) override;
};
