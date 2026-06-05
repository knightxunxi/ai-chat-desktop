# codexx_memory — 三层记忆系统
add_library(codexx_memory STATIC
    MemoryEntry.h
    DailyMemoryWriter.h
    DailyMemoryWriter.cpp
    ProjectMemoryManager.h
    ProjectMemoryManager.cpp
)

target_link_libraries(codexx_memory PUBLIC
    codexx_core
    Qt6::Core
)

target_include_directories(codexx_memory PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
