#pragma once

#include "tools/ToolResult.h"

#include <QString>

namespace ProjectFindService {

// 功能：在项目目录中按 glob 模式搜索文件。
// 排除 .git/、build-*/、node_modules/ 等目录。
// maxResults 默认 100。
ToolResult findFiles(const QString &projectDirectory,
                     const QString &pattern,
                     int maxResults = 100);

} // namespace ProjectFindService
