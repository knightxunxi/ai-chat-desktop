#include "tools/core/FileInteractionService.h"

#include "support/AppLogger.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
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

// V18: 精确编辑 — 在文件中查找 old_str 并替换为 new_str
ToolResult editTextFile(const QString &filePath, const QString &oldStr, const QString &newStr)
{
    const QString trimmedPath = filePath.trimmed();
    if (trimmedPath.isEmpty())
        return failedWithLog(QStringLiteral("edit_text"), filePath, QStringLiteral("File path is empty."));
    if (oldStr.isEmpty())
        return failedWithLog(QStringLiteral("edit_text"), trimmedPath, QStringLiteral("old_str is empty."));

    QFileInfo fileInfo(trimmedPath);
    if (!fileInfo.exists())
        return failedWithLog(QStringLiteral("edit_text"), trimmedPath, QStringLiteral("File does not exist."));
    if (!fileInfo.isFile())
        return failedWithLog(QStringLiteral("edit_text"), trimmedPath, QStringLiteral("Path is not a file."));

    QFile file(trimmedPath);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return failedWithLog(QStringLiteral("edit_text"), trimmedPath, QStringLiteral("Failed to open file for reading."));

    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    const int matchCount = content.count(oldStr);
    if (matchCount == 0)
        return failedWithLog(QStringLiteral("edit_text"), trimmedPath,
            QStringLiteral("old_str not found in file."));
    if (matchCount > 1)
        return failedWithLog(QStringLiteral("edit_text"), trimmedPath,
            QStringLiteral("old_str found %1 times (expected exactly 1). Provide more context to make it unique.").arg(matchCount));

    QString updated = content;
    updated.replace(oldStr, newStr);

    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        return failedWithLog(QStringLiteral("edit_text"), trimmedPath, QStringLiteral("Failed to open file for writing."));

    const QByteArray encoded = updated.toUtf8();
    if (file.write(encoded) != encoded.size())
        return failedWithLog(QStringLiteral("edit_text"), trimmedPath, QStringLiteral("Failed to write all content."));

    return succeededWithLog(QStringLiteral("edit_text"), trimmedPath,
        QStringLiteral("Replaced 1 occurrence in %1.").arg(fileInfo.fileName()));
}

// V18: 内容搜索 — 正则表达式搜索文件内容
ToolResult grep(const QString &searchPath, const QString &pattern, const QString &fileGlob,
                bool ignoreCase, int maxResults)
{
    const QString trimmedPath = searchPath.trimmed();
    if (trimmedPath.isEmpty())
        return failedWithLog(QStringLiteral("grep"), searchPath, QStringLiteral("Search path is empty."));
    if (pattern.isEmpty())
        return failedWithLog(QStringLiteral("grep"), trimmedPath, QStringLiteral("Search pattern is empty."));

    QFileInfo pathInfo(trimmedPath);
    if (!pathInfo.exists())
        return failedWithLog(QStringLiteral("grep"), trimmedPath, QStringLiteral("Path does not exist."));

    QRegularExpression regex(pattern);
    if (!regex.isValid())
        return failedWithLog(QStringLiteral("grep"), trimmedPath,
            QStringLiteral("Invalid regex: %1").arg(regex.errorString()));

    if (ignoreCase)
        regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    QStringList results;
    const int maxFileSize = 1024 * 1024; // 1 MiB per file

    auto searchFile = [&](const QString &filePath) {
        if (results.size() >= maxResults) return;
        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) return;
        if (file.size() > maxFileSize) { file.close(); return; }
        int lineNo = 0;
        while (!file.atEnd() && results.size() < maxResults) {
            const QString line = QString::fromUtf8(file.readLine()).trimmed();
            ++lineNo;
            if (regex.match(line).hasMatch()) {
                const QString displayLine = line.length() > 120 ? line.left(120) + QStringLiteral("...") : line;
                results.append(QStringLiteral("%1:%2: %3").arg(filePath, QString::number(lineNo), displayLine));
            }
        }
    };

    if (pathInfo.isFile()) {
        searchFile(trimmedPath);
    } else {
        QDirIterator it(trimmedPath, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext() && results.size() < maxResults) {
            const QString filePath = it.next();
            if (!fileGlob.isEmpty()) {
                QRegularExpression globRegex(QRegularExpression::wildcardToRegularExpression(fileGlob));
                if (!globRegex.match(QFileInfo(filePath).fileName()).hasMatch()) continue;
            }
            searchFile(filePath);
        }
    }

    if (results.isEmpty()) {
        return succeededWithLog(QStringLiteral("grep"), trimmedPath,
            QStringLiteral("No matches found for pattern: %1").arg(pattern));
    }

    QString output = results.join(QStringLiteral("\n"));
    if (results.size() >= maxResults)
        output += QStringLiteral("\n... (truncated at %1 results)").arg(maxResults);

    return succeededWithLog(QStringLiteral("grep"), trimmedPath, output);
}

