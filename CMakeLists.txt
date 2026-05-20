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
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    resources/resources.qrc
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
