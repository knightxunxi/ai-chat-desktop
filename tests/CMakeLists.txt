set(AGENT_TOOL_REGISTRY_SOURCES
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolRegistry.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolRegistry.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandRunner.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandRunner.h
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonCompactTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonFormatTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LocalTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/MarkdownCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspaceFileService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspaceFileService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/GitReviewService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/GitReviewService.h
    ${PROJECT_SOURCE_DIR}/src/tools/LogSummaryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LogSummaryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/CsvDataService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CsvDataService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectFindService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectFindService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WindowDetector.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WindowDetector.h
    ${PROJECT_SOURCE_DIR}/src/tools/ScreenCaptureService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ScreenCaptureService.h
    ${PROJECT_SOURCE_DIR}/src/tools/SystemInfoService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/SystemInfoService.h
    ${PROJECT_SOURCE_DIR}/src/tools/OcrService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/OcrService.h
    ${PROJECT_SOURCE_DIR}/src/tools/UiAutomationService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/UiAutomationService.h
    ${PROJECT_SOURCE_DIR}/src/tools/InputSimulator.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/InputSimulator.h
    ${PROJECT_SOURCE_DIR}/src/tools/ForegroundValidator.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ForegroundValidator.h
    ${PROJECT_SOURCE_DIR}/src/tools/AssistantService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AssistantService.h
)

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
    ${PROJECT_SOURCE_DIR}/src/core/ProviderPreset.cpp
)

target_link_libraries(ProviderPresetTest PRIVATE
    Qt6::Core
)

target_include_directories(ProviderPresetTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME ProviderPresetTest COMMAND ProviderPresetTest)

add_executable(SessionSummaryListTest
    app/SessionSummaryListTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/SessionSummaryList.cpp
)

target_link_libraries(SessionSummaryListTest PRIVATE
    Qt6::Core
)

target_include_directories(SessionSummaryListTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME SessionSummaryListTest COMMAND SessionSummaryListTest)

add_executable(ProjectInstructionServiceTest
    app/ProjectInstructionServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/ProjectInstructionService.cpp
    ${PROJECT_SOURCE_DIR}/src/app/ProjectInstructionService.h
)

target_link_libraries(ProjectInstructionServiceTest PRIVATE
    Qt6::Core
)

target_include_directories(ProjectInstructionServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME ProjectInstructionServiceTest COMMAND ProjectInstructionServiceTest)

add_executable(AgentPlanParserTest
    app/AgentPlanParserTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanParser.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanParser.h
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentPlanParserTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentPlanParserTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentPlanParserTest COMMAND AgentPlanParserTest)

add_executable(AgentPlanExecutorTest
    app/AgentPlanExecutorTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanExecutor.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanExecutor.h
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolRegistry.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolRegistry.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandRunner.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandRunner.h
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonCompactTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonFormatTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LocalTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/MarkdownCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspaceFileService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspaceFileService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/GitReviewService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/GitReviewService.h
    ${PROJECT_SOURCE_DIR}/src/tools/LogSummaryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LogSummaryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/CsvDataService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CsvDataService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectFindService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectFindService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WindowDetector.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WindowDetector.h
    ${PROJECT_SOURCE_DIR}/src/tools/ScreenCaptureService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ScreenCaptureService.h
    ${PROJECT_SOURCE_DIR}/src/tools/SystemInfoService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/SystemInfoService.h
    ${PROJECT_SOURCE_DIR}/src/tools/OcrService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/OcrService.h
    ${PROJECT_SOURCE_DIR}/src/tools/UiAutomationService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/UiAutomationService.h
    ${PROJECT_SOURCE_DIR}/src/tools/InputSimulator.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/InputSimulator.h
    ${PROJECT_SOURCE_DIR}/src/tools/ForegroundValidator.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ForegroundValidator.h
    ${PROJECT_SOURCE_DIR}/src/tools/AssistantService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AssistantService.h
)

target_link_libraries(AgentPlanExecutorTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentPlanExecutorTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentPlanExecutorTest COMMAND AgentPlanExecutorTest)

add_executable(AgentPlanPromptBuilderTest
    app/AgentPlanPromptBuilderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillCatalog.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanPromptBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanPromptBuilder.h
    ${PROJECT_SOURCE_DIR}/src/app/ProjectInstructionService.h
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentPlanPromptBuilderTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentPlanPromptBuilderTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentPlanPromptBuilderTest COMMAND AgentPlanPromptBuilderTest)

