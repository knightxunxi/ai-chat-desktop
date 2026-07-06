#pragma once

#include "tools/registry/AgentToolCatalog.h"
#include "tools/registry/ToolResult.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include <functional>

class HookManager;
struct McpToolDefinition;
class McpConnector;
class PythonSidecarClient;

struct PluginToolInfo;
class PluginInterface;

struct AgentToolExecutionContext {
    QString workspaceDirectory; // 功能：Agent 工作目录；使用模块：workspace.* 工具执行。
    QString projectDirectory;   // 功能：项目命令工作目录；使用模块：command.* 工具执行。
    PythonSidecarClient *sidecarClient = nullptr; // V19 #16: Python sidecar 客户端指针，用于 listProviders 等调用。
};

struct AgentToolDefinition {
    AgentToolDescriptor descriptor; // 功能：工具目录描述；使用模块：Prompt、计划解析和 UI。
    QJsonObject parameterSchema;    // 功能：Function Calling 参数 schema；使用模块：V8.3 工具调用兼容层。
    QString functionName;           // 功能：无点号函数名；使用模块：Function Calling 请求体。
    bool executableFromPlanPreview = false; // 功能：是否允许计划窗口直接执行；使用模块：AgentPlanExecutor。
    std::function<ToolResult(const QJsonObject &, const AgentToolExecutionContext &)> execute; // 功能：统一执行入口；使用模块：ToolRegistry。
};

class AgentToolRegistry
{
public:
    explicit AgentToolRegistry(const QVector<AgentToolDefinition> &definitions = {});

    // 功能：返回所有工具定义；使用模块：测试和 Function Calling schema 生成。
    const QVector<AgentToolDefinition> &definitions() const;
    // 功能：返回所有工具描述；使用模块：AgentToolCatalog。
    QVector<AgentToolDescriptor> descriptors() const;
    // 功能：按工具 ID 查找定义；使用模块：计划执行器。
    const AgentToolDefinition *findById(const QString &toolId) const;
    // 功能：按 Function Calling 函数名查找定义；使用模块：函数调用结果映射。
    const AgentToolDefinition *findByFunctionName(const QString &functionName) const;
    // 功能：判断工具是否能从计划预览直接执行；使用模块：AgentPlanDialog 按钮状态。
    bool canExecuteDirectly(const QString &toolId) const;
    // 功能：执行指定工具；使用模块：AgentPlanExecutor 和 AgentLoopController。
    // V13.3: 新增 HookManager* 参数（默认 nullptr），用于 on_tool_execute Hook。
    ToolResult execute(const QString &toolId, const QJsonObject &parameters, const AgentToolExecutionContext &context,
                       HookManager *hooks = nullptr) const;
    // 功能：生成 OpenAI-compatible tools 数组；使用模块：V8.3 Function Calling 兼容层。
    QJsonArray functionToolSchemas(AppLanguage language) const;

    // V15.4: 注册 MCP 外部工具；使用模块：ApplicationController 初始化时注册 MCP 工具。
    void registerExternalTools(const QVector<McpToolDefinition> &mcpTools,
                               McpConnector *connector);

    // N4: 注册插件工具；使用模块：AgentOrchestrator 初始化时注册插件工具。
    void registerPluginTools(const QVector<PluginToolInfo> &pluginTools,
                             PluginInterface *pluginInstance);

private:
    QVector<AgentToolDefinition> m_definitions;
};

namespace AgentToolRegistryFactory {

// 功能：把工具 ID 转成函数名；使用模块：Function Calling schema。
QString functionNameForToolId(const QString &toolId);

// 功能：返回默认工具注册表；使用模块：工具目录、计划执行和测试。
AgentToolRegistry defaultRegistry();

} // namespace AgentToolRegistryFactory
