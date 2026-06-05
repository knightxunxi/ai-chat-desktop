add_library(codexx_tools_dev STATIC
    GitReviewService.h
    GitReviewService.cpp
    LogSummaryService.h
    LogSummaryService.cpp
    CsvDataService.h
    CsvDataService.cpp
    ProjectFindService.h
    ProjectFindService.cpp
)
target_link_libraries(codexx_tools_dev PUBLIC
    codexx_tools_registry
    codexx_tools_core
    codexx_core
    Qt6::Core
)
target_include_directories(codexx_tools_dev PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
