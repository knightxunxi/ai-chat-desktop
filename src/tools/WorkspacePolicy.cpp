#include "tools/core/WorkspacePolicy.h"

#include "core/AppConfig.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace {

QString absoluteCleanPath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

QString comparePath(const QString &path)
{
    QString normalized = QDir::cleanPath(path);
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
#ifdef Q_OS_WIN
    normalized = normalized.toLower();
#endif
    return normalized;
}

QString workspaceRoot(const QString &workspaceDirectory)
{
    const QString directory = workspaceDirectory.trimmed().isEmpty()
                                  ? WorkspacePolicy::defaultWorkspaceDirectory()
                                  : workspaceDirectory.trimmed();
    return absoluteCleanPath(directory);
}

bool pathStartsWithRoot(const QString &rootPath, const QString &targetPath)
{
    const QString root = comparePath(rootPath);
    const QString target = comparePath(targetPath);
    return target == root || target.startsWith(root + QLatin1Char('/'));
}

bool isUnsafeWorkspaceRoot(const QString &workspaceDirectory)
{
    const QString root = comparePath(absoluteCleanPath(workspaceDirectory));
    if (root == comparePath(QDir::rootPath())) {
        return true;
    }

    QStringList blockedRoots;
    const QString windowsDirectory = qEnvironmentVariable("WINDIR");
    const QString programFiles = qEnvironmentVariable("ProgramFiles");
    const QString programFilesX86 = qEnvironmentVariable("ProgramFiles(x86)");
    const QString programData = qEnvironmentVariable("ProgramData");

    if (!windowsDirectory.trimmed().isEmpty()) {
        blockedRoots.append(comparePath(windowsDirectory));
    }
    if (!programFiles.trimmed().isEmpty()) {
        blockedRoots.append(comparePath(programFiles));
    }
    if (!programFilesX86.trimmed().isEmpty()) {
        blockedRoots.append(comparePath(programFilesX86));
    }
    if (!programData.trimmed().isEmpty()) {
        blockedRoots.append(comparePath(programData));
    }

    for (const QString &blockedRoot : blockedRoots) {
        if (root == blockedRoot || root.startsWith(blockedRoot + QLatin1Char('/'))) {
            return true;
        }
    }

    return false;
}

bool isDestructiveOperation(WorkspaceOperation operation)
{
    return operation == WorkspaceOperation::Write
        || operation == WorkspaceOperation::Overwrite
        || operation == WorkspaceOperation::Delete;
}

} // namespace

namespace WorkspacePolicy {

QString defaultWorkspaceDirectory()
{
    return AppConfig::defaultAgentWorkspaceDirectory();
}

QString resolveWorkspacePath(const QString &workspaceDirectory, const QString &requestedPath)
{
    const QString trimmedPath = requestedPath.trimmed();
    if (trimmedPath.isEmpty()) {
        return QString();
    }

    if (QDir::isAbsolutePath(trimmedPath)) {
        return absoluteCleanPath(trimmedPath);
    }

    return absoluteCleanPath(QDir(workspaceRoot(workspaceDirectory)).filePath(trimmedPath));
}

bool isPathInsideWorkspace(const QString &workspaceDirectory, const QString &targetPath)
{
    const QString root = workspaceRoot(workspaceDirectory);
    const QString target = QDir::isAbsolutePath(targetPath)
                               ? absoluteCleanPath(targetPath)
                               : resolveWorkspacePath(root, targetPath);
    return pathStartsWithRoot(root, target);
}

bool isProtectedPath(const QString &path)
{
    const QString normalized = comparePath(path);
    const QStringList parts = normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.contains(QStringLiteral(".git"))) {
        return true;
    }

    const QFileInfo fileInfo(path);
    const QString fileName = fileInfo.fileName().toLower();
    const QString suffix = fileInfo.suffix().toLower();

    const QStringList protectedNames = {
        QStringLiteral(".env"),
        QStringLiteral(".env.local"),
        QStringLiteral(".env.production"),
        QStringLiteral("credentials.json"),
        QStringLiteral("secrets.json"),
        QStringLiteral("id_rsa"),
        QStringLiteral("id_dsa"),
        QStringLiteral("known_hosts")
    };

    if (protectedNames.contains(fileName)) {
        return true;
    }

    const QStringList protectedSuffixes = {
        QStringLiteral("key"),
        QStringLiteral("pem"),
        QStringLiteral("pfx"),
        QStringLiteral("p12"),
        QStringLiteral("crt"),
        QStringLiteral("cer"),
        QStringLiteral("der")
    };

    return protectedSuffixes.contains(suffix);
}

WorkspacePolicyDecision evaluateWorkspaceOperation(
    const QString &workspaceDirectory,
    const QString &requestedPath,
    WorkspaceOperation operation)
{
    WorkspacePolicyDecision decision;
    const QString root = workspaceRoot(workspaceDirectory);
    decision.normalizedPath = resolveWorkspacePath(root, requestedPath);

    if (requestedPath.trimmed().isEmpty()) {
        decision.reason = QStringLiteral("Requested path is empty.");
        return decision;
    }

    // V17.4: 移除工作目录沙箱限制 — 允许访问任意路径。保留对敏感文件的保护。

    if (isDestructiveOperation(operation) && isProtectedPath(decision.normalizedPath)) {
        decision.reason = QStringLiteral("Protected files cannot be created, deleted, or overwritten automatically.");
        return decision;
    }

    decision.allowed = true;
    decision.requiresConfirmation = false;
    decision.reason = QStringLiteral("Operation is allowed.");
    return decision;
}

} // namespace WorkspacePolicy
