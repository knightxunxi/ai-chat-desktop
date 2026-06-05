add_library(codexx_tools_input STATIC
    InputSimulator.h
    InputSimulator.cpp
    UiAutomationService.h
    UiAutomationService.cpp
    ForegroundValidator.h
    ForegroundValidator.cpp
)
target_link_libraries(codexx_tools_input PUBLIC
    codexx_tools_registry
    codexx_core
    Qt6::Core
    Qt6::Widgets
)
target_include_directories(codexx_tools_input PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
