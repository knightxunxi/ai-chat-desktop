#include "storage/PromptTemplateStorage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <utility>

namespace {

constexpr auto VersionKey = "version";
constexpr auto TemplatesKey = "templates";
constexpr auto IdKey = "id";
constexpr auto NameKey = "name";
constexpr auto ContentKey = "content";

QJsonObject toJson(const PromptTemplate &promptTemplate)
{
    QJsonObject object;
    object.insert(QString::fromLatin1(IdKey), promptTemplate.id);
    object.insert(QString::fromLatin1(NameKey), promptTemplate.name);
    object.insert(QString::fromLatin1(ContentKey), promptTemplate.content);
    return object;
}

PromptTemplate fromJson(const QJsonObject &object)
{
    PromptTemplate promptTemplate;
    promptTemplate.id = object.value(QString::fromLatin1(IdKey)).toString().trimmed();
    promptTemplate.name = object.value(QString::fromLatin1(NameKey)).toString().trimmed();
    promptTemplate.content = object.value(QString::fromLatin1(ContentKey)).toString().trimmed();
    return promptTemplate;
}

void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

bool hasTemplateId(const QVector<PromptTemplate> &templates, const QString &id)
{
    const QString normalizedId = id.trimmed();
    if (normalizedId.isEmpty()) {
        return false;
    }

    for (const PromptTemplate &promptTemplate : templates) {
        if (promptTemplate.id == normalizedId) {
            return true;
        }
    }

    return false;
}

bool hasTemplateName(const QVector<PromptTemplate> &templates, const QString &name)
{
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty()) {
        return false;
    }

    for (const PromptTemplate &promptTemplate : templates) {
        if (promptTemplate.name.compare(normalizedName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

QString uniqueTemplateName(const QVector<PromptTemplate> &templates, const QString &name)
{
    const QString baseName = name.trimmed().isEmpty() ? QStringLiteral("Imported Template") : name.trimmed();
    if (!hasTemplateName(templates, baseName)) {
        return baseName;
    }

    for (int index = 2;; ++index) {
        const QString candidate = QStringLiteral("%1 (%2)").arg(baseName, QString::number(index));
        if (!hasTemplateName(templates, candidate)) {
            return candidate;
        }
    }
}

} // namespace

PromptTemplateStorage::PromptTemplateStorage(QString filePath)
    : m_filePath(std::move(filePath))
{
}

QVector<PromptTemplate> PromptTemplateStorage::load(QString *error) const
{
    const QString path = resolvedFilePath();
    if (!QFileInfo::exists(path)) {
        return defaultTemplates();
    }

    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        setError(error, QStringLiteral("Failed to open prompt templates: %1").arg(path));
        return defaultTemplates();
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(error, QStringLiteral("Failed to parse prompt templates: %1").arg(parseError.errorString()));
        return defaultTemplates();
    }

    QVector<PromptTemplate> templates;
    const QJsonArray array = document.object().value(QString::fromLatin1(TemplatesKey)).toArray();
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }

        const PromptTemplate promptTemplate = fromJson(value.toObject());
        if (promptTemplate.isValid()) {
            templates.append(promptTemplate);
        }
    }

    return templates;
}

bool PromptTemplateStorage::save(const QVector<PromptTemplate> &templates, QString *error) const
{
    return exportTemplates(resolvedFilePath(), templates, error);
}

QString PromptTemplateStorage::filePath() const
{
    return resolvedFilePath();
}

QVector<PromptTemplate> PromptTemplateStorage::defaultTemplates()
{
    QVector<PromptTemplate> templates;
    templates.append(PromptTemplate{
        QStringLiteral("default-assistant"),
        QStringLiteral("Default Assistant"),
        QStringLiteral("You are a helpful, concise AI assistant. Answer clearly and ask clarifying questions when needed.")
    });
    templates.append(PromptTemplate{
        QStringLiteral("cpp-mentor"),
        QStringLiteral("C++ Mentor"),
        QStringLiteral("You are a patient C++ mentor. Explain concepts with practical examples, point out common mistakes, and keep answers beginner-friendly.")
    });
    templates.append(PromptTemplate{
        QStringLiteral("translator"),
        QStringLiteral("Translator"),
        QStringLiteral("You are a precise translation assistant. Translate faithfully, preserve meaning and tone, and explain ambiguous phrases when useful.")
    });

    return templates;
}

QVector<PromptTemplate> PromptTemplateStorage::importTemplates(const QString &filePath, QString *error)
{
    if (error != nullptr) {
        error->clear();
    }

    if (!QFileInfo::exists(filePath)) {
        setError(error, QStringLiteral("Prompt template file does not exist: %1").arg(filePath));
        return {};
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        setError(error, QStringLiteral("Failed to open prompt templates: %1").arg(filePath));
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(error, QStringLiteral("Failed to parse prompt templates: %1").arg(parseError.errorString()));
        return {};
    }

    QVector<PromptTemplate> templates;
    const QJsonArray array = document.object().value(QString::fromLatin1(TemplatesKey)).toArray();
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }

        const PromptTemplate promptTemplate = fromJson(value.toObject());
        if (promptTemplate.isValid()) {
            templates.append(promptTemplate);
        }
    }

    return templates;
}

bool PromptTemplateStorage::exportTemplates(const QString &filePath, const QVector<PromptTemplate> &templates, QString *error)
{
    if (error != nullptr) {
        error->clear();
    }

    QDir directory(QFileInfo(filePath).absolutePath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        setError(error, QStringLiteral("Failed to create prompt template directory: %1").arg(directory.absolutePath()));
        return false;
    }

    QJsonArray array;
    for (const PromptTemplate &promptTemplate : templates) {
        if (promptTemplate.isValid()) {
            array.append(toJson(promptTemplate));
        }
    }

    QJsonObject root;
    root.insert(QString::fromLatin1(VersionKey), 1);
    root.insert(QString::fromLatin1(TemplatesKey), array);

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        setError(error, QStringLiteral("Failed to write prompt templates: %1").arg(filePath));
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

QVector<PromptTemplate> PromptTemplateStorage::mergeTemplates(const QVector<PromptTemplate> &currentTemplates,
                                                              const QVector<PromptTemplate> &importedTemplates)
{
    QVector<PromptTemplate> merged = currentTemplates;
    for (const PromptTemplate &promptTemplate : importedTemplates) {
        if (!promptTemplate.isValid()) {
            continue;
        }

        const bool duplicateId = hasTemplateId(merged, promptTemplate.id);
        const bool duplicateName = hasTemplateName(merged, promptTemplate.name);
        if (!duplicateId && !duplicateName) {
            merged.append(promptTemplate);
            continue;
        }

        merged.append(PromptTemplate::create(uniqueTemplateName(merged, promptTemplate.name), promptTemplate.content));
    }

    return merged;
}

QString PromptTemplateStorage::resolvedFilePath() const
{
    if (!m_filePath.trimmed().isEmpty()) {
        return m_filePath;
    }

    QString directoryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directoryPath.trimmed().isEmpty()) {
        directoryPath = QDir::temp().filePath(QStringLiteral("AIChatDesktop"));
    }

    return QDir(directoryPath).filePath(QStringLiteral("prompt-templates.json"));
}
