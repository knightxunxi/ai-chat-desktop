#include "tools/WindowDetector.h"

#include <cassert>

int main()
{
    // Test 1: listWindows
    {
        const ToolResult result = WindowDetector::listWindows(10);
        assert(result.ok);
        // Should at least return something (even if no windows, it says "No visible windows")
    }

    // Test 2: foregroundWindowTitle
    {
        const ToolResult result = WindowDetector::foregroundWindowTitle();
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("Foreground window")));
    }

    return 0;
}
