#pragma once

#include "core/AppLanguage.h"
#include "tools/AgentToolCatalog.h"

#include <QString>
#include <QVector>

namespace AgentPlanPromptBuilder {

// 功能：生成要求 AI 返回结构化计划的提示词；使用模块：V7 AI 计划生成入口。
QString buildPlanningPrompt(
    const QString &userGoal,
    const QVector<AgentToolDescriptor> &toolCatalog,
    AppLanguage language,
    int maxSteps,
    const QString &projectInstructionsSection = QString(),
    const QString &commandSkillSection = QString(),
    const QString &projectMemorySection = QString());

// 功能：生成统一模式提示词，AI 可返回聊天文本或执行计划；使用模块：聊天内 Agent。
QString buildUnifiedPrompt(
    const QString &userMessage,
    const QVector<AgentToolDescriptor> &toolCatalog,
    AppLanguage language,
    int maxSteps,
    const QString &projectInstructionsSection = QString(),
    const QString &commandSkillSection = QString(),
    const QString &projectMemorySection = QString());

} // namespace AgentPlanPromptBuilder

// 学习注释：统一模式的 AI 响应类型，让 AI 自己判断该聊天还是执行。
enum class UnifiedResponseKind {
    Chat,    // 纯聊天回复
    Plan,    // 任务执行计划
    Invalid  // 解析失败
};

// 学习注释：统一响应解析结果。
struct UnifiedResponse {
    UnifiedResponseKind kind = UnifiedResponseKind::Invalid;
    QString chatMessage;          // kind == Chat 时有效
    QString planJson;             // kind == Plan 时有效
    QString rawResponse;          // AI 原始响应
};

namespace UnifiedResponseParser {

// 功能：解析 AI 返回的统一响应；使用模块：ApplicationController 统一消息处理。
UnifiedResponse parse(const QString &aiResponse, AppLanguage language);

} // namespace UnifiedResponseParser
