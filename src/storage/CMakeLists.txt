# codexx_storage — 持久化存储层
add_library(codexx_storage STATIC
    ConfigStorage.h
    ConfigStorage.cpp
    CredentialStorage.h
    WindowsCredentialStorage.h
    WindowsCredentialStorage.cpp
    ChatHistoryStorage.h
    ChatHistoryStorage.cpp
    ChatSessionExporter.h
    ChatSessionExporter.cpp
    PromptTemplateStorage.h
    PromptTemplateStorage.cpp
)

target_link_libraries(codexx_storage PUBLIC
    codexx_core
    Qt6::Core
    Qt6::Sql
)

target_include_directories(codexx_storage PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
