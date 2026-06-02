#include "app/AgentPlanPromptBuilder.h"

#include <QStringList>

namespace {

QString boolLabel(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString languageName(AppLanguage language)
{
    return language == AppLanguage::Chinese ? QStringLiteral("Chinese") : QStringLiteral("English");
}

} // namespace

namespace AgentPlanPromptBuilder {

QString buildPlanningPrompt(
    const QString &userGoal,
    const QVector<AgentToolDescriptor> &toolCatalog,
    AppLanguage language,
    int maxSteps)
{
    QStringList toolLines;
    for (const AgentToolDescriptor &tool : toolCatalog) {
        if (!tool.enabledForAgent) {
            continue;
        }

        const QString description = language == AppLanguage::Chinese ? tool.chineseDescription : tool.englishDescription;
        toolLines.append(QStringLiteral("- id=%1 | risk=%2 | sensitiveResult=%3 | name=%4 | description=%5 | inputPolicy=%6")
                             .arg(tool.id,
                                  agentToolRiskToString(tool.risk),
                                  boolLabel(tool.resultMayContainSensitiveContent),
                                  agentToolDisplayName(tool, language),
                                  description,
                                  tool.inputPolicy));
    }

    return QStringLiteral(
               "You are planning controlled tool-assisted work inside AI Chat Desktop.\n"
               "Return only valid JSON. Do not include markdown fences or natural-language commentary outside JSON.\n"
               "The UI will not execute any step until the user confirms it.\n"
               "Do not suggest shell commands, scripts, keyboard/mouse automation, background tasks, deletion, moving files, or silent overwrite.\n"
               "Use only tool IDs from the catalog below.\n"
               "Use %1 for user-facing text.\n"
               "Maximum steps: %2.\n\n"
               "Allowed tool catalog:\n%3\n\n"
               "Required JSON schema:\n"
               "{\n"
               "  \"summary\": \"short plan summary\",\n"
               "  \"steps\": [\n"
               "    {\n"
               "      \"id\": \"step-1\",\n"
               "      \"title\": \"step title\",\n"
               "      \"toolId\": \"one catalog tool id\",\n"
               "      \"reason\": \"why this step is needed\",\n"
               "      \"risk\": \"low|medium|high\",\n"
               "      \"parameters\": {}\n"
               "    }\n"
               "  ]\n"
               "}\n\n"
               "User goal:\n%4")
        .arg(languageName(language),
             QString::number(maxSteps),
             toolLines.join(QLatin1Char('\n')),
             userGoal.trimmed());
}

} // namespace AgentPlanPromptBuilder
