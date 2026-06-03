#pragma once

#include "tools/CommandPolicy.h"
#include "tools/ToolResult.h"

#include <QString>

namespace CommandRunner {

constexpr int DefaultMaxOutputCharacters = 4000;

// 功能：脱敏命令输出；使用模块：CommandRunner 和测试。
QString redactSensitiveOutput(const QString &text);

// 功能：截断命令输出；使用模块：CommandRunner。
QString truncateOutput(const QString &text, int maxCharacters = DefaultMaxOutputCharacters);

// 功能：执行经过 CommandPolicy 校验的命令；使用模块：AgentToolRegistry 的 command.* 工具。
ToolResult run(const CommandSpec &command, int maxOutputCharacters = DefaultMaxOutputCharacters);

} // namespace CommandRunner
