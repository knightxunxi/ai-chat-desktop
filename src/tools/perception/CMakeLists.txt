add_library(codexx_tools_perception STATIC
    ScreenCaptureService.h
    ScreenCaptureService.cpp
    OcrService.h
    OcrService.cpp
    WindowDetector.h
    WindowDetector.cpp
)
target_link_libraries(codexx_tools_perception PUBLIC
    codexx_tools_registry
    codexx_tools_core
    codexx_core
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)
target_include_directories(codexx_tools_perception PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
