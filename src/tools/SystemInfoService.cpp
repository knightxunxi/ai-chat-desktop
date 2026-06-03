#include "tools/SystemInfoService.h"

#include <QCryptographicHash>
#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace SystemInfoService {

ToolResult readEnvVariable(const QString &name)
{
    if (name.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("Variable name must not be empty."));
    }

    // 安全检查：拒绝敏感变量名
    const QString lowerName = name.trimmed().toLower();
    const QStringList dangerousKeywords = {
        QStringLiteral("password"), QStringLiteral("passwd"),
        QStringLiteral("key"), QStringLiteral("token"),
        QStringLiteral("secret"), QStringLiteral("credential"),
        QStringLiteral("api")
    };
    for (const QString &keyword : dangerousKeywords) {
        if (lowerName.contains(keyword)) {
            return ToolResult::failure(
                QStringLiteral("Cannot read potentially sensitive environment variable: %1").arg(name));
        }
    }

#ifdef Q_OS_WIN
    // 优先读取系统环境变量
    const QString value = QProcessEnvironment::systemEnvironment().value(name);
    if (value.isEmpty()) {
        // 回退到 Qt 标准路径映射
        if (lowerName == QStringLiteral("userprofile") || lowerName == QStringLiteral("home")) {
            const QString home = QDir::homePath();
            if (!home.isEmpty()) {
                return ToolResult::success(home);
            }
        }
        return ToolResult::success(QStringLiteral("(empty or not set)"));
    }

    // 对路径变量截断显示（保护隐私）
    if (value.size() > 512) {
        return ToolResult::success(value.left(512) + QStringLiteral("...(truncated)"));
    }
    return ToolResult::success(value);
#else
    return ToolResult::failure(QStringLiteral("Environment variable reading is only supported on Windows."));
#endif
}

ToolResult systemPath(const QString &kind)
{
    const QString lowerKind = kind.trimmed().toLower();
    QString path;

    if (lowerKind == QStringLiteral("desktop")) {
        path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    } else if (lowerKind == QStringLiteral("documents")) {
        path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    } else if (lowerKind == QStringLiteral("home") || lowerKind == QStringLiteral("userprofile")) {
        path = QDir::homePath();
    } else if (lowerKind == QStringLiteral("temp")) {
        path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    } else if (lowerKind == QStringLiteral("appdata")) {
        path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    } else {
        return ToolResult::failure(
            QStringLiteral("Unknown system path kind: %1. Supported: desktop, documents, home, temp, appdata").arg(kind));
    }

    if (path.isEmpty()) {
        return ToolResult::failure(QStringLiteral("Cannot determine %1 path.").arg(kind));
    }

    return ToolResult::success(path);
}

} // namespace SystemInfoService
