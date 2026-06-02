#include "tools/AgentToolCatalog.h"

namespace {

AgentToolDescriptor makeDescriptor(
    const QString &id,
    const QString &englishName,
    const QString &chineseName,
    const QString &englishDescription,
    const QString &chineseDescription,
    const QString &inputPolicy,
    AgentToolRisk risk,
    bool resultMayContainSensitiveContent)
{
    AgentToolDescriptor descriptor;
    descriptor.id = id;
    descriptor.englishName = englishName;
    descriptor.chineseName = chineseName;
    descriptor.englishDescription = englishDescription;
    descriptor.chineseDescription = chineseDescription;
    descriptor.inputPolicy = inputPolicy;
    descriptor.risk = risk;
    descriptor.requiresUserConfirmation = true;
    descriptor.resultMayContainSensitiveContent = resultMayContainSensitiveContent;
    descriptor.enabledForAgent = true;
    return descriptor;
}

} // namespace

QString agentToolRiskToString(AgentToolRisk risk)
{
    switch (risk) {
    case AgentToolRisk::Low:
        return QStringLiteral("low");
    case AgentToolRisk::Medium:
        return QStringLiteral("medium");
    case AgentToolRisk::High:
        return QStringLiteral("high");
    }

    return QStringLiteral("high");
}

bool agentToolRiskFromString(const QString &value, AgentToolRisk *risk)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("low")) {
        if (risk != nullptr) {
            *risk = AgentToolRisk::Low;
        }
        return true;
    }

    if (normalized == QStringLiteral("medium")) {
        if (risk != nullptr) {
            *risk = AgentToolRisk::Medium;
        }
        return true;
    }

    if (normalized == QStringLiteral("high")) {
        if (risk != nullptr) {
            *risk = AgentToolRisk::High;
        }
        return true;
    }

    return false;
}

AgentToolRisk maxAgentToolRisk(AgentToolRisk left, AgentToolRisk right)
{
    return static_cast<int>(left) >= static_cast<int>(right) ? left : right;
}

QString agentToolDisplayName(const AgentToolDescriptor &descriptor, AppLanguage language)
{
    return language == AppLanguage::Chinese ? descriptor.chineseName : descriptor.englishName;
}

QVector<AgentToolDescriptor> defaultAgentToolCatalog()
{
    QVector<AgentToolDescriptor> catalog;
    catalog.reserve(8);

    catalog.append(makeDescriptor(
        QStringLiteral("json.format"),
        QStringLiteral("JSON Format"),
        QStringLiteral("JSON 格式化"),
        QStringLiteral("Format JSON with indentation."),
        QStringLiteral("将 JSON 转为缩进格式。"),
        QStringLiteral("Input must be user-provided JSON text."),
        AgentToolRisk::Low,
        false));

    catalog.append(makeDescriptor(
        QStringLiteral("json.compact"),
        QStringLiteral("JSON Compact"),
        QStringLiteral("JSON 压缩"),
        QStringLiteral("Compact JSON into one line."),
        QStringLiteral("将 JSON 转为单行紧凑格式。"),
        QStringLiteral("Input must be user-provided JSON text."),
        AgentToolRisk::Low,
        false));

    catalog.append(makeDescriptor(
        QStringLiteral("markdown.cleanup"),
        QStringLiteral("Markdown Cleanup"),
        QStringLiteral("Markdown 整理"),
        QStringLiteral("Clean low-risk Markdown whitespace while preserving code blocks."),
        QStringLiteral("清理 Markdown 空白并保留代码块内容。"),
        QStringLiteral("Input must be user-provided Markdown text."),
        AgentToolRisk::Low,
        false));

    catalog.append(makeDescriptor(
        QStringLiteral("text.cleanup"),
        QStringLiteral("Text Cleanup"),
        QStringLiteral("文本清理"),
        QStringLiteral("Normalize line endings and repeated blank lines."),
        QStringLiteral("统一换行并压缩连续空行。"),
        QStringLiteral("Input must be user-provided plain text."),
        AgentToolRisk::Low,
        false));

    catalog.append(makeDescriptor(
        QStringLiteral("file.read_text"),
        QStringLiteral("Read Text File"),
        QStringLiteral("读取文本文件"),
        QStringLiteral("Read a user-selected text file."),
        QStringLiteral("读取用户选择的文本文件。"),
        QStringLiteral("Path must come from a user file picker. Result may contain file content."),
        AgentToolRisk::Medium,
        true));

    catalog.append(makeDescriptor(
        QStringLiteral("file.list_directory"),
        QStringLiteral("List Folder"),
        QStringLiteral("列出文件夹"),
        QStringLiteral("List entries under a user-selected folder."),
        QStringLiteral("列出用户选择文件夹下的条目。"),
        QStringLiteral("Directory must come from a user folder picker. Result may reveal local filenames."),
        AgentToolRisk::Medium,
        true));

    catalog.append(makeDescriptor(
        QStringLiteral("file.save_text"),
        QStringLiteral("Save Text"),
        QStringLiteral("保存文本"),
        QStringLiteral("Save approved text to a user-selected file."),
        QStringLiteral("把用户确认的文本保存到指定文件。"),
        QStringLiteral("Save path must come from a user save dialog. Existing files require confirmation."),
        AgentToolRisk::Medium,
        true));

    catalog.append(makeDescriptor(
        QStringLiteral("file.open_path"),
        QStringLiteral("Open Path"),
        QStringLiteral("打开路径"),
        QStringLiteral("Open a user-confirmed file or folder with the operating system."),
        QStringLiteral("用系统打开用户确认后的文件或文件夹。"),
        QStringLiteral("Path must be selected by the user and confirmed before opening."),
        AgentToolRisk::Medium,
        true));

    return catalog;
}

const AgentToolDescriptor *findAgentToolDescriptor(const QVector<AgentToolDescriptor> &catalog, const QString &toolId)
{
    const QString normalizedToolId = toolId.trimmed();
    for (const AgentToolDescriptor &descriptor : catalog) {
        if (descriptor.id == normalizedToolId) {
            return &descriptor;
        }
    }

    return nullptr;
}
