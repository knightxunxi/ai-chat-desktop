# ──────────────────────────────────────────────────────────────────────
# CodeXX 测试套件
#
# 重构后：所有测试通过 target_link_libraries() 链接对应子库，
#         不再直接编译源文件。手动维护变量已全部移除。
# ──────────────────────────────────────────────────────────────────────

# =========================================================================
# Core 测试
# =========================================================================

add_executable(CoreModelSmokeTest
    core/ModelSmokeTest.cpp
)

target_link_libraries(CoreModelSmokeTest PRIVATE
    Qt6::Core
)

target_include_directories(CoreModelSmokeTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME CoreModelSmokeTest COMMAND CoreModelSmokeTest)

add_executable(ProviderPresetTest
    core/ProviderPresetTest.cpp
)

target_link_libraries(ProviderPresetTest PRIVATE
    Qt6::Core
    codexx_core
)

add_test(NAME ProviderPresetTest COMMAND ProviderPresetTest)

# =========================================================================
# App 测试
# =========================================================================

add_executable(SessionSummaryListTest
    app/SessionSummaryListTest.cpp
)

target_link_libraries(SessionSummaryListTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME SessionSummaryListTest COMMAND SessionSummaryListTest)

add_executable(ProjectInstructionServiceTest
    app/ProjectInstructionServiceTest.cpp
)

target_link_libraries(ProjectInstructionServiceTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME ProjectInstructionServiceTest COMMAND ProjectInstructionServiceTest)

add_executable(AgentPlanParserTest
    app/AgentPlanParserTest.cpp
)

target_link_libraries(AgentPlanParserTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentPlanParserTest COMMAND AgentPlanParserTest)

add_executable(AgentPlanExecutorTest
    app/AgentPlanExecutorTest.cpp
)

target_link_libraries(AgentPlanExecutorTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentPlanExecutorTest COMMAND AgentPlanExecutorTest)

add_executable(AgentPlanPromptBuilderTest
    app/AgentPlanPromptBuilderTest.cpp
)

target_link_libraries(AgentPlanPromptBuilderTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentPlanPromptBuilderTest COMMAND AgentPlanPromptBuilderTest)

add_executable(AgentCommandSkillCatalogTest
    app/AgentCommandSkillCatalogTest.cpp
)

target_link_libraries(AgentCommandSkillCatalogTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentCommandSkillCatalogTest COMMAND AgentCommandSkillCatalogTest)

add_executable(AgentCommandSkillFileServiceTest
    app/AgentCommandSkillFileServiceTest.cpp
)

target_link_libraries(AgentCommandSkillFileServiceTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentCommandSkillFileServiceTest COMMAND AgentCommandSkillFileServiceTest)

add_executable(AgentToolCallPlanBuilderTest
    app/AgentToolCallPlanBuilderTest.cpp
)

target_link_libraries(AgentToolCallPlanBuilderTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentToolCallPlanBuilderTest COMMAND AgentToolCallPlanBuilderTest)

add_executable(AgentLoopActionParserTest
    app/AgentLoopActionParserTest.cpp
)

target_link_libraries(AgentLoopActionParserTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentLoopActionParserTest COMMAND AgentLoopActionParserTest)

add_executable(AgentLoopPromptBuilderTest
    app/AgentLoopPromptBuilderTest.cpp
)

target_link_libraries(AgentLoopPromptBuilderTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentLoopPromptBuilderTest COMMAND AgentLoopPromptBuilderTest)

add_executable(AgentLoopControllerTest
    app/AgentLoopControllerTest.cpp
)

target_link_libraries(AgentLoopControllerTest PRIVATE
    Qt6::Core
    codexx_app
)

add_test(NAME AgentLoopControllerTest COMMAND AgentLoopControllerTest)

# =========================================================================
# Services 测试
# =========================================================================

add_executable(StreamParserTest
    services/StreamParserTest.cpp
)

target_link_libraries(StreamParserTest PRIVATE
    Qt6::Core
    codexx_services
)

add_test(NAME StreamParserTest COMMAND StreamParserTest)

# V12.3 流式工具执行测试
add_executable(StreamParserV12Test
    services/StreamParserV12Test.cpp
)

target_link_libraries(StreamParserV12Test PRIVATE
    Qt6::Core
    codexx_services
)

add_test(NAME StreamParserV12Test COMMAND StreamParserV12Test)

