#pragma once

#include <QString>
#include <QStringList>
#include <QTypeInfo>

// ============================================================================
// SkillMetadata — YAML frontmatter 解析后的技能元数据
// ============================================================================

struct SkillMetadata {
    QString name;              // 必填，唯一标识
    QString description;       // 必填，一句话描述
    QStringList triggers;      // 必填，触发关键词列表
    int priority = 0;          // 选填，优先级 0-100，默认 0
    bool enabled = true;       // 选填，默认 true
    QString version;           // 必填，语义化版本
    QString author;            // 选填

    // 功能：校验必填字段非空；使用模块：SkillFileParser 解析后校验。
    bool isValid() const
    {
        return !name.trimmed().isEmpty()
            && !description.trimmed().isEmpty()
            && !triggers.isEmpty()
            && !version.trimmed().isEmpty();
    }
};

// ============================================================================
// SkillDefinition — 完整的技能定义（元数据 + 指令体 + 来源信息）
// ============================================================================

struct SkillDefinition {
    SkillMetadata metadata;      // 技能元数据
    QString instructions;         // Markdown 指令体
    QString sourcePath;           // SKILL.md 文件路径
    QString sourceType;           // "user" 或 "project"

    // 功能：判断技能是否启用；使用模块：SkillManager 匹配时过滤。
    bool isEnabled() const
    {
        return metadata.enabled && metadata.isValid();
    }

    // 功能：返回技能唯一键（name），用于判重和日志；使用模块：SkillManager::mergeSkills。
    QString key() const
    {
        return metadata.name.trimmed();
    }
};

// Qt6 容器兼容性：声明为可重定位类型，避免 noexcept 析构函数要求
Q_DECLARE_TYPEINFO(SkillMetadata, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(SkillDefinition, Q_RELOCATABLE_TYPE);
