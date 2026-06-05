#pragma once

#include "hooks/HookDefinition.h"

#include <QObject>
#include <QMap>
#include <QString>
#include <QVector>

// ============================================================================
// HookManager — Hook 注册、分发、执行协调器
//
// 内部 QMap<HookPoint, QVector<HookBase*>> 按 Hook 点分组存储。
// Hook 执行不阻塞 Agent 循环（脚本超时后跳过）。
// ============================================================================

class HookManager : public QObject {
    Q_OBJECT

public:
    explicit HookManager(QObject *parent = nullptr);

    // 功能：注册 Hook（转移所有权）；使用模块：ApplicationController::initialize。
    void registerHook(HookBase *hook);

    // 功能：按名称注销 Hook；使用模块：热更新和测试。
    void unregisterHook(const QString &name);

    // 功能：执行指定 Hook 点的所有 Hook，返回结果列表；
    // 使用模块：AgentLoopController 和 ApplicationController。
    QVector<HookResult> executeHooks(HookPoint point, const HookContext &ctx);

    // 功能：返回指定 Hook 点的所有 Hook；使用模块：UI 展示和测试。
    QVector<HookBase*> hooksForPoint(HookPoint point) const;

    // 功能：返回已注册 Hook 总数；使用模块：测试。
    int hookCount() const;

    // 功能：返回指定 Hook 点的 Hook 数量；使用模块：测试。
    int hookCountForPoint(HookPoint point) const;

    // 功能：从双目录加载脚本 Hook；使用模块：ApplicationController::initialize。
    void loadScriptHooks(const QString &userHooksDir, const QString &projectHooksDir);

    // 功能：清空所有 Hook（释放内存）；使用模块：测试和重置。
    void clear();

private:
    QMap<HookPoint, QVector<HookBase*>> m_hooks;
};