add_executable(AgentCommandSkillCatalogTest
    app/AgentCommandSkillCatalogTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillCatalog.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
)

target_link_libraries(AgentCommandSkillCatalogTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentCommandSkillCatalogTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentCommandSkillCatalogTest COMMAND AgentCommandSkillCatalogTest)

add_executable(AgentCommandSkillFileServiceTest
    app/AgentCommandSkillFileServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillCatalog.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillFileService.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillFileService.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentCommandSkillFileServiceTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentCommandSkillFileServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentCommandSkillFileServiceTest COMMAND AgentCommandSkillFileServiceTest)

add_executable(AgentToolCallPlanBuilderTest
    app/AgentToolCallPlanBuilderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentToolCallPlanBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentToolCallPlanBuilder.h
    ${PROJECT_SOURCE_DIR}/src/services/ToolCall.h
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentToolCallPlanBuilderTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentToolCallPlanBuilderTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentToolCallPlanBuilderTest COMMAND AgentToolCallPlanBuilderTest)

add_executable(AgentLoopActionParserTest
    app/AgentLoopActionParserTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopActionParser.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopActionParser.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentLoopActionParserTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentLoopActionParserTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentLoopActionParserTest COMMAND AgentLoopActionParserTest)

add_executable(AgentLoopPromptBuilderTest
    app/AgentLoopPromptBuilderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopPromptBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopPromptBuilder.h
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentLoopPromptBuilderTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentLoopPromptBuilderTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentLoopPromptBuilderTest COMMAND AgentLoopPromptBuilderTest)

add_executable(AgentLoopControllerTest
    app/AgentLoopControllerTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopController.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopController.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopActionParser.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopActionParser.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopPromptBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopPromptBuilder.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/services/AIClient.h
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentLoopControllerTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentLoopControllerTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentLoopControllerTest COMMAND AgentLoopControllerTest)

add_executable(StreamParserTest
    services/StreamParserTest.cpp
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.cpp
    ${PROJECT_SOURCE_DIR}/src/services/ToolCall.h
)

target_link_libraries(StreamParserTest PRIVATE
    Qt6::Core
)

target_include_directories(StreamParserTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME StreamParserTest COMMAND StreamParserTest)

# V12.3 流式工具执行测试
add_executable(StreamParserV12Test
    services/StreamParserV12Test.cpp
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.cpp
    ${PROJECT_SOURCE_DIR}/src/services/ToolCall.h
)

target_link_libraries(StreamParserV12Test PRIVATE
    Qt6::Core
)

target_include_directories(StreamParserV12Test PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME StreamParserV12Test COMMAND StreamParserV12Test)

add_executable(OpenAICompatibleClientTest
    services/OpenAICompatibleClientTest.cpp
    ${PROJECT_SOURCE_DIR}/src/services/AIClient.h
    ${PROJECT_SOURCE_DIR}/src/services/OpenAICompatibleClient.h
    ${PROJECT_SOURCE_DIR}/src/services/OpenAICompatibleClient.cpp
    ${PROJECT_SOURCE_DIR}/src/services/RequestErrorCategory.h
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.h
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.cpp
    ${PROJECT_SOURCE_DIR}/src/services/ToolCall.h
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.h
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
)

target_link_libraries(OpenAICompatibleClientTest PRIVATE
    Qt6::Core
    Qt6::Network
)

target_include_directories(OpenAICompatibleClientTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME OpenAICompatibleClientTest COMMAND OpenAICompatibleClientTest)

add_executable(AppLoggerTest
    support/AppLoggerTest.cpp
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
)

target_link_libraries(AppLoggerTest PRIVATE
    Qt6::Core
)

target_include_directories(AppLoggerTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AppLoggerTest COMMAND AppLoggerTest)

add_executable(LogFileReaderTest
    support/LogFileReaderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/support/LogFileReader.cpp
)

target_link_libraries(LogFileReaderTest PRIVATE
    Qt6::Core
)

target_include_directories(LogFileReaderTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME LogFileReaderTest COMMAND LogFileReaderTest)

add_executable(FileInteractionServiceTest
    tools/FileInteractionServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)

target_link_libraries(FileInteractionServiceTest PRIVATE
    Qt6::Core
)

target_include_directories(FileInteractionServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME FileInteractionServiceTest COMMAND FileInteractionServiceTest)

