#pragma once

#include "core/PromptTemplate.h"

#include <QString>
#include <QVector>

class PromptTemplateStorage
{
public:
    explicit PromptTemplateStorage(QString filePath = QString());

    QVector<PromptTemplate> load(QString *error = nullptr) const;
    bool save(const QVector<PromptTemplate> &templates, QString *error = nullptr) const;
    QString filePath() const;

    static QVector<PromptTemplate> defaultTemplates();
    static QVector<PromptTemplate> importTemplates(const QString &filePath, QString *error = nullptr);
    static bool exportTemplates(const QString &filePath, const QVector<PromptTemplate> &templates, QString *error = nullptr);
    static QVector<PromptTemplate> mergeTemplates(const QVector<PromptTemplate> &currentTemplates,
                                                  const QVector<PromptTemplate> &importedTemplates);

private:
    QString resolvedFilePath() const;

    QString m_filePath;
};
