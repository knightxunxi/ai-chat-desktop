#include "app/AgentCommandSkillCatalog.h"

#include <QSet>

#include <cassert>

int main()
{
    const QVector<AgentCommandSkill> skills = AgentCommandSkillCatalog::defaultSkills();
    assert(skills.size() >= 4);

    QSet<QString> ids;
    for (const AgentCommandSkill &skill : skills) {
        assert(!skill.id.isEmpty());
        assert(!ids.contains(skill.id));
        ids.insert(skill.id);
        assert(!skill.englishName.isEmpty());
        assert(!skill.chineseName.isEmpty());
        assert(!skill.steps.isEmpty());
        for (const AgentCommandSkillStep &step : skill.steps) {
            assert(step.toolId.startsWith(QStringLiteral("command.")));
            assert(!step.englishTitle.isEmpty());
            assert(!step.chineseTitle.isEmpty());
        }
    }

    const AgentCommandSkill *preCommit = AgentCommandSkillCatalog::findSkill(skills, QStringLiteral("developer.pre_commit_check"));
    assert(preCommit != nullptr);
    assert(preCommit->steps.size() == 3);
    assert(preCommit->steps[0].toolId == QStringLiteral("command.git_diff_check"));
    assert(preCommit->steps[1].toolId == QStringLiteral("command.cmake_build"));
    assert(preCommit->steps[2].toolId == QStringLiteral("command.ctest"));

    const QString promptSection = AgentCommandSkillCatalog::promptSection(AppLanguage::Chinese);
    assert(promptSection.contains(QStringLiteral("Recommended developer command skills")));
    assert(promptSection.contains(QStringLiteral("developer.pre_commit_check")));
    assert(promptSection.contains(QStringLiteral("command.cmake_build")));

    AgentCommandSkill externalSkill;
    externalSkill.id = QStringLiteral("external.quick_check");
    externalSkill.englishName = QStringLiteral("Quick Check");
    externalSkill.chineseName = QStringLiteral("快速检查");
    externalSkill.englishDescription = QStringLiteral("Run a quick external check.");
    externalSkill.chineseDescription = QStringLiteral("运行外部快速检查。");
    externalSkill.steps.append(preCommit->steps.first());
    const QString externalPromptSection = AgentCommandSkillCatalog::promptSection(QVector<AgentCommandSkill>{externalSkill}, AppLanguage::English);
    assert(externalPromptSection.contains(QStringLiteral("external.quick_check")));
    assert(!externalPromptSection.contains(QStringLiteral("developer.pre_commit_check")));

    const AgentPlan plan = AgentCommandSkillCatalog::planForSkill(*preCommit, AppLanguage::Chinese);
    assert(plan.summary.contains(QStringLiteral("提交前")));
    assert(plan.steps.size() == preCommit->steps.size());
    assert(plan.steps[0].id == QStringLiteral("developer.pre_commit_check-step-1"));
    assert(plan.steps[0].toolId == QStringLiteral("command.git_diff_check"));
    assert(plan.steps[0].parameters.isEmpty());
    assert(plan.steps[1].risk == AgentToolRisk::Medium);

    assert(AgentCommandSkillCatalog::findSkill(skills, QStringLiteral("missing.skill")) == nullptr);

    return 0;
}
