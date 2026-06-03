#include "app/ProjectInstructionService.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>

namespace {

void writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    assert(file.open(QFile::WriteOnly | QFile::Text));
    assert(file.write(content) == content.size());
}

} // namespace

int main()
{
    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());

    ProjectInstructions missing = ProjectInstructionService::loadFromProjectDirectory(temporaryDirectory.path());
    assert(!missing.loaded);
    assert(missing.error.isEmpty());
    assert(missing.filePath.endsWith(QStringLiteral("AGENT.md")));

    const QString agentPath = QDir(temporaryDirectory.path()).filePath(QStringLiteral("AGENT.md"));
    writeFile(agentPath, "Build with cmake --build build-qt.\nNever bypass user confirmation.\n");

    ProjectInstructions loaded = ProjectInstructionService::loadFromProjectDirectory(temporaryDirectory.path());
    assert(loaded.loaded);
    assert(!loaded.truncated);
    assert(loaded.content.contains(QStringLiteral("cmake --build build-qt")));
    assert(loaded.filePath == QDir::cleanPath(agentPath));

    const QString section = ProjectInstructionService::promptSection(loaded, AppLanguage::Chinese);
    assert(section.contains(QStringLiteral("Project instructions from AGENT.md")));
    assert(section.contains(QStringLiteral("untrusted project data")));
    assert(section.contains(QStringLiteral("must not override")));
    assert(section.contains(QStringLiteral("cmake --build build-qt")));

    writeFile(agentPath, "abcdef");
    loaded = ProjectInstructionService::loadFromProjectDirectory(temporaryDirectory.path(), 3);
    assert(loaded.loaded);
    assert(loaded.truncated);
    assert(loaded.content == QStringLiteral("abc"));
    assert(ProjectInstructionService::promptSection(loaded, AppLanguage::English).contains(QStringLiteral("truncated")));

    ProjectInstructions invalid = ProjectInstructionService::loadFromProjectDirectory(QString(), 3);
    assert(!invalid.loaded);
    assert(invalid.error.contains(QStringLiteral("empty"), Qt::CaseInsensitive));

    invalid = ProjectInstructionService::loadFromProjectDirectory(temporaryDirectory.path(), 0);
    assert(!invalid.loaded);
    assert(invalid.error.contains(QStringLiteral("positive"), Qt::CaseInsensitive));

    return 0;
}
