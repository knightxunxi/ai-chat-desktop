#pragma once

#include "tools/text/LocalTool.h"

// 学习注释：Markdown 简单整理工具，清理代码块外部的空白并保留代码内容。
// 使用模块：ToolsDialog 后续通过 LocalTool 接口调用，测试模块验证代码块保护逻辑。
class MarkdownCleanupTool final : public LocalTool
{
public:
    QString id() const override;
    QString displayName(AppLanguage language) const override;
    QString description(AppLanguage language) const override;
    ToolResult run(const QString &input) const override;
};
