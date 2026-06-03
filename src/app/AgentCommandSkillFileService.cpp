#include "app/AgentCommandSkillFileService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

#include <algorithm>

namespace {

QString normalizedProjectPath(const QString &projectDirectory)
{
    const QString trimmed = projectDirectory.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    return QDir::cleanPath(QFileInfo(trimmed).absoluteFilePath());
}

QString extractJsonCandidate(QString text)
{
    text = text.trimmed();
    const int fenceStart = text.indexOf(QStringLiteral("```json"));
    if (fenceStart < 0) {
        return text;
    }

    const int contentStart = text.indexOf(QLatin1Char('\n'), fenceStart);
    if (contentStart < 0) {
        return text;
    }

    const int fenceEnd = text.indexOf(QStringLiteral("```"), contentStart);
    if (fenceEnd < 0) {
        return text.mid(contentStart + 1).trimmed();
    }

    return text.mid(contentStart + 1, fenceEnd - contentStart - 1).trimmed();
}

bool requiredString(const QJsonObject &object, const QString &fieldName, QString *value, QString *error)
{
    const QJsonValue jsonValue = object.value(fieldName);
    if (!jsonValue.isString() || jsonValue.toString().trimmed().isEmpty()) {
        *error = QStringLiteral("%1 must be a non-empty string.").arg(fieldName);
        return false;
    }

    *value = jsonValue.toString().trimmed();
    return true;
}

bool parseSkillStep(
    const QJsonObject &stepObject,
    const AgentToolRegistry &registry,
    AgentCommandSkillStep *step,
    QString *error)
{
    if (!requiredString(stepObject, QStringLiteral("englishTitle"), &step->englishTitle, error) ||
        !requiredString(stepObject, QStringLiteral("chineseTitle"), &step->chineseTitle, error) ||
        !requiredString(stepObject, QStringLiteral("toolId"), &step->toolId, error) ||
        !requiredString(stepObject, QStringLiteral("englishReason"), &step->englishReason, error) ||
        !requiredString(stepObject, QStringLiteral("chineseReason"), &step->chineseReason, error)) {
        return false;
    }

    QString riskText;
    if (!requiredString(stepObject, QStringLiteral("risk"), &riskText, error)) {
        return false;
    }

    if (!agentToolRiskFromString(riskText, &step->risk)) {
        *error = QStringLiteral("risk must be one of low, medium, or high.");
        return false;
    }

    const AgentToolDefinition *definition = registry.findById(step->toolId);
    if (definition == nullptr || !definition->descriptor.enabledForAgent || !definition->executableFromPlanPreview) {
        *error = QStringLiteral("toolId is not available for direct Agent execution: %1").arg(step->toolId);
        return false;
    }

    step->risk = maxAgentToolRisk(step->risk, definition->descriptor.risk);
    return true;
}

bool parseSkill(
    const QByteArray &content,
    const AgentToolRegistry &registry,
    AgentCommandSkill *skill,
    QString *error)
{
    const QString jsonText = extractJsonCandidate(QString::fromUtf8(content));
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        *error = QStringLiteral("JSON parse error at offset %1: %2")
                     .arg(parseError.offset)
                     .arg(parseError.errorString());
        return false;
    }

    if (!document.isObject()) {
        *error = QStringLiteral("Skill root must be a JSON object.");
        return false;
    }

    const QJsonObject root = document.object();
    if (!requiredString(root, QStringLiteral("id"), &skill->id, error) ||
        !requiredString(root, QStringLiteral("englishName"), &skill->englishName, error) ||
        !requiredString(root, QStringLiteral("chineseName"), &skill->chineseName, error) ||
        !requiredString(root, QStringLiteral("englishDescription"), &skill->englishDescription, error) ||
        !requiredString(root, QStringLiteral("chineseDescription"), &skill->chineseDescription, error)) {
        return false;
    }

    const QJsonValue stepsValue = root.value(QStringLiteral("steps"));
    if (!stepsValue.isArray() || stepsValue.toArray().isEmpty()) {
        *error = QStringLiteral("steps must be a non-empty array.");
        return false;
    }

    const QJsonArray steps = stepsValue.toArray();
    for (int index = 0; index < steps.size(); ++index) {
        if (!steps.at(index).isObject()) {
            *error = QStringLiteral("Step %1 must be an object.").arg(index + 1);
            return false;
        }

        AgentCommandSkillStep step;
        if (!parseSkillStep(steps.at(index).toObject(), registry, &step, error)) {
            *error = QStringLiteral("Step %1: %2").arg(index + 1).arg(*error);
            return false;
        }

        skill->steps.append(step);
    }

    return true;
}

} // namespace

namespace AgentCommandSkillFileService {

AgentCommandSkillLoadResult loadFromProjectDirectory(
    const QString &projectDirectory,
    const AgentToolRegistry &registry,
    int maxFiles,
    qint64 maxFileBytes)
{
    AgentCommandSkillLoadResult result;

    const QString normalizedDirectory = normalizedProjectPath(projectDirectory);
    if (normalizedDirectory.isEmpty()) {
        result.errors.append(QStringLiteral("Project directory is empty."));
        return result;
    }

    if (maxFiles <= 0) {
        result.errors.append(QStringLiteral("Maximum skill file count must be positive."));
        return result;
    }

    if (maxFileBytes <= 0) {
        result.errors.append(QStringLiteral("Maximum skill file size must be positive."));
        return result;
    }

    const QDir skillsDirectory(QDir(normalizedDirectory).filePath(QStringLiteral("skills")));
    if (!skillsDirectory.exists()) {
        return result;
    }

    const QFileInfoList files = skillsDirectory.entryInfoList(
        {QStringLiteral("*.skill.md")},
        QDir::Files | QDir::Readable,
        QDir::Name);

    QSet<QString> ids;
    const int availableFileCount = static_cast<int>(files.size());
    const int fileCount = std::min(availableFileCount, maxFiles);
    if (availableFileCount > maxFiles) {
        result.errors.append(QStringLiteral("Only the first %1 skill files were loaded.").arg(maxFiles));
    }

    for (int index = 0; index < fileCount; ++index) {
        const QFileInfo &fileInfo = files.at(index);
        if (fileInfo.size() > maxFileBytes) {
            result.errors.append(QStringLiteral("%1: skill file exceeds size limit.").arg(fileInfo.fileName()));
            continue;
        }

        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            result.errors.append(QStringLiteral("%1: %2").arg(fileInfo.fileName(), file.errorString()));
            continue;
        }

        AgentCommandSkill skill;
        QString error;
        if (!parseSkill(file.readAll(), registry, &skill, &error)) {
            result.errors.append(QStringLiteral("%1: %2").arg(fileInfo.fileName(), error));
            continue;
        }

        if (ids.contains(skill.id)) {
            result.errors.append(QStringLiteral("%1: duplicate skill id %2.").arg(fileInfo.fileName(), skill.id));
            continue;
        }

        ids.insert(skill.id);
        result.skills.append(skill);
    }

    return result;
}

} // namespace AgentCommandSkillFileService
