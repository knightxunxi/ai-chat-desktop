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

add_executable(AgentPlanParserTest
    app/AgentPlanParserTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanParser.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanParser.h
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
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
    ${PROJECT_SOURCE_DIR}/src/tools/JsonCompactTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonFormatTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LocalTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/MarkdownCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
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
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanPromptBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanPromptBuilder.h
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
)

target_link_libraries(AgentPlanPromptBuilderTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentPlanPromptBuilderTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentPlanPromptBuilderTest COMMAND AgentPlanPromptBuilderTest)

add_executable(StreamParserTest
    services/StreamParserTest.cpp
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.cpp
)

target_link_libraries(StreamParserTest PRIVATE
    Qt6::Core
)

target_include_directories(StreamParserTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME StreamParserTest COMMAND StreamParserTest)

add_executable(OpenAICompatibleClientTest
    services/OpenAICompatibleClientTest.cpp
    ${PROJECT_SOURCE_DIR}/src/services/AIClient.h
    ${PROJECT_SOURCE_DIR}/src/services/OpenAICompatibleClient.h
    ${PROJECT_SOURCE_DIR}/src/services/OpenAICompatibleClient.cpp
    ${PROJECT_SOURCE_DIR}/src/services/RequestErrorCategory.h
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.h
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.cpp
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

add_executable(AgentToolCatalogTest
    tools/AgentToolCatalogTest.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
)

target_link_libraries(AgentToolCatalogTest PRIVATE
    Qt6::Core
)

target_include_directories(AgentToolCatalogTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME AgentToolCatalogTest COMMAND AgentToolCatalogTest)

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
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlan.h
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanExecutor.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AgentPlanExecutor.h
    ${PROJECT_SOURCE_DIR}/src/support/AppLogger.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/AgentToolCatalog.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonCompactTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonFormatTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/JsonTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/LocalTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/MarkdownCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextCleanupTool.h
    ${PROJECT_SOURCE_DIR}/src/tools/TextTools.cpp
    ${PROJECT_SOURCE_DIR}/src/tools/ToolResult.h
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
