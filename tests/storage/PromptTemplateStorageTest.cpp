#include "storage/PromptTemplateStorage.h"

#include <QFileInfo>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    QTemporaryDir directory;
    assert(directory.isValid());

    const QString missingPath = directory.filePath(QStringLiteral("missing-templates.json"));
    PromptTemplateStorage missingStorage(missingPath);
    const QVector<PromptTemplate> defaults = missingStorage.load();
    assert(defaults.size() >= 2);
    assert(defaults.first().isValid());

    const QString path = directory.filePath(QStringLiteral("prompt-templates.json"));
    PromptTemplateStorage storage(path);

    QVector<PromptTemplate> templates;
    templates.append(PromptTemplate::create(QStringLiteral("Code Reviewer"), QStringLiteral("Review C++ code for correctness.")));
    templates.append(PromptTemplate::create(QStringLiteral("Writer"), QStringLiteral("Improve clarity and tone.")));

    QString error;
    assert(storage.save(templates, &error));
    assert(error.isEmpty());
    assert(QFileInfo::exists(path));

    const QVector<PromptTemplate> loaded = storage.load(&error);
    assert(error.isEmpty());
    assert(loaded.size() == 2);
    assert(loaded[0].id == templates[0].id);
    assert(loaded[0].name == QStringLiteral("Code Reviewer"));
    assert(loaded[0].content == QStringLiteral("Review C++ code for correctness."));

    const QString exportPath = directory.filePath(QStringLiteral("exported-templates.json"));
    assert(PromptTemplateStorage::exportTemplates(exportPath, templates, &error));
    assert(error.isEmpty());

    const QVector<PromptTemplate> imported = PromptTemplateStorage::importTemplates(exportPath, &error);
    assert(error.isEmpty());
    assert(imported.size() == 2);
    assert(imported[1].name == QStringLiteral("Writer"));

    QVector<PromptTemplate> duplicateTemplates;
    duplicateTemplates.append(PromptTemplate{
        templates[0].id,
        templates[0].name,
        QStringLiteral("Imported duplicate content.")
    });
    duplicateTemplates.append(PromptTemplate::create(QStringLiteral("Fresh Template"), QStringLiteral("Fresh content.")));

    const QVector<PromptTemplate> merged = PromptTemplateStorage::mergeTemplates(templates, duplicateTemplates);
    assert(merged.size() == 4);
    assert(merged[2].id != templates[0].id);
    assert(merged[2].name.startsWith(QStringLiteral("Code Reviewer")));
    assert(merged[2].content == QStringLiteral("Imported duplicate content."));
    assert(merged[3].name == QStringLiteral("Fresh Template"));

    const QVector<PromptTemplate> missingImport = PromptTemplateStorage::importTemplates(directory.filePath(QStringLiteral("missing.json")), &error);
    assert(missingImport.isEmpty());
    assert(!error.isEmpty());

    return 0;
}
