#include "memory/ProjectMemoryManager.h"
#include "memory/DailyMemoryWriter.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QStringList>

namespace {

constexpr int kRecentLogsForSection = 50; // buildMemorySection 中取最近 50 条 L3 日志
constexpr int kMaxDaysForSection = 14;     // buildMemorySection 中查最近 14 天
constexpr int kMaxMemorySectionChars = 30000; // buildMemorySection 总字符硬上限
constexpr int kMaxDailyLogChars = 2000;    // appendDailyLog 单条截断上限

// 功能：检查文本是否包含敏感关键词（大小写不敏感）。
bool containsSensitiveKeyword(const QString &text)
{
    static const QStringList keywords = {
        QStringLiteral("api_key"),
        QStringLiteral("apikey"),
        QStringLiteral("token"),
        QStringLiteral("password"),
        QStringLiteral("secret"),
        QStringLiteral("bearer"),
        QStringLiteral("credential"),
        QStringLiteral("private_key"),
        QStringLiteral("privatekey"),
    };

    const QString lower = text.toLower();
    for (const QString &kw : keywords) {
        if (lower.contains(kw)) {
            return true;
        }
    }

    return false;
}

// 功能：检查文本是否像密钥/Token 格式。
bool matchesSecretPattern(const QString &text)
{
    // 检查 "sk-" 开头的 OpenAI/类似密钥格式
    static const QRegularExpression skPattern(
        QStringLiteral(R"(\bsk-[A-Za-z0-9]{20,}\b)"));
    // 检查 base64 风格的 token (长随机字符串)
    static const QRegularExpression base64TokenPattern(
        QStringLiteral(R"(\b[A-Za-z0-9+/=]{40,}\b)"));

    if (skPattern.match(text).hasMatch()) {
        return true;
    }

    if (base64TokenPattern.match(text).hasMatch()) {
        return true;
    }

    return false;
}

} // namespace

ProjectMemoryManager::ProjectMemoryManager(const QString &projectDir)
    : m_projectDir(QDir::cleanPath(projectDir))
{
}

QString ProjectMemoryManager::buildMemorySection() const
{
    QString section;
    bool hasContent = false;

    // L1: 用户级 ~/.codex/MEMORY.md
    const QString l1 = readFile(L1Path());
    if (!l1.isEmpty()) {
        section += QStringLiteral("[Long-term Memory — User Level]\n");
        section += l1;
        section += QStringLiteral("\n");
        hasContent = true;
    }

    // L2: 项目级 .workbuddy/memory/MEMORY.md
    const QString l2 = readFile(L2Path());
    if (!l2.isEmpty()) {
        if (hasContent) {
            section += QStringLiteral("\n");
        }
        section += QStringLiteral("[Long-term Memory — Project Level]\n");
        section += l2;
        section += QStringLiteral("\n");
        hasContent = true;
    }

    // L3: 最近 50 条每日日志（14 天内）
    DailyMemoryWriter dailyWriter(m_projectDir);
    const MemoryEntryList recent = dailyWriter.recentLogs(kMaxDaysForSection);
    if (!recent.isEmpty()) {
        if (hasContent) {
            section += QStringLiteral("\n");
        }
        section += QStringLiteral("[Recent Activity]\n");

        // 取最近 kRecentLogsForSection 条（从最新的开始）
        const int count = qMin(recent.size(), kRecentLogsForSection);
        // recentLogs 返回从旧到新排列，需要取最后 count 条
        for (int i = recent.size() - count; i < recent.size(); ++i) {
            section += recent[i].formatLine();
            section += QStringLiteral("\n");
        }
        hasContent = true;
    }

    // V13.2: 注入压缩摘要
    const QStringList compressed = compressedFiles();
    if (!compressed.isEmpty()) {
        if (hasContent) {
            section += QStringLiteral("\n");
        }
        for (const QString &filePath : compressed) {
            const QString content = readFile(filePath, kMaxMemorySectionChars);
            if (content.isEmpty()) {
                continue;
            }
            // 提取摘要文本（跳过标题行 "# Compressed Memory — Week YYYY-Www"）
            QStringList lines = content.split(QLatin1Char('\n'));
            if (lines.size() >= 2) {
                const QString summary = lines.mid(1).join(QStringLiteral(" ")).trimmed();
                // 从文件名提取周标签
                QFileInfo fi(filePath);
                const QString baseName = fi.completeBaseName(); // "YYYY-Www-compressed"
                const QString weekLabel = baseName.left(baseName.lastIndexOf(QLatin1Char('-')));
                if (!summary.isEmpty()) {
                    section += QStringLiteral("[Compressed] Week %1: %2\n")
                                   .arg(weekLabel, summary);
                    hasContent = true;
                }
            }
        }
    }

    if (!hasContent) {
        return QString();
    }

    // V13.2: 总字符硬上限截断
    if (section.size() > kMaxMemorySectionChars) {
        section = section.left(kMaxMemorySectionChars);
        section += QStringLiteral("\n[memory truncated at budget]");
    }

    return section.trimmed();
}

