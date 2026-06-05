# codexx_ui — 用户界面层
add_library(codexx_ui STATIC
    AgentStepWidget.h
    AgentStepWidget.cpp
    AgentPlanDialog.h
    AgentPlanDialog.cpp
    ChatView.h
    ChatView.cpp
    CodeHighlighter.h
    CodeHighlighter.cpp
    ConfirmToolDialog.h
    ConfirmToolDialog.cpp
    FileToolsDialog.h
    FileToolsDialog.cpp
    LogViewerDialog.h
    LogViewerDialog.cpp
    MainWindow.h
    MainWindow.cpp
    MessageWidget.h
    MessageWidget.cpp
    RolePromptDialog.h
    RolePromptDialog.cpp
    ScheduledTaskDialog.h
    ScheduledTaskDialog.cpp
    SettingsDialog.h
    SettingsDialog.cpp
    TokenBar.h
    TokenBar.cpp
    ToolsDialog.h
    ToolsDialog.cpp
    TypingIndicator.h
    TypingIndicator.cpp
)

target_link_libraries(codexx_ui PUBLIC
    codexx_core
    codexx_app
    codexx_tools
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(codexx_ui PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
