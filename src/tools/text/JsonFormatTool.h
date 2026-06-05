#pragma once

#include "tools/text/LocalTool.h"

// 学习注释：JSON 格式化工具，把压缩 JSON 转为便于阅读的缩进文本。
// 使用模块：ToolsDialog 后续通过 LocalTool 接口调用，测试模块验证转换逻辑。
class JsonFormatTool final : public LocalTool
{
public:
    QString id() const override;
    QString displayName(AppLanguage language) const override;
    QString description(AppLanguage language) const override;
    ToolResult run(const QString &input) const override;
};
