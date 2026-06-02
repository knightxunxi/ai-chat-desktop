#pragma once

#include "core/AppLanguage.h"

#include <QString>
#include <QVector>

// 学习注释：Agent 工具风险等级，用于限制 AI 建议工具时的安全边界。
// 使用模块：AgentToolCatalog 和 AgentPlanParser。
enum class AgentToolRisk {
    Low,
    Medium,
    High
};

// 学习注释：Agent 可见工具的机器可读描述，避免 AI 直接调用未登记能力。
// 使用模块：V7 计划生成 prompt、计划解析器和后续计划预览 UI。
struct AgentToolDescriptor {
    QString id;                         // 功能：稳定工具 ID；使用模块：AI 计划和本地校验。
    QString englishName;                // 功能：英文显示名；使用模块：英文 UI 和 prompt。
    QString chineseName;                // 功能：中文显示名；使用模块：中文 UI 和 prompt。
    QString englishDescription;         // 功能：英文用途说明；使用模块：工具目录展示。
    QString chineseDescription;         // 功能：中文用途说明；使用模块：工具目录展示。
    QString inputPolicy;                // 功能：输入限制说明；使用模块：计划预览和安全提示。
    AgentToolRisk risk = AgentToolRisk::Low; // 功能：工具目录权威风险等级；使用模块：计划解析器。
    bool requiresUserConfirmation = true;    // 功能：是否必须确认；使用模块：后续 Agent UI。
    bool resultMayContainSensitiveContent = false; // 功能：结果是否可能敏感；使用模块：回传确认提示。
    bool enabledForAgent = true;        // 功能：是否允许 AI 建议；使用模块：计划解析器。
};

// 功能：返回风险等级稳定字符串；使用模块：计划解析、日志和测试。
QString agentToolRiskToString(AgentToolRisk risk);

// 功能：解析风险等级字符串；使用模块：计划解析器校验 AI JSON。
bool agentToolRiskFromString(const QString &value, AgentToolRisk *risk);

// 功能：返回更高风险等级；使用模块：AI 风险标注低于工具目录时提升风险。
AgentToolRisk maxAgentToolRisk(AgentToolRisk left, AgentToolRisk right);

// 功能：根据语言返回工具显示名；使用模块：计划预览 UI。
QString agentToolDisplayName(const AgentToolDescriptor &descriptor, AppLanguage language);

// 功能：返回 V7 Agent 可见工具目录；使用模块：计划生成和计划解析。
QVector<AgentToolDescriptor> defaultAgentToolCatalog();

// 功能：按 ID 查找工具描述；使用模块：计划解析器校验 toolId。
const AgentToolDescriptor *findAgentToolDescriptor(const QVector<AgentToolDescriptor> &catalog, const QString &toolId);
