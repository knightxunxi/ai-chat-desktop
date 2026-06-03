#pragma once

#include <QString>
#include <QVector>

// 学习注释：OpenAI-compatible Function Calling 返回的单个工具调用。
// 使用模块：StreamParser 聚合流式 tool_calls，ApplicationController 转换为 AgentPlanStep。
struct ToolCall {
    QString id;           // 功能：服务端返回的工具调用 ID；使用模块：日志和后续工具结果回传预留。
    QString functionName; // 功能：Function Calling 函数名；使用模块：AgentToolRegistry 映射回工具 ID。
    QString arguments;    // 功能：函数参数 JSON 字符串；使用模块：Agent 工具调用计划转换。
};

using ToolCallList = QVector<ToolCall>;
