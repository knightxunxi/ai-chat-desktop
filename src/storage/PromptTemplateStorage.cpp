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
        if (error != nullptr) {
            *error = QStringLiteral("Failed to open prompt templates: %1").arg(path);
        }
        return defaultTemplates();
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to parse prompt templates: %1").arg(parseError.errorString());
        }
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
    const QString path = resolvedFilePath();
    QDir directory(QFileInfo(path).absolutePath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to create prompt template directory: %1").arg(directory.absolutePath());
        }
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

    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to write prompt templates: %1").arg(path);
        }
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
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
