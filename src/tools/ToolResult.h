#pragma once

#include <QString>

// 学习注释：本地工具的统一执行结果，避免 UI 通过解析字符串判断成功或失败。
// 使用模块：LocalTool 派生类负责返回，ToolsDialog 后续负责展示 output 或 error。
struct ToolResult {
    bool ok = false; // 功能：标记工具是否执行成功；使用模块：工具窗口决定显示结果还是错误提示。
    QString output;  // 功能：成功时的文本输出；使用模块：复制输出、插入聊天输入框。
    QString error;   // 功能：失败时的错误说明；使用模块：工具窗口错误提示、日志记录。

    // 功能：创建成功结果；使用模块：具体工具实现，例如 JSON 格式化工具。
    static ToolResult success(const QString &outputText)
    {
        ToolResult result;
        result.ok = true;
        result.output = outputText;
        return result;
    }

    // 功能：创建失败结果；使用模块：具体工具在输入非法或执行失败时返回。
    static ToolResult failure(const QString &errorText)
    {
        ToolResult result;
        result.ok = false;
        result.error = errorText;
        return result;
    }
};
