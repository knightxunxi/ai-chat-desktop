#include "app/AgentPlanPromptBuilder.h"
#include "tools/AgentToolCatalog.h"

#include <cassert>

int main()
{
    QVector<AgentToolDescriptor> catalog = defaultAgentToolCatalog();
    AgentToolDescriptor disabled = catalog.first();
    disabled.id = QStringLiteral("disabled.tool");
    disabled.enabledForAgent = false;
    catalog.append(disabled);

    const QString prompt = AgentPlanPromptBuilder::buildPlanningPrompt(
        QStringLiteral("整理一段 JSON，并说明下一步。"),
        catalog,
        AppLanguage::Chinese,
        5);

    assert(prompt.contains(QStringLiteral("Return only valid JSON")));
    assert(prompt.contains(QStringLiteral("Maximum steps: 5")));
    assert(prompt.contains(QStringLiteral("\"summary\"")));
    assert(prompt.contains(QStringLiteral("\"steps\"")));
    assert(prompt.contains(QStringLiteral("json.format")));
    assert(prompt.contains(QStringLiteral("file.read_text")));
    assert(prompt.contains(QStringLiteral("workspace.write_text")));
    assert(prompt.contains(QStringLiteral("workspace.delete_file")));
    assert(prompt.contains(QStringLiteral("risk=medium")));
    assert(prompt.contains(QStringLiteral("risk=high")));
    assert(prompt.contains(QStringLiteral("Do not suggest shell commands")));
    assert(prompt.contains(QStringLiteral("configured Agent workspace")));
    assert(prompt.contains(QStringLiteral("untrusted data")));
    assert(prompt.contains(QStringLiteral("整理一段 JSON")));
    assert(!prompt.contains(QStringLiteral("disabled.tool")));

    const QString englishPrompt = AgentPlanPromptBuilder::buildPlanningPrompt(
        QStringLiteral("Clean text"),
        defaultAgentToolCatalog(),
        AppLanguage::English,
        3);
    assert(englishPrompt.contains(QStringLiteral("Use English for user-facing text.")));
    assert(englishPrompt.contains(QStringLiteral("Maximum steps: 3")));

    return 0;
}
