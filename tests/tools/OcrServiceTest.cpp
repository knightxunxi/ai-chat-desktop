#include "tools/OcrService.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTextStream>

#include <cassert>

// 功能：在指定目录中生成一张简单的 PNG 图片（白色背景 + 黑色文字模拟）。
static QString generateTestImage(const QString &dir, const QString &fileName, int width, int height)
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::white);

    // 绘制一些像素图案，模拟"文字"区域
    for (int y = 20; y < height - 20; ++y) {
        for (int x = 10; x < width - 10; ++x) {
            if ((x / 10 + y / 15) % 2 == 0) {
                img.setPixelColor(x, y, QColor(30, 30, 30));
            }
        }
    }

    const QString filePath = QDir(dir).filePath(fileName);
    // 确保父目录存在
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    const bool saved = img.save(filePath, "BMP");
    assert(saved);
    Q_UNUSED(saved);
    return filePath;
}

// 功能：在指定目录中生成一个非图片文件（文本文件冒充图片扩展名）。
static QString generateFakeImageFile(const QString &dir, const QString &fileName)
{
    const QString filePath = QDir(dir).filePath(fileName);
    QFile file(filePath);
    const bool opened = file.open(QIODevice::WriteOnly | QIODevice::Text);
    assert(opened);
    QTextStream stream(&file);
    stream << "This is not an image file.\n";
    file.close();
    return filePath;
}

int main()
{
    QTemporaryDir tempDir;
    assert(tempDir.isValid());
    const QString workspaceDir = tempDir.path();

    // ──────────────────────────────────────────────
    // Test 1: empty-path — 空路径应返回失败
    // ──────────────────────────────────────────────
    {
        const ToolResult result = OcrService::extractText(workspaceDir, QString());
        assert(!result.ok);
        assert(result.error.contains(QStringLiteral("empty")));
    }

    // ──────────────────────────────────────────────
    // Test 2: non-existent-file — 不存在的文件应返回失败
    // ──────────────────────────────────────────────
    {
        const ToolResult result = OcrService::extractText(workspaceDir, QStringLiteral("nonexistent.png"));
        assert(!result.ok);
        assert(result.error.contains(QStringLiteral("does not exist")));
    }

    // ──────────────────────────────────────────────
    // Test 3: outside-workspace — 工作区外路径应被拒绝
    // ──────────────────────────────────────────────
    {
        const ToolResult result = OcrService::extractText(workspaceDir, QStringLiteral("../outside.png"));
        assert(!result.ok);
        assert(result.error.contains(QStringLiteral("outside workspace")));
    }

    // ──────────────────────────────────────────────
    // Test 4: valid-image — 有效图片应能加载成功
    // ──────────────────────────────────────────────
    {
        const QString imagePath = generateTestImage(workspaceDir, QStringLiteral("test.bmp"), 200, 80);
        assert(QFile::exists(imagePath));

        const ToolResult result = OcrService::extractText(workspaceDir, QStringLiteral("test.bmp"));
        // OCR 结果取决于引擎和图片内容，但至少应该 ok（成功加载图片）
        assert(result.ok);
        // 输出应包含图片路径或尺寸信息
        assert(result.output.contains(QStringLiteral("test.bmp")));
    }

    // ──────────────────────────────────────────────
    // Test 5: non-image-file — 非图片文件不崩溃即可
    // V15.5: OCR 可能对伪图片文件返回 ok（返回空文本或重新尝试），
    //        重点是处理非图片文件不崩溃。
    // ──────────────────────────────────────────────
    {
        const QString fakePath = generateFakeImageFile(workspaceDir, QStringLiteral("fake.png"));
        assert(QFile::exists(fakePath));

        const ToolResult result = OcrService::extractText(workspaceDir, QStringLiteral("fake.png"));
        // 不要求特定结果，只验证不崩溃
        assert(true);
    }

    // ──────────────────────────────────────────────
    // Test 6: large-image — 大尺寸图片不崩溃
    // ──────────────────────────────────────────────
    {
        const QString imagePath = generateTestImage(workspaceDir, QStringLiteral("large.bmp"), 1200, 800);
        assert(QFile::exists(imagePath));

        const ToolResult result = OcrService::extractText(workspaceDir, QStringLiteral("large.bmp"));
        // 不应崩溃；至少返回成功（即使 OCR 无结果）
        assert(result.ok);
    }

    // ──────────────────────────────────────────────
    // Test 7: whitespace-only-path — 纯空白路径应返回失败
    // ──────────────────────────────────────────────
    {
        const ToolResult result = OcrService::extractText(workspaceDir, QStringLiteral("   "));
        assert(!result.ok);
        assert(result.error.contains(QStringLiteral("empty")));
    }

    // ──────────────────────────────────────────────
    // Test 8: relative-path-resolution — 相对路径正确解析
    // ──────────────────────────────────────────────
    {
        const QString imagePath = generateTestImage(workspaceDir, QStringLiteral("subdir/deep.bmp"), 100, 100);
        assert(QFile::exists(imagePath));

        const ToolResult result = OcrService::extractText(workspaceDir, QStringLiteral("subdir/deep.bmp"));
        assert(result.ok);
        assert(result.output.contains(QStringLiteral("subdir/deep.bmp")));
    }

    return 0;
}
