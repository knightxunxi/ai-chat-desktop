#include "skills/SkillManager.h"

#include "skills/SkillFileParser.h"
#include "support/AppLogger.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>

namespace {

// 每个目录最多加载的技能文件数
constexpr int kMaxSkillsPerDir = 20;

// 功能：按 priority 降序排序；使用模块：mergeSkills 和 matchSkills。
bool skillPriorityDesc(const SkillDefinition &a, const SkillDefinition &b)
{
    return a.metadata.priority > b.metadata.priority;
}

} // namespace

SkillManager::SkillManager(QObject *parent)
    : QObject(parent)
{
}

void SkillManager::initialize(const QString &userSkillsDir, const QString &projectSkillsDir)
{
    m_userSkillsDir = userSkillsDir;
    m_projectSkillsDir = projectSkillsDir;
    reload();
}

int SkillManager::reload()
{
    m_skills.clear();

    // 1. 加载用户级技能
    if (!m_userSkillsDir.isEmpty()) {
        const QVector<SkillDefinition> userSkills = loadFromDirectory(m_userSkillsDir, QStringLiteral("user"));
        mergeSkills(userSkills);
    }

    // 2. 加载项目级技能（覆盖用户级同名）
    if (!m_projectSkillsDir.isEmpty()) {
        const QVector<SkillDefinition> projectSkills = loadFromDirectory(m_projectSkillsDir, QStringLiteral("project"));
        mergeSkills(projectSkills);
    }

    // 3. 按 priority 降序排序
    std::sort(m_skills.begin(), m_skills.end(), skillPriorityDesc);

    AppLogger::info(QStringLiteral("SkillManager"),
                    QStringLiteral("Skills reloaded. total=%1").arg(m_skills.size()));

    return m_skills.size();
}

QVector<SkillDefinition> SkillManager::allSkills() const
{
    return m_skills;
}

QVector<SkillDefinition> SkillManager::matchSkills(const QString &userInput) const
{
    QVector<SkillDefinition> matched;

    if (userInput.trimmed().isEmpty()) {
        return matched;
    }

    for (const SkillDefinition &skill : m_skills) {
        // 跳过禁用的技能
        if (!skill.isEnabled()) {
            continue;
        }

        // 跳过空 triggers 的技能
        if (skill.metadata.triggers.isEmpty()) {
            continue;
        }

        // 子串匹配：任意 trigger 匹配用户输入（大小写不敏感）
        bool triggered = false;
        for (const QString &trigger : skill.metadata.triggers) {
            if (trigger.trimmed().isEmpty()) {
                continue;
            }

            if (userInput.contains(trigger, Qt::CaseInsensitive)) {
                triggered = true;
                AppLogger::info(QStringLiteral("SkillManager"),
                                QStringLiteral("matched: %1, trigger: %2").arg(skill.metadata.name, trigger));
                break;
            }
        }

        if (triggered) {
            matched.append(skill);
        }
    }

    return matched;
}

QString SkillManager::matchedSkillsPrompt(const QVector<SkillDefinition> &skills, AppLanguage language) const
{
    if (skills.isEmpty()) {
        return QString();
    }

    QStringList lines;
    lines.append(QStringLiteral("[Active Skills]"));

    for (const SkillDefinition &skill : skills) {
        lines.append(QStringLiteral("[SKILL] %1: %2")
                         .arg(skill.metadata.name, skill.metadata.description));
        if (!skill.instructions.isEmpty()) {
            lines.append(skill.instructions);
        }
    }

    return lines.join(QStringLiteral("\n"));
}

QString SkillManager::skillSummary(const QVector<SkillDefinition> &skills) const
{
    if (skills.isEmpty()) {
        return QString();
    }

    QStringList names;
    for (const SkillDefinition &skill : skills) {
        names.append(skill.metadata.name);
    }

    // 中文格式
    return QStringLiteral("本轮使用了 %1 个技能：%2")
        .arg(skills.size())
        .arg(names.join(QStringLiteral(", ")));
}

int SkillManager::totalCount() const
{
    return m_skills.size();
}

QStringList SkillManager::scanDirectory(const QString &dir) const
{
    QStringList result;
    const QDir skillsDir(dir);

    if (!skillsDir.exists()) {
        return result;
    }

    // 递归查找子目录下的 SKILL.md（对齐 WorkBuddy skill 目录规范）
    QDirIterator iterator(dir, QStringList() << QStringLiteral("SKILL.md"),
                          QDir::Files | QDir::Readable, QDirIterator::Subdirectories);

    while (iterator.hasNext()) {
        if (result.size() >= kMaxSkillsPerDir) {
            break;
        }
        result.append(iterator.next());
    }

    return result;
}

QVector<SkillDefinition> SkillManager::loadFromDirectory(const QString &dir, const QString &sourceType) const
{
    QVector<SkillDefinition> skills;
    const QStringList filePaths = scanDirectory(dir);

    for (const QString &filePath : filePaths) {
        std::optional<SkillDefinition> skill = SkillFileParser::parseFile(filePath);
        if (skill.has_value()) {
            SkillDefinition def = skill.value();
            def.sourceType = sourceType;
            skills.append(def);
        }
    }

    return skills;
}

void SkillManager::mergeSkills(const QVector<SkillDefinition> &incoming)
{
    for (const SkillDefinition &newSkill : incoming) {
        // 查找是否已有同名技能
        bool found = false;
        for (int i = 0; i < m_skills.size(); ++i) {
            if (m_skills[i].key() == newSkill.key()) {
                // 项目级覆盖用户级
                if (newSkill.sourceType == QStringLiteral("project")) {
                    m_skills[i] = newSkill;
                    AppLogger::info(QStringLiteral("SkillManager"),
                                    QStringLiteral("Project skill overrides user skill: %1").arg(newSkill.key()));
                } else {
                    AppLogger::info(QStringLiteral("SkillManager"),
                                    QStringLiteral("User skill skipped (project-level already exists): %1").arg(newSkill.key()));
                }
                found = true;
                break;
            }
        }

        if (!found) {
            m_skills.append(newSkill);
        }
    }
}
