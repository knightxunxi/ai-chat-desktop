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
    int maxSteps);

} // namespace AgentPlanPromptBuilder
