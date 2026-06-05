#pragma once

#include "core/AppLanguage.h"
#include "skills/SkillDefinition.h"
#include "tools/AgentToolCatalog.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace AgentLoopPromptBuilder {

// 功能：生成 Agentic Loop 单步动作规划提示词；使用模块：V8.2 后续真实 AI 单步规划。
// V13.3: 新增 matchedSkills 参数，在工具定义之后注入技能指令。
QString buildNextActionPrompt(
    const QString &userGoal,
    const QStringList &observations,
    const QVector<AgentToolDescriptor> &toolCatalog,
    AppLanguage language,
    int completedSteps,
    int maxSteps,
    const QVector<SkillDefinition> &matchedSkills = {});

} // namespace AgentLoopPromptBuilder
