add_library(codexx_tools_assistant STATIC
    SystemInfoService.h
    SystemInfoService.cpp
    AssistantService.h
    AssistantService.cpp
)
target_link_libraries(codexx_tools_assistant PUBLIC
    codexx_tools_registry
    codexx_core
    Qt6::Core
)
target_include_directories(codexx_tools_assistant PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