void ProjectMemoryManager::appendDailyLog(const QString &category, const QString &entry)
{
    if (entry.trimmed().isEmpty()) {
        return;
    }

    // 安全检查
    QString safeContent = entry.trimmed();
    if (safeContent.size() > kMaxDailyLogChars) {
        safeContent = safeContent.left(kMaxDailyLogChars) + QStringLiteral(" [truncated]");
    }

    DailyMemoryWriter writer(m_projectDir);
    const MemoryEntry memoryEntry = MemoryEntry::create(category, safeContent,
                                                        QStringLiteral("agent_auto"));
    writer.append(memoryEntry);
}

void ProjectMemoryManager::remember(const QString &fact, const QString &source)
{
    if (fact.trimmed().isEmpty()) {
        return;
    }

    if (containsSensitiveContent(fact)) {
        return;
    }

    const QString dirPath = QDir::cleanPath(m_projectDir + QStringLiteral("/.workbuddy/memory"));
    const QString filePath = QDir::cleanPath(dirPath + QStringLiteral("/MEMORY.md"));

    // 确保目录存在
    QDir dir;
    if (!dir.mkpath(dirPath)) {
        return;
    }

    const bool fileExists = QFileInfo::exists(filePath);

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Append | QFile::Text)) {
        return;
    }

    QByteArray data;

    if (!fileExists) {
        const QString header = QStringLiteral(
            "# Project Memory\n\n"
            "Only store information the user explicitly asked to remember.\n\n");
        data.append(header.toUtf8());
    }

    const QString entry = QStringLiteral("## %1\nSource: %2\n\n%3\n\n")
                              .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
                                   source,
                                   fact.trimmed());
    data.append(entry.toUtf8());
    file.write(data);
}

MemoryEntryList ProjectMemoryManager::recentLogs(int days) const
{
    DailyMemoryWriter writer(m_projectDir);
    return writer.recentLogs(days);
}

MemoryEntryList ProjectMemoryManager::search(const QString &keyword) const
{
    DailyMemoryWriter writer(m_projectDir);
    return writer.search(keyword);
}

bool ProjectMemoryManager::containsSensitiveContent(const QString &text)
{
    if (containsSensitiveKeyword(text)) {
        return true;
    }

    if (looksLikeSecret(text)) {
        return true;
    }

    return false;
}

bool ProjectMemoryManager::looksLikeSecret(const QString &text)
{
    return matchesSecretPattern(text);
}

QString ProjectMemoryManager::L1Path()
{
    return QDir::cleanPath(QDir::homePath() + QStringLiteral("/.codex/MEMORY.md"));
}

QString ProjectMemoryManager::L2Path() const
{
    return QDir::cleanPath(m_projectDir + QStringLiteral("/.workbuddy/memory/MEMORY.md"));
}

QString ProjectMemoryManager::readFile(const QString &path, int maxBytes)
{
    if (path.isEmpty()) {
        return QString();
    }

    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return QString();
    }

    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return QString();
    }

    QByteArray content = file.read(maxBytes + 1);
    if (content.size() > maxBytes) {
        content.truncate(maxBytes);
    }

    return QString::fromUtf8(content).trimmed();
}

// V13.2: 列出所有压缩文件（*-compressed.md）
QStringList ProjectMemoryManager::compressedFiles() const
{
    QStringList result;
    const QString dirPath = QDir::cleanPath(m_projectDir + QStringLiteral("/.workbuddy/memory"));
    const QDir dir(dirPath);

    if (!dir.exists()) {
        return result;
    }

    static const QRegularExpression compressedRe(
        QStringLiteral(R"(^\d{4}-W\d{2}-compressed\.md$)"));
    const QStringList files = dir.entryList({QStringLiteral("*-compressed.md")},
                                            QDir::Files, QDir::Name);

    for (const QString &fileName : files) {
        if (compressedRe.match(fileName).hasMatch()) {
            result.append(dir.absoluteFilePath(fileName));
        }
    }

    return result;
}

