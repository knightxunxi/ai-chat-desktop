#pragma once

#include "core/AppLanguage.h"

#include <QString>

// 学习注释：项目级 Agent 指令文件读取结果。
// 使用模块：ApplicationController 读取 AGENT.md 后注入 Agent 计划 Prompt。
struct ProjectInstructions {
    bool loaded = false;    // 功能：是否成功读取 AGENT.md；使用模块：判断是否注入 Prompt。
    bool truncated = false; // 功能：内容是否因大小限制被截断；使用模块：Prompt 中提示模型。
    QString filePath;       // 功能：实际读取路径；使用模块：日志和 Prompt 来源说明。
    QString content;        // 功能：AGENT.md 文本内容；使用模块：Prompt 注入。
    QString error;          // 功能：读取失败原因；使用模块：诊断和测试。
};

namespace ProjectInstructionService {

constexpr qint64 DefaultMaxInstructionBytes = 16 * 1024; // 功能：AGENT.md 最大读取字节数；使用模块：loadFromProjectDirectory。

// 功能：从项目目录读取 AGENT.md；使用模块：V10 项目级指令。
ProjectInstructions loadFromProjectDirectory(
    const QString &projectDirectory,
    qint64 maxBytes = DefaultMaxInstructionBytes);

// 功能：把项目指令包装成带安全边界的 Prompt 片段；使用模块：AgentPlanPromptBuilder 输入。
QString promptSection(const ProjectInstructions &instructions, AppLanguage language);

} // namespace ProjectInstructionService