add_executable(OpenAICompatibleClientTest
    services/OpenAICompatibleClientTest.cpp
)

target_link_libraries(OpenAICompatibleClientTest PRIVATE
    Qt6::Core
    Qt6::Network
    codexx_services
)

add_test(NAME OpenAICompatibleClientTest COMMAND OpenAICompatibleClientTest)

add_executable(PythonSidecarProtocolTest
    services/PythonSidecarProtocolTest.cpp
)

target_link_libraries(PythonSidecarProtocolTest PRIVATE
    Qt6::Core
    codexx_services
)

add_test(NAME PythonSidecarProtocolTest COMMAND PythonSidecarProtocolTest)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME PythonAgentSidecarTest
        COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${PROJECT_SOURCE_DIR}/python/agent_sidecar/tests
    )
    set_tests_properties(PythonAgentSidecarTest PROPERTIES
        ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/python/agent_sidecar"
    )

    add_executable(PythonSidecarClientTest
        services/PythonSidecarClientTest.cpp
    )

    target_compile_definitions(PythonSidecarClientTest PRIVATE
        CODEXX_PYTHON_EXECUTABLE="${Python3_EXECUTABLE}"
        CODEXX_PYTHON_SIDECAR_DIR="${PROJECT_SOURCE_DIR}/python/agent_sidecar"
    )

    target_link_libraries(PythonSidecarClientTest PRIVATE
        Qt6::Core
        codexx_services
    )

    add_test(NAME PythonSidecarClientTest COMMAND PythonSidecarClientTest)
endif()

# =========================================================================
# Support 测试
# =========================================================================

add_executable(AppLoggerTest
    support/AppLoggerTest.cpp
)

target_link_libraries(AppLoggerTest PRIVATE
    Qt6::Core
    codexx_support
)

add_test(NAME AppLoggerTest COMMAND AppLoggerTest)

add_executable(LogFileReaderTest
    support/LogFileReaderTest.cpp
)

target_link_libraries(LogFileReaderTest PRIVATE
    Qt6::Core
    codexx_support
)

add_test(NAME LogFileReaderTest COMMAND LogFileReaderTest)

# =========================================================================
# Tools 测试
# =========================================================================

add_executable(FileInteractionServiceTest
    tools/FileInteractionServiceTest.cpp
)

target_link_libraries(FileInteractionServiceTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME FileInteractionServiceTest COMMAND FileInteractionServiceTest)

add_executable(WorkspaceFileServiceTest
    tools/WorkspaceFileServiceTest.cpp
)

target_link_libraries(WorkspaceFileServiceTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME WorkspaceFileServiceTest COMMAND WorkspaceFileServiceTest)

add_executable(ProjectMemoryServiceTest
    tools/ProjectMemoryServiceTest.cpp
)

target_link_libraries(ProjectMemoryServiceTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME ProjectMemoryServiceTest COMMAND ProjectMemoryServiceTest)

add_executable(CommandPolicyTest
    tools/CommandPolicyTest.cpp
)

target_link_libraries(CommandPolicyTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME CommandPolicyTest COMMAND CommandPolicyTest)

add_executable(CommandRunnerTest
    tools/CommandRunnerTest.cpp
)

target_link_libraries(CommandRunnerTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME CommandRunnerTest COMMAND CommandRunnerTest)

add_executable(AgentToolCatalogTest
    tools/AgentToolCatalogTest.cpp
)

target_link_libraries(AgentToolCatalogTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME AgentToolCatalogTest COMMAND AgentToolCatalogTest)

add_executable(AgentToolRegistryTest
    tools/AgentToolRegistryTest.cpp
)

target_link_libraries(AgentToolRegistryTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME AgentToolRegistryTest COMMAND AgentToolRegistryTest)

add_executable(WorkspacePolicyTest
    tools/WorkspacePolicyTest.cpp
)

target_link_libraries(WorkspacePolicyTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME WorkspacePolicyTest COMMAND WorkspacePolicyTest)

add_executable(LocalToolContractTest
    tools/LocalToolContractTest.cpp
)

target_link_libraries(LocalToolContractTest PRIVATE
    Qt6::Core
)

target_include_directories(LocalToolContractTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME LocalToolContractTest COMMAND LocalToolContractTest)

add_executable(JsonToolsTest
    tools/JsonToolsTest.cpp
)

