#include "skills/SkillManager.h"
#include "skills/SkillFileParser.h"
#include "skills/SkillDefinition.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>
#include <cstdio>

namespace {

static int testCount = 0;
static int passCount = 0;

void test(const char *name, std::function<void()> body)
{
    ++testCount;
    try { body(); ++passCount; printf("  PASS: %s\n", name); }
    catch (const std::exception &e) { printf("  FAIL: %s — %s\n", name, e.what()); }
    catch (...) { printf("  FAIL: %s — unknown error\n", name); }
}

void writeSkillFile(const QString &dirPath, const QString &content)
{
    QDir dir(dirPath);
    QString filePath = dir.filePath(QStringLiteral("SKILL.md"));
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        throw std::runtime_error("Failed to create test file");
    file.write(content.toUtf8());
    file.close();
}

QString validSkillContent(const QString &name, const QString &description,
                          const QStringList &triggers, int priority = 0,
                          bool enabled = true, const QString &version = QStringLiteral("1.0"))
{
    QString yaml;
    yaml += QStringLiteral("---\n");
    yaml += QStringLiteral("name: %1\n").arg(name);
    yaml += QStringLiteral("description: %1\n").arg(description);
    yaml += QStringLiteral("version: \"%1\"\n").arg(version);
    if (!triggers.isEmpty()) {
        yaml += QStringLiteral("triggers:\n");
        for (const QString &t : triggers) yaml += QStringLiteral("  - %1\n").arg(t);
    }
    yaml += QStringLiteral("priority: %1\n").arg(priority);
    yaml += QStringLiteral("enabled: %1\n").arg(enabled ? QStringLiteral("true") : QStringLiteral("false"));
    yaml += QStringLiteral("---\n");
    yaml += QStringLiteral("# %1\n\n").arg(description);
    yaml += QStringLiteral("This is the instruction body for %1. It contains enough text to "
                           "exceed the minimum body length requirement of fifty characters.\n").arg(name);
    return yaml;
}

} // namespace

