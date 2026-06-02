#include "tools/LocalTool.h"

#include <cassert>
#include <memory>

class EchoTool final : public LocalTool
{
public:
    QString id() const override
    {
        return QStringLiteral("echo");
    }

    QString displayName(AppLanguage language) const override
    {
        return language == AppLanguage::Chinese ? QStringLiteral("回显") : QStringLiteral("Echo");
    }

    QString description(AppLanguage language) const override
    {
        return language == AppLanguage::Chinese
                   ? QStringLiteral("返回输入文本")
                   : QStringLiteral("Returns the input text");
    }

    ToolResult run(const QString &input) const override
    {
        if (input.isEmpty()) {
            return ToolResult::failure(QStringLiteral("Input is empty."));
        }

        return ToolResult::success(input);
    }
};

int main()
{
    std::unique_ptr<LocalTool> tool = std::make_unique<EchoTool>();

    assert(tool->id() == QStringLiteral("echo"));
    assert(tool->displayName(AppLanguage::Chinese) == QStringLiteral("回显"));
    assert(tool->displayName(AppLanguage::English) == QStringLiteral("Echo"));
    assert(!tool->description(AppLanguage::Chinese).isEmpty());

    ToolResult result = tool->run(QStringLiteral("hello"));
    assert(result.ok);
    assert(result.output == QStringLiteral("hello"));
    assert(result.error.isEmpty());

    result = tool->run(QString());
    assert(!result.ok);
    assert(result.output.isEmpty());
    assert(!result.error.isEmpty());

    return 0;
}