target_link_libraries(JsonToolsTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME JsonToolsTest COMMAND JsonToolsTest)

add_executable(MarkdownTextToolsTest
    tools/MarkdownTextToolsTest.cpp
)

target_link_libraries(MarkdownTextToolsTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME MarkdownTextToolsTest COMMAND MarkdownTextToolsTest)

# =========================================================================
# Storage 测试
# =========================================================================

add_executable(ChatHistoryStorageTest
    storage/ChatHistoryStorageTest.cpp
)

target_link_libraries(ChatHistoryStorageTest PRIVATE
    Qt6::Core
    Qt6::Sql
    codexx_storage
)

add_test(NAME ChatHistoryStorageTest COMMAND ChatHistoryStorageTest)

add_executable(ChatSessionExporterTest
    storage/ChatSessionExporterTest.cpp
)

target_link_libraries(ChatSessionExporterTest PRIVATE
    Qt6::Core
    codexx_storage
)

add_test(NAME ChatSessionExporterTest COMMAND ChatSessionExporterTest)

add_executable(ConfigStorageTest
    storage/ConfigStorageTest.cpp
)

target_link_libraries(ConfigStorageTest PRIVATE
    Qt6::Core
    codexx_storage
)

if(WIN32)
    target_link_libraries(ConfigStorageTest PRIVATE Advapi32)
endif()

add_test(NAME ConfigStorageTest COMMAND ConfigStorageTest)

add_executable(PromptTemplateStorageTest
    storage/PromptTemplateStorageTest.cpp
)

target_link_libraries(PromptTemplateStorageTest PRIVATE
    Qt6::Core
    codexx_storage
)

add_test(NAME PromptTemplateStorageTest COMMAND PromptTemplateStorageTest)

# =========================================================================
# UI 测试
# =========================================================================

add_executable(SettingsDialogSmokeTest
    ui/SettingsDialogSmokeTest.cpp
)

target_link_libraries(SettingsDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    codexx_ui
)

add_test(NAME SettingsDialogSmokeTest COMMAND SettingsDialogSmokeTest)

add_executable(RolePromptDialogSmokeTest
    ui/RolePromptDialogSmokeTest.cpp
)

target_link_libraries(RolePromptDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    codexx_ui
)

add_test(NAME RolePromptDialogSmokeTest COMMAND RolePromptDialogSmokeTest)

add_executable(ToolsDialogSmokeTest
    ui/ToolsDialogSmokeTest.cpp
)

target_link_libraries(ToolsDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    codexx_ui
)

add_test(NAME ToolsDialogSmokeTest COMMAND ToolsDialogSmokeTest)

add_executable(ScheduledTaskDialogSmokeTest
    ui/ScheduledTaskDialogSmokeTest.cpp
)

target_link_libraries(ScheduledTaskDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    codexx_ui
    codexx_app
)

add_test(NAME ScheduledTaskDialogSmokeTest COMMAND ScheduledTaskDialogSmokeTest)

add_executable(AgentPlanDialogSmokeTest
    ui/AgentPlanDialogSmokeTest.cpp
)

target_link_libraries(AgentPlanDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    codexx_ui
    codexx_app
)

add_test(NAME AgentPlanDialogSmokeTest COMMAND AgentPlanDialogSmokeTest)

add_executable(FileToolsDialogSmokeTest
    ui/FileToolsDialogSmokeTest.cpp
)

target_link_libraries(FileToolsDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    codexx_ui
)

add_test(NAME FileToolsDialogSmokeTest COMMAND FileToolsDialogSmokeTest)

add_executable(MessageWidgetTest
    ui/MessageWidgetTest.cpp
)

target_link_libraries(MessageWidgetTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    codexx_ui
)

add_test(NAME MessageWidgetTest COMMAND MessageWidgetTest)

# --- V16.2 代码高亮测试 ---
add_executable(CodeHighlighterTest
    ui/CodeHighlighterTest.cpp
)

target_link_libraries(CodeHighlighterTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    codexx_ui
)

add_test(NAME CodeHighlighterTest COMMAND CodeHighlighterTest)

# --- T04 ChatView 测试 ---
add_executable(ChatViewTest
    ui/ChatViewTest.cpp
)

target_link_libraries(ChatViewTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    codexx_ui
)

add_test(NAME ChatViewTest COMMAND ChatViewTest)

# --- T04 MainWindow 冒烟测试 ---
add_executable(MainWindowSmokeTest
    ui/MainWindowSmokeTest.cpp
)

