#include "hooks/ScriptHookRunner.h"

#include "support/AppLogger.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

ScriptHookRunner::ScriptHookRunner(QObject *parent)
    : QObject(parent)
{
}

bool ScriptHookRunner::validateScriptPath(const QString &path, QString *error) const
{
    const QFileInfo fileInfo(path);

    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (error != nullptr) {
            *error = QStringLiteral("Script file does not exist or is not a regular file: %1").arg(path);
        }
        return false;
    }

    const QString absolutePath = fileInfo.absoluteFilePath();
    const QDir absoluteDir = fileInfo.absoluteDir();

    // 白名单路径 1: ~/.codex/hooks/
    const QString homeHooksDir = QDir::cleanPath(
        QDir::homePath() + QStringLiteral("/.codex/hooks"));
    if (absolutePath.startsWith(homeHooksDir, Qt::CaseInsensitive)) {
        return true;
    }

    // 白名单路径 2: .workbuddy/hooks/ （相对于当前工作目录或检查包含 pattern）
    if (absoluteDir.dirName() == QStringLiteral("hooks") &&
        (absolutePath.contains(QStringLiteral(".workbuddy/hooks/"), Qt::CaseInsensitive) ||
         absolutePath.contains(QStringLiteral("workbuddy/hooks/"), Qt::CaseInsensitive))) {
        return true;
    }

    if (error != nullptr) {
        *error = QStringLiteral("Script path is not in the allowed whitelist: %1. "
                                "Allowed paths: ~/.codex/hooks/ and .workbuddy/hooks/")
                     .arg(path);
    }

    AppLogger::warning(QStringLiteral("ScriptHookRunner"),
                       QStringLiteral("Path rejected: %1").arg(path));
    return false;
}

QStringList ScriptHookRunner::allowedEnvVars()
{
    return {
        QStringLiteral("PATH"),
        QStringLiteral("HOME"),
        QStringLiteral("USERPROFILE"),
        QStringLiteral("TEMP"),
        QStringLiteral("SYSTEMROOT")
    };
}

HookResult ScriptHookRunner::run(const QString &scriptPath,
                                  const QJsonObject &stdinJson,
                                  int timeoutMs)
{
    HookResult result;

    // 1. 路径校验
    QString pathError;
    if (!validateScriptPath(scriptPath, &pathError)) {
        result.action = HookAction::Pass;
        result.error = pathError;
        return result;
    }

    // 2. 启动 QProcess
    QProcess process;

    // 3. 设置环境变量白名单
    QProcessEnvironment env;
    const QStringList allowed = allowedEnvVars();
    for (const QString &varName : allowed) {
        const QString value = qEnvironmentVariable(varName.toUtf8().constData());
        if (!value.isEmpty()) {
            env.insert(varName, value);
        }
    }
    process.setProcessEnvironment(env);

    // 4. 设置工作目录为临时目录
    process.setWorkingDirectory(QDir::tempPath());

    // 5. 写入 stdin JSON
    const QByteArray inputData = QJsonDocument(stdinJson).toJson(QJsonDocument::Compact);

    // 6. 启动进程
    process.start(scriptPath, QStringList());
    if (!process.waitForStarted(5000)) {
        AppLogger::warning(QStringLiteral("ScriptHookRunner"),
                           QStringLiteral("Failed to start script: %1, error: %2")
                               .arg(scriptPath, process.errorString()));
        result.action = HookAction::Pass;
        result.error = QStringLiteral("Failed to start script: %1").arg(process.errorString());
        return result;
    }

    // 7. 写入 stdin
    process.write(inputData);
    process.closeWriteChannel();

    // 8. 等待完成（带超时）
    if (!process.waitForFinished(timeoutMs)) {
        AppLogger::warning(QStringLiteral("ScriptHookRunner"),
                           QStringLiteral("Script timeout: %1, limit: %2ms")
                               .arg(scriptPath).arg(timeoutMs));
        process.kill();
        process.waitForFinished(2000);

        result.action = HookAction::Pass;
        result.error = QStringLiteral("timeout");
        return result;
    }

    // 9. 读取 stdout
    const QByteArray output = process.readAllStandardOutput();
    if (output.size() > m_maxOutputBytes) {
        AppLogger::warning(QStringLiteral("ScriptHookRunner"),
                           QStringLiteral("Script output exceeds max size: %1, size: %2")
                               .arg(scriptPath).arg(output.size()));
        result.action = HookAction::Pass;
        result.error = QStringLiteral("Output exceeds maximum size");
        return result;
    }

    // 10. 解析 stdout JSON
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(output, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        AppLogger::warning(QStringLiteral("ScriptHookRunner"),
                           QStringLiteral("Script returned invalid JSON: %1, error: %2")
                               .arg(scriptPath, parseError.errorString()));
        result.action = HookAction::Pass;
        result.error = QStringLiteral("Invalid JSON output from script");
        return result;
    }

    const QJsonObject outputObj = doc.object();

    // 解析 action 字段
    const QString actionStr = outputObj.value(QStringLiteral("action")).toString(QStringLiteral("pass"));
    if (actionStr == QStringLiteral("pass")) {
        result.action = HookAction::Pass;
    } else if (actionStr == QStringLiteral("modify")) {
        result.action = HookAction::Modify;
        result.modifiedContext = outputObj.value(QStringLiteral("modified_context")).toObject();
    } else if (actionStr == QStringLiteral("reject")) {
        result.action = HookAction::Reject;
        result.error = outputObj.value(QStringLiteral("reason")).toString(QStringLiteral("Rejected by script"));
    } else {
        result.action = HookAction::Pass;
    }

    return result;
}
