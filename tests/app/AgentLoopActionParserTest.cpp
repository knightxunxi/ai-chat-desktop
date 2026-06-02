#include "app/AgentLoopActionParser.h"
#include "tools/AgentToolCatalog.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <cassert>

namespace {

QString compactJson(const QJsonObject &object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QJsonObject stepObject(const QString &toolId, const QString &risk = QStringLiteral("low"))
{
    QJsonObject parameters;
    parameters.insert(QStringLiteral("path"), QStringLiteral("notes/hello.txt"));
    parameters.insert(QStringLiteral("content"), QStringLiteral("hello"));

    QJsonObject step;
    step.insert(QStringLiteral("id"), QStringLiteral("step-1"));
    step.insert(QStringLiteral("title"), QStringLiteral("Write file"));
    step.insert(QStringLiteral("toolId"), toolId);
    step.insert(QStringLiteral("reason"), QStringLiteral("The user asked to create a file."));
    step.insert(QStringLiteral("risk"), risk);
    step.insert(QStringLiteral("parameters"), parameters);
    return step;
}

} // namespace

int main()
{
    QJsonObject action;
    action.insert(QStringLiteral("done"), false);
    action.insert(QStringLiteral("message"), QStringLiteral("Next action."));
    action.insert(QStringLiteral("step"), stepObject(QStringLiteral("workspace.write_text"), QStringLiteral("low")));

    AgentLoopActionParseResult result = AgentLoopActionParser::parseJsonAction(compactJson(action), defaultAgentToolCatalog());
    assert(result.ok);
    assert(!result.action.done);
    assert(result.action.message == QStringLiteral("Next action."));
    assert(result.action.step.toolId == QStringLiteral("workspace.write_text"));
    assert(result.action.step.risk == AgentToolRisk::Medium);
    assert(result.action.step.parameters.value(QStringLiteral("path")).toString() == QStringLiteral("notes/hello.txt"));

    const QString fenced = QStringLiteral("```json\n%1\n```").arg(compactJson(action));
    result = AgentLoopActionParser::parseJsonAction(fenced, defaultAgentToolCatalog());
    assert(result.ok);

    QJsonObject doneAction;
    doneAction.insert(QStringLiteral("done"), true);
    doneAction.insert(QStringLiteral("message"), QStringLiteral("Finished."));
    result = AgentLoopActionParser::parseJsonAction(compactJson(doneAction), defaultAgentToolCatalog());
    assert(result.ok);
    assert(result.action.done);
    assert(result.action.message == QStringLiteral("Finished."));

    doneAction.insert(QStringLiteral("step"), stepObject(QStringLiteral("workspace.write_text")));
    result = AgentLoopActionParser::parseJsonAction(compactJson(doneAction), defaultAgentToolCatalog());
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("omitted")));

    action.insert(QStringLiteral("done"), false);
    action.remove(QStringLiteral("step"));
    result = AgentLoopActionParser::parseJsonAction(compactJson(action), defaultAgentToolCatalog());
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("step")));

    action.insert(QStringLiteral("step"), stepObject(QStringLiteral("missing.tool")));
    result = AgentLoopActionParser::parseJsonAction(compactJson(action), defaultAgentToolCatalog());
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("catalog")));

    action.insert(QStringLiteral("step"), stepObject(QStringLiteral("workspace.write_text"), QStringLiteral("critical")));
    result = AgentLoopActionParser::parseJsonAction(compactJson(action), defaultAgentToolCatalog());
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("risk")));

    result = AgentLoopActionParser::parseJsonAction(QStringLiteral("not-json"), defaultAgentToolCatalog());
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("parse error"), Qt::CaseInsensitive));

    return 0;
}
