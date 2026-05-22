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

private:
    QString resolvedFilePath() const;

    QString m_filePath;
};
