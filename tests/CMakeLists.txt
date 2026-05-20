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
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.h
    ${PROJECT_SOURCE_DIR}/src/services/StreamParser.cpp
)

target_link_libraries(OpenAICompatibleClientTest PRIVATE
    Qt6::Core
    Qt6::Network
)

target_include_directories(OpenAICompatibleClientTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME OpenAICompatibleClientTest COMMAND OpenAICompatibleClientTest)

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

add_executable(SettingsDialogSmokeTest
    ui/SettingsDialogSmokeTest.cpp
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
