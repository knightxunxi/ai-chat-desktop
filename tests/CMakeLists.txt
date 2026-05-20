add_executable(CoreModelSmokeTest
    core/ModelSmokeTest.cpp
)

target_link_libraries(CoreModelSmokeTest PRIVATE
    Qt6::Core
)

target_include_directories(CoreModelSmokeTest PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_test(NAME CoreModelSmokeTest COMMAND CoreModelSmokeTest)
