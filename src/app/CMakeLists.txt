# codexx_app — 应用核心逻辑层
add_library(codexx_app STATIC
    AgentPlan.h
    AgentPlan.cpp
    AgentCommandSkillCatalog.h
    AgentCommandSkillCatalog.cpp
    AgentCommandSkillFileService.h
    AgentCommandSkillFileService.cpp
    AgentToolCallPlanBuilder.h
    AgentToolCallPlanBuilder.cpp
    AgentLoopActionParser.h
    AgentLoopActionParser.cpp
    AgentLoopController.h
    AgentLoopController.cpp
    AgentLoopPromptBuilder.h
    AgentLoopPromptBuilder.cpp
    AgentPlanExecutor.h
    AgentPlanExecutor.cpp
    AgentPlanParser.h
    AgentPlanParser.cpp
    AgentPlanPromptBuilder.h
    AgentPlanPromptBuilder.cpp
    TokenEstimator.h
    TokenEstimator.cpp
    SummaryAPIClient.h
    SummaryAPIClient.cpp
    ContextWindowManager.h
    ContextWindowManager.cpp
    ApplicationController.h
    ApplicationController.cpp
    ConfigCoordinator.h
    ConfigCoordinator.cpp
    SessionCoordinator.h
    SessionCoordinator.cpp
    AgentOrchestrator.h
    AgentOrchestrator.cpp
    ProjectInstructionService.h
    ProjectInstructionService.cpp
    SessionSummaryList.h
    SessionSummaryList.cpp
    AgentJsonHelpers.h
)

target_link_libraries(codexx_app PUBLIC
    codexx_core
    codexx_support
    codexx_storage
    codexx_services
    codexx_memory
    codexx_skills
    codexx_hooks
    codexx_scheduler
    codexx_mcp
    codexx_platform
    codexx_plugins
    codexx_tools
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
)

target_include_directories(codexx_app PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