int main()
{
    // TC-01: 正常解析 YAML frontmatter
    test("parse valid skill", [] {
        const QString content = validSkillContent(
            QStringLiteral("code-review"), QStringLiteral("Code review helper"),
            {QStringLiteral("review"), QStringLiteral("code review")}, 10);

        auto result = SkillFileParser::parseContent(content, QStringLiteral("test/SKILL.md"), QStringLiteral("project"));
        assert(result.has_value());
        assert(result->metadata.name == QStringLiteral("code-review"));
        assert(result->metadata.description == QStringLiteral("Code review helper"));
        assert(result->metadata.priority == 10);
        assert(result->metadata.enabled);
        assert(result->metadata.version == QStringLiteral("1.0"));
        assert(result->metadata.triggers.size() == 2);
        assert(result->sourceType == QStringLiteral("project"));
        assert(!result->instructions.isEmpty());
    });

    // TC-02: 缺失必填字段返回 nullopt
    test("parse missing required fields", [] {
        QString content = QStringLiteral(
            "---\n"
            "description: Missing name\n"
            "triggers:\n  - test\n"
            "---\n"
            "This is a body long enough to pass the minimum fifty character count requirement.\n");
        auto result = SkillFileParser::parseContent(content, QStringLiteral("test/SKILL.md"), QStringLiteral("project"));
        assert(!result.has_value());
    });

    // TC-03: 双目录扫描与合并
    test("dual directory load", [] {
        QTemporaryDir userDir, projectDir;
        assert(userDir.isValid() && projectDir.isValid());

        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("user-skill"), QStringLiteral("User skill"), {QStringLiteral("user")}));
        writeSkillFile(projectDir.path(), validSkillContent(
            QStringLiteral("project-skill"), QStringLiteral("Project skill"), {QStringLiteral("project")}));

        SkillManager manager;
        manager.initialize(userDir.path(), projectDir.path());
        assert(manager.totalCount() == 2);
    });

    // TC-04: 项目级覆盖用户级同名技能
    test("project overrides user", [] {
        QTemporaryDir userDir, projectDir;
        assert(userDir.isValid() && projectDir.isValid());

        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("shared-skill"), QStringLiteral("User version"), {QStringLiteral("test")}, 5));
        writeSkillFile(projectDir.path(), validSkillContent(
            QStringLiteral("shared-skill"), QStringLiteral("Project version"), {QStringLiteral("test")}, 20));

        SkillManager manager;
        manager.initialize(userDir.path(), projectDir.path());
        assert(manager.totalCount() == 1);
        const auto all = manager.allSkills();
        assert(all.size() == 1);
        assert(all[0].metadata.name == QStringLiteral("shared-skill"));
        assert(all[0].sourceType == QStringLiteral("project"));
        assert(all[0].metadata.priority == 20);
    });

    // TC-05: 子串匹配触发
    test("substring match", [] {
        QTemporaryDir userDir;
        assert(userDir.isValid());
        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("git-helper"), QStringLiteral("Git helper"),
            {QStringLiteral("git commit"), QStringLiteral("提交代码")}));

        SkillManager manager;
        manager.initialize(userDir.path(), QString());
        const auto matched = manager.matchSkills(QStringLiteral("Please help me git commit"));
        assert(matched.size() == 1);
        assert(matched[0].metadata.name == QStringLiteral("git-helper"));
    });

    // TC-06: 未匹配不返回
    test("no match", [] {
        QTemporaryDir userDir;
        assert(userDir.isValid());
        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("git-helper"), QStringLiteral("Git helper"), {QStringLiteral("git commit")}));
        SkillManager manager;
        manager.initialize(userDir.path(), QString());
        const auto matched = manager.matchSkills(QStringLiteral("Hello world"));
        assert(matched.size() == 0);
    });

    // TC-07: disabled 技能不被匹配
    test("disabled skill not matched", [] {
        QTemporaryDir userDir;
        assert(userDir.isValid());
        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("disabled-skill"), QStringLiteral("Disabled"), {QStringLiteral("test")}, 0, false));
        SkillManager manager;
        manager.initialize(userDir.path(), QString());
        const auto matched = manager.matchSkills(QStringLiteral("test trigger"));
        assert(matched.size() == 0);
        assert(manager.totalCount() == 1); // still in allSkills
    });

    // TC-08: 空 triggers 不匹配
    test("empty triggers not matched", [] {
        QTemporaryDir userDir;
        assert(userDir.isValid());
        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("empty-triggers"), QStringLiteral("No triggers"), {}));
        SkillManager manager;
        manager.initialize(userDir.path(), QString());
        const auto matched = manager.matchSkills(QStringLiteral("anything"));
        assert(matched.size() == 0);
    });

    // TC-09: 大小写不敏感
    test("case insensitive match", [] {
        QTemporaryDir userDir;
        assert(userDir.isValid());
        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("case-test"), QStringLiteral("Case test"),
            {QStringLiteral("Code Review"), QStringLiteral("UPPERCASE")}));
        SkillManager manager;
        manager.initialize(userDir.path(), QString());
        assert(manager.matchSkills(QStringLiteral("code review please")).size() == 1);
        assert(manager.matchSkills(QStringLiteral("using uppercase tool")).size() == 1);
    });

    // TC-10: priority 排序
    test("priority ordering", [] {
        QTemporaryDir userDir;
        assert(userDir.isValid());
        QDir(userDir.path()).mkpath(QStringLiteral("high"));
        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("low-skill"), QStringLiteral("Low"), {QStringLiteral("low")}, 1));
        writeSkillFile(userDir.path() + QStringLiteral("/high"), validSkillContent(
            QStringLiteral("high-skill"), QStringLiteral("High"), {QStringLiteral("high")}, 100));
        SkillManager manager;
        manager.initialize(userDir.path(), QString());
        assert(manager.totalCount() == 2);
        const auto all = manager.allSkills();
        assert(all.size() >= 2);
        assert(all[0].metadata.name == QStringLiteral("high-skill"));
        assert(all[1].metadata.name == QStringLiteral("low-skill"));
    });

    // TC-11: 空用户输入不匹配
    test("empty user input", [] {
        QTemporaryDir userDir;
        assert(userDir.isValid());
        writeSkillFile(userDir.path(), validSkillContent(
            QStringLiteral("test-skill"), QStringLiteral("Test"), {QStringLiteral("test")}));
        SkillManager manager;
        manager.initialize(userDir.path(), QString());
        assert(manager.matchSkills(QString()).size() == 0);
        assert(manager.matchSkills(QStringLiteral("   ")).size() == 0);
    });

    printf("\n%d/%d tests passed\n", passCount, testCount);
    return (passCount == testCount) ? 0 : 1;
}
