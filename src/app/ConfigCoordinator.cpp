#include "app/ConfigCoordinator.h"

ConfigCoordinator::ConfigCoordinator(QObject *parent)
    : QObject(parent)
{
}

void ConfigCoordinator::initialize()
{
    m_config = m_configStorage.load();
    emit configChanged();

    QString templateError;
    m_promptTemplates = m_promptTemplateStorage.load(&templateError);
    if (!templateError.isEmpty()) {
        emit startupWarning(
            QStringLiteral("Prompt templates are unavailable. Default templates will be used.\n\n%1").arg(templateError),
            QStringLiteral("角色提示词模板不可用，将使用默认模板。\n\n%1").arg(templateError));
    }
    emit promptTemplatesChanged();
}

const AppConfig &ConfigCoordinator::config() const
{
    return m_config;
}

const QVector<PromptTemplate> &ConfigCoordinator::promptTemplates() const
{
    return m_promptTemplates;
}

QString ConfigCoordinator::text(const QString &english, const QString &chinese) const
{
    return m_config.language == AppLanguage::English ? english : chinese;
}

bool ConfigCoordinator::saveConfig(const AppConfig &config)
{
    m_config = config;
    QString error;
    const bool saved = m_configStorage.save(m_config, &error);
    emit configChanged();
    if (!saved) {
        emit startupWarning(
            QStringLiteral("Settings were applied for this session, but could not be fully saved.\n\n%1").arg(error),
            QStringLiteral("设置已应用到本次会话，但未能完整保存到本机。\n\n%1").arg(error));
    }
    return saved;
}

void ConfigCoordinator::savePromptTemplates(const QVector<PromptTemplate> &templates)
{
    m_promptTemplates = templates;
    m_promptTemplateStorage.save(m_promptTemplates);
    emit promptTemplatesChanged();
}
