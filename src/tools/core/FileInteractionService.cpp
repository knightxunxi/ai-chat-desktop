#include "tools/core/FileInteractionService.h"

#include "support/AppLogger.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace {

constexpr int DirectoryHashLength = 12;

QString logSafePathSummary(const QString &path)
{
    return FileInteractionService::pathSummary(path);
}

bool containsNullByte(const QByteArray &content)
{
    return content.contains('\0');
}

QString bytesLabel(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }

    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KiB").arg(bytes / 1024);
    }

    return QStringLiteral("%1 MiB").arg(bytes / (1024 * 1024));
}

ToolResult failedWithLog(const QString &operation, const QString &path, const QString &error)
{
    AppLogger::warning(QStringLiteral("FileInteraction"),
                       QStringLiteral("%1 failed. path=%2 error=%3")
                           .arg(operation, logSafePathSummary(path), error));
    return ToolResult::failure(error);
}

ToolResult succeededWithLog(const QString &operation, const QString &path, const QString &output)
{
    AppLogger::info(QStringLiteral("FileInteraction"),
                    QStringLiteral("%1 succeeded. path=%2 outputLength=%3")
                        .arg(operation, logSafePathSummary(path))
                        .arg(output.size()));
    return ToolResult::success(output);
}

} // namespace

namespace FileInteractionService {

ToolResult readTextFile(const QString &filePath, qint64 maxBytes)
{
    const QString trimmedPath = filePath.trimmed();
    if (trimmedPath.isEmpty()) {
        return failedWithLog(QStringLiteral("read_text_file"), filePath, QStringLiteral("File path is empty."));
    }

    QFileInfo fileInfo(trimmedPath);
    if (!fileInfo.exists()) {
        return failedWithLog(QStringLiteral("read_text_file"), trimmedPath, QStringLiteral("File does not exist."));
    }

    if (!fileInfo.isFile()) {
        return failedWithLog(QStringLiteral("read_text_file"), trimmedPath, QStringLiteral("Path is not a file."));
    }

    if (fileInfo.size() > maxBytes) {
        return failedWithLog(
            QStringLiteral("read_text_file"),
            trimmedPath,
            QStringLiteral("File is too large. Limit is %1.").arg(bytesLabel(maxBytes)));
    }

    QFile file(trimmedPath);
    if (!file.open(QFile::ReadOnly)) {
        return failedWithLog(QStringLiteral("read_text_file"), trimmedPath, QStringLiteral("Failed to open file for reading."));
    }

    const QByteArray content = file.readAll();
    if (containsNullByte(content)) {
        return failedWithLog(QStringLiteral("read_text_file"), trimmedPath, QStringLiteral("File appears to be binary."));
    }

    return succeededWithLog(QStringLiteral("read_text_file"), trimmedPath, QString::fromUtf8(content));
}

ToolResult listDirectory(const QString &directoryPath, int maxEntries)
{
    const QString trimmedPath = directoryPath.trimmed();
    if (trimmedPath.isEmpty()) {
        return failedWithLog(QStringLiteral("list_directory"), directoryPath, QStringLiteral("Directory path is empty."));
    }

    QFileInfo directoryInfo(trimmedPath);
    if (!directoryInfo.exists()) {
        return failedWithLog(QStringLiteral("list_directory"), trimmedPath, QStringLiteral("Directory does not exist."));
    }

    if (!directoryInfo.isDir()) {
        return failedWithLog(QStringLiteral("list_directory"), trimmedPath, QStringLiteral("Path is not a directory."));
    }

    QDir directory(trimmedPath);
    const QFileInfoList entries = directory.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    QStringList lines;
    int count = 0;
    for (const QFileInfo &entry : entries) {
        if (count >= maxEntries) {
            lines.append(QStringLiteral("... (%1 more entries)").arg(entries.size() - count));
            break;
        }

        lines.append(entry.isDir()
                         ? QStringLiteral("[DIR]  %1").arg(entry.fileName())
                         : QStringLiteral("[FILE] %1 (%2)").arg(entry.fileName(), bytesLabel(entry.size())));
        ++count;
    }

    if (lines.isEmpty()) {
        lines.append(QStringLiteral("(empty directory)"));
    }

    return succeededWithLog(QStringLiteral("list_directory"), trimmedPath, lines.join(QLatin1Char('\n')));
}

ToolResult saveTextFile(const QString &filePath, const QString &content, bool allowOverwrite)
{
    const QString trimmedPath = filePath.trimmed();
    if (trimmedPath.isEmpty()) {
        return failedWithLog(QStringLiteral("save_text_file"), filePath, QStringLiteral("File path is empty."));
    }

    QFileInfo fileInfo(trimmedPath);
    QDir directory = fileInfo.absoluteDir();
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        return failedWithLog(QStringLiteral("save_text_file"), trimmedPath, QStringLiteral("Failed to create parent directory."));
    }

    if (fileInfo.exists() && !allowOverwrite) {
        return failedWithLog(QStringLiteral("save_text_file"), trimmedPath, QStringLiteral("File already exists."));
    }

    QFile file(trimmedPath);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        return failedWithLog(QStringLiteral("save_text_file"), trimmedPath, QStringLiteral("Failed to open file for writing."));
    }

    const QByteArray encoded = content.toUtf8();
    if (file.write(encoded) != encoded.size()) {
        return failedWithLog(QStringLiteral("save_text_file"), trimmedPath, QStringLiteral("Failed to write all content."));
    }

    return succeededWithLog(
        QStringLiteral("save_text_file"),
        trimmedPath,
        QStringLiteral("Saved %1 characters to %2.").arg(content.size()).arg(fileInfo.fileName()));
}

ToolResult validateOpenPath(const QString &path)
{
    const QString trimmedPath = path.trimmed();
    if (trimmedPath.isEmpty()) {
        return failedWithLog(QStringLiteral("validate_open_path"), path, QStringLiteral("Path is empty."));
    }

    QFileInfo pathInfo(trimmedPath);
    if (!pathInfo.exists()) {
        return failedWithLog(QStringLiteral("validate_open_path"), trimmedPath, QStringLiteral("Path does not exist."));
    }

    if (!pathInfo.isFile() && !pathInfo.isDir()) {
        return failedWithLog(QStringLiteral("validate_open_path"), trimmedPath, QStringLiteral("Path is not a file or directory."));
    }

    AppLogger::info(QStringLiteral("FileInteraction"),
                    QStringLiteral("validate_open_path succeeded. path=%1")
                        .arg(logSafePathSummary(trimmedPath)));
    return ToolResult::success(QStringLiteral("Path is ready to open."));
}

QString pathSummary(const QString &path)
{
    QFileInfo fileInfo(path);
    const QString directory = fileInfo.absoluteDir().absolutePath();
    const QByteArray hash = QCryptographicHash::hash(directory.toUtf8(), QCryptographicHash::Sha256)
                                .toHex()
                                .left(DirectoryHashLength);
    return QStringLiteral("name=%1 dirHash=%2").arg(fileInfo.fileName(), QString::fromLatin1(hash));
}

} // namespace FileInteractionService
