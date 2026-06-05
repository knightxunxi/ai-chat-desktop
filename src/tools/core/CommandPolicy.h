#pragma once

#include "tools/registry/AgentToolCatalog.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

struct CommandSpec {
    QString templateId;       // 功能：命令模板 ID；使用模块：CommandPolicy 和 AgentToolRegistry。
    QString program;          // 功能：外部程序名；使用模块：CommandRunner。
    QStringList arguments;    // 功能：固定参数数组；使用模块：CommandRunner，避免 shell 拼接。
    QString workingDirectory; // 功能：命令执行目录；使用模块：CommandRunner。
    int timeoutMs = 15000;    // 功能：命令超时；使用模块：CommandRunner。
    AgentToolRisk risk = AgentToolRisk::Low; // 功能：命令风险等级；使用模块：工具目录和计划解析。
    bool internalOnly = false; // 功能：是否为内部命令；使用模块：项目文件列表工具。
};

struct CommandPolicyDecision {
    bool allowed = false; // 功能：是否允许执行；使用模块：AgentToolRegistry。
    QString reason;       // 功能：拒绝或允许原因；使用模块：UI 和测试。
    CommandSpec command;  // 功能：校验后的命令规格；使用模块：CommandRunner。
};

namespace CommandPolicy {

// 功能：返回第一版允许的固定命令模板；使用模块：工具注册表和测试。
QVector<CommandSpec> allowedCommandTemplates();

// 功能：按模板 ID 查找命令；使用模块：策略校验和测试。
std::optional<CommandSpec> commandTemplate(const QString &templateId);

// 功能：判断命令工作目录是否安全；使用模块：evaluateCommand 和测试。
bool isSafeWorkingDirectory(const QString &workingDirectory);

// 功能：校验模板 ID 和项目目录并生成可执行命令；使用模块：AgentToolRegistry。
CommandPolicyDecision evaluateCommand(const QString &templateId, const QString &projectDirectory);

// 功能：生成命令展示文本；使用模块：执行结果摘要和 UI。
QString commandDisplayText(const CommandSpec &command);

} // namespace CommandPolicy
