#include "app/AgentCommandSkillFileService.h"

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

QByteArray skillJson(const QString &id, const QString &toolId)
{
    return QStringLiteral(R"({
  "id": "%1",
  "englishName": "External Build",
  "chineseName": "外部构建",
  "englishDescription": "Run a project-specific build step.",
  "chineseDescription": "运行项目指定的构建步骤。",
  "steps": [
    {
      "englishTitle": "Build project",
      "chineseTitle": "构建项目",
      "toolId": "%2",
      "englishReason": "Verify the project still compiles.",
      "chineseReason": "验证项目仍能编译。",
      "risk": "low"
    }
  ]
})")
        .arg(id, toolId)
        .toUtf8();
}

} // namespace

int main()
{
    QTemporaryDir temporaryDirectory;
    assert(temporaryDirectory.isValid());

    const AgentToolRegistry registry = AgentToolRegistryFactory::defaultRegistry();

    AgentCommandSkillLoadResult result = AgentCommandSkillFileService::loadFromProjectDirectory(
        temporaryDirectory.path(),
        registry);
    assert(result.skills.isEmpty());
    assert(result.errors.isEmpty());

    const QString skillsPath = QDir(temporaryDirectory.path()).filePath(QStringLiteral("skills"));
    assert(QDir().mkpath(skillsPath));

    const QString validSkillPath = QDir(skillsPath).filePath(QStringLiteral("external-build.skill.md"));
    writeFile(validSkillPath, QByteArray("```json\n") + skillJson(QStringLiteral("external.build"), QStringLiteral("command.cmake_build")) + QByteArray("\n```\n"));

    result = AgentCommandSkillFileService::loadFromProjectDirectory(
        temporaryDirectory.path(),
        registry);
    assert(result.skills.size() == 1);
    assert(result.errors.isEmpty());
    assert(result.skills.first().id == QStringLiteral("external.build"));
    assert(result.skills.first().steps.size() == 1);
    assert(result.skills.first().steps.first().toolId == QStringLiteral("command.cmake_build"));
    assert(result.skills.first().steps.first().risk == AgentToolRisk::Medium);

    const QString invalidSkillPath = QDir(skillsPath).filePath(QStringLiteral("invalid.skill.md"));
    writeFile(invalidSkillPath, skillJson(QStringLiteral("external.invalid"), QStringLiteral("missing.tool")));
    result = AgentCommandSkillFileService::loadFromProjectDirectory(
        temporaryDirectory.path(),
        registry);
    assert(result.skills.size() == 1);
    assert(!result.errors.isEmpty());
    assert(result.errors.join(QLatin1Char('\n')).contains(QStringLiteral("missing.tool")));

    const QString duplicateSkillPath = QDir(skillsPath).filePath(QStringLiteral("duplicate.skill.md"));
    writeFile(duplicateSkillPath, skillJson(QStringLiteral("external.build"), QStringLiteral("command.ctest")));
    result = AgentCommandSkillFileService::loadFromProjectDirectory(
        temporaryDirectory.path(),
        registry);
    assert(result.skills.size() == 1);
    assert(result.errors.join(QLatin1Char('\n')).contains(QStringLiteral("duplicate")));

    result = AgentCommandSkillFileService::loadFromProjectDirectory(QString(), registry);
    assert(result.skills.isEmpty());
    assert(result.errors.join(QLatin1Char('\n')).contains(QStringLiteral("empty"), Qt::CaseInsensitive));

    return 0;
}
