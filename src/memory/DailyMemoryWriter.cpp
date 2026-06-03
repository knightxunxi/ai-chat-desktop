#include "memory/DailyMemoryWriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

namespace {

// 功能：解析单行日志格式 "HH:mm:ss [category] (source) content..."
// 格式示例: "20:15:30 [log] (agent_auto) 执行 workspace.write_text → OK"
bool parseLine(const QString &line, MemoryEntry &out)
{
    static const QRegularExpression re(
        QStringLiteral(
            R"(^(\d{2}:\d{2}:\d{2})\s+\[(log|decision|convention|preference)\]\s+\(([^)]+)\)\s+(.*)$)"));

    const QRegularExpressionMatch match = re.match(line);
    if (!match.hasMatch()) {
        return false;
    }

    const QString timeStr = match.captured(1);
    const QString category = match.captured(2);
    const QString source = match.captured(3);
    const QString content = match.captured(4).trimmed();

    // 用今天的日期 + 时间字符串构造 timestamp
    const QDate today = QDate::currentDate();
    const QStringList timeParts = timeStr.split(QLatin1Char(':'));
    if (timeParts.size() != 3) {
        return false;
    }

    out.timestamp = QDateTime(today,
                              QTime(timeParts[0].toInt(),
                                    timeParts[1].toInt(),
                                    timeParts[2].toInt()));
    out.category = category;
    out.source = source;
    out.content = content;
    return true;
}

// 功能：从文件名 "YYYY-MM-DD.md" 中提取日期。
QDate dateFromFileName(const QString &fileName)
{
    static const QRegularExpression dateRe(QStringLiteral(R"(^(\d{4})-(\d{2})-(\d{2})\.md$)"));
    const QRegularExpressionMatch match = dateRe.match(fileName);
    if (!match.hasMatch()) {
        return QDate();
    }
    return QDate(match.captured(1).toInt(),
                 match.captured(2).toInt(),
                 match.captured(3).toInt());
}

} // namespace

// --- MemoryEntry ---

MemoryEntry MemoryEntry::create(const QString &category,
                                const QString &content,
                                const QString &source)
{
    MemoryEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.category = category;
    entry.content = content;
    entry.source = source;
    return entry;
}

QString MemoryEntry::formatLine() const
{
    const QString timeStr = timestamp.toString(QStringLiteral("HH:mm:ss"));
    return QStringLiteral("%1 [%2] (%3) %4")
        .arg(timeStr, category, source, content);
}

// --- DailyMemoryWriter ---

DailyMemoryWriter::DailyMemoryWriter(const QString &projectDir)
    : m_projectDir(QDir::cleanPath(projectDir))
{
}

bool DailyMemoryWriter::append(const MemoryEntry &entry)
{
    const QString dirPath = memoryDir();
    const QString filePath = todayFilePath();

    // 确保目录存在
    QDir dir;
    if (!dir.mkpath(dirPath)) {
        return false;
    }

    const bool fileExists = QFileInfo::exists(filePath);

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Append | QFile::Text)) {
        return false;
    }

    QByteArray data;

    // 首次创建文件时写入头部
    if (!fileExists) {
        const QString header = QStringLiteral("# %1\n\n## Agent 执行日志\n\n")
                                   .arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
        data.append(header.toUtf8());
    }

    const QString logLine = entry.formatLine() + QStringLiteral("\n");
    data.append(logLine.toUtf8());

    if (file.write(data) != data.size()) {
        return false;
    }

    return true;
}

int DailyMemoryWriter::appendAll(const MemoryEntryList &entries)
{
    int count = 0;
    for (const MemoryEntry &entry : entries) {
        if (append(entry)) {
            ++count;
        }
    }
    return count;
}

MemoryEntryList DailyMemoryWriter::recentLogs(int days) const
{
    MemoryEntryList result;
    const QString dirPath = memoryDir();
    const QDir dir(dirPath);

    if (!dir.exists()) {
        return result;
    }

    const QDate today = QDate::currentDate();
    const QStringList files = dir.entryList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);

    // 从最新到最旧遍历
    QStringList reversed = files;
    std::reverse(reversed.begin(), reversed.end());

    for (const QString &fileName : reversed) {
        const QDate fileDate = dateFromFileName(fileName);
        if (!fileDate.isValid()) {
            continue;
        }

        // 检查是否在 N 天范围内
        const qint64 age = fileDate.daysTo(today);
        if (age < 0 || age >= days) {
            continue;
        }

        const QString filePath = dir.absoluteFilePath(fileName);
        const MemoryEntryList entries = parseFile(filePath);

        // 给每个 entry 设置正确的 timestamp（日期来自文件名，时间来自日志行）
        for (const MemoryEntry &entry : entries) {
            result.append(entry);
        }
    }

    return result;
}

MemoryEntryList DailyMemoryWriter::search(const QString &keyword) const
{
    MemoryEntryList result;
    const QString filePath = todayFilePath();

    if (!QFileInfo::exists(filePath)) {
        return result;
    }

    const MemoryEntryList allEntries = parseFile(filePath);
    for (const MemoryEntry &entry : allEntries) {
        if (entry.content.contains(keyword, Qt::CaseInsensitive)) {
            result.append(entry);
        }
    }

    return result;
}

QString DailyMemoryWriter::todayLogPath() const
{
    return todayFilePath();
}

QStringList DailyMemoryWriter::logFiles() const
{
    QStringList result;
    const QString dirPath = memoryDir();
    const QDir dir(dirPath);

    if (!dir.exists()) {
        return result;
    }

    static const QRegularExpression logFileRe(
        QStringLiteral(R"(^\d{4}-\d{2}-\d{2}\.md$)"));
    const QStringList files = dir.entryList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);

    for (const QString &fileName : files) {
        if (logFileRe.match(fileName).hasMatch()) {
            result.append(dir.absoluteFilePath(fileName));
        }
    }

    return result;
}

MemoryEntryList DailyMemoryWriter::entriesForDate(const QDate &date) const
{
    const QString filePath = QDir::cleanPath(
        memoryDir() + QStringLiteral("/") +
        date.toString(QStringLiteral("yyyy-MM-dd")) +
        QStringLiteral(".md"));

    if (!QFileInfo::exists(filePath)) {
        return MemoryEntryList();
    }

    return parseFile(filePath);
}

bool DailyMemoryWriter::writeFile(const QString &filePath, const QString &content)
{
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
        return false;
    }

    const QByteArray data = content.toUtf8();
    return file.write(data) == data.size();
}

bool DailyMemoryWriter::removeFile(const QString &filePath)
{
    return QFile::remove(filePath);
}

QString DailyMemoryWriter::memoryDir() const
{
    return QDir::cleanPath(m_projectDir + QStringLiteral("/.workbuddy/memory"));
}

QString DailyMemoryWriter::todayFilePath() const
{
    return QDir::cleanPath(
        memoryDir() + QStringLiteral("/") +
        QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")) +
        QStringLiteral(".md"));
}

MemoryEntryList DailyMemoryWriter::parseFile(const QString &filePath)
{
    MemoryEntryList result;

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return result;
    }

    const QDate fileDate = dateFromFileName(QFileInfo(filePath).fileName());

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty()) {
            continue;
        }

        MemoryEntry entry;
        if (!parseLine(line, entry)) {
            continue;
        }

        // 用文件日期修正 timestamp 的日期部分
        if (fileDate.isValid()) {
            entry.timestamp = QDateTime(fileDate, entry.timestamp.time());
        }

        result.append(entry);
    }

    return result;
}
