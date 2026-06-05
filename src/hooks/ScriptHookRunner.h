#pragma once

#include "hooks/HookDefinition.h"

#include <QJsonObject>
#include <QObject>
#include <QString>

// ============================================================================
// ScriptHookRunner — QProcess 脚本执行器
//
// 白名单路径校验、stdin JSON 写入、stdout JSON 解析、10s 超时。
// 环境变量白名单：仅传递 PATH, HOME, USERPROFILE, TEMP, SYSTEMROOT。
// ============================================================================

class ScriptHookRunner : public QObject {
    Q_OBJECT

public:
    explicit ScriptHookRunner(QObject *parent = nullptr);

    // 功能：执行脚本并解析结果；使用模块：ScriptHook::execute。
    HookResult run(const QString &scriptPath,
                   const QJsonObject &stdinJson,
                   int timeoutMs = 10000);

    // 功能：校验脚本路径在白名单内；使用模块：run 前校验。
    bool validateScriptPath(const QString &path, QString *error = nullptr) const;

private:
    // 功能：返回允许传递的环境变量名列表；使用模块：QProcess 环境设置。
    static QStringList allowedEnvVars();

    int m_defaultTimeoutMs = 10000;
    int m_maxOutputBytes = 64 * 1024; // 64 KB
};
