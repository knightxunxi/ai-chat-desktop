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
        set_tests_properties(AppLaunchSmokeTest PROPERTIES TIMEOUT 30)
    endif()
endif()
