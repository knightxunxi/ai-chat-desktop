#pragma once

#include "core/AppLanguage.h"
#include "tools/registry/ToolResult.h"

#include <QString>

// 学习注释：项目工作记忆读取结果。
// 使用模块：ApplicationController 读取 AGENT_MEMORY.md 后注入 Agent Prompt。
struct ProjectMemory {
    bool loaded = false;    // 功能：是否读取到有效记忆；使用模块：Prompt 注入。
    bool truncated = false; // 功能：内容是否被截断；使用模块：Prompt 中提示模型。
    QString filePath;       // 功能：记忆文件路径；使用模块：日志和 Prompt 来源说明。
    QString content;        // 功能：记忆正文；使用模块：Prompt 注入。
    QString error;          // 功能：读取失败原因；使用模块：测试和诊断。
};

namespace ProjectMemoryService {

constexpr qint64 DefaultMaxMemoryBytes = 16 * 1024; // 功能：AGENT_MEMORY.md 最大读取字节数；使用模块：loadFromProjectDirectory。
constexpr int DefaultMaxNoteCharacters = 2000;      // 功能：单条记忆最大字符数；使用模块：appendProjectNote。

// 功能：从项目目录读取 AGENT_MEMORY.md；使用模块：V10.3 工作记忆 Prompt 注入。
ProjectMemory loadFromProjectDirectory(
    const QString &projectDirectory,
    qint64 maxBytes = DefaultMaxMemoryBytes);

// 功能：追加一条项目记忆；使用模块：memory.append_project_note 工具。
ToolResult appendProjectNote(
    const QString &projectDirectory,
    const QString &content,
    const QString &source = QStringLiteral("user"),
    int maxNoteCharacters = DefaultMaxNoteCharacters);

// 功能：把工作记忆包装成带安全边界的 Prompt 片段；使用模块：AgentPlanPromptBuilder 输入。
QString promptSection(const ProjectMemory &memory, AppLanguage language);

} // namespace ProjectMemoryService
