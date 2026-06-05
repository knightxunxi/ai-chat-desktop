#pragma once

#include "tools/registry/ToolResult.h"

#include <QString>

namespace SystemInfoService {

// 功能：读取 Windows 环境变量值（如 USERPROFILE、APPDATA、TEMP）。
// 限制：不读取含 password/key/token/secret 的变量。
ToolResult readEnvVariable(const QString &name);

// 功能：获取常用系统路径（Desktop、Documents、Home）。
ToolResult systemPath(const QString &kind);

} // namespace SystemInfoService
