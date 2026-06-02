#include "tools/WorkspacePolicy.h"

#include <QDir>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    assert(!WorkspacePolicy::defaultWorkspaceDirectory().trimmed().isEmpty());

    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());
    const QString workspace = QDir(temporaryDirectory.path()).filePath(QStringLiteral("workspace"));

    WorkspacePolicyDecision decision = WorkspacePolicy::evaluateWorkspaceOperation(
        workspace,
        QStringLiteral("generated/hello.cpp"),
        WorkspaceOperation::Write);
    assert(decision.allowed);
    assert(!decision.requiresConfirmation);
    assert(WorkspacePolicy::isPathInsideWorkspace(workspace, decision.normalizedPath));

    decision = WorkspacePolicy::evaluateWorkspaceOperation(
        workspace,
        QStringLiteral("../outside.txt"),
        WorkspaceOperation::Write);
    assert(!decision.allowed);

    const QString absoluteInsidePath = QDir(workspace).filePath(QStringLiteral("notes.md"));
    decision = WorkspacePolicy::evaluateWorkspaceOperation(
        workspace,
        absoluteInsidePath,
        WorkspaceOperation::Overwrite);
    assert(decision.allowed);

    const QString absoluteOutsidePath = QDir(temporaryDirectory.path()).filePath(QStringLiteral("outside.md"));
    decision = WorkspacePolicy::evaluateWorkspaceOperation(
        workspace,
        absoluteOutsidePath,
        WorkspaceOperation::Write);
    assert(!decision.allowed);

    decision = WorkspacePolicy::evaluateWorkspaceOperation(
        workspace,
        QStringLiteral(".git/config"),
        WorkspaceOperation::Delete);
    assert(!decision.allowed);

    decision = WorkspacePolicy::evaluateWorkspaceOperation(
        workspace,
        QStringLiteral("secrets/production.pem"),
        WorkspaceOperation::Overwrite);
    assert(!decision.allowed);

    decision = WorkspacePolicy::evaluateWorkspaceOperation(
        workspace,
        QStringLiteral("drafts/old-note.txt"),
        WorkspaceOperation::Delete);
    assert(decision.allowed);

    decision = WorkspacePolicy::evaluateWorkspaceOperation(
        QDir::rootPath(),
        QStringLiteral("danger.txt"),
        WorkspaceOperation::Write);
    assert(!decision.allowed);

    return 0;
}
