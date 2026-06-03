#include "app/AgentCommandSkillCatalog.h"

#include <QStringList>

namespace {

QString displayText(AppLanguage language, const QString &english, const QString &chinese)
{
    return language == AppLanguage::English ? english : chinese;
}

AgentCommandSkillStep skillStep(
    const QString &englishTitle,
    const QString &chineseTitle,
    const QString &toolId,
    const QString &englishReason,
    const QString &chineseReason,
    AgentToolRisk risk)
{
    AgentCommandSkillStep step;
    step.englishTitle = englishTitle;
    step.chineseTitle = chineseTitle;
    step.toolId = toolId;
    step.englishReason = englishReason;
    step.chineseReason = chineseReason;
    step.risk = risk;
    return step;
}

AgentCommandSkill skill(
    const QString &id,
    const QString &englishName,
    const QString &chineseName,
    const QString &englishDescription,
    const QString &chineseDescription,
    const QVector<AgentCommandSkillStep> &steps)
{
    AgentCommandSkill commandSkill;
    commandSkill.id = id;
    commandSkill.englishName = englishName;
    commandSkill.chineseName = chineseName;
    commandSkill.englishDescription = englishDescription;
    commandSkill.chineseDescription = chineseDescription;
    commandSkill.steps = steps;
    return commandSkill;
}

QString stepToolList(const AgentCommandSkill &skill)
{
    QStringList toolIds;
    for (const AgentCommandSkillStep &step : skill.steps) {
        toolIds.append(step.toolId);
    }

    return toolIds.join(QStringLiteral(" -> "));
}

} // namespace