// V18: 递归删除目录
ToolResult deleteDirectory(const QString &directoryPath)
{
    const QString trimmedPath = directoryPath.trimmed();
    if (trimmedPath.isEmpty())
        return failedWithLog(QStringLiteral("delete_directory"), directoryPath, QStringLiteral("Directory path is empty."));

    QFileInfo dirInfo(trimmedPath);
    if (!dirInfo.exists())
        return failedWithLog(QStringLiteral("delete_directory"), trimmedPath, QStringLiteral("Directory does not exist."));
    if (!dirInfo.isDir())
        return failedWithLog(QStringLiteral("delete_directory"), trimmedPath, QStringLiteral("Path is not a directory."));

    // 保护系统关键目录
    static const QStringList protectedDirs = {
        QStringLiteral("C:/Windows"), QStringLiteral("C:/Program Files"),
        QStringLiteral("C:/Program Files (x86)"), QStringLiteral("/System"),
        QStringLiteral("/etc"), QStringLiteral("/usr"), QStringLiteral("/bin")
    };
    const QString normalized = QDir::toNativeSeparators(QDir::cleanPath(trimmedPath));
    for (const QString &protected_ : protectedDirs) {
        if (normalized.startsWith(QDir::toNativeSeparators(QDir::cleanPath(protected_)), Qt::CaseInsensitive)) {
            return failedWithLog(QStringLiteral("delete_directory"), trimmedPath,
                QStringLiteral("Cannot delete system directory."));
        }
    }

    QDir dir(trimmedPath);
    const int fileCount = static_cast<int>(dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name).size());

    if (!dir.removeRecursively())
        return failedWithLog(QStringLiteral("delete_directory"), trimmedPath,
            QStringLiteral("Failed to delete directory."));

    return succeededWithLog(QStringLiteral("delete_directory"), trimmedPath,
        QStringLiteral("Deleted directory with %1 entries.").arg(fileCount));
}

