#include "tools/ProjectFindService.h"

#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>

// 学习注释：ProjectFindService 在项目目录内按 glob 模式搜索文件。
// 自动排除 .git、build-*、node_modules 等常见非源码目录。

namespace {

// 功能：将简单 glob 模式转为 QRegularExpression；使用模块：findFiles。
// 支持 * 和 ? 通配符。
QRegularExpression globToRegex(const QString &pattern)
{
    QString regexPattern;
    regexPattern += QLatin1Char('^');
    for (int i = 0; i < pattern.size(); ++i) {
        const QChar ch = pattern.at(i);
        if (ch == QLatin1Char('*')) {
            regexPattern += QStringLiteral(".*");
        } else if (ch == QLatin1Char('?')) {
            regexPattern += QLatin1Char('.');
        } else if (ch == QLatin1Char('.')) {
            regexPattern += QStringLiteral("\\.");
        } else {
            regexPattern += QRegularExpression::escape(QString(ch));
        }
    }
    regexPattern += QLatin1Char('$');
    return QRegularExpression(regexPattern, QRegularExpression::CaseInsensitiveOption);
}

// 功能：检查目录名是否应跳过；使用模块：findFiles。
bool shouldSkipDirectory(const QString &dirName)
{
    static const QStringList skipPatterns = {
        QStringLiteral(".git"),
        QStringLiteral("__pycache__"),
        QStringLiteral("node_modules"),
        QStringLiteral(".svn"),
        QStringLiteral(".hg"),
    };

    for (const QString &pattern : skipPatterns) {
        if (dirName == pattern) {
            return true;
        }
    }

    // 跳过 build-* 开头的目录
    if (dirName.startsWith(QStringLiteral("build-")) || dirName.startsWith(QStringLiteral("build_qt"))) {
        return true;
    }

    return false;
}

constexpr int MaxFindResults = 500;

} // namespace

namespace ProjectFindService {

ToolResult findFiles(const QString &projectDirectory, const QString &pattern, int maxResults)
{
    if (pattern.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Search pattern must not be empty."));
    }
    if (maxResults <= 0 || maxResults > MaxFindResults) {
        maxResults = MaxFindResults;
    }

    const QDir projectDir(projectDirectory);
    if (!projectDir.exists()) {
        return ToolResult::failure(QStringLiteral("Project directory does not exist: %1").arg(projectDirectory));
    }

    const QRegularExpression regex = globToRegex(pattern);
    if (!regex.isValid()) {
        return ToolResult::failure(QStringLiteral("Invalid search pattern: %1").arg(pattern));
    }

    QStringList results;
    QDirIterator it(projectDirectory, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while (it.hasNext() && results.size() < maxResults) {
        it.next();

        // 跳过排除目录中的文件
        bool skip = false;
        const QStringList pathParts = it.filePath().mid(projectDirectory.size() + 1).split(QLatin1Char('/'));
        for (const QString &part : pathParts) {
            if (shouldSkipDirectory(part)) {
                skip = true;
                break;
            }
        }
        if (skip) {
            continue;
        }

        // 匹配文件名
        if (regex.match(it.fileName()).hasMatch()) {
            const QString relativePath = projectDir.relativeFilePath(it.filePath());
            results.append(relativePath);
        }
    }

    if (results.isEmpty()) {
        return ToolResult::success(
            QStringLiteral("No files found matching \"%1\" in %2").arg(pattern, projectDirectory));
    }

    QString output;
    output += QStringLiteral("=== Found %1 file(s) matching \"%2\" ===\n").arg(results.size()).arg(pattern);
    for (const QString &filePath : results) {
        output += filePath + QLatin1Char('\n');
    }

    if (results.size() >= maxResults) {
        output += QStringLiteral("... (results truncated at %1)\n").arg(maxResults);
    }

    return ToolResult::success(output);
}

} // namespace ProjectFindService
