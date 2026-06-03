#include "memory/MemoryEntry.h"
#include "memory/DailyMemoryWriter.h"
#include "memory/ProjectMemoryManager.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <cassert>
#include <cstdio>

namespace {

QString readFile(const QString &path)
{
    QFile file(path);
    assert(file.open(QFile::ReadOnly | QFile::Text));
    return QString::fromUtf8(file.readAll());
}

void removeFile(const QString &path)
{
    QFile::remove(path);
}

} // namespace

// 辅助：在指定目录下创建 L2 文件（.workbuddy/memory/MEMORY.md）
void writeL2(const QString &projectDir, const QString &content)
{
    const QString dirPath = projectDir + QStringLiteral("/.workbuddy/memory");
    QDir().mkpath(dirPath);
    QFile f(dirPath + QStringLiteral("/MEMORY.md"));
    assert(f.open(QFile::WriteOnly | QFile::Text));
    f.write(content.toUtf8());
}

// 辅助：在 ~/.codex/MEMORY.md 创建 L1 文件
void writeL1(const QString &content)
{
    const QString dirPath = QDir::homePath() + QStringLiteral("/.codex");
    QDir().mkpath(dirPath);
    QFile f(dirPath + QStringLiteral("/MEMORY.md"));
    assert(f.open(QFile::WriteOnly | QFile::Text));
    f.write(content.toUtf8());
}

void removeL1()
{
    removeFile(QDir::homePath() + QStringLiteral("/.codex/MEMORY.md"));
}

