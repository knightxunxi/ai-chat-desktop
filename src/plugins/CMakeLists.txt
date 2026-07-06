# codexx_plugins — 插件系统（管理器 + 示例插件）
add_library(codexx_plugins STATIC
    PluginInterface.h
    PluginManager.h
    PluginManager.cpp
)

target_link_libraries(codexx_plugins PUBLIC
    codexx_core
    codexx_support
    codexx_tools
    Qt6::Core
)

target_include_directories(codexx_plugins PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

# ── 示例插件（独立的 SHARED library，供 QPluginLoader 加载） ─────
add_library(example_plugin SHARED
    examples/ExamplePlugin.h
    examples/ExamplePlugin.cpp
)

target_link_libraries(example_plugin PUBLIC
    codexx_plugins
    Qt6::Core
)

target_include_directories(example_plugin PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

# 输出到插件目录
set_target_properties(example_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/example_plugin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/example_plugin"
)

# 生成 plugin.json
file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/plugins/example_plugin/plugin.json"
    CONTENT "{
    \"id\": \"example_plugin\",
    \"name\": \"Example Plugin\",
    \"version\": \"1.0.0\",
    \"tools\": [
        {
            \"id\": \"plugin.hello_world\",
            \"name\": \"Hello World\",
            \"description\": \"A sample plugin tool that returns a greeting.\",
            \"risk\": 0
        }
    ]
}"
)
