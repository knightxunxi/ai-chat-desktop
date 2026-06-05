#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace AssistantService {

// 功能：从项目目录生成工作日报摘要（git log + 文件变更）。
ToolResult workJournal(const QString &projectDirectory, const QString &workspaceDirectory);

// 功能：运行项目综合检查（当前返回占位，完整版集成构建+测试+diff）。
ToolResult projectCheck(const QString &projectDirectory);

// 功能：按后缀分类移动工作目录内的文件到子目录。
ToolResult fileOrganize(const QString &workspaceDirectory, const QString &sourcePattern, const QString &targetSubDir);

// 功能：保存提醒文本到工作目录文件。
ToolResult saveReminder(const QString &workspaceDirectory, const QString &title, const QString &content);

} // namespace AssistantService
