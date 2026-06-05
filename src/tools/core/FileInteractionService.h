#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

// 学习注释：受控本地文件交互服务，集中处理文件读取、目录列出、保存和路径校验。
// 使用模块：FileToolsDialog 调用它执行用户确认后的操作，测试模块验证安全边界和错误处理。
namespace FileInteractionService {

constexpr qint64 DefaultMaxTextFileBytes = 1024 * 1024; // 功能：默认文本读取上限 1 MiB；使用模块：读取文件前避免 UI 卡顿。

// 功能：读取用户指定的文本文件；使用模块：文件工具窗口和文件服务测试。
ToolResult readTextFile(const QString &filePath, qint64 maxBytes = DefaultMaxTextFileBytes);

// 功能：列出用户指定目录下的条目；使用模块：文件工具窗口和文件服务测试。
ToolResult listDirectory(const QString &directoryPath, int maxEntries = 200);

// 功能：保存文本到用户指定文件；使用模块：文件工具窗口保存输出。
ToolResult saveTextFile(const QString &filePath, const QString &content, bool allowOverwrite);

// 功能：检查路径是否存在且可打开；使用模块：文件工具窗口打开文件或目录前验证。
ToolResult validateOpenPath(const QString &path);

// 功能：生成不含完整目录的路径摘要；使用模块：日志记录，避免直接暴露完整路径。
QString pathSummary(const QString &path);

// V18: 精确编辑 — 在文件中查找 old_str 并替换为 new_str，仅支持单次匹配。
ToolResult editTextFile(const QString &filePath, const QString &oldStr, const QString &newStr);

// V18: 内容搜索 — 在目录或文件中使用正则表达式搜索文本内容。
ToolResult grep(const QString &searchPath, const QString &pattern, const QString &fileGlob,
                bool ignoreCase, int maxResults);

// V18: 递归删除目录 — 删除目录及其所有内容。
ToolResult deleteDirectory(const QString &directoryPath);

// V18.3: 文件复制 — 复制文件或目录到目标位置。
ToolResult copyFile(const QString &sourcePath, const QString &targetPath);

// V18.3: 文件移动 — 移动/重命名文件或目录。
ToolResult moveFile(const QString &sourcePath, const QString &targetPath);

// V18.3: 追加文本 — 在文件末尾追加内容（不覆盖）。
ToolResult appendTextFile(const QString &filePath, const QString &content);

// V18.3: 文件信息 — 获取文件元信息（大小/时间/权限）。
ToolResult getFileInfo(const QString &filePath);

} // namespace FileInteractionService
