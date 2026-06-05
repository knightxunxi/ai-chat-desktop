# codexx_skills — 技能系统
add_library(codexx_skills STATIC
    SkillDefinition.h
    SkillFileParser.h
    SkillFileParser.cpp
    SkillManager.h
    SkillManager.cpp
)

target_link_libraries(codexx_skills PUBLIC
    codexx_core
    codexx_support
    Qt6::Core
)

target_include_directories(codexx_skills PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
