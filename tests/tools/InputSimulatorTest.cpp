// V14.2 InputSimulator 单元测试
// 测试 sendText / sendKeyCombo 的输入校验和真实 SendInput 行为（Windows）

#include "tools/InputSimulator.h"
#include "tools/ToolResult.h"

#include <cassert>

int main()
{
    // --- sendText 测试 ---

    // 测试1: 空文本 → 返回失败
    {
        const ToolResult result = InputSimulator::sendText(QStringLiteral(""));
        assert(!result.ok);
        assert(!result.error.isEmpty());
    }

    // 测试2: 非空文本 → 返回成功（Windows 下真实调用 SendInput）
    {
        const ToolResult result = InputSimulator::sendText(QStringLiteral("Hello"));
        assert(result.ok);
        assert(!result.output.isEmpty());
    }

    // --- sendKeyCombo 测试 ---

    // 测试3: 空组合键 → 返回失败
    {
        const ToolResult result = InputSimulator::sendKeyCombo(QStringLiteral(""));
        assert(!result.ok);
        assert(!result.error.isEmpty());
    }

    // 测试4: Ctrl+C → 返回成功
    {
        const ToolResult result = InputSimulator::sendKeyCombo(QStringLiteral("Ctrl+C"));
        assert(result.ok);
        assert(!result.output.isEmpty());
    }

    // 测试5: Alt+F4 → 返回成功
    {
        const ToolResult result = InputSimulator::sendKeyCombo(QStringLiteral("Alt+F4"));
        assert(result.ok);
        assert(!result.output.isEmpty());
    }

    // 测试6: 无效组合键 (Foo+Bar) → 应返回失败（无匹配键）
    {
        const ToolResult result = InputSimulator::sendKeyCombo(QStringLiteral("Foo+Bar"));
        assert(!result.ok);
    }

    return 0;
}
