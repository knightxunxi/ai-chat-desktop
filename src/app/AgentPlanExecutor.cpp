#include "app/AgentPlanExecutor.h"

#include "support/AppLogger.h"
#include "tools/JsonCompactTool.h"
#include "tools/JsonFormatTool.h"
#include "tools/MarkdownCleanupTool.h"
#include "tools/TextCleanupTool.h"

#include <QJsonValue>

namespace {

QString stepInputText(const AgentPlanStep &step)
{
    const QStringList candidateKeys = {
        QStringLiteral("input"),
        QStringLiteral("text"),
        QStringLiteral("content")
    };

    for (const QString &key : candidateKeys) {
        const QJsonValue value = step.parameters.value(key);
        if (value.isString() && !value.toString().trimmed().isEmpty()) {
            return value.toString();
        }
    }

    return QString();
}

ToolResult runTextTool(const LocalTool &tool, const AgentPlanStep &step)
{
    const QString input = stepInputText(step);
    if (input.trimmed().isEmpty()) {
        return ToolResult::failure(QStringLiteral("This step requires a text parameter named input, text, or content."));
    }

    const ToolResult result = tool.run(input);
    AppLogger::info(QStringLiteral("AgentPlan"),
                    QStringLiteral("Step tool executed. stepId=%1 toolId=%2 ok=%3 outputLength=%4")
                        .arg(step.id, step.toolId, result.ok ? QStringLiteral("true") : QStringLiteral("false"))
                        .arg(result.output.size()));
    return result;
}

} // namespace

namespace AgentPlanExecutor {

bool canExecuteDirectly(const AgentPlanStep &step)
{
    return step.toolId == QStringLiteral("json.format")
        || step.toolId == QStringLiteral("json.compact")
        || step.toolId == QStringLiteral("markdown.cleanup")
        || step.toolId == QStringLiteral("text.cleanup");
}

ToolResult executeStep(const AgentPlanStep &step)
{
    if (step.toolId == QStringLiteral("json.format")) {
        const JsonFormatTool tool;
        return runTextTool(tool, step);
    }

    if (step.toolId == QStringLiteral("json.compact")) {
        const JsonCompactTool tool;
        return runTextTool(tool, step);
    }

    if (step.toolId == QStringLiteral("markdown.cleanup")) {
        const MarkdownCleanupTool tool;
        return runTextTool(tool, step);
    }

    if (step.toolId == QStringLiteral("text.cleanup")) {
        const TextCleanupTool tool;
        return runTextTool(tool, step);
    }

    AppLogger::warning(QStringLiteral("AgentPlan"),
                       QStringLiteral("Step tool cannot execute directly. stepId=%1 toolId=%2")
                           .arg(step.id, step.toolId));
    return ToolResult::failure(
        QStringLiteral("This tool requires a dedicated confirmation flow, such as a file picker, and cannot be executed directly from the plan preview."));
}

} // namespace AgentPlanExecutor
