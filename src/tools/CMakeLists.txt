# codexx_tools — umbrella library: sub-libraries + forwarding headers + composition root
add_subdirectory(registry)
add_subdirectory(core)
add_subdirectory(perception)
add_subdirectory(input)
add_subdirectory(dev)
add_subdirectory(text)
add_subdirectory(assistant)

# Umbrella library — contains only AgentToolRegistry.cpp (composition root)
# Forwarding headers are header-only and don't need to be listed as sources
add_library(codexx_tools STATIC
    AgentToolRegistry.cpp
)

target_link_libraries(codexx_tools PUBLIC
    codexx_tools_registry
    codexx_tools_core
    codexx_tools_perception
    codexx_tools_input
    codexx_tools_dev
    codexx_tools_text
    codexx_tools_assistant
    codexx_core
    codexx_memory
    codexx_mcp
    codexx_hooks
    codexx_support
    codexx_services
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
)

target_include_directories(codexx_tools PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
