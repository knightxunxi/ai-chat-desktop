#pragma once

#include "tools/text/LocalTool.h"

// 学习注释：普通文本清理工具，用于统一换行、裁剪空白并压缩多余空行。
// 使用模块：ToolsDialog 后续通过 LocalTool 接口调用，测试模块验证文本整理逻辑。
class TextCleanupTool final : public LocalTool
{
public:
    QString id() const override;
    QString displayName(AppLanguage language) const override;
    QString description(AppLanguage language) const override;
    ToolResult run(const QString &input) const override;
};
