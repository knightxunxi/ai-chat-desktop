#pragma once

#include "tools/LocalTool.h"

// 学习注释：JSON 压缩工具，把格式化 JSON 转为单行紧凑文本。
// 使用模块：ToolsDialog 后续通过 LocalTool 接口调用，测试模块验证转换逻辑。
class JsonCompactTool final : public LocalTool
{
public:
    QString id() const override;
    QString displayName(AppLanguage language) const override;
    QString description(AppLanguage language) const override;
    ToolResult run(const QString &input) const override;
};