add_executable(WorkspaceFileServiceTest
    tools/WorkspaceFileServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspaceFileService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspaceFileService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.h
)

target_link_libraries(WorkspaceFileServiceTest PRIVATE
    Qt6::Core
)

target_include_directories(WorkspaceFileServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME WorkspaceFileServiceTest COMMAND WorkspaceFileServiceTest)

add_executable(ProjectMemoryServiceTest
    tools/ProjectMemoryServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)

target_link_libraries(ProjectMemoryServiceTest PRIVATE
    Qt6::Core
)

target_include_directories(ProjectMemoryServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME ProjectMemoryServiceTest COMMAND ProjectMemoryServiceTest)

add_executable(CommandPolicyTest
    tools/CommandPolicyTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.h
)

target_link_libraries(CommandPolicyTest PRIVATE
    Qt6::Core
)

target_include_directories(CommandPolicyTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME CommandPolicyTest COMMAND CommandPolicyTest)

add_executable(CommandRunnerTest
    tools/CommandRunnerTest.cpp
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandRunner.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandRunner.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)

target_link_libraries(CommandRunnerTest PRIVATE
    Qt6::Core
)

target_include_directories(CommandRunnerTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME CommandRunnerTest COMMAND CommandRunnerTest)

add_executable(AgentToolCatalogTest
    tools/AgentToolCatalogTest.cpp
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentToolCatalogTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentToolCatalogTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentToolCatalogTest COMMAND AgentToolCatalogTest)

add_executable(AgentToolRegistryTest
    tools/AgentToolRegistryTest.cpp
    ${AGENT_TOOL_REGISTRY_SOURCES}
)

target_link_libraries(AgentToolRegistryTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentToolRegistryTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentToolRegistryTest COMMAND AgentToolRegistryTest)

add_executable(WorkspacePolicyTest
    tools/WorkspacePolicyTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.h
)

target_link_libraries(WorkspacePolicyTest PRIVATE
    Qt6::Core
)

target_include_directories(WorkspacePolicyTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
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
    ${PROJECT_SOURCE_DIR}/src/tools/JsonCompactTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonFormatTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LocalTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)

target_link_libraries(JsonToolsTest PRIVATE
    Qt6::Core
)

target_include_directories(JsonToolsTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME JsonToolsTest COMMAND JsonToolsTest)

add_executable(MarkdownTextToolsTest
    tools/MarkdownTextToolsTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LocalTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/MarkdownCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)

target_link_libraries(MarkdownTextToolsTest PRIVATE
    Qt6::Core
)

target_include_directories(MarkdownTextToolsTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME MarkdownTextToolsTest COMMAND MarkdownTextToolsTest)

add_executable(ChatHistoryStorageTest
    storage/ChatHistoryStorageTest.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/ChatHistoryStorage.cpp
)

target_link_libraries(ChatHistoryStorageTest PRIVATE
    Qt6::Core
    Qt6::Sql
)

target_include_directories(ChatHistoryStorageTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME ChatHistoryStorageTest COMMAND ChatHistoryStorageTest)

add_executable(ChatSessionExporterTest
    storage/ChatSessionExporterTest.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/ChatSessionExporter.cpp
)

target_link_libraries(ChatSessionExporterTest PRIVATE
    Qt6::Core
)

target_include_directories(ChatSessionExporterTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME ChatSessionExporterTest COMMAND ChatSessionExporterTest)

add_executable(ConfigStorageTest
    storage/ConfigStorageTest.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/ConfigStorage.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/CredentialStorage.h
    ${PROJECT_SOURCE_DIR}/src/storage/WindowsCredentialStorage.h
    ${PROJECT_SOURCE_DIR}/src/storage/WindowsCredentialStorage.cpp
)

target_link_libraries(ConfigStorageTest PRIVATE
    Qt6::Core
)

if(WIN32)
    target_link_libraries(ConfigStorageTest PRIVATE Advapi32)
endif()

target_include_directories(ConfigStorageTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME ConfigStorageTest COMMAND ConfigStorageTest)

add_executable(PromptTemplateStorageTest
    storage/PromptTemplateStorageTest.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/PromptTemplateStorage.cpp
)

target_link_libraries(PromptTemplateStorageTest PRIVATE
    Qt6::Core
)

target_include_directories(PromptTemplateStorageTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME PromptTemplateStorageTest COMMAND PromptTemplateStorageTest)

