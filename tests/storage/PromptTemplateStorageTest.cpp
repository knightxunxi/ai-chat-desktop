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

    return 0;
}
