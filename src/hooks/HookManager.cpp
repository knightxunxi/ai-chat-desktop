#include "hooks/HookManager.h"

#include "support/AppLogger.h"

#include <QElapsedTimer>

HookManager::HookManager(QObject *parent)
    : QObject(parent)
{
}

void HookManager::registerHook(HookBase *hook)
{
    if (hook == nullptr) {
        return;
    }

    const HookPoint point = hook->hookPoint();
    m_hooks[point].append(hook);

    AppLogger::info(QStringLiteral("HookManager"),
                    QStringLiteral("Registered hook: %1, type: %2, point: %3")
                        .arg(hook->name(), hook->hookType(), QString::number(static_cast<int>(point))));
}

void HookManager::unregisterHook(const QString &name)
{
    for (auto it = m_hooks.begin(); it != m_hooks.end(); ++it) {
        QVector<HookBase*> &hooks = it.value();
        for (int i = hooks.size() - 1; i >= 0; --i) {
            if (hooks[i]->name() == name) {
                delete hooks[i];
                hooks.removeAt(i);
                AppLogger::info(QStringLiteral("HookManager"),
                                QStringLiteral("Unregistered hook: %1").arg(name));
            }
        }
    }
}

QVector<HookResult> HookManager::executeHooks(HookPoint point, const HookContext &ctx)
{
    QVector<HookResult> results;

    if (!m_hooks.contains(point)) {
        return results;
    }

    const QVector<HookBase*> &hooks = m_hooks[point];
    HookContext currentCtx = ctx;

    for (HookBase *hook : hooks) {
        // 执行 Hook，带超时保护
        QElapsedTimer timer;
        timer.start();

        HookResult result = hook->execute(currentCtx);
        const qint64 elapsedMs = timer.elapsed();

        // 检查是否超时
        if (elapsedMs > hook->timeoutMs()) {
            AppLogger::warning(QStringLiteral("HookManager"),
                               QStringLiteral("timeout: %1, limit: %2ms, elapsed: %3ms")
                                   .arg(hook->name())
                                   .arg(hook->timeoutMs())
                                   .arg(elapsedMs));
            // 超时后返回 Pass，不阻塞
            result = HookResult{};
            result.action = HookAction::Pass;
            result.error = QStringLiteral("timeout");
        }

        AppLogger::info(QStringLiteral("HookManager"),
                        QStringLiteral("executed: %1, action: %2, elapsedMs: %3")
                            .arg(hook->name(),
                                 result.isPass() ? QStringLiteral("pass")
                                 : result.isReject() ? QStringLiteral("reject")
                                 : QStringLiteral("modify"))
                            .arg(elapsedMs));

        results.append(result);

        // 遇到 Reject 立即停止后续 Hook
        if (result.isReject()) {
            break;
        }

        // Modify：更新 context 继续
        if (result.action == HookAction::Modify) {
            // 合并 modifiedContext 到当前 context
            const QStringList keys = result.modifiedContext.keys();
            for (const QString &key : keys) {
                currentCtx.context.insert(key, result.modifiedContext.value(key));
            }
        }
    }

    return results;
}

QVector<HookBase*> HookManager::hooksForPoint(HookPoint point) const
{
    return m_hooks.value(point);
}

int HookManager::hookCount() const
{
    int count = 0;
    for (auto it = m_hooks.begin(); it != m_hooks.end(); ++it) {
        count += it.value().size();
    }
    return count;
}

int HookManager::hookCountForPoint(HookPoint point) const
{
    return m_hooks.value(point).size();
}

void HookManager::loadScriptHooks(const QString &userHooksDir, const QString &projectHooksDir)
{
    // P1: 脚本 Hook 加载 —— 扫描 .workbuddy/hooks/ 和 ~/.codex/hooks/
    // 暂时仅记录，脚本 Hook 注册通过 ScriptHook 类单独管理
    Q_UNUSED(userHooksDir);
    Q_UNUSED(projectHooksDir);
    AppLogger::info(QStringLiteral("HookManager"), QStringLiteral("Script hooks loading deferred (P1 feature)."));
}

void HookManager::clear()
{
    for (auto it = m_hooks.begin(); it != m_hooks.end(); ++it) {
        QVector<HookBase*> &hooks = it.value();
        for (HookBase *hook : hooks) {
            delete hook;
        }
        hooks.clear();
    }
    m_hooks.clear();
}
