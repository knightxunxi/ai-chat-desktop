add_library(codexx_tools_text STATIC
    JsonCompactTool.h
    JsonFormatTool.h
    JsonTools.cpp
    LocalTool.h
    MarkdownCleanupTool.h
    TextCleanupTool.h
    TextTools.cpp
    ProjectMemoryService.h
    ProjectMemoryService.cpp
)
target_link_libraries(codexx_tools_text PUBLIC
    codexx_tools_registry
    codexx_memory
    codexx_core
    Qt6::Core
)
target_include_directories(codexx_tools_text PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