int main()
{
    // ── 1. MemoryEntry 基本功能 ──
    {
        MemoryEntry entry = MemoryEntry::create(
            QStringLiteral("log"),
            QStringLiteral("执行 workspace.write_text → OK"),
            QStringLiteral("agent_auto"));
        assert(entry.category == QStringLiteral("log"));
        assert(entry.source == QStringLiteral("agent_auto"));
        assert(entry.content.contains(QStringLiteral("write_text")));

        const QString line = entry.formatLine();
        assert(line.contains(QStringLiteral("[log]")));
        assert(line.contains(QStringLiteral("(agent_auto)")));
        assert(line.contains(QStringLiteral("write_text")));
    }

    // ── 2. buildMemorySection - 三层记忆正确拼接 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        writeL1(QStringLiteral("User preference: prefer C++17\n"));
        writeL2(projectDir, QStringLiteral("Project decision: use Qt6\n"));

        DailyMemoryWriter dailyWriter(projectDir);
        MemoryEntry entry = MemoryEntry::create(
            QStringLiteral("log"),
            QStringLiteral("执行 workspace.write_text → OK"),
            QStringLiteral("agent_auto"));
        assert(dailyWriter.append(entry));

        ProjectMemoryManager mgr(projectDir);
        const QString section = mgr.buildMemorySection();

        assert(!section.isEmpty());
        assert(section.contains(QStringLiteral("User preference: prefer C++17")));
        assert(section.contains(QStringLiteral("Project decision: use Qt6")));
        assert(section.contains(QStringLiteral("User Level")));
        assert(section.contains(QStringLiteral("Project Level")));
        assert(section.contains(QStringLiteral("Recent Activity")));

        removeL1();
    }

    // ── 3. buildMemorySection - L1 不存在时返回 L2+L3 拼接 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        // 确保 L1 不存在
        removeL1();

        writeL2(projectDir, QStringLiteral("L2 only content\n"));

        DailyMemoryWriter dailyWriter(projectDir);
        MemoryEntry entry = MemoryEntry::create(
            QStringLiteral("log"), QStringLiteral("test entry"), QStringLiteral("agent_auto"));
        assert(dailyWriter.append(entry));

        ProjectMemoryManager mgr(projectDir);
        const QString section = mgr.buildMemorySection();

        assert(!section.isEmpty());
        assert(section.contains(QStringLiteral("L2 only content")));
        assert(section.contains(QStringLiteral("Project Level")));
        assert(section.contains(QStringLiteral("Recent Activity")));
        // 不包含 User Level
        assert(!section.contains(QStringLiteral("User Level")));
    }

    // ── 4. appendDailyLog - 自动创建日志文件 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        ProjectMemoryManager mgr(projectDir);
        mgr.appendDailyLog(QStringLiteral("log"),
                           QStringLiteral("测试日志条目"));

        DailyMemoryWriter writer(projectDir);
        const QString filePath = writer.todayLogPath();
        assert(QFileInfo::exists(filePath));

        const QString content = readFile(filePath);
        assert(content.contains(QStringLiteral("测试日志条目")));
        assert(content.contains(QStringLiteral("[log]")));
        assert(content.contains(QStringLiteral("(agent_auto)")));
    }

    // ── 5. appendDailyLog - 追加而非覆盖 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        ProjectMemoryManager mgr(projectDir);
        mgr.appendDailyLog(QStringLiteral("log"), QStringLiteral("第一条"));
        mgr.appendDailyLog(QStringLiteral("decision"), QStringLiteral("第二条"));

        DailyMemoryWriter writer(projectDir);
        const QString content = readFile(writer.todayLogPath());

        assert(content.contains(QStringLiteral("第一条")));
        assert(content.contains(QStringLiteral("第二条")));

        // 第一条在前，第二条在后
        const int pos1 = content.indexOf(QStringLiteral("第一条"));
        const int pos2 = content.indexOf(QStringLiteral("第二条"));
        assert(pos1 < pos2);
    }

    // ── 6. remember - 记忆持久化到文件 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        ProjectMemoryManager mgr(projectDir);
        mgr.remember(QStringLiteral("用户偏好使用 Tab 缩进"),
                     QStringLiteral("user"));

        const QString l2Path = projectDir + QStringLiteral("/.workbuddy/memory/MEMORY.md");
        assert(QFileInfo::exists(l2Path));

        const QString content = readFile(l2Path);
        assert(content.contains(QStringLiteral("用户偏好使用 Tab 缩进")));
        assert(content.contains(QStringLiteral("Source: user")));
    }

    // ── 7. search - 按关键词检索 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        ProjectMemoryManager mgr(projectDir);
        mgr.appendDailyLog(QStringLiteral("log"),
                           QStringLiteral("Python 脚本执行成功"));
        mgr.appendDailyLog(QStringLiteral("log"),
                           QStringLiteral("C++ 编译完成"));
        mgr.appendDailyLog(QStringLiteral("decision"),
                           QStringLiteral("选择使用 Python 处理数据"));

        const MemoryEntryList results = mgr.search(QStringLiteral("Python"));
        assert(results.size() == 2);

        const MemoryEntryList results2 = mgr.search(QStringLiteral("C++"));
        assert(results2.size() == 1);
        assert(results2[0].content.contains(QStringLiteral("C++")));

        const MemoryEntryList results3 = mgr.search(QStringLiteral("nonexistent"));
        assert(results3.size() == 0);
    }

    // ── 8. recentLogs - 按天数限制 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        ProjectMemoryManager mgr(projectDir);
        mgr.appendDailyLog(QStringLiteral("log"), QStringLiteral("今日日志"));

        const MemoryEntryList results = mgr.recentLogs(1);
        assert(results.size() >= 1);

        bool found = false;
        for (const MemoryEntry &e : results) {
            if (e.content.contains(QStringLiteral("今日日志"))) {
                found = true;
                break;
            }
        }
        assert(found);

        const MemoryEntryList empty = mgr.recentLogs(0);
        assert(empty.size() == 0);
    }

    // ── 9. 敏感内容检测 - API Key ──
    {
        assert(ProjectMemoryManager::containsSensitiveContent(
            QStringLiteral("my api_key is abc123")));
        assert(ProjectMemoryManager::containsSensitiveContent(
            QStringLiteral("API_KEY=sk-test123")));
    }

    // ── 10. 敏感内容检测 - 密码 ──
    {
        assert(ProjectMemoryManager::containsSensitiveContent(
            QStringLiteral("password: mysecret123")));
        assert(ProjectMemoryManager::containsSensitiveContent(
            QStringLiteral("PASSWORD=admin")));
    }

    // ── 11. 敏感内容检测 - Token ──
    {
        assert(ProjectMemoryManager::containsSensitiveContent(
            QStringLiteral("token: eyJhbGciOiJIUzI1NiJ9")));
        assert(ProjectMemoryManager::containsSensitiveContent(
            QStringLiteral("Bearer abcdef123456")));
    }

    // ── 12. looksLikeSecret - sk- 密钥格式 ──
    {
        assert(ProjectMemoryManager::looksLikeSecret(
            QStringLiteral("sk-abcdefghijklmnopqrstuvwxyz123456")));
    }

    // ── 13. 正常内容不应被拒绝 ──
    {
        assert(!ProjectMemoryManager::containsSensitiveContent(
            QStringLiteral("执行计划成功")));
        assert(!ProjectMemoryManager::containsSensitiveContent(
            QStringLiteral("用户偏好使用 Tab 缩进")));
    }

    // ── 14. appendDailyLog - 长内容截断 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        // 生成超过 1000 字符的内容
        QString longContent;
        for (int i = 0; i < 100; ++i) {
            longContent += QStringLiteral("0123456789ABCDEF"); // 16 * 100 = 1600
        }

        ProjectMemoryManager mgr(projectDir);
        mgr.appendDailyLog(QStringLiteral("log"), longContent);

        DailyMemoryWriter writer(projectDir);
        const QString content = readFile(writer.todayLogPath());

        // 应包含 [truncated] 标记
        assert(content.contains(QStringLiteral("[truncated]")));
        // 不应该包含完整的长内容
        assert(!content.contains(longContent));
    }

    // ── 15. DailyMemoryWriter - 解析已存在的日志文件 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        ProjectMemoryManager mgr(projectDir);
        mgr.appendDailyLog(QStringLiteral("log"), QStringLiteral("日志1"));
        mgr.appendDailyLog(QStringLiteral("decision"), QStringLiteral("决策1"));

        const MemoryEntryList entries = mgr.recentLogs(7);
        assert(entries.size() >= 2);
    }

    // ── 16. appendDailyLog - 2000 字符截断 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();

        // 生成超过 2000 字符的内容（约 2500 字符）
        QString longContent;
        for (int i = 0; i < 250; ++i) {
            longContent += QStringLiteral("0123456789"); // 10 * 250 = 2500
        }

        ProjectMemoryManager mgr(projectDir);
        mgr.appendDailyLog(QStringLiteral("log"), longContent);

        DailyMemoryWriter writer(projectDir);
        const QString content = readFile(writer.todayLogPath());

        // 应包含 [truncated] 标记
        assert(content.contains(QStringLiteral("[truncated]")));
        // 不应该包含完整的长内容
        assert(!content.contains(longContent));
        // 截断后的内容应 ≤ ~2030 字符（2000 + "[truncated]" ≈ 2013）
        // 实际日志行还包含时间戳和前缀，所以用宽松范围
        assert(content.size() <= 2100);
    }

    // ── 17. buildCompressionPrompts - 分组正确 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();
        const QString memDir = projectDir + QStringLiteral("/.workbuddy/memory");
        QDir().mkpath(memDir);

        // 创建 2 天的日志文件（昨天和前天）
        const QDate yesterday = QDate::currentDate().addDays(-1);
        const QDate dayBefore = QDate::currentDate().addDays(-2);

        DailyMemoryWriter writer(projectDir);

        // Day 1
        {
            const QString filePath = memDir + QStringLiteral("/") +
                dayBefore.toString(QStringLiteral("yyyy-MM-dd")) + QStringLiteral(".md");
            const QString content = QStringLiteral(
                "# %1\n\n## Agent 执行日志\n\n"
                "10:00:00 [log] (agent_auto) 执行任务A\n"
                "11:00:00 [decision] (agent_auto) 决定使用方案X\n")
                .arg(dayBefore.toString(QStringLiteral("yyyy-MM-dd")));
            assert(writer.writeFile(filePath, content));
        }

        // Day 2
        {
            const QString filePath = memDir + QStringLiteral("/") +
                yesterday.toString(QStringLiteral("yyyy-MM-dd")) + QStringLiteral(".md");
            const QString content = QStringLiteral(
                "# %1\n\n## Agent 执行日志\n\n"
                "12:00:00 [log] (agent_auto) 执行任务B\n")
                .arg(yesterday.toString(QStringLiteral("yyyy-MM-dd")));
            assert(writer.writeFile(filePath, content));
        }

        ProjectMemoryManager mgr(projectDir);
        const QMap<QString, QString> prompts = mgr.buildCompressionPrompts(0);

        // 应该有至少 1 个 ISO 周
        assert(!prompts.isEmpty());

        // 检查周标签格式: YYYY-Www
        for (auto it = prompts.constBegin(); it != prompts.constEnd(); ++it) {
            const QString &key = it.key();
            assert(key.contains(QLatin1Char('-')));
            // 验证 key 模式: 4 digits + "-W" + 2 digits
            assert(key.length() >= 7);
            assert(key[4] == QLatin1Char('-'));

            const QString &prompt = it.value();
            assert(prompt.contains(QStringLiteral("[Memory Compression]")));
            assert(prompt.contains(QStringLiteral("Entries:")));
            assert(prompt.contains(QStringLiteral("执行任务")));
        }
    }

    // ── 18. applyCompression - 落盘 + 删除原始文件 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();
        const QString memDir = projectDir + QStringLiteral("/.workbuddy/memory");
        QDir().mkpath(memDir);

        const QDate pastDate = QDate::currentDate().addDays(-3);

        DailyMemoryWriter writer(projectDir);

        // 创建测试日志文件
        const QString logFileName = pastDate.toString(QStringLiteral("yyyy-MM-dd")) +
            QStringLiteral(".md");
        const QString logFilePath = memDir + QStringLiteral("/") + logFileName;
        const QString logContent = QStringLiteral(
            "# %1\n\n## Agent 执行日志\n\n"
            "10:00:00 [log] (agent_auto) 修复了登录Bug\n"
            "11:00:00 [decision] (agent_auto) 采用Redis做缓存\n")
            .arg(pastDate.toString(QStringLiteral("yyyy-MM-dd")));
        assert(writer.writeFile(logFilePath, logContent));

        // 验证原始文件存在
        assert(QFileInfo::exists(logFilePath));

        ProjectMemoryManager mgr(projectDir);

        // 构建压缩提示词
        const QMap<QString, QString> prompts = mgr.buildCompressionPrompts(0);
        assert(!prompts.isEmpty());

        // 手动构造周摘要
        const int weekNumber = pastDate.weekNumber();
        int isoYear = pastDate.year();
        if (pastDate.month() == 1 && weekNumber >= 52) {
            isoYear = pastDate.year() - 1;
        }
        const QString weekKey = QStringLiteral("%1-W%2")
                                    .arg(isoYear, 4, 10, QLatin1Char('0'))
                                    .arg(weekNumber, 2, 10, QLatin1Char('0'));

        QMap<QString, QString> summaries;
        summaries.insert(weekKey, QStringLiteral("本周修复了登录Bug，决定采用Redis做缓存。"));

        // 执行压缩
        assert(mgr.applyCompression(0, summaries));

        // 验证原始文件被删除
        assert(!QFileInfo::exists(logFilePath));

        // 验证压缩文件存在
        const QString compressedPath = memDir + QStringLiteral("/") + weekKey +
            QStringLiteral("-compressed.md");
        assert(QFileInfo::exists(compressedPath));

        // 验证压缩文件内容
        const QString compressedContent = readFile(compressedPath);
        assert(compressedContent.contains(QStringLiteral("# Compressed Memory — Week")));
        assert(compressedContent.contains(weekKey));
        assert(compressedContent.contains(QStringLiteral("Redis")));
    }

    // ── 19. buildMemorySection - 包含压缩摘要 ──
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString projectDir = tempDir.path();
        const QString memDir = projectDir + QStringLiteral("/.workbuddy/memory");
        QDir().mkpath(memDir);

        // 先创建压缩文件
        const QString weekKey = QStringLiteral("2026-W22");
        const QString compressedContent = QStringLiteral(
            "# Compressed Memory — Week 2026-W22\n\n"
            "本周修复了登录Bug，决定采用Redis做缓存。\n");
        const QString compressedPath = memDir + QStringLiteral("/") + weekKey +
            QStringLiteral("-compressed.md");

        DailyMemoryWriter writer(projectDir);
        assert(writer.writeFile(compressedPath, compressedContent));

        ProjectMemoryManager mgr(projectDir);
        const QString section = mgr.buildMemorySection();

        // 验证 section 包含压缩摘要
        assert(section.contains(QStringLiteral("[Compressed]")));
        assert(section.contains(QStringLiteral("Week 2026-W22")));
        assert(section.contains(QStringLiteral("Redis")));
    }

    return 0;
}