// V13.2: 记忆压缩 — 阶段1：构建 LLM 压缩提示词
QMap<QString, QString> ProjectMemoryManager::buildCompressionPrompts(int olderThanDays) const
{
    QMap<QString, QString> prompts;
    DailyMemoryWriter writer(m_projectDir);

    const QStringList files = writer.logFiles();
    const QDate today = QDate::currentDate();

    // 按 ISO 周分组的条目: QMap<weekKey, QStringList>
    QMap<QString, QStringList> weekEntries;

    for (const QString &filePath : files) {
        // 从文件名提取日期
        const QFileInfo fi(filePath);
        static const QRegularExpression dateRe(QStringLiteral(R"(^(\d{4})-(\d{2})-(\d{2})\.md$)"));
        const QRegularExpressionMatch match = dateRe.match(fi.fileName());
        if (!match.hasMatch()) {
            continue;
        }
        const QDate fileDate(match.captured(1).toInt(),
                             match.captured(2).toInt(),
                             match.captured(3).toInt());
        if (!fileDate.isValid()) {
            continue;
        }

        // 检查是否超过 olderThanDays 天
        const qint64 age = fileDate.daysTo(today);
        if (age < olderThanDays) {
            continue;
        }

        // 获取 ISO 周号
        const int weekNumber = fileDate.weekNumber();
        const int year = fileDate.year();
        // 对于 ISO 周跨年情况：如果日期在1月但周号≥52，则属于前一年
        int isoYear = year;
        if (fileDate.month() == 1 && weekNumber >= 52) {
            isoYear = year - 1;
        }
        const QString weekKey = QStringLiteral("%1-W%2")
                                    .arg(isoYear, 4, 10, QLatin1Char('0'))
                                    .arg(weekNumber, 2, 10, QLatin1Char('0'));

        // 解析该文件的条目
        const MemoryEntryList entries = writer.entriesForDate(fileDate);
        for (const MemoryEntry &entry : entries) {
            const QString line = QStringLiteral("- %1 [%2] %3")
                                     .arg(entry.timestamp.toString(QStringLiteral("HH:mm")),
                                          entry.category,
                                          entry.content);
            weekEntries[weekKey].append(line);
        }
    }

    // 为每个周构建提示词
    for (auto it = weekEntries.constBegin(); it != weekEntries.constEnd(); ++it) {
        const QString &weekKey = it.key();
        const QStringList &lines = it.value();

        if (lines.isEmpty()) {
            continue;
        }

        QString prompt;
        prompt += QStringLiteral("[Memory Compression]\n");
        prompt += QStringLiteral("Compress the following daily memory entries for week %1 into a concise summary\n")
                      .arg(weekKey);
        prompt += QStringLiteral("(max 500 Chinese characters). Preserve: key decisions, conventions established,\n");
        prompt += QStringLiteral("bugs fixed, and architectural changes. Discard: routine status updates.\n");
        prompt += QStringLiteral("\nEntries:\n");
        for (const QString &line : lines) {
            prompt += line;
            prompt += QStringLiteral("\n");
        }

        prompts.insert(weekKey, prompt);
    }

    return prompts;
}

// V13.2: 记忆压缩 — 阶段2：将 LLM 摘要落盘，删除原始日志
bool ProjectMemoryManager::applyCompression(int olderThanDays,
                                            const QMap<QString, QString> &weekSummaries)
{
    DailyMemoryWriter writer(m_projectDir);
    const QStringList files = writer.logFiles();
    const QDate today = QDate::currentDate();

    // 删除 olderThanDays 天前的所有原始日志文件
    for (const QString &filePath : files) {
        const QFileInfo fi(filePath);
        static const QRegularExpression dateRe(QStringLiteral(R"(^(\d{4})-(\d{2})-(\d{2})\.md$)"));
        const QRegularExpressionMatch match = dateRe.match(fi.fileName());
        if (!match.hasMatch()) {
            continue;
        }
        const QDate fileDate(match.captured(1).toInt(),
                             match.captured(2).toInt(),
                             match.captured(3).toInt());
        if (!fileDate.isValid()) {
            continue;
        }

        const qint64 age = fileDate.daysTo(today);
        if (age >= olderThanDays) {
            writer.removeFile(filePath);
        }
    }

    // 写入压缩文件
    for (auto it = weekSummaries.constBegin(); it != weekSummaries.constEnd(); ++it) {
        const QString &weekKey = it.key();
        const QString &summary = it.value();

        const QString memoryDirPath = QDir::cleanPath(
            m_projectDir + QStringLiteral("/.workbuddy/memory"));
        const QString compressedPath = QDir::cleanPath(
            memoryDirPath + QStringLiteral("/") + weekKey +
            QStringLiteral("-compressed.md"));

        QString content;
        content += QStringLiteral("# Compressed Memory — Week %1\n\n").arg(weekKey);
        content += summary;
        content += QStringLiteral("\n");

        if (!writer.writeFile(compressedPath, content)) {
            return false;
        }
    }

    return true;
}
