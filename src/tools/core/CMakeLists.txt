add_library(codexx_tools_core STATIC
    CommandPolicy.h
    CommandPolicy.cpp
    CommandRunner.h
    CommandRunner.cpp
    WorkspacePolicy.h
    WorkspacePolicy.cpp
    WorkspaceFileService.h
    WorkspaceFileService.cpp
    FileInteractionService.h
    FileInteractionService.cpp
)
target_link_libraries(codexx_tools_core PUBLIC
    codexx_tools_registry
    codexx_core
    Qt6::Core
    Qt6::Widgets
)
target_include_directories(codexx_tools_core PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
