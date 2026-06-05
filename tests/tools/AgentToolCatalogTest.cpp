#include "tools/AgentToolCatalog.h"

#include <QSet>

#include <cassert>

int main()
{
    const QVector<AgentToolDescriptor> catalog = defaultAgentToolCatalog();
    assert(catalog.size() >= 19);

    QSet<QString> ids;
    for (const AgentToolDescriptor &descriptor : catalog) {
        assert(!descriptor.id.isEmpty());
        assert(!ids.contains(descriptor.id));
        ids.insert(descriptor.id);
        assert(!descriptor.englishName.isEmpty());
        assert(!descriptor.chineseName.isEmpty());
        assert(!descriptor.inputPolicy.isEmpty());
        assert(descriptor.requiresUserConfirmation);
        assert(descriptor.enabledForAgent);
    }

    const AgentToolDescriptor *jsonTool = findAgentToolDescriptor(catalog, QStringLiteral("json.format"));
    assert(jsonTool != nullptr);
    assert(jsonTool->risk == AgentToolRisk::Low);
    assert(!jsonTool->resultMayContainSensitiveContent);
    assert(agentToolDisplayName(*jsonTool, AppLanguage::Chinese) == QStringLiteral("JSON 格式化"));

    const AgentToolDescriptor *fileTool = findAgentToolDescriptor(catalog, QStringLiteral("file.read_text"));
    assert(fileTool != nullptr);
    assert(fileTool->risk == AgentToolRisk::Low);
    assert(!fileTool->resultMayContainSensitiveContent);

    const AgentToolDescriptor *workspaceWriteTool = findAgentToolDescriptor(catalog, QStringLiteral("workspace.write_text"));
    assert(workspaceWriteTool != nullptr);
    assert(workspaceWriteTool->risk == AgentToolRisk::Medium);
    assert(!workspaceWriteTool->resultMayContainSensitiveContent);

    const AgentToolDescriptor *workspaceReadTool = findAgentToolDescriptor(catalog, QStringLiteral("workspace.read_text"));
    assert(workspaceReadTool != nullptr);
    assert(workspaceReadTool->risk == AgentToolRisk::Medium);
    assert(workspaceReadTool->resultMayContainSensitiveContent);

    const AgentToolDescriptor *workspaceDeleteTool = findAgentToolDescriptor(catalog, QStringLiteral("workspace.delete_file"));
    assert(workspaceDeleteTool != nullptr);
    assert(workspaceDeleteTool->risk == AgentToolRisk::High);

    const AgentToolDescriptor *gitStatusTool = findAgentToolDescriptor(catalog, QStringLiteral("command.git_status"));
    assert(gitStatusTool != nullptr);
    assert(gitStatusTool->risk == AgentToolRisk::Low);
    assert(gitStatusTool->resultMayContainSensitiveContent);

    const AgentToolDescriptor *cmakeBuildTool = findAgentToolDescriptor(catalog, QStringLiteral("command.cmake_build"));
    assert(cmakeBuildTool != nullptr);
    assert(cmakeBuildTool->risk == AgentToolRisk::Medium);

    const AgentToolDescriptor *memoryTool = findAgentToolDescriptor(catalog, QStringLiteral("memory.append_project_note"));
    assert(memoryTool != nullptr);
    assert(memoryTool->risk == AgentToolRisk::Medium);
    assert(!memoryTool->resultMayContainSensitiveContent);

    assert(findAgentToolDescriptor(catalog, QStringLiteral("missing.tool")) == nullptr);

    AgentToolRisk risk = AgentToolRisk::High;
    assert(agentToolRiskFromString(QStringLiteral("low"), &risk));
    assert(risk == AgentToolRisk::Low);
    assert(agentToolRiskFromString(QStringLiteral(" medium "), &risk));
    assert(risk == AgentToolRisk::Medium);
    assert(agentToolRiskFromString(QStringLiteral("HIGH"), &risk));
    assert(risk == AgentToolRisk::High);
    assert(!agentToolRiskFromString(QStringLiteral("critical"), &risk));

    assert(agentToolRiskToString(AgentToolRisk::Low) == QStringLiteral("low"));
    assert(agentToolRiskToString(AgentToolRisk::Medium) == QStringLiteral("medium"));
    assert(agentToolRiskToString(AgentToolRisk::High) == QStringLiteral("high"));
    assert(maxAgentToolRisk(AgentToolRisk::Low, AgentToolRisk::Medium) == AgentToolRisk::Medium);
    assert(maxAgentToolRisk(AgentToolRisk::High, AgentToolRisk::Medium) == AgentToolRisk::High);

    return 0;
}
