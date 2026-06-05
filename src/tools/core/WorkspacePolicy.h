#pragma once

#include <QString>

enum class WorkspaceOperation {
    Read,
    List,
    Write,
    Overwrite,
    Delete,
    CreateDirectory
};

struct WorkspacePolicyDecision {
    bool allowed = false;              // 功能：是否允许自动执行；使用模块：后续 Agent 文件执行器。
    bool requiresConfirmation = false; // 功能：是否需要额外确认；使用模块：后续高风险操作 UI。
    QString reason;                    // 功能：说明允许或拒绝原因；使用模块：状态提示和测试。
    QString normalizedPath;            // 功能：归一化后的目标路径；使用模块：执行前路径校验。
};

namespace WorkspacePolicy {

// 功能：返回 Agent 默认工作目录；使用模块：设置窗口和后续文件生成工具。
QString defaultWorkspaceDirectory();

// 功能：把相对路径解析到工作目录内；使用模块：自动文件生成前路径整理。
QString resolveWorkspacePath(const QString &workspaceDirectory, const QString &requestedPath);

// 功能：判断目标路径是否位于工作目录内；使用模块：防止路径穿越。
bool isPathInsideWorkspace(const QString &workspaceDirectory, const QString &targetPath);

// 功能：判断路径是否属于敏感或项目关键文件；使用模块：覆盖和删除前保护。
bool isProtectedPath(const QString &path);

// 功能：根据工作目录、目标路径和操作类型判断是否允许自动执行；使用模块：后续 Agent 文件执行器。
WorkspacePolicyDecision evaluateWorkspaceOperation(
    const QString &workspaceDirectory,
    const QString &requestedPath,
    WorkspaceOperation operation);

} // namespace WorkspacePolicy
