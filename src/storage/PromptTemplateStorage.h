#pragma once

#include "core/PromptTemplate.h"

#include <QString>
#include <QVector>

// 学习注释：角色提示词模板 JSON 存储，负责默认模板、导入导出和重复模板合并。
// 使用模块：ApplicationController 加载/保存模板，RolePromptDialog 导入导出模板。
class PromptTemplateStorage
{
public:
    // 功能：指定模板文件路径；使用模块：生产环境用默认路径，测试可传临时路径。
    explicit PromptTemplateStorage(QString filePath = QString());

    // 功能：读取模板文件，失败时返回默认模板；使用模块：ApplicationController::initialize。
    QVector<PromptTemplate> load(QString *error = nullptr) const;
    // 功能：保存模板到本地 JSON；使用模块：ApplicationController::savePromptTemplates。
    bool save(const QVector<PromptTemplate> &templates, QString *error = nullptr) const;
    // 功能：返回实际使用的模板文件路径；使用模块：测试和排查本地数据位置。
    QString filePath() const;

    // 功能：提供内置模板；使用模块：首次启动或模板文件损坏时使用。
    static QVector<PromptTemplate> defaultTemplates();
    // 功能：从 JSON 文件导入模板；使用模块：RolePromptDialog::importTemplates。
    static QVector<PromptTemplate> importTemplates(const QString &filePath, QString *error = nullptr);
    // 功能：导出模板到 JSON 文件；使用模块：RolePromptDialog::exportTemplates。
    static bool exportTemplates(const QString &filePath, const QVector<PromptTemplate> &templates, QString *error = nullptr);
    // 功能：合并导入模板并处理重复 ID/名称；使用模块：导入模板后更新当前列表。
    static QVector<PromptTemplate> mergeTemplates(const QVector<PromptTemplate> &currentTemplates,
                                                  const QVector<PromptTemplate> &importedTemplates);

private:
    // 功能：解析默认 AppData 路径或构造时传入的路径；使用模块：load/save/filePath。
    QString resolvedFilePath() const;

    QString m_filePath; // 功能：可选自定义模板文件路径；使用模块：测试隔离和默认路径覆盖。
};
