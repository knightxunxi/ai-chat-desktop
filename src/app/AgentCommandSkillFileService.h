#pragma once

#include "app/AgentCommandSkillCatalog.h"
#include "tools/AgentToolRegistry.h"

#include <QStringList>

// 学习注释：外部技能文件读取结果。
// 使用模块：ApplicationController 从项目目录加载 skills/*.skill.md。
struct AgentCommandSkillLoadResult {
    QVector<AgentCommandSkill> skills; // 功能：成功解析并通过校验的技能；使用模块：Prompt 注入。
    QStringList errors;                // 功能：读取或校验失败摘要；使用模块：日志和测试。
};

namespace AgentCommandSkillFileService {

constexpr int DefaultMaxSkillFiles = 20;              // 功能：单个项目最多读取技能文件数；使用模块：loadFromProjectDirectory。
constexpr qint64 DefaultMaxSkillFileBytes = 32 * 1024; // 功能：单个技能文件最大字节数；使用模块：loadFromProjectDirectory。

// 功能：从项目目录 skills/*.skill.md 读取外部技能；使用模块：V10.2 外部技能系统。
AgentCommandSkillLoadResult loadFromProjectDirectory(
    const QString &projectDirectory,
    const AgentToolRegistry &registry,
    int maxFiles = DefaultMaxSkillFiles,
    qint64 maxFileBytes = DefaultMaxSkillFileBytes);

} // namespace AgentCommandSkillFileService
