# codexx_support — 日志与基础工具
add_library(codexx_support STATIC
    AppLogger.h
    AppLogger.cpp
    LogFileReader.h
    LogFileReader.cpp
)

target_link_libraries(codexx_support PUBLIC
    codexx_core
    Qt6::Core
)

target_include_directories(codexx_support PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
