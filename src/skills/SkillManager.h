#pragma once

#include "skills/SkillDefinition.h"
#include "core/AppLanguage.h"

#include <QObject>
#include <QString>
#include <QVector>

// ============================================================================
// SkillManager — 技能发现、加载、匹配、prompt 生成
//
// 双目录扫描：~/.codex/skills/（用户级）和 .workbuddy/skills/（项目级）
// 项目级同名技能覆盖用户级；同级别按 priority 降序。
// ============================================================================

class SkillManager : public QObject {
    Q_OBJECT

public:
    explicit SkillManager(QObject *parent = nullptr);

    // 功能：设置双目录路径并初始加载；使用模块：ApplicationController::initialize。
    void initialize(const QString &userSkillsDir, const QString &projectSkillsDir);

    // 功能：清空并重新扫描加载双目录；使用模块：热重载（P1）和测试。
    int reload();

    // 功能：返回所有已加载的技能；使用模块：技能列表 UI（P1）和测试。
    QVector<SkillDefinition> allSkills() const;

    // 功能：根据用户输入做子串匹配（大小写不敏感），返回匹配的技能列表；
    // 使用模块：AgentLoopController pre_send 阶段。
    QVector<SkillDefinition> matchSkills(const QString &userInput) const;

    // 功能：生成注入 prompt 的技能指令文本；使用模块：AgentLoopPromptBuilder。
    QString matchedSkillsPrompt(const QVector<SkillDefinition> &skills, AppLanguage language) const;

    // 功能：生成人类可读的技能摘要；使用模块：ApplicationController 循环完成后。
    QString skillSummary(const QVector<SkillDefinition> &skills) const;

    // 功能：返回已加载技能总数；使用模块：测试和状态栏。
    int totalCount() const;

private:
    // 功能：扫描指定目录下的 SKILL.md 文件；使用模块：loadFromDirectory。
    QStringList scanDirectory(const QString &dir) const;

    // 功能：从目录加载所有技能文件；使用模块：reload。
    QVector<SkillDefinition> loadFromDirectory(const QString &dir, const QString &sourceType) const;

    // 功能：合并技能列表（项目级覆盖用户级同名）；使用模块：reload。
    void mergeSkills(const QVector<SkillDefinition> &incoming);

    QVector<SkillDefinition> m_skills;
    QString m_userSkillsDir;
    QString m_projectSkillsDir;
};