add_executable(SettingsDialogSmokeTest
    ui/SettingsDialogSmokeTest.cpp
    ${PROJECT_SOURCE_DIR}/src/core/ProviderPreset.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/SettingsDialog.cpp
)

target_link_libraries(SettingsDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(SettingsDialogSmokeTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME SettingsDialogSmokeTest COMMAND SettingsDialogSmokeTest)

add_executable(RolePromptDialogSmokeTest
    ui/RolePromptDialogSmokeTest.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/PromptTemplateStorage.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/RolePromptDialog.cpp
)

target_link_libraries(RolePromptDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(RolePromptDialogSmokeTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME RolePromptDialogSmokeTest COMMAND RolePromptDialogSmokeTest)

add_executable(ToolsDialogSmokeTest
    ui/ToolsDialogSmokeTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/JsonCompactTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonFormatTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LocalTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/MarkdownCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
    ${PROJECT_SOURCE_DIR}/src/ui/ToolsDialog.h
    ${PROJECT_SOURCE_DIR}/src/ui/ToolsDialog.cpp
)

target_link_libraries(ToolsDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(ToolsDialogSmokeTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME ToolsDialogSmokeTest COMMAND ToolsDialogSmokeTest)

add_executable(AgentPlanDialogSmokeTest
    ui/AgentPlanDialogSmokeTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopController.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopController.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopActionParser.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopActionParser.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopPromptBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopPromptBuilder.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanExecutor.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanExecutor.h
    ${PROJECT_SOURCE_DIR}/src/services/AIClient.h
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolRegistry.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolRegistry.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandPolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/CommandRunner.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CommandRunner.h
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonCompactTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonFormatTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LocalTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/MarkdownCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectMemoryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspaceFileService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspaceFileService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/GitReviewService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/GitReviewService.h
    ${PROJECT_SOURCE_DIR}/src/tools/LogSummaryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LogSummaryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/CsvDataService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CsvDataService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectFindService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectFindService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WindowDetector.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WindowDetector.h
    ${PROJECT_SOURCE_DIR}/src/tools/ScreenCaptureService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ScreenCaptureService.h
    ${PROJECT_SOURCE_DIR}/src/tools/SystemInfoService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/SystemInfoService.h
    ${PROJECT_SOURCE_DIR}/src/tools/OcrService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/OcrService.h
    ${PROJECT_SOURCE_DIR}/src/tools/UiAutomationService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/UiAutomationService.h
    ${PROJECT_SOURCE_DIR}/src/tools/InputSimulator.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/InputSimulator.h
    ${PROJECT_SOURCE_DIR}/src/tools/ForegroundValidator.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ForegroundValidator.h
    ${PROJECT_SOURCE_DIR}/src/tools/AssistantService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AssistantService.h
    ${PROJECT_SOURCE_DIR}/src/ui/AgentPlanDialog.h
    ${PROJECT_SOURCE_DIR}/src/ui/AgentPlanDialog.cpp
)

target_link_libraries(AgentPlanDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(AgentPlanDialogSmokeTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentPlanDialogSmokeTest COMMAND AgentPlanDialogSmokeTest)

add_executable(FileToolsDialogSmokeTest
    ui/FileToolsDialogSmokeTest.cpp
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/FileInteractionService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
    ${PROJECT_SOURCE_DIR}/src/ui/FileToolsDialog.h
    ${PROJECT_SOURCE_DIR}/src/ui/FileToolsDialog.cpp
)

target_link_libraries(FileToolsDialogSmokeTest PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(FileToolsDialogSmokeTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME FileToolsDialogSmokeTest COMMAND FileToolsDialogSmokeTest)

add_executable(MessageWidgetTest
    ui/MessageWidgetTest.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/MessageWidget.cpp
)

target_link_libraries(MessageWidgetTest PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(MessageWidgetTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME MessageWidgetTest COMMAND MessageWidgetTest)

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

# --- V11 工具测试 ---

add_executable(GitReviewServiceTest
    tools/GitReviewServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/GitReviewService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/GitReviewService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)
target_link_libraries(GitReviewServiceTest PRIVATE
    Qt6::Core
)
target_include_directories(GitReviewServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME GitReviewServiceTest COMMAND GitReviewServiceTest)

add_executable(LogSummaryServiceTest
    tools/LogSummaryServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LogSummaryService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LogSummaryService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
)
target_link_libraries(LogSummaryServiceTest PRIVATE
    Qt6::Core
)
target_include_directories(LogSummaryServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME LogSummaryServiceTest COMMAND LogSummaryServiceTest)

add_executable(CsvDataServiceTest
    tools/CsvDataServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CsvDataService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/CsvDataService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)
target_link_libraries(CsvDataServiceTest PRIVATE
    Qt6::Core
)
target_include_directories(CsvDataServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME CsvDataServiceTest COMMAND CsvDataServiceTest)

add_executable(ProjectFindServiceTest
    tools/ProjectFindServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectFindService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ProjectFindService.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)
target_link_libraries(ProjectFindServiceTest PRIVATE
    Qt6::Core
)
target_include_directories(ProjectFindServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME ProjectFindServiceTest COMMAND ProjectFindServiceTest)

# --- V12 测试 ---

add_executable(WindowDetectorTest
    tools/WindowDetectorTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WindowDetector.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WindowDetector.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)
target_link_libraries(WindowDetectorTest PRIVATE
    Qt6::Core
)
target_include_directories(WindowDetectorTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME WindowDetectorTest COMMAND WindowDetectorTest)

add_executable(ScreenCaptureServiceTest
    tools/ScreenCaptureServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ScreenCaptureService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ScreenCaptureService.h
    ${PROJECT_SOURCE_DIR}/src/tools/SystemInfoService.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/SystemInfoService.h
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/WorkspacePolicy.h
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
)
target_link_libraries(ScreenCaptureServiceTest PRIVATE
    Qt6::Core
)
target_include_directories(ScreenCaptureServiceTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME ScreenCaptureServiceTest COMMAND ScreenCaptureServiceTest)

# --- V12.1 上下文窗口管理测试 ---

add_executable(TokenEstimatorTest
    app/TestTokenEstimator.cpp
    ${PROJECT_SOURCE_DIR}/src/app/TokenEstimator.cpp
)
target_link_libraries(TokenEstimatorTest PRIVATE
    Qt6::Core
)
target_include_directories(TokenEstimatorTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME TokenEstimatorTest COMMAND TokenEstimatorTest)

add_executable(ContextWindowManagerTest
    app/TestContextWindowManager.cpp
    ${PROJECT_SOURCE_DIR}/src/app/TokenEstimator.cpp
    ${PROJECT_SOURCE_DIR}/src/app/ContextWindowManager.cpp
    ${PROJECT_SOURCE_DIR}/src/app/SummaryAPIClient.cpp
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
)
target_link_libraries(ContextWindowManagerTest PRIVATE
    Qt6::Core
    Qt6::Network
)
target_include_directories(ContextWindowManagerTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME ContextWindowManagerTest COMMAND ContextWindowManagerTest)

add_executable(SummaryAPIClientTest
    app/TestSummaryAPIClient.cpp
    ${PROJECT_SOURCE_DIR}/src/app/SummaryAPIClient.cpp
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
)
target_link_libraries(SummaryAPIClientTest PRIVATE
    Qt6::Core
    Qt6::Network
)
target_include_directories(SummaryAPIClientTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
add_test(NAME SummaryAPIClientTest COMMAND SummaryAPIClientTest)

# --- V12.4 Chat 模式自动执行工具路由测试 ---

set(APPLICATION_CONTROLLER_TEST_SOURCES
    ${PROJECT_SOURCE_DIR}/src/app/ApplicationController.cpp
    ${PROJECT_SOURCE_DIR}/src/app/ApplicationController.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanExecutor.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanExecutor.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopPromptBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentLoopPromptBuilder.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillCatalog.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillFileService.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentCommandSkillFileService.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanParser.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanParser.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanPromptBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanPromptBuilder.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentToolCallPlanBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentToolCallPlanBuilder.h
    ${PROJECT_SOURCE_DIR}/src/app/ContextWindowManager.cpp
    ${PROJECT_SOURCE_DIR}/src/app/ContextWindowManager.h
    ${PROJECT_SOURCE_DIR}/src/app/ProjectInstructionService.cpp
    ${PROJECT_SOURCE_DIR}/src/app/ProjectInstructionService.h
    ${PROJECT_SOURCE_DIR}/src/app/SessionSummaryList.cpp
    ${PROJECT_SOURCE_DIR}/src/app/SessionSummaryList.h
    ${PROJECT_SOURCE_DIR}/src/app/SummaryAPIClient.cpp
    ${PROJECT_SOURCE_DIR}/src/app/SummaryAPIClient.h
    ${PROJECT_SOURCE_DIR}/src/app/TokenEstimator.cpp
    ${PROJECT_SOURCE_DIR}/src/app/TokenEstimator.h
    ${PROJECT_SOURCE_DIR}/src/core/ProviderPreset.cpp
    ${PROJECT_SOURCE_DIR}/src/core/ProviderPreset.h
    ${PROJECT_SOURCE_DIR}/src/services/OpenAICompatibleClient.cpp
    ${PROJECT_SOURCE_DIR}/src/services/OpenAICompatibleClient.h
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.cpp
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.h
    ${PROJECT_SOURCE_DIR}/src/services/ToolCall.h
    ${PROJECT_SOURCE_DIR}/src/services/AIClient.h
    ${PROJECT_SOURCE_DIR}/src/services/RequestErrorCategory.h
    ${PROJECT_SOURCE_DIR}/src/storage/ChatHistoryStorage.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/ChatHistoryStorage.h
    ${PROJECT_SOURCE_DIR}/src/storage/ChatSessionExporter.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/ChatSessionExporter.h
    ${PROJECT_SOURCE_DIR}/src/storage/ConfigStorage.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/ConfigStorage.h
    ${PROJECT_SOURCE_DIR}/src/storage/CredentialStorage.h
    ${PROJECT_SOURCE_DIR}/src/storage/PromptTemplateStorage.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/PromptTemplateStorage.h
    ${PROJECT_SOURCE_DIR}/src/storage/WindowsCredentialStorage.cpp
    ${PROJECT_SOURCE_DIR}/src/storage/WindowsCredentialStorage.h
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.h
    ${PROJECT_SOURCE_DIR}/src/support/LogFileReader.cpp
    ${PROJECT_SOURCE_DIR}/src/support/LogFileReader.h
    ${AGENT_TOOL_REGISTRY_SOURCES}
    ${PROJECT_SOURCE_DIR}/src/memory/MemoryEntry.h
    ${PROJECT_SOURCE_DIR}/src/memory/DailyMemoryWriter.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/DailyMemoryWriter.h
    ${PROJECT_SOURCE_DIR}/src/memory/ProjectMemoryManager.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/ProjectMemoryManager.h
)

add_executable(ChatToolExecutionTest
    services/ChatToolExecutionTest.cpp
    ${APPLICATION_CONTROLLER_TEST_SOURCES}
)

target_link_libraries(ChatToolExecutionTest PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
)

if(WIN32)
    target_link_libraries(ChatToolExecutionTest PRIVATE Advapi32)
endif()

target_include_directories(ChatToolExecutionTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

# MSVC 需要 /Zc:__cplusplus 以正确报告 C++17 标准
if(MSVC)
    target_compile_options(ChatToolExecutionTest PRIVATE /Zc:__cplusplus)
endif()

add_test(NAME ChatToolExecutionTest COMMAND ChatToolExecutionTest)

# --- V12.6 Agent 连续循环执行测试 ---

add_executable(AgentLoopExecutionTest
    app/AgentLoopExecutionTest.cpp
    ${APPLICATION_CONTROLLER_TEST_SOURCES}
)

target_link_libraries(AgentLoopExecutionTest PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
)

if(WIN32)
    target_link_libraries(AgentLoopExecutionTest PRIVATE Advapi32)
endif()

target_include_directories(AgentLoopExecutionTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(AgentLoopExecutionTest PRIVATE /Zc:__cplusplus)
endif()

add_test(NAME AgentLoopExecutionTest COMMAND AgentLoopExecutionTest)

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

# --- V13.1 三层记忆系统测试 ---

add_executable(ProjectMemoryManagerTest
    memory/ProjectMemoryManagerTest.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/MemoryEntry.h
    ${PROJECT_SOURCE_DIR}/src/memory/DailyMemoryWriter.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/DailyMemoryWriter.h
    ${PROJECT_SOURCE_DIR}/src/memory/ProjectMemoryManager.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/ProjectMemoryManager.h
)

target_link_libraries(ProjectMemoryManagerTest PRIVATE
    Qt6::Core
)

target_include_directories(ProjectMemoryManagerTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME ProjectMemoryManagerTest COMMAND ProjectMemoryManagerTest)
