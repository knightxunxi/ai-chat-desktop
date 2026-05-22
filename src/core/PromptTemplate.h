#pragma once

#include <QString>
#include <QUuid>

struct PromptTemplate {
    QString id;
    QString name;
    QString content;

    static PromptTemplate create(const QString &name, const QString &content)
    {
        PromptTemplate promptTemplate;
        promptTemplate.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        promptTemplate.name = name.trimmed();
        promptTemplate.content = content.trimmed();
        return promptTemplate;
    }

    bool isValid() const
    {
        return !id.trimmed().isEmpty() &&
               !name.trimmed().isEmpty() &&
               !content.trimmed().isEmpty();
    }
};
