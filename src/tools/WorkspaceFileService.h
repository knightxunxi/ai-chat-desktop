#pragma once

#include "tools/ToolResult.h"

#include <QString>

// 学习注释：Agent 工作目录专用文件服务，只允许在配置的 workspace 内自动读写。
// 使用模块：AgentPlanExecutor 执行 workspace.* 计划步骤，测试模块验证路径和注入边界。
namespace WorkspaceFileService {

constexpr qint64 DefaultMaxWorkspaceTextFileBytes = 1024 * 1024; // 功能：默认读取上限 1 MiB；使用模块：readText 防止大文件拖慢 UI。

// 功能：在工作目录内创建新文本文件，不允许静默覆盖；使用模块：workspace.write_text。
ToolResult writeText(const QString &workspaceDirectory, const QString &requestedPath, const QString &content);

// 功能：读取工作目录内文本文件，并把结果标记为不可信数据；使用模块：workspace.read_text。
ToolResult readText(
    const QString &workspaceDirectory,
    const QString &requestedPath,
    qint64 maxBytes = DefaultMaxWorkspaceTextFileBytes);

// 功能：列出工作目录内目录条目；使用模块：workspace.list_directory。
ToolResult listDirectory(const QString &workspaceDirectory, const QString &requestedPath, int maxEntries = 200);

// 功能：覆盖工作目录内普通文本文件，覆盖前生成 .bak 备份；使用模块：workspace.overwrite_text。
ToolResult overwriteText(const QString &workspaceDirectory, const QString &requestedPath, const QString &content);

// 功能：把工作目录内普通文件移动到 .trash；使用模块：workspace.delete_file。
ToolResult deleteFile(const QString &workspaceDirectory, const QString &requestedPath);

// 功能：返回相对工作目录的路径显示；使用模块：日志和执行结果摘要。
QString relativeWorkspacePath(const QString &workspaceDirectory, const QString &absolutePath);

} // namespace WorkspaceFileService