namespace AgentCommandSkillCatalog {

QVector<AgentCommandSkill> defaultSkills()
{
    QVector<AgentCommandSkill> skills;
    skills.reserve(4);
    skills.append(skill(
        QStringLiteral("developer.inspect_changes"),
        QStringLiteral("Inspect Current Changes"),
        QStringLiteral("检查当前改动"),
        QStringLiteral("Check repository status and summarize the current diff size before making decisions."),
        QStringLiteral("检查仓库状态和当前 diff 规模，辅助判断后续动作。"),
        {
            skillStep(
                QStringLiteral("Check Git status"),
                QStringLiteral("检查 Git 状态"),
                QStringLiteral("command.git_status"),
                QStringLiteral("Identify changed, untracked, and staged files."),
                QStringLiteral("识别已修改、未跟踪和已暂存文件。"),
                AgentToolRisk::Low),
            skillStep(
                QStringLiteral("Check diff statistics"),
                QStringLiteral("检查 diff 统计"),
                QStringLiteral("command.git_diff_stat"),
                QStringLiteral("Estimate the size and affected areas of the current changes."),
                QStringLiteral("估算当前改动规模和影响范围。"),
                AgentToolRisk::Low)
        }));

    skills.append(skill(
        QStringLiteral("developer.pre_commit_check"),
        QStringLiteral("Pre-Commit Check"),
        QStringLiteral("提交前检查"),
        QStringLiteral("Run whitespace diff checks, build, and tests before asking the user to commit."),
        QStringLiteral("提交前执行空白 diff 检查、构建和测试。"),
        {
            skillStep(
                QStringLiteral("Check diff whitespace"),
                QStringLiteral("检查 diff 空白问题"),
                QStringLiteral("command.git_diff_check"),
                QStringLiteral("Catch whitespace errors before commit."),
                QStringLiteral("提交前发现空白错误。"),
                AgentToolRisk::Low),
            skillStep(
                QStringLiteral("Build project"),
                QStringLiteral("构建项目"),
                QStringLiteral("command.cmake_build"),
                QStringLiteral("Verify the project still compiles."),
                QStringLiteral("验证项目仍能编译。"),
                AgentToolRisk::Medium),
            skillStep(
                QStringLiteral("Run tests"),
                QStringLiteral("运行测试"),
                QStringLiteral("command.ctest"),
                QStringLiteral("Verify automated tests before commit."),
                QStringLiteral("提交前验证自动化测试。"),
                AgentToolRisk::Medium)
        }));

    skills.append(skill(
        QStringLiteral("developer.build_and_test"),
        QStringLiteral("Build And Test"),
        QStringLiteral("构建并测试"),
        QStringLiteral("Build the project and run the full test suite."),
        QStringLiteral("构建项目并运行完整测试。"),
        {
            skillStep(
                QStringLiteral("Build project"),
                QStringLiteral("构建项目"),
                QStringLiteral("command.cmake_build"),
                QStringLiteral("Compile the current code."),
                QStringLiteral("编译当前代码。"),
                AgentToolRisk::Medium),
            skillStep(
                QStringLiteral("Run tests"),
                QStringLiteral("运行测试"),
                QStringLiteral("command.ctest"),
                QStringLiteral("Run the configured CTest suite."),
                QStringLiteral("运行配置好的 CTest 测试集。"),
                AgentToolRisk::Medium)
        }));

    skills.append(skill(
        QStringLiteral("developer.diagnose_tests"),
        QStringLiteral("Diagnose Test Failure"),
        QStringLiteral("定位测试失败"),
        QStringLiteral("Run tests and inspect repository state to help diagnose failures."),
        QStringLiteral("运行测试并检查仓库状态，辅助定位失败。"),
        {
            skillStep(
                QStringLiteral("Run tests"),
                QStringLiteral("运行测试"),
                QStringLiteral("command.ctest"),
                QStringLiteral("Collect failing test output."),
                QStringLiteral("收集失败测试输出。"),
                AgentToolRisk::Medium),
            skillStep(
                QStringLiteral("Check Git status"),
                QStringLiteral("检查 Git 状态"),
                QStringLiteral("command.git_status"),
                QStringLiteral("Check whether uncommitted changes may explain the failure."),
                QStringLiteral("检查未提交改动是否可能导致失败。"),
                AgentToolRisk::Low)
        }));

    return skills;
}

const AgentCommandSkill *findSkill(const QVector<AgentCommandSkill> &skills, const QString &skillId)
{
    const QString normalizedSkillId = skillId.trimmed();
    for (const AgentCommandSkill &skill : skills) {
        if (skill.id == normalizedSkillId) {
            return &skill;
        }
    }

    return nullptr;
}

QString promptSection(AppLanguage language)
{
    return promptSection(defaultSkills(), language);
}

QString promptSection(const QVector<AgentCommandSkill> &skills, AppLanguage language)
{
    QStringList lines;
    lines.append(QStringLiteral("Recommended developer command skills:"));
    for (const AgentCommandSkill &skill : skills) {
        lines.append(QStringLiteral("- skill=%1 | name=%2 | description=%3 | tools=%4")
                         .arg(skill.id,
                              displayText(language, skill.englishName, skill.chineseName),
                              displayText(language, skill.englishDescription, skill.chineseDescription),
                              stepToolList(skill)));
    }

    return lines.join(QLatin1Char('\n'));
}

AgentPlan planForSkill(const AgentCommandSkill &skill, AppLanguage language)
{
    AgentPlan plan;
    plan.summary = displayText(language, skill.englishDescription, skill.chineseDescription);
    for (int index = 0; index < skill.steps.size(); ++index) {
        const AgentCommandSkillStep &templateStep = skill.steps[index];
        AgentPlanStep step;
        step.id = QStringLiteral("%1-step-%2").arg(skill.id, QString::number(index + 1));
        step.title = displayText(language, templateStep.englishTitle, templateStep.chineseTitle);
        step.toolId = templateStep.toolId;
        step.reason = displayText(language, templateStep.englishReason, templateStep.chineseReason);
        step.risk = templateStep.risk;
        plan.steps.append(step);
    }

    return plan;
}

} // namespace AgentCommandSkillCatalog
