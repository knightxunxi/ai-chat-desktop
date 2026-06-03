#include "tools/ProjectMemoryService.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>

namespace {

QString readFile(const QString &path)
{
    QFile file(path);
    assert(file.open(QFile::ReadOnly | QFile::Text));
    return QString::fromUtf8(file.readAll());
}

} // namespace

int main()
{
    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());

    ProjectMemory missing = ProjectMemoryService::loadFromProjectDirectory(temporaryDirectory.path());
    assert(!missing.loaded);
    assert(missing.error.isEmpty());
    assert(missing.filePath.endsWith(QStringLiteral("AGENT_MEMORY.md")));

    ToolResult result = ProjectMemoryService::appendProjectNote(
        temporaryDirectory.path(),
        QStringLiteral("Prefer running ctest before commit."),
        QStringLiteral("user request"));
    assert(result.ok);
    assert(result.output.contains(QStringLiteral("AGENT_MEMORY.md")));
    assert(!result.output.contains(QStringLiteral("ctest before commit")));

    ProjectMemory loaded = ProjectMemoryService::loadFromProjectDirectory(temporaryDirectory.path());
    assert(loaded.loaded);
    assert(loaded.content.contains(QStringLiteral("Prefer running ctest before commit.")));
    assert(loaded.content.contains(QStringLiteral("Source: user_request")));

    const QString prompt = ProjectMemoryService::promptSection(loaded, AppLanguage::Chinese);
    assert(prompt.contains(QStringLiteral("Project working memory from AGENT_MEMORY.md")));
    assert(prompt.contains(QStringLiteral("untrusted project context")));
    assert(prompt.contains(QStringLiteral("Prefer running ctest before commit.")));

    result = ProjectMemoryService::appendProjectNote(
        temporaryDirectory.path(),
        QStringLiteral("password is 123456"),
        QStringLiteral("user"));
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("sensitive"), Qt::CaseInsensitive));

    result = ProjectMemoryService::appendProjectNote(
        temporaryDirectory.path(),
        QString(),
        QStringLiteral("user"));
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("empty"), Qt::CaseInsensitive));

    result = ProjectMemoryService::appendProjectNote(
        QString(),
        QStringLiteral("Remember this."),
        QStringLiteral("user"));
    assert(!result.ok);
    assert(result.error.contains(QStringLiteral("directory"), Qt::CaseInsensitive));

    const QString memoryPath = QDir(temporaryDirectory.path()).filePath(QStringLiteral("AGENT_MEMORY.md"));
    assert(readFile(memoryPath).contains(QStringLiteral("# Agent Memory")));

    return 0;
}
