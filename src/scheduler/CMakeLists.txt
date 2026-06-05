# codexx_scheduler — 定时任务调度
add_library(codexx_scheduler STATIC
    ScheduledTask.h
    TaskScheduler.h
    TaskScheduler.cpp
    TaskStorage.h
    TaskStorage.cpp
)

target_link_libraries(codexx_scheduler PUBLIC
    codexx_core
    Qt6::Core
)

target_include_directories(codexx_scheduler PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
