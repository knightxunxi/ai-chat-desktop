# codexx_hooks — 钩子系统
add_library(codexx_hooks STATIC
    HookDefinition.h
    HookManager.h
    HookManager.cpp
    BuiltinHooks.h
    BuiltinHooks.cpp
    ScriptHookRunner.h
    ScriptHookRunner.cpp
)

target_link_libraries(codexx_hooks PUBLIC
    codexx_core
    codexx_support
    Qt6::Core
)

target_include_directories(codexx_hooks PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
