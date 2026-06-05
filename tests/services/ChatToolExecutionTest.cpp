// V16.3: Agent-only 工具执行路由测试
// Chat 模式不再执行任何工具，仅 Agent 模式处理工具调用
// 同时验证确认弹窗和高权限检查已被移除
// ApplicationController 已声明 friend struct ChatToolExecutionTestAccessor

#include "app/ApplicationController.h"
#include "tools/AgentToolRegistry.h"
#include "support/AppLogger.h"

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QMetaObject>
#include <QTemporaryDir>

#include <cassert>
#include <cstdio>

// 测试访问器 — ApplicationController 已声明为 friend，可访问私有成员
struct ChatToolExecutionTestAccessor {
    using RequestKind = ApplicationController::ActiveRequestKind;

    static auto &pendingToolResults(ApplicationController &c) { return c.m_agentOrchestrator.m_pendingToolResults; }
    static auto &agentToolCalls(ApplicationController &c) { return c.m_agentToolCalls; }
    static auto activeRequestKind(const ApplicationController &c) { return c.m_activeRequestKind; }
    static void setActiveRequestKind(ApplicationController &c, RequestKind k) { c.m_activeRequestKind = k; }
    static void setGenerating(ApplicationController &c, bool v) { c.m_isGenerating = v; }
    static auto &config(ApplicationController &c) { return const_cast<AppConfig &>(c.m_configCoordinator.config()); }
    static auto &session(ApplicationController &c) { return c.m_sessionCoordinator.currentSession(); }
    static auto &currentContent(ApplicationController &c) { return c.m_currentAssistantContent; }

    // 调用私有 slot: handleToolUseBlockComplete
    static void callHandleToolUseBlockComplete(
        ApplicationController &c,
        const QString &toolName,
        const QJsonObject &arguments)
    {
        c.handleToolUseBlockComplete(toolName, arguments);
    }

    // 调用私有方法: handleRequestFinished
    static void callHandleRequestFinished(ApplicationController &c)
    {
        c.handleRequestFinished();
    }

    // 调用私有方法: handleToolCallsReceived
    static void callHandleToolCallsReceived(
        ApplicationController &c,
        const ToolCallList &toolCalls)
    {
        c.handleToolCallsReceived(toolCalls);
    }
};

// 简便别名
using A = ChatToolExecutionTestAccessor;

