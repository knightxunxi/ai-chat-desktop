#pragma once

#include "tools/AgentToolCatalog.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

// 学习注释：Agent 计划步骤状态，用于后续计划预览和单步执行流程。
// 使用模块：V7 Agent 状态流转和测试。
enum class AgentPlanStepStatus {
    Pending,
    Confirmed,
    Skipped,
    Running,
    Completed,
    Failed,
    Canceled
};

// 学习注释：AI 生成计划中的单个步骤，经过本地解析和工具目录校验后才可信。
// 使用模块：计划预览 UI、工具执行器和测试。
struct AgentPlanStep {
    QString id;                         // 功能：步骤稳定 ID；使用模块：日志和 UI 定位。
    QString title;                      // 功能：步骤标题；使用模块：计划预览 UI。
    QString toolId;                     // 功能：建议工具 ID；使用模块：工具目录查找。
    QString reason;                     // 功能：AI 给出的步骤原因；使用模块：用户确认前展示。
    AgentToolRisk risk = AgentToolRisk::Low; // 功能：本地确认后的有效风险等级；使用模块：确认提示。
    QJsonObject parameters;             // 功能：工具参数对象；使用模块：计划预览和后续执行。
    QString output;                      // 功能：步骤执行输出；使用模块：计划预览、复制、插入和继续规划。
    AgentPlanStepStatus status = AgentPlanStepStatus::Pending; // 功能：执行状态；使用模块：Agent 流程控制。
};

// 学习注释：AI 生成的结构化任务计划，必须由 AgentPlanParser 解析生成。
// 使用模块：V7 计划预览和后续工具执行。
struct AgentPlan {
    QString summary;              // 功能：计划摘要；使用模块：计划预览顶部说明。
    QVector<AgentPlanStep> steps; // 功能：计划步骤列表；使用模块：单步确认和执行。
    int continuationDepth = 0;    // 功能：继续规划轮次；使用模块：防止 Agent 无限循环。
};

constexpr int AgentPlanMaxContinuationDepth = 2; // 功能：最大继续规划轮次；使用模块：AgentPlanDialog 和 ApplicationController。

// 学习注释：计划解析结果，避免调用方通过异常或空对象判断解析状态。
// 使用模块：AI 计划生成入口、计划 UI 和测试。
struct AgentPlanParseResult {
    bool ok = false;     // 功能：标记计划是否解析成功；使用模块：调用方分支判断。
    AgentPlan plan;      // 功能：解析成功后的计划；使用模块：计划预览。
    QString error;       // 功能：解析失败原因；使用模块：用户提示和测试。

    // 功能：创建成功解析结果；使用模块：AgentPlanParser。
    static AgentPlanParseResult success(const AgentPlan &plan);

    // 功能：创建失败解析结果；使用模块：AgentPlanParser。
    static AgentPlanParseResult failure(const QString &error);
};

// 功能：返回步骤状态字符串；使用模块：日志和测试。
QString agentPlanStepStatusToString(AgentPlanStepStatus status);
