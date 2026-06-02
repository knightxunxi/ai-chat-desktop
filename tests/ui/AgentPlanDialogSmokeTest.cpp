#include "ui/AgentPlanDialog.h"

#include <QApplication>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>

#include <cassert>

namespace {

AgentPlan makePlan()
{
    AgentPlan plan;
    plan.summary = QStringLiteral("Format JSON safely.");

    AgentPlanStep step;
    step.id = QStringLiteral("step-1");
    step.title = QStringLiteral("Format JSON");
    step.toolId = QStringLiteral("json.format");
    step.reason = QStringLiteral("The JSON should be easier to inspect.");
    step.risk = AgentToolRisk::Low;
    step.parameters.insert(QStringLiteral("input"), QStringLiteral("{\"name\":\"test\"}"));
    plan.steps.append(step);

    AgentPlanStep fileStep;
    fileStep.id = QStringLiteral("step-2");
    fileStep.title = QStringLiteral("Read a file");
    fileStep.toolId = QStringLiteral("file.read_text");
    fileStep.reason = QStringLiteral("Needs explicit file selection.");
    fileStep.risk = AgentToolRisk::Medium;
    plan.steps.append(fileStep);

    return plan;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AgentPlanDialog dialog(makePlan(), defaultAgentToolCatalog(), AppLanguage::Chinese);

    auto *summaryLabel = dialog.findChild<QLabel *>(QStringLiteral("agentPlanSummaryLabel"));
    auto *statusLabel = dialog.findChild<QLabel *>(QStringLiteral("agentPlanStatusLabel"));
    auto *stepList = dialog.findChild<QListWidget *>(QStringLiteral("agentPlanStepList"));
    auto *detailEdit = dialog.findChild<QPlainTextEdit *>(QStringLiteral("agentPlanStepDetailEdit"));
    auto *outputEdit = dialog.findChild<QPlainTextEdit *>(QStringLiteral("agentPlanOutputEdit"));
    auto *executeButton = dialog.findChild<QPushButton *>(QStringLiteral("executeAgentPlanStepButton"));
    auto *skipButton = dialog.findChild<QPushButton *>(QStringLiteral("skipAgentPlanStepButton"));
    auto *copyButton = dialog.findChild<QPushButton *>(QStringLiteral("copyAgentPlanOutputButton"));
    auto *insertButton = dialog.findChild<QPushButton *>(QStringLiteral("insertAgentPlanOutputButton"));
    auto *continueButton = dialog.findChild<QPushButton *>(QStringLiteral("continueAgentPlanButton"));
    auto *closeButton = dialog.findChild<QPushButton *>(QStringLiteral("closeAgentPlanButton"));

    assert(summaryLabel != nullptr);
    assert(statusLabel != nullptr);
    assert(stepList != nullptr);
    assert(detailEdit != nullptr);
    assert(outputEdit != nullptr);
    assert(executeButton != nullptr);
    assert(skipButton != nullptr);
    assert(copyButton != nullptr);
    assert(insertButton != nullptr);
    assert(continueButton != nullptr);
    assert(closeButton != nullptr);

    assert(stepList->count() == 2);
    assert(stepList->currentRow() == 0);
    assert(executeButton->isEnabled());
    assert(skipButton->isEnabled());
    assert(!copyButton->isEnabled());
    assert(!insertButton->isEnabled());
    assert(!continueButton->isEnabled());
    assert(outputEdit->toPlainText().isEmpty());

    executeButton->click();
    assert(outputEdit->toPlainText().contains(QStringLiteral("\"name\": \"test\"")));
    assert(copyButton->isEnabled());
    assert(insertButton->isEnabled());
    assert(continueButton->isEnabled());

    QString insertedOutput;
    QObject::connect(&dialog, &AgentPlanDialog::outputInsertionRequested, [&insertedOutput](const QString &output) {
        insertedOutput = output;
    });
    insertButton->click();
    assert(insertedOutput == outputEdit->toPlainText());

    QString continuationGoal;
    int continuationDepth = -1;
    QObject::connect(&dialog, &AgentPlanDialog::continuePlanningRequested, [&continuationGoal, &continuationDepth](const QString &goal, int depth) {
        continuationGoal = goal;
        continuationDepth = depth;
    });
    continueButton->click();
    assert(continuationDepth == 1);
    assert(continuationGoal.contains(QStringLiteral("Format JSON")));
    assert(continuationGoal.contains(QStringLiteral("\"name\": \"test\"")));

    AgentPlanDialog secondDialog(makePlan(), defaultAgentToolCatalog(), AppLanguage::Chinese);
    auto *secondStepList = secondDialog.findChild<QListWidget *>(QStringLiteral("agentPlanStepList"));
    auto *secondExecuteButton = secondDialog.findChild<QPushButton *>(QStringLiteral("executeAgentPlanStepButton"));
    auto *secondSkipButton = secondDialog.findChild<QPushButton *>(QStringLiteral("skipAgentPlanStepButton"));
    auto *secondOutputEdit = secondDialog.findChild<QPlainTextEdit *>(QStringLiteral("agentPlanOutputEdit"));
    secondStepList->setCurrentRow(1);
    assert(secondOutputEdit->toPlainText().isEmpty());
    assert(!secondExecuteButton->isEnabled());
    assert(secondSkipButton->isEnabled());
    secondSkipButton->click();
    assert(secondStepList->item(1)->text().contains(QStringLiteral("skipped")));
    closeButton->click();
    assert(dialog.result() == QDialog::Accepted);

    return 0;
}
