// V12.4: Chat 模式自动执行工具 — 路由逻辑测试
// 不需要真实 AI，直接测试 ApplicationController 的路由逻辑
// ApplicationController 已声明 friend struct ChatToolExecutionTestAccessor

#include "app/ApplicationController.h"
#include "tools/AgentToolRegistry.h"
#include "support/AppLogger.h"

#include <QDir>
#include <QJsonObject>
#include <QMetaObject>
#include <QTemporaryDir>

#include <cassert>
#include <cstdio>

// 测试访问器 — ApplicationController 已声明为 friend，可访问私有成员
struct ChatToolExecutionTestAccessor {
    using RequestKind = ApplicationController::ActiveRequestKind;

    static auto &pendingToolResults(ApplicationController &c) { return c.m_pendingToolResults; }
    static bool chatAutoExecute(const ApplicationController &c) { return c.m_chatAutoExecute; }
    static auto activeRequestKind(const ApplicationController &c) { return c.m_activeRequestKind; }
    static void setActiveRequestKind(ApplicationController &c, RequestKind k) { c.m_activeRequestKind = k; }
    static void setChatAutoExecute(ApplicationController &c, bool v) { c.m_chatAutoExecute = v; }
    static void setHighPermissionMode(ApplicationController &c, bool v) { c.m_highPermissionMode = v; }
    static void setGenerating(ApplicationController &c, bool v) { c.m_isGenerating = v; }
    static auto &config(ApplicationController &c) { return c.m_config; }
    static auto &session(ApplicationController &c) { return c.m_session; }
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
    // Test 1: chat-mode-no-tools-by-default
    // Chat 模式 + autoExecute=false → sendMessage 不走 tools
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 验证初始状态
        assert(A::chatAutoExecute(controller) == false);

        // 验证 sendMessage 不会设置 chatAutoExecute
        assert(!A::chatAutoExecute(controller));

