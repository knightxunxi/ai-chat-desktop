add_library(codexx_tools_registry STATIC
    AgentToolCatalog.h
    AgentToolCatalog.cpp
    AgentToolRegistry.h
    ToolResult.h
)
target_link_libraries(codexx_tools_registry PUBLIC
    codexx_core
    Qt6::Core
    Qt6::Widgets
)
target_include_directories(codexx_tools_registry PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
