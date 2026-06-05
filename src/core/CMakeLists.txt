# codexx_core — 核心数据结构定义
# 类型: STATIC (ProviderPreset 需要编译 .cpp)
add_library(codexx_core STATIC
    AppConfig.h
    AppLanguage.h
    AgentLoopState.h
    ChatMessage.h
    ChatSession.h
    MessageRole.h
    PromptTemplate.h
    ProviderPreset.h
    ProviderPreset.cpp
    SessionListFilter.h
)

target_link_libraries(codexx_core PUBLIC
    Qt6::Core
)

target_include_directories(codexx_core PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
