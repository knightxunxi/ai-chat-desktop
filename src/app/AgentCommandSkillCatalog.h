#pragma once

#include "app/AgentPlan.h"
#include "core/AppLanguage.h"
#include "tools/AgentToolCatalog.h"

#include <QString>
#include <QVector>

struct AgentCommandSkillStep {
    QString englishTitle;       // 功能：英文步骤标题；使用模块：技能展开计划。
    QString chineseTitle;       // 功能：中文步骤标题；使用模块：技能展开计划。
    QString toolId;             // 功能：命令工具 ID；使用模块：AgentPlanExecutor。
    QString englishReason;      // 功能：英文步骤原因；使用模块：技能展开计划。
    QString chineseReason;      // 功能：中文步骤原因；使用模块：技能展开计划。
    AgentToolRisk risk = AgentToolRisk::Low; // 功能：步骤风险等级；使用模块：计划展示。
};

struct AgentCommandSkill {
    QString id;                       // 功能：稳定技能 ID；使用模块：Prompt 和测试。
    QString englishName;              // 功能：英文技能名称；使用模块：Prompt。
    QString chineseName;              // 功能：中文技能名称；使用模块：Prompt。
    QString englishDescription;       // 功能：英文技能说明；使用模块：Prompt。
    QString chineseDescription;       // 功能：中文技能说明；使用模块：Prompt。
    QVector<AgentCommandSkillStep> steps; // 功能：技能步骤模板；使用模块：展开为 AgentPlan。
};

namespace AgentCommandSkillCatalog {

// 功能：返回内置开发者命令技能；使用模块：Prompt、测试和后续技能 UI。
QVector<AgentCommandSkill> defaultSkills();

// 功能：按 ID 查找技能；使用模块：测试和后续技能入口。
const AgentCommandSkill *findSkill(const QVector<AgentCommandSkill> &skills, const QString &skillId);

// 功能：生成技能 Prompt 片段；使用模块：AgentPlanPromptBuilder。
QString promptSection(AppLanguage language);

// 功能：按指定技能列表生成 Prompt 片段；使用模块：V10.2 外部技能注入。
QString promptSection(const QVector<AgentCommandSkill> &skills, AppLanguage language);

// 功能：把技能模板展开为 Agent 计划；使用模块：测试和后续技能执行入口。
AgentPlan planForSkill(const AgentCommandSkill &skill, AppLanguage language);

} // namespace AgentCommandSkillCatalog
