#pragma once

#include "core/AppLanguage.h"
#include "tools/ToolResult.h"

#include <QString>

// 学习注释：本地工具抽象接口，让工具逻辑脱离 MainWindow 和聊天发送流程独立扩展。
// 使用模块：ToolsDialog 后续持有 LocalTool 列表，JsonFormatTool 等具体工具实现该接口。
class LocalTool
{
public:
    virtual ~LocalTool() = default;

    // 功能：返回工具稳定 ID；使用模块：工具注册、测试定位和后续日志记录。
    virtual QString id() const = 0;

    // 功能：返回当前语言下的工具显示名；使用模块：工具窗口下拉选择。
    virtual QString displayName(AppLanguage language) const = 0;

    // 功能：返回当前语言下的工具说明；使用模块：工具窗口展示用途和输入要求。
    virtual QString description(AppLanguage language) const = 0;

    // 功能：执行工具文本转换；使用模块：工具窗口运行按钮和单元测试。
    virtual ToolResult run(const QString &input) const = 0;
};
