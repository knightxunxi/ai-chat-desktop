#include "hooks/HookManager.h"
#include "hooks/HookDefinition.h"
#include "hooks/BuiltinHooks.h"
#include "hooks/ScriptHookRunner.h"

#include <QDir>
#include <QJsonObject>
#include <QTemporaryDir>

#include <cassert>
#include <cstdio>
#include <functional>

namespace {

static int testCount = 0;
static int passCount = 0;

void test(const char *name, std::function<void()> body)
{
    ++testCount;
    try { body(); ++passCount; printf("  PASS: %s\n", name); }
    catch (const std::exception &e) { printf("  FAIL: %s — %s\n", name, e.what()); }
    catch (...) { printf("  FAIL: %s — unknown error\n", name); }
}

// Test hooks
class PassTestHook : public HookBase {
public:
    HookResult execute(const HookContext &) override {
        HookResult r; r.action = HookAction::Pass; return r;
    }
    QString name() const override { return QStringLiteral("test.pass"); }
    HookPoint hookPoint() const override { return HookPoint::PreSend; }
    QString hookType() const override { return QStringLiteral("test"); }
};

class RejectTestHook : public HookBase {
public:
    HookResult execute(const HookContext &) override {
        HookResult r; r.action = HookAction::Reject; r.error = QStringLiteral("Test rejection"); return r;
    }
    QString name() const override { return QStringLiteral("test.reject"); }
    HookPoint hookPoint() const override { return HookPoint::PreSend; }
    QString hookType() const override { return QStringLiteral("test"); }
};

} // namespace

int main()
{
    // TC-01: HookManager 注册与分发
    test("register and dispatch", [] {
        HookManager manager;
        assert(manager.hookCount() == 0);
        manager.registerHook(new PassTestHook());
        assert(manager.hookCount() == 1);
        assert(manager.hookCountForPoint(HookPoint::PreSend) == 1);
        assert(manager.hookCountForPoint(HookPoint::PostReceive) == 0);
    });

    // TC-02: executeHooks 正常执行 (Pass)
    test("execute hooks pass", [] {
        HookManager manager;
        manager.registerHook(new PassTestHook());
        HookContext ctx;
        ctx.hookPoint = HookPoint::PreSend;
        ctx.context.insert(QStringLiteral("prompt"), QStringLiteral("test prompt"));
        auto results = manager.executeHooks(HookPoint::PreSend, ctx);
        assert(results.size() == 1);
        assert(results[0].isPass());
    });

    // TC-03: TimestampHook 注入时间戳
    test("timestamp hook", [] {
        TimestampHook hook;
        HookContext ctx;
        ctx.hookPoint = HookPoint::PreSend;
        ctx.context.insert(QStringLiteral("prompt"), QStringLiteral("Hello"));
        HookResult result = hook.execute(ctx);
        assert(result.action == HookAction::Modify);
        QString modified = result.modifiedContext.value(QStringLiteral("prompt")).toString();
        assert(modified.contains(QStringLiteral("[Current time:")));
        assert(modified.startsWith(QStringLiteral("Hello")));
    });

    // TC-04: RateLimitHook 正常通过
    test("rate limit hook pass", [] {
        RateLimitHook hook(5);
        HookContext ctx;
        ctx.hookPoint = HookPoint::PreSend;
        ctx.metadata.insert(QStringLiteral("session_id"), QStringLiteral("test-session"));
        assert(hook.execute(ctx).isPass());
    });

    // TC-05: RateLimitHook 超频拒绝
    test("rate limit hook reject", [] {
        RateLimitHook hook(2);
        HookContext ctx;
        ctx.hookPoint = HookPoint::PreSend;
        ctx.metadata.insert(QStringLiteral("session_id"), QStringLiteral("test-session-2"));
        assert(hook.execute(ctx).isPass());
        assert(hook.execute(ctx).isPass());
        HookResult r3 = hook.execute(ctx);
        assert(r3.isReject());
        assert(r3.error.contains(QStringLiteral("Rate limit")));
    });

    // TC-06: SensitiveFilterHook 过滤 API Key
    test("sensitive filter hook", [] {
        SensitiveFilterHook hook;
        HookContext ctx;
        ctx.hookPoint = HookPoint::PostReceive;
        ctx.context.insert(QStringLiteral("response"),
            QStringLiteral("Here is your key: sk-abc123def456ghi789jklmno"));
        HookResult result = hook.execute(ctx);
        assert(result.action == HookAction::Modify);
        QString filtered = result.modifiedContext.value(QStringLiteral("response")).toString();
        assert(!filtered.contains(QStringLiteral("sk-abc123")));
        assert(filtered.contains(QStringLiteral("[REDACTED]")));
    });

    // TC-07: Hook Reject 停止后续
    test("reject stops chain", [] {
        HookManager manager;
        manager.registerHook(new RejectTestHook());
        manager.registerHook(new PassTestHook());
        HookContext ctx;
        ctx.hookPoint = HookPoint::PreSend;
        auto results = manager.executeHooks(HookPoint::PreSend, ctx);
        assert(results.size() == 1);
        assert(results[0].isReject());
    });

    // TC-08: ScriptHookRunner 白名单拒绝
    test("script hook path whitelist reject", [] {
        ScriptHookRunner runner;
        QString error;
        bool valid = runner.validateScriptPath(QStringLiteral("C:/Windows/System32/calc.exe"), &error);
        assert(!valid);
        assert(!error.isEmpty());
    });

    // TC-09: 空 Hook 点正常
    test("empty hook point", [] {
        HookManager manager;
        HookContext ctx;
        ctx.hookPoint = HookPoint::OnError;
        auto results = manager.executeHooks(HookPoint::OnError, ctx);
        assert(results.size() == 0);
    });

    // TC-10: HookContext 工厂方法
    test("hook context factory methods", [] {
        auto pre = HookContext::forPreSend(QStringLiteral("test prompt"), QStringLiteral("test goal"), 3, QStringLiteral("abc-123"));
        assert(pre.hookPoint == HookPoint::PreSend);
        assert(pre.context.value(QStringLiteral("prompt")).toString() == QStringLiteral("test prompt"));
        assert(pre.metadata.value(QStringLiteral("iteration")).toInt() == 3);

        auto post = HookContext::forPostReceive(QStringLiteral("resp"), 5);
        assert(post.hookPoint == HookPoint::PostReceive);

        QJsonObject params; params.insert(QStringLiteral("path"), QStringLiteral("test.txt"));
        auto tool = HookContext::forToolExecute(QStringLiteral("file.read"), params, true);
        assert(tool.hookPoint == HookPoint::OnToolExecute);
        assert(tool.metadata.value(QStringLiteral("before")).toBool());

        auto err = HookContext::forError(QStringLiteral("network error"), 2);
        assert(err.hookPoint == HookPoint::OnError);
    });

    // TC-11: HookManager clear 清空
    test("clear all hooks", [] {
        HookManager manager;
        manager.registerHook(new PassTestHook());
        manager.registerHook(new PassTestHook());
        assert(manager.hookCount() == 2);
        manager.clear();
        assert(manager.hookCount() == 0);
    });

    printf("\n%d/%d tests passed\n", passCount, testCount);
    return (passCount == testCount) ? 0 : 1;
}