// V18.3: 文件复制
ToolResult copyFile(const QString &sourcePath, const QString &targetPath)
{
    if (sourcePath.trimmed().isEmpty()) return ToolResult::failure(QStringLiteral("Source path is empty."));
    if (targetPath.trimmed().isEmpty()) return ToolResult::failure(QStringLiteral("Target path is empty."));

    const QString src = QDir::toNativeSeparators(sourcePath.trimmed());
    const QString dst = QDir::toNativeSeparators(targetPath.trimmed());

    QFileInfo srcInfo(src);
    if (!srcInfo.exists()) return ToolResult::failure(QStringLiteral("Source does not exist."));

    QFileInfo dstInfo(dst);
    if (srcInfo.isDir()) {
        // 目录复制：创建目标目录
        if (dstInfo.exists() && dstInfo.isFile())
            return ToolResult::failure(QStringLiteral("Cannot overwrite a file with a directory."));
        if (!dstInfo.exists() && !QDir().mkpath(dst)) {
            return ToolResult::failure(QStringLiteral("Failed to create target directory."));
        }

        QDirIterator it(src, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        int copied = 0;
        while (it.hasNext()) {
            it.next();
            const QString relPath = QDir(src).relativeFilePath(it.filePath());
            const QString targetFile = QDir(dst).filePath(relPath);
            if (it.fileInfo().isDir()) {
                if (!QDir().mkpath(targetFile)) {
                    return ToolResult::failure(QStringLiteral("Failed to create target subdirectory: %1").arg(relPath));
                }
            } else {
                if (!QDir().mkpath(QFileInfo(targetFile).absolutePath())) {
                    return ToolResult::failure(QStringLiteral("Failed to create parent directory for: %1").arg(relPath));
                }
                if (QFileInfo::exists(targetFile)) {
                    return ToolResult::failure(QStringLiteral("Target file already exists during directory copy: %1").arg(relPath));
                }
                if (!QFile::copy(it.filePath(), targetFile)) {
                    return ToolResult::failure(QStringLiteral("Failed to copy file: %1").arg(relPath));
                }
                copied++;
            }
        }
        return ToolResult::success(QStringLiteral("Copied directory: %1 files.").arg(copied));
    }

    if (dstInfo.exists()) return ToolResult::failure(QStringLiteral("Target already exists."));
    QDir().mkpath(dstInfo.absolutePath());
    if (!QFile::copy(src, dst)) return ToolResult::failure(QStringLiteral("Failed to copy file."));
    return ToolResult::success(QStringLiteral("Copied: %1").arg(QFileInfo(dst).fileName()));
}

// V18.3: 文件移动/重命名
ToolResult moveFile(const QString &sourcePath, const QString &targetPath)
{
    if (sourcePath.trimmed().isEmpty()) return ToolResult::failure(QStringLiteral("Source path is empty."));
    if (targetPath.trimmed().isEmpty()) return ToolResult::failure(QStringLiteral("Target path is empty."));

    const QString src = QDir::toNativeSeparators(sourcePath.trimmed());
    const QString dst = QDir::toNativeSeparators(targetPath.trimmed());

    QFileInfo srcInfo(src);
    if (!srcInfo.exists()) return ToolResult::failure(QStringLiteral("Source does not exist."));
    if (QFileInfo::exists(dst)) return ToolResult::failure(QStringLiteral("Target already exists."));

    QDir().mkpath(QFileInfo(dst).absolutePath());
    if (!QFile::rename(src, dst)) {
        // rename 失败 → 跨盘符场景 → copy + remove
        if (srcInfo.isDir()) {
            const ToolResult cp = copyFile(src, dst);
            if (!cp.ok) {
                QDir(dst).removeRecursively();
                return ToolResult::failure(QStringLiteral("Move failed: cannot copy directory: %1").arg(cp.error));
            }
            if (!QDir(src).removeRecursively()) {
                return ToolResult::failure(QStringLiteral("Move failed: copied directory but could not remove source."));
            }
        } else {
            if (!QFile::copy(src, dst)) return ToolResult::failure(QStringLiteral("Move failed: cannot copy file."));
            if (!QFile::remove(src)) {
                QFile::remove(dst);
                return ToolResult::failure(QStringLiteral("Move failed: copied file but could not remove source."));
            }
        }
    }
    return ToolResult::success(QStringLiteral("Moved: %1").arg(QFileInfo(dst).fileName()));
}

// V18.3: 追加文本
ToolResult appendTextFile(const QString &filePath, const QString &content)
{
    const QString trim = filePath.trimmed();
    if (trim.isEmpty()) return ToolResult::failure(QStringLiteral("File path is empty."));

    QDir().mkpath(QFileInfo(trim).absolutePath());
    QFile file(trim);
    if (!file.open(QFile::WriteOnly | QFile::Append | QFile::Text))
        return ToolResult::failure(QStringLiteral("Failed to open file for appending."));
    const QByteArray encoded = content.toUtf8();
    if (file.write(encoded) != encoded.size()) {
        return ToolResult::failure(QStringLiteral("Failed to append all content."));
    }
    return ToolResult::success(QStringLiteral("Appended %1 chars to %2.").arg(content.size()).arg(QFileInfo(trim).fileName()));
}

// V18.3: 获取文件信息
ToolResult getFileInfo(const QString &filePath)
{
    const QString trim = filePath.trimmed();
    if (trim.isEmpty()) return ToolResult::failure(QStringLiteral("File path is empty."));

    QFileInfo info(trim);
    if (!info.exists()) return ToolResult::failure(QStringLiteral("File does not exist."));

    const QString type = info.isDir() ? QStringLiteral("directory") : QStringLiteral("file");
    return ToolResult::success(QStringLiteral(
        "Path: %1\nType: %2\nSize: %3 bytes\nCreated: %4\nModified: %5\nReadable: %6, Writable: %7")
        .arg(info.absoluteFilePath(), type, QString::number(info.size()),
             info.birthTime().toString(Qt::ISODate),
             info.lastModified().toString(Qt::ISODate),
             info.isReadable() ? QStringLiteral("yes") : QStringLiteral("no"),
             info.isWritable() ? QStringLiteral("yes") : QStringLiteral("no")));
}

} // namespace FileInteractionService