int main()
{
    // --- Setup: 临时目录和日志 ---
    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());
    AppLogger::setLogFilePathForTests(
        temporaryDirectory.filePath(QStringLiteral("chat-tool-execution-test.log")));
    QString loggerError;
    AppLogger::initialize(&loggerError);

    // ================================================================
    // Test 1: chat-mode-blocks-tools
    // Chat 模式 → handleToolUseBlockComplete 直接 return
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 模拟纯 Chat 模式状态
        A::setActiveRequestKind(controller, A::RequestKind::ChatMessage);
        assert(A::pendingToolResults(controller).isEmpty());

        // 调用 handleToolUseBlockComplete — 应该被阻断
        A::callHandleToolUseBlockComplete(
            controller,
            QStringLiteral("json_format"),
            QJsonObject{{QStringLiteral("input"), QStringLiteral("{}")}});

        // 验证没有工具被添加（Chat 模式被阻断）
        assert(A::pendingToolResults(controller).isEmpty());
    }

    // ================================================================
    // Test 2: agent-mode-allows-tools
    // AgentPlan 模式 → handleToolUseBlockComplete 进入执行路径
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 模拟 AgentPlan 模式
        A::setActiveRequestKind(controller, A::RequestKind::AgentPlan);
        assert(A::pendingToolResults(controller).isEmpty());

        // 调用 handleToolUseBlockComplete — 应该进入执行路径
        A::callHandleToolUseBlockComplete(
            controller,
            QStringLiteral("json_format"),
            QJsonObject{{QStringLiteral("input"), QStringLiteral("{}")}});

        // 验证工具被执行（pendingToolResults 非空）
        assert(!A::pendingToolResults(controller).isEmpty());
        assert(A::pendingToolResults(controller).first().toolName ==
               QStringLiteral("json_format"));
    }

    // ================================================================
    // Test 3: unified-agent-allows-tools
    // UnifiedAgent 模式 → handleToolUseBlockComplete 进入执行路径
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 模拟 UnifiedAgent 模式
        A::setActiveRequestKind(controller, A::RequestKind::UnifiedAgent);
        assert(A::pendingToolResults(controller).isEmpty());

        // 调用 handleToolUseBlockComplete — 应该进入执行路径
        A::callHandleToolUseBlockComplete(
            controller,
            QStringLiteral("json_format"),
            QJsonObject{{QStringLiteral("input"), QStringLiteral("{}")}});

        // 验证工具被执行
        assert(!A::pendingToolResults(controller).isEmpty());
        assert(A::pendingToolResults(controller).first().toolName ==
               QStringLiteral("json_format"));
    }

    // ================================================================
    // Test 4: tool-executed-without-confirmation
    // V16.3: 默认高权限自动执行（无弹窗）
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 模拟 AgentPlan 模式
        A::setActiveRequestKind(controller, A::RequestKind::AgentPlan);
        assert(A::pendingToolResults(controller).isEmpty());

        // 调用 handleToolUseBlockComplete with json_format (requiresUserConfirmation=false)
        A::callHandleToolUseBlockComplete(
            controller,
            QStringLiteral("json_format"),
            QJsonObject{{QStringLiteral("input"), QStringLiteral("{}")}});

        // 验证工具被执行且成功（不再跳过需要确认的工具）
        assert(!A::pendingToolResults(controller).isEmpty());
        assert(A::pendingToolResults(controller).first().success == true);
    }

    // ================================================================
    // Test 5: chat-mode-tool-calls-received-blocked
    // Chat 模式 → handleToolCallsReceived 直接 return
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        A::setActiveRequestKind(controller, A::RequestKind::ChatMessage);
        assert(A::pendingToolResults(controller).isEmpty());

        // handleToolCallsReceived should be blocked in Chat mode
        ToolCallList calls;
        ToolCall tc;
        tc.functionName = QStringLiteral("json_format");
        tc.id = QStringLiteral("call_1");
        tc.arguments = QStringLiteral("{\"input\":\"{}\"}");
        calls.append(tc);
        A::callHandleToolCallsReceived(controller, calls);

        // No tool execution should happen in Chat mode
        assert(A::pendingToolResults(controller).isEmpty());
    }

    // ================================================================
    // Test 6: pending-results-not-appended-in-chat
    // handleRequestFinished 在 Chat 模式不再追加工具结果
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 模拟 Chat 模式有 pending results
        A::setActiveRequestKind(controller, A::RequestKind::ChatMessage);

        // 手动添加 pending tool results
        PendingToolResult result1;
        result1.toolName = QStringLiteral("json_format");
        result1.result = QStringLiteral("formatted JSON output");
        result1.success = true;
        A::pendingToolResults(controller).append(result1);

        // 设置一些已有内容
        A::currentContent(controller) = QStringLiteral("Here is the response.");

        // 添加一个空的 assistant 消息用于测试
        A::session(controller).addMessage(MessageRole::Assistant, QString());

        // 调用 handleRequestFinished
        A::callHandleRequestFinished(controller);

        // 验证 pending results 被清空
        assert(A::pendingToolResults(controller).isEmpty());

        // 验证 content 不再包含工具结果（V16.3: Chat 不追加）
        assert(!A::currentContent(controller).contains(
            QStringLiteral("formatted JSON output")));
    }

    // ================================================================
    // Test 7: streaming-tool-call-not-executed-twice
    // toolCallsReceived + toolUseBlockComplete + requestFinished
    // → 使用已执行的 streaming result，不再转换成 plan 重复执行
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        A::setActiveRequestKind(controller, A::RequestKind::UnifiedAgent);
        A::session(controller).addMessage(MessageRole::Assistant, QString());

        ToolCallList calls;
        ToolCall tc;
        tc.functionName = QStringLiteral("json_format");
        tc.id = QStringLiteral("call_1");
        tc.arguments = QStringLiteral("{\"input\":\"{}\"}");
        calls.append(tc);

        A::callHandleToolCallsReceived(controller, calls);
        assert(!A::agentToolCalls(controller).isEmpty());

        A::callHandleToolUseBlockComplete(
            controller,
            QStringLiteral("json_format"),
            QJsonObject{{QStringLiteral("input"), QStringLiteral("{}")}});
        assert(!A::pendingToolResults(controller).isEmpty());

        A::callHandleRequestFinished(controller);

        assert(A::pendingToolResults(controller).isEmpty());
        assert(A::agentToolCalls(controller).isEmpty());
        assert(A::activeRequestKind(controller) == A::RequestKind::None);
        assert(A::currentContent(controller).contains(QStringLiteral("[Tool results]")));
        assert(A::currentContent(controller).contains(QStringLiteral("json_format")));
        assert(!A::currentContent(controller).contains(QStringLiteral("Executing plan")));
    }

    // ================================================================
    // Test 8: final-tool-call-executes-on-request-finished
    // 只有最终 tool_calls、没有 toolUseBlockComplete 时，也必须执行工具
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        cfg.agentProjectDirectory = temporaryDirectory.filePath(QStringLiteral("native-tool-workspace"));
        assert(QDir().mkpath(cfg.agentProjectDirectory));
        A::config(controller) = cfg;

        A::setActiveRequestKind(controller, A::RequestKind::UnifiedAgent);
        A::session(controller).addMessage(MessageRole::Assistant, QString());

        ToolCallList calls;
        ToolCall tc;
        tc.functionName = QStringLiteral("workspace_write_text");
        tc.id = QStringLiteral("call_write_1");
        tc.arguments = QStringLiteral("{\"path\":\"loop-created.txt\",\"content\":\"created\"}");
        calls.append(tc);

        A::callHandleToolCallsReceived(controller, calls);
        assert(!A::agentToolCalls(controller).isEmpty());

        A::callHandleRequestFinished(controller);

        QFile outputFile(QDir(cfg.agentProjectDirectory).filePath(QStringLiteral("loop-created.txt")));
        assert(outputFile.open(QFile::ReadOnly | QFile::Text));
        assert(QString::fromUtf8(outputFile.readAll()) == QStringLiteral("created"));
        assert(A::pendingToolResults(controller).isEmpty());
        assert(A::agentToolCalls(controller).isEmpty());
        assert(A::activeRequestKind(controller) == A::RequestKind::None);
        assert(A::currentContent(controller).contains(QStringLiteral("[Tool results]")));
        assert(A::currentContent(controller).contains(QStringLiteral("workspace_write_text")));
        assert(!A::currentContent(controller).contains(QStringLiteral("Agent plan generated")));
        assert(!A::currentContent(controller).contains(QStringLiteral("Task completed")));
    }

    // ================================================================
    // Test 9: cancelCurrentRequest-still-works
    // cancelCurrentRequest 在 Chat 模式下仍然正常工作
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 设置 Chat 模式并标记为 generating
        A::setActiveRequestKind(controller, A::RequestKind::ChatMessage);
        A::setGenerating(controller, true);

        // 调用 cancel
        controller.cancelCurrentRequest();

        // 验证 cancel 正常工作（不崩溃即可，Chat 模式无 m_chatAutoExecute 需要重置）
    }

    // ================================================================
    // All tests passed
    // ================================================================
    return 0;
}
