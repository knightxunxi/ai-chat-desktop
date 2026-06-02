#pragma once

#include "app/AgentPlan.h"
#include "tools/AgentToolCatalog.h"

#include <QString>
#include <QVector>

struct AgentLoopAction {
    bool done = false;       // 功能：模型是否声明任务完成；使用模块：Agentic Loop 终止判断。
    QString message;         // 功能：模型对下一步或完成状态的说明；使用模块：审计和 UI 提示。
    AgentPlanStep step;      // 功能：单个下一步动作；使用模块：AgentLoopController 后续 AI 接入。
};

struct AgentLoopActionParseResult {
    bool ok = false;         // 功能：解析是否成功；使用模块：Agentic Loop 请求结果处理。
    AgentLoopAction action;  // 功能：解析后的下一步动作；使用模块：循环执行。
    QString error;           // 功能：解析失败原因；使用模块：用户提示和测试。

    static AgentLoopActionParseResult success(const AgentLoopAction &action);
    static AgentLoopActionParseResult failure(const QString &error);
};

namespace AgentLoopActionParser {

// 功能：解析单轮 Agentic Loop action JSON；使用模块：V8.2 后续真实 AI 单步规划。
AgentLoopActionParseResult parseJsonAction(
    const QString &jsonText,
    const QVector<AgentToolDescriptor> &toolCatalog);

} // namespace AgentLoopActionParser