        // 验证初始 activeRequestKind
        assert(A::activeRequestKind(controller) == A::RequestKind::None);
    }

    // ================================================================
    // Test 2: chat-mode-with-tools
    // Chat 模式 + autoExecute=true → sendMessageWithTools 携带 tools
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 设置自动执行
        controller.setChatAutoExecute(true);
        assert(A::chatAutoExecute(controller) == true);

        // 重置
        controller.setChatAutoExecute(false);
        assert(A::chatAutoExecute(controller) == false);
    }

    // ================================================================
    // Test 3: tool-calls-blocked-in-plain-chat
    // Chat 模式 + autoExecute=false → handleToolUseBlockComplete 直接 return
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
        A::setChatAutoExecute(controller, false);
        A::setHighPermissionMode(controller, false);
        assert(A::pendingToolResults(controller).isEmpty());

        // 调用 handleToolUseBlockComplete — 应该被阻断
        A::callHandleToolUseBlockComplete(
            controller,
            QStringLiteral("json_format"),
            QJsonObject{{QStringLiteral("input"), QStringLiteral("{}")}});

        // 验证没有工具被添加（被阻断）
        assert(A::pendingToolResults(controller).isEmpty());
    }

    // ================================================================
    // Test 4: tool-calls-allowed-in-chat-auto
    // Chat 模式 + autoExecute=true + highPermission=true
    // → handleToolUseBlockComplete 进入执行路径
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 模拟 Chat + autoExecute + highPermission 状态
        A::setActiveRequestKind(controller, A::RequestKind::ChatMessage);
        A::setChatAutoExecute(controller, true);
        A::setHighPermissionMode(controller, true);
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
    // Test 5: high-permission-tool-skipped
    // Chat 模式 + autoExecute=true + highPermission=false
    // + requiresUserConfirmation=true → 工具被跳过
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 模拟 Chat + autoExecute + 无高权限
        A::setActiveRequestKind(controller, A::RequestKind::ChatMessage);
        A::setChatAutoExecute(controller, true);
        A::setHighPermissionMode(controller, false);
        assert(A::pendingToolResults(controller).isEmpty());

        // 调用 handleToolUseBlockComplete
        // json_format 工具 requiresUserConfirmation=true
        A::callHandleToolUseBlockComplete(
            controller,
            QStringLiteral("json_format"),
            QJsonObject{{QStringLiteral("input"), QStringLiteral("{}")}});

        // 验证工具被跳过（pendingToolResults 有记录但 success=false）
        assert(!A::pendingToolResults(controller).isEmpty());
        assert(A::pendingToolResults(controller).first().success == false);
        assert(A::pendingToolResults(controller).first().result.contains(
            QStringLiteral("跳过")) ||
            A::pendingToolResults(controller).first().result.contains(
                QStringLiteral("Skipped")));
    }

    // ================================================================
    // Test 6: high-permission-tool-executed
    // Chat 模式 + autoExecute=true + highPermission=true
    // + requiresUserConfirmation=true → 工具正常执行
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 模拟 Chat + autoExecute + 高权限
        A::setActiveRequestKind(controller, A::RequestKind::ChatMessage);
        A::setChatAutoExecute(controller, true);
        A::setHighPermissionMode(controller, true);
        assert(A::pendingToolResults(controller).isEmpty());

        // 调用 handleToolUseBlockComplete
        A::callHandleToolUseBlockComplete(
            controller,
            QStringLiteral("json_format"),
            QJsonObject{{QStringLiteral("input"), QStringLiteral("{}")}});

        // 验证工具被执行且成功
        assert(!A::pendingToolResults(controller).isEmpty());
        assert(A::pendingToolResults(controller).first().success == true);
        assert(A::pendingToolResults(controller).first().toolName ==
               QStringLiteral("json_format"));
    }

    // ================================================================
    // Test 7: pending-results-appended
    // handleRequestFinished 在 Chat 模式 + pendingToolResults 非空时
    // 正确追加结果到 m_currentAssistantContent
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
        A::setChatAutoExecute(controller, true);

        // 手动添加 pending tool results
        PendingToolResult result1;
        result1.toolName = QStringLiteral("json_format");
        result1.result = QStringLiteral("formatted JSON output");
        result1.success = true;
        A::pendingToolResults(controller).append(result1);

        PendingToolResult result2;
        result2.toolName = QStringLiteral("text_cleanup");
        result2.result = QStringLiteral("cleaned text");
        result2.success = true;
        A::pendingToolResults(controller).append(result2);

        // 设置一些已有内容
        A::currentContent(controller) = QStringLiteral("Here is the response.");

        // 添加一个空的 assistant 消息用于测试
        A::session(controller).addMessage(MessageRole::Assistant, QString());

        // 调用 handleRequestFinished
        A::callHandleRequestFinished(controller);

        // 验证 pending results 被清空
        assert(A::pendingToolResults(controller).isEmpty());

        // 验证 m_chatAutoExecute 被重置
        assert(A::chatAutoExecute(controller) == false);

        // 验证 content 包含工具结果
        assert(A::currentContent(controller).contains(
            QStringLiteral("formatted JSON output")));
        assert(A::currentContent(controller).contains(
            QStringLiteral("cleaned text")));
        assert(A::currentContent(controller).contains(
            QStringLiteral("json_format")));
        assert(A::currentContent(controller).contains(
            QStringLiteral("text_cleanup")));
    }

    // ================================================================
    // Test 8: cancel-resets-auto-execute
    // cancelCurrentRequest 重置 m_chatAutoExecute 为 false
    // ================================================================
    {
        ApplicationController controller;
        AppConfig cfg;
        cfg.apiKey = QStringLiteral("test-key");
        cfg.baseUrl = QStringLiteral("http://localhost:8080/v1");
        cfg.modelName = QStringLiteral("test-model");
        A::config(controller) = cfg;

        // 设置 activeRequestKind 和 m_chatAutoExecute
        A::setActiveRequestKind(controller, A::RequestKind::ChatMessage);
        A::setChatAutoExecute(controller, true);
        A::setGenerating(controller, true);

        // 调用 cancel
        controller.cancelCurrentRequest();

        // 验证 m_chatAutoExecute 被重置
        assert(A::chatAutoExecute(controller) == false);
    }

    // ================================================================
    // All tests passed
    // ================================================================
    return 0;
}
