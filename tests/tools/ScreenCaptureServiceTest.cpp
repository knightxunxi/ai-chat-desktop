#include "tools/ScreenCaptureService.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    QTemporaryDir tempDir;
    assert(tempDir.isValid());
    const QString workspaceDir = tempDir.path();

    // Test 1: capture to file
    {
        const ToolResult result = ScreenCaptureService::captureToFile(workspaceDir, QStringLiteral("screenshot.png"));
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("Screenshot saved")));

        const QString filePath = QDir(workspaceDir).filePath(QStringLiteral("screenshot.png"));
        assert(QFile::exists(filePath));
    }

    // Test 2: empty path
    {
        const ToolResult result = ScreenCaptureService::captureToFile(workspaceDir, QString());
        assert(!result.ok);
    }

    // Test 3: path outside workspace
    {
        const ToolResult result = ScreenCaptureService::captureToFile(workspaceDir, QStringLiteral("../outside.png"));
        assert(!result.ok);
    }

    return 0;
}
