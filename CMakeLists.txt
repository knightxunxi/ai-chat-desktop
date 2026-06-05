cmake_minimum_required(VERSION 3.22)

project(AIChatDesktop
    VERSION 1.0
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
    Gui
)

qt_standard_project_setup()

# ── 子库定义（按依赖顺序排列）──
add_subdirectory(src/core)      # codexx_core (STATIC)
add_subdirectory(src/support)   # codexx_support
add_subdirectory(src/storage)   # codexx_storage
add_subdirectory(src/services)  # codexx_services
add_subdirectory(src/memory)    # codexx_memory
add_subdirectory(src/skills)    # codexx_skills
add_subdirectory(src/hooks)     # codexx_hooks
add_subdirectory(src/scheduler) # codexx_scheduler
add_subdirectory(src/mcp)       # codexx_mcp
add_subdirectory(src/tools)     # codexx_tools
add_subdirectory(src/app)       # codexx_app
add_subdirectory(src/ui)        # codexx_ui

# ── 主可执行文件 ──
qt_add_executable(AIChatDesktop
    src/main.cpp
)

target_link_libraries(AIChatDesktop PRIVATE
    codexx_ui
    codexx_app
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)

target_include_directories(AIChatDesktop PRIVATE
    src
)

qt_add_resources(AIChatDesktop "app_resources"
    PREFIX "/"
    BASE "${PROJECT_SOURCE_DIR}/resources"
    FILES
        "${PROJECT_SOURCE_DIR}/resources/styles/app.qss"
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

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
