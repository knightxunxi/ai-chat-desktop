#pragma once

#include <QString>
#include <QUuid>

// 学习注释：角色提示词模板模型，用于保存可复用的 system prompt。
// 使用模块：RolePromptDialog 负责编辑，PromptTemplateStorage 负责本地 JSON 导入导出。
struct PromptTemplate {
    QString id;       // 功能：模板唯一标识；使用模块：下拉框选中项和导入去重。
    QString name;     // 功能：模板显示名称；使用模块：RolePromptDialog 模板下拉框。
    QString content;  // 功能：模板提示词内容；使用模块：应用到 ChatSession::systemPrompt。

    // 功能：创建带 UUID 的模板；使用模块：保存自定义模板和处理导入重复模板。
    static PromptTemplate create(const QString &name, const QString &content)
    {
        PromptTemplate promptTemplate;
        promptTemplate.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        promptTemplate.name = name.trimmed();
        promptTemplate.content = content.trimmed();
        return promptTemplate;
    }

    // 功能：校验模板是否具备可保存的必要字段；使用模块：PromptTemplateStorage 读写和导入过滤。
    bool isValid() const
    {
        return !id.trimmed().isEmpty() &&
               !name.trimmed().isEmpty() &&
               !content.trimmed().isEmpty();
    }
};
