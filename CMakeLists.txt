cmake_minimum_required(VERSION 3.22)

project(AIChatDesktop
    VERSION 0.3.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Sql
    Test
)

qt_standard_project_setup()

qt_add_executable(AIChatDesktop
    src/main.cpp
    src/app/AgentPlan.h
    src/app/AgentPlan.cpp
    src/app/AgentCommandSkillCatalog.h
    src/app/AgentCommandSkillCatalog.cpp
    src/app/AgentCommandSkillFileService.h
    src/app/AgentCommandSkillFileService.cpp
    src/app/AgentToolCallPlanBuilder.h
    src/app/AgentToolCallPlanBuilder.cpp
    src/app/AgentLoopActionParser.h
    src/app/AgentLoopActionParser.cpp
    src/app/AgentLoopController.h
    src/app/AgentLoopController.cpp
    src/app/AgentLoopPromptBuilder.h
    src/app/AgentLoopPromptBuilder.cpp
    src/app/AgentPlanExecutor.h
    src/app/AgentPlanExecutor.cpp
    src/app/AgentPlanParser.h
    src/app/AgentPlanParser.cpp
    src/app/AgentPlanPromptBuilder.h
    src/app/AgentPlanPromptBuilder.cpp
    src/app/TokenEstimator.h
    src/app/TokenEstimator.cpp
    src/app/SummaryAPIClient.h
    src/app/SummaryAPIClient.cpp
    src/app/ContextWindowManager.h
    src/app/ContextWindowManager.cpp
    src/app/ApplicationController.h
    src/app/ApplicationController.cpp
    src/app/ProjectInstructionService.h
    src/app/ProjectInstructionService.cpp
    src/app/SessionSummaryList.h
    src/app/SessionSummaryList.cpp
    src/core/AppConfig.h
    src/core/AppLanguage.h
    src/core/ChatMessage.h
    src/core/ChatSession.h
    src/core/MessageRole.h
    src/core/PromptTemplate.h
    src/core/ProviderPreset.h
    src/core/ProviderPreset.cpp
    src/core/SessionListFilter.h
    src/services/AIClient.h
    src/services/OpenAICompatibleClient.h
    src/services/OpenAICompatibleClient.cpp
    src/services/RequestErrorCategory.h
    src/services/StreamParser.h
    src/services/StreamParser.cpp
    src/services/ToolCall.h
    src/support/AppLogger.h
    src/support/AppLogger.cpp
    src/support/LogFileReader.h
    src/support/LogFileReader.cpp
    src/skills/SkillDefinition.h
    src/skills/SkillFileParser.h
    src/skills/SkillFileParser.cpp
    src/skills/SkillManager.h
    src/skills/SkillManager.cpp
    src/hooks/HookDefinition.h
    src/hooks/HookManager.h
    src/hooks/HookManager.cpp
    src/hooks/BuiltinHooks.h
    src/hooks/BuiltinHooks.cpp
    src/hooks/ScriptHookRunner.h
    src/hooks/ScriptHookRunner.cpp
    src/tools/AgentToolCatalog.h
    src/tools/AgentToolCatalog.cpp
    src/tools/AgentToolRegistry.h
    src/tools/AgentToolRegistry.cpp
    src/tools/CommandPolicy.h
    src/tools/CommandPolicy.cpp
    src/tools/CommandRunner.h
    src/tools/CommandRunner.cpp
    src/tools/FileInteractionService.h
    src/tools/FileInteractionService.cpp
    src/tools/JsonCompactTool.h
    src/tools/JsonFormatTool.h
    src/tools/JsonTools.cpp
    src/tools/LocalTool.h
    src/tools/MarkdownCleanupTool.h
    src/tools/ProjectMemoryService.h
    src/tools/ProjectMemoryService.cpp
    src/memory/MemoryEntry.h
    src/memory/DailyMemoryWriter.h
    src/memory/DailyMemoryWriter.cpp
    src/memory/ProjectMemoryManager.h
    src/memory/ProjectMemoryManager.cpp
    src/tools/TextCleanupTool.h
    src/tools/TextTools.cpp
    src/tools/ToolResult.h
    src/tools/WorkspaceFileService.h
    src/tools/WorkspaceFileService.cpp
    src/tools/GitReviewService.h
    src/tools/GitReviewService.cpp
    src/tools/LogSummaryService.h
    src/tools/LogSummaryService.cpp
    src/tools/CsvDataService.h
    src/tools/CsvDataService.cpp
    src/tools/ProjectFindService.h
    src/tools/ProjectFindService.cpp
    src/tools/WindowDetector.h
    src/tools/WindowDetector.cpp
    src/tools/ScreenCaptureService.h
    src/tools/ScreenCaptureService.cpp
    src/tools/SystemInfoService.h
    src/tools/SystemInfoService.cpp
    src/tools/OcrService.h
    src/tools/OcrService.cpp
    src/tools/UiAutomationService.h
    src/tools/UiAutomationService.cpp
    src/tools/InputSimulator.h
    src/tools/InputSimulator.cpp
    src/tools/ForegroundValidator.h
    src/tools/ForegroundValidator.cpp
    src/tools/AssistantService.h
    src/tools/AssistantService.cpp
    src/tools/WorkspacePolicy.h
    src/tools/WorkspacePolicy.cpp
    src/scheduler/ScheduledTask.h
    src/scheduler/TaskScheduler.h
    src/scheduler/TaskScheduler.cpp
    src/scheduler/TaskStorage.h
    src/scheduler/TaskStorage.cpp
    src/mcp/McpConnector.h
    src/mcp/McpConnector.cpp
    src/mcp/McpRegistry.h
    src/mcp/McpRegistry.cpp
    src/storage/CredentialStorage.h
    src/storage/WindowsCredentialStorage.h
    src/storage/WindowsCredentialStorage.cpp
    src/storage/ConfigStorage.h
    src/storage/ConfigStorage.cpp
    src/storage/ChatHistoryStorage.h
    src/storage/ChatHistoryStorage.cpp
    src/storage/ChatSessionExporter.h
    src/storage/ChatSessionExporter.cpp
    src/storage/PromptTemplateStorage.h
    src/storage/PromptTemplateStorage.cpp
    src/ui/ChatView.h
    src/ui/ChatView.cpp
    src/ui/FileToolsDialog.h
    src/ui/FileToolsDialog.cpp
    src/ui/LogViewerDialog.h
    src/ui/LogViewerDialog.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/MessageWidget.h
    src/ui/MessageWidget.cpp
    src/ui/RolePromptDialog.h
    src/ui/RolePromptDialog.cpp
    src/ui/ScheduledTaskDialog.h
    src/ui/ScheduledTaskDialog.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/ToolsDialog.h
    src/ui/ToolsDialog.cpp
    src/ui/ScheduledTaskDialog.h
    src/ui/ScheduledTaskDialog.cpp
)

qt_add_resources(AIChatDesktop "app_resources"
    PREFIX "/"
    BASE "${PROJECT_SOURCE_DIR}/resources"
    FILES
        "${PROJECT_SOURCE_DIR}/resources/styles/app.qss"
)

target_link_libraries(AIChatDesktop PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)

if(WIN32)
    target_link_libraries(AIChatDesktop PRIVATE Advapi32)
endif()

if(MINGW)
    # MinGW + Qt6 EntryPoint: WIN32_EXECUTABLE 会导致 __imp___argc 链接错误
    # 使用 -mwindows 手动抑制控制台窗口
    target_link_options(AIChatDesktop PRIVATE -mwindows)
    set_target_properties(AIChatDesktop PROPERTIES WIN32_EXECUTABLE FALSE)
else()
    set_target_properties(AIChatDesktop PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(AIChatDesktop PRIVATE
    src
)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
