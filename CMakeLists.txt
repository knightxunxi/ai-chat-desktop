cmake_minimum_required(VERSION 3.22)

project(AIChatDesktop
    VERSION 0.1.0
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
)

qt_standard_project_setup()

qt_add_executable(AIChatDesktop
    src/main.cpp
    src/app/ApplicationController.h
    src/app/ApplicationController.cpp
    src/core/AppConfig.h
    src/core/AppLanguage.h
    src/core/ChatMessage.h
    src/core/ChatSession.h
    src/core/MessageRole.h
    src/services/AIClient.h
    src/services/OpenAICompatibleClient.h
    src/services/OpenAICompatibleClient.cpp
    src/services/StreamParser.h
    src/services/StreamParser.cpp
    src/storage/ConfigStorage.h
    src/storage/ConfigStorage.cpp
    src/storage/ChatHistoryStorage.h
    src/storage/ChatHistoryStorage.cpp
    src/ui/ChatView.h
    src/ui/ChatView.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/MessageWidget.h
    src/ui/MessageWidget.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
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

target_include_directories(AIChatDesktop PRIVATE
    src
)

set_target_properties(AIChatDesktop PROPERTIES
    WIN32_EXECUTABLE TRUE
)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
