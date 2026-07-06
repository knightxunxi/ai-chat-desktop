# codexx_platform — 跨平台服务抽象层 (N5)
add_library(codexx_platform STATIC
    PlatformServices.h
    PlatformServices.cpp
)

target_link_libraries(codexx_platform PUBLIC
    codexx_core
    codexx_tools
    Qt6::Core
)

target_include_directories(codexx_platform PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

# Windows-only: 链接 Win32 API
if(WIN32)
    target_link_libraries(codexx_platform PUBLIC
        advapi32
        user32
        gdi32
    )
endif()