target_link_libraries(MainWindowSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    codexx_ui
    codexx_app
)

if(WIN32)
    target_link_libraries(MainWindowSmokeTest PRIVATE Advapi32)
endif()

add_test(NAME MainWindowSmokeTest COMMAND MainWindowSmokeTest)

qt_add_executable(StyleResourceTest
    ui/StyleResourceTest.cpp
)

qt_add_resources(StyleResourceTest "style_test_resources"
    PREFIX "/"
    BASE "${PROJECT_SOURCE_DIR}/resources"
    FILES
        "${PROJECT_SOURCE_DIR}/resources/styles/app.qss"
)

target_link_libraries(StyleResourceTest PRIVATE
    Qt6::Core
)

add_test(NAME StyleResourceTest COMMAND StyleResourceTest)

# =========================================================================
# V11 工具测试
# =========================================================================

add_executable(GitReviewServiceTest
    tools/GitReviewServiceTest.cpp
)
target_link_libraries(GitReviewServiceTest PRIVATE
    Qt6::Core
    codexx_tools
)
add_test(NAME GitReviewServiceTest COMMAND GitReviewServiceTest)

add_executable(LogSummaryServiceTest
    tools/LogSummaryServiceTest.cpp
)
target_link_libraries(LogSummaryServiceTest PRIVATE
    Qt6::Core
    codexx_tools
)
add_test(NAME LogSummaryServiceTest COMMAND LogSummaryServiceTest)

add_executable(CsvDataServiceTest
    tools/CsvDataServiceTest.cpp
)
target_link_libraries(CsvDataServiceTest PRIVATE
    Qt6::Core
    codexx_tools
)
add_test(NAME CsvDataServiceTest COMMAND CsvDataServiceTest)

add_executable(ProjectFindServiceTest
    tools/ProjectFindServiceTest.cpp
)
target_link_libraries(ProjectFindServiceTest PRIVATE
    Qt6::Core
    codexx_tools
)
add_test(NAME ProjectFindServiceTest COMMAND ProjectFindServiceTest)

# =========================================================================
# V12 测试
# =========================================================================

add_executable(WindowDetectorTest
    tools/WindowDetectorTest.cpp
)
target_link_libraries(WindowDetectorTest PRIVATE
    Qt6::Core
    codexx_tools
)
add_test(NAME WindowDetectorTest COMMAND WindowDetectorTest)

add_executable(ScreenCaptureServiceTest
    tools/ScreenCaptureServiceTest.cpp
)
target_link_libraries(ScreenCaptureServiceTest PRIVATE
    Qt6::Core
    codexx_tools
)
add_test(NAME ScreenCaptureServiceTest COMMAND ScreenCaptureServiceTest)

# =========================================================================
# V12.1 上下文窗口管理测试
# =========================================================================

add_executable(TokenEstimatorTest
    app/TestTokenEstimator.cpp
)
target_link_libraries(TokenEstimatorTest PRIVATE
    Qt6::Core
    codexx_app
)
add_test(NAME TokenEstimatorTest COMMAND TokenEstimatorTest)

add_executable(ContextWindowManagerTest
    app/TestContextWindowManager.cpp
)
target_link_libraries(ContextWindowManagerTest PRIVATE
    Qt6::Core
    Qt6::Network
    codexx_app
)
add_test(NAME ContextWindowManagerTest COMMAND ContextWindowManagerTest)

add_executable(SummaryAPIClientTest
    app/TestSummaryAPIClient.cpp
)
target_link_libraries(SummaryAPIClientTest PRIVATE
    Qt6::Core
    Qt6::Network
    codexx_app
)
add_test(NAME SummaryAPIClientTest COMMAND SummaryAPIClientTest)

# =========================================================================
# V12.4 Chat 模式自动执行工具路由测试
# =========================================================================

add_executable(ChatToolExecutionTest
    services/ChatToolExecutionTest.cpp
)

target_link_libraries(ChatToolExecutionTest PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    codexx_app
)

if(WIN32)
    target_link_libraries(ChatToolExecutionTest PRIVATE Advapi32)
endif()

# MSVC 需要 /Zc:__cplusplus 以正确报告 C++17 标准
if(MSVC)
    target_compile_options(ChatToolExecutionTest PRIVATE /Zc:__cplusplus)
endif()

