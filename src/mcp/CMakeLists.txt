# codexx_mcp — MCP 协议连接器
# 注意: codexx_mcp 仅通过 include path 使用 tools/ToolResult.h (纯头文件)，
#       不在链接层面依赖 codexx_tools，以避免循环依赖。
add_library(codexx_mcp STATIC
    McpConnector.h
    McpConnector.cpp
    McpRegistry.h
    McpRegistry.cpp
)

target_link_libraries(codexx_mcp PUBLIC
    codexx_core
    Qt6::Core
    Qt6::Network
)

target_include_directories(codexx_mcp PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
