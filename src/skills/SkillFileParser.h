#pragma once

#include "skills/SkillDefinition.h"

#include <QString>

#include <optional>

// ============================================================================
// SkillFileParser — YAML frontmatter + Markdown 体解析器（静态工具类）
//
// 行级状态机解析 --- 分隔的 YAML frontmatter。
// 仅解析 7 个已知字段，忽略未知字段。
// ============================================================================

namespace SkillFileParser {

// 功能：从文件路径解析 SKILL.md；使用模块：SkillManager::loadFromDirectory。
std::optional<SkillDefinition> parseFile(const QString &filePath);

// 功能：从原始内容字符串解析；使用模块：AgentCommandSkillFileService 重构路径。
std::optional<SkillDefinition> parseContent(const QString &rawContent,
                                            const QString &sourcePath,
                                            const QString &sourceType);

} // namespace SkillFileParser
