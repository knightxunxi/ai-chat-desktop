# codexx_services — AI 客户端与服务层
add_library(codexx_services STATIC
    AIClient.h
    OpenAICompatibleClient.h
    OpenAICompatibleClient.cpp
    PythonSidecarClient.h
    PythonSidecarClient.cpp
    PythonSidecarProtocol.h
    PythonSidecarProtocol.cpp
    PythonSidecarAIClient.h
    PythonSidecarAIClient.cpp
    RequestErrorCategory.h
    StreamParser.h
    StreamParser.cpp
    ToolCall.h
    UpdateChecker.h
    UpdateChecker.cpp
)

target_link_libraries(codexx_services PUBLIC
    codexx_core
    codexx_support
    Qt6::Core
    Qt6::Network
)

target_include_directories(codexx_services PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