add_test(NAME ChatToolExecutionTest COMMAND ChatToolExecutionTest)

# =========================================================================
# V12.6 Agent 连续循环执行测试
# =========================================================================

add_executable(AgentLoopExecutionTest
    app/AgentLoopExecutionTest.cpp
)

target_link_libraries(AgentLoopExecutionTest PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    codexx_app
)

if(WIN32)
    target_link_libraries(AgentLoopExecutionTest PRIVATE Advapi32)
endif()

if(MSVC)
    target_compile_options(AgentLoopExecutionTest PRIVATE /Zc:__cplusplus)
endif()

add_test(NAME AgentLoopExecutionTest COMMAND AgentLoopExecutionTest)

# =========================================================================
# AppLaunch 冒烟测试 (仅 Windows)
# =========================================================================

if(WIN32)
    find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell)
    if(POWERSHELL_EXECUTABLE)
        add_test(
            NAME AppLaunchSmokeTest
            COMMAND ${POWERSHELL_EXECUTABLE}
                    -NoProfile
                    -ExecutionPolicy
                    Bypass
                    -File
                    ${CMAKE_CURRENT_SOURCE_DIR}/ui/AppLaunchSmokeTest.ps1
                    $<TARGET_FILE:AIChatDesktop>
        )
        set_tests_properties(AppLaunchSmokeTest PROPERTIES TIMEOUT 45)
    endif()
endif()

# =========================================================================
# V13.1 三层记忆系统测试
# =========================================================================

add_executable(ProjectMemoryManagerTest
    memory/ProjectMemoryManagerTest.cpp
)

target_link_libraries(ProjectMemoryManagerTest PRIVATE
    Qt6::Core
    codexx_memory
)

add_test(NAME ProjectMemoryManagerTest COMMAND ProjectMemoryManagerTest)

# =========================================================================
# V13.3 Skills + Hooks 测试
# =========================================================================

add_executable(SkillManagerTest
    skills/SkillManagerTest.cpp
)

target_link_libraries(SkillManagerTest PRIVATE
    Qt6::Core
    codexx_skills
)

add_test(NAME SkillManagerTest COMMAND SkillManagerTest)

add_executable(HookManagerTest
    hooks/HookManagerTest.cpp
)

target_link_libraries(HookManagerTest PRIVATE
    Qt6::Core
    codexx_hooks
)

add_test(NAME HookManagerTest COMMAND HookManagerTest)

# =========================================================================
# V14.1 OcrService 测试
# =========================================================================

add_executable(OcrServiceTest
    tools/OcrServiceTest.cpp
)

target_link_libraries(OcrServiceTest PRIVATE
    Qt6::Core
    Qt6::Gui
    codexx_tools
)

add_test(NAME OcrServiceTest COMMAND OcrServiceTest)

# =========================================================================
# V14.2 InputSimulator 测试
# =========================================================================

add_executable(InputSimulatorTest
    tools/InputSimulatorTest.cpp
)

target_link_libraries(InputSimulatorTest PRIVATE
    Qt6::Core
    codexx_tools
)

add_test(NAME InputSimulatorTest COMMAND InputSimulatorTest)

# =========================================================================
# V15.1 调度器测试
# =========================================================================

add_executable(CronParserTest
    scheduler/CronParserTest.cpp
)

target_link_libraries(CronParserTest PRIVATE
    Qt6::Core
    codexx_scheduler
)

add_test(NAME CronParserTest COMMAND CronParserTest)

add_executable(TaskSchedulerTest
    scheduler/TaskSchedulerTest.cpp
)

target_link_libraries(TaskSchedulerTest PRIVATE
    Qt6::Core
    Qt6::Test
    codexx_scheduler
)

add_test(NAME TaskSchedulerTest COMMAND TaskSchedulerTest)

# =========================================================================
# V15.4 MCP 测试
# =========================================================================

add_executable(McpConnectorTest
    mcp/McpConnectorTest.cpp
)

target_link_libraries(McpConnectorTest PRIVATE
    Qt6::Core
    codexx_mcp
)

add_test(NAME McpConnectorTest COMMAND McpConnectorTest)

add_executable(McpRegistryTest
    mcp/McpRegistryTest.cpp
)

target_link_libraries(McpRegistryTest PRIVATE
    Qt6::Core
    codexx_mcp
)

add_test(NAME McpRegistryTest COMMAND McpRegistryTest)
