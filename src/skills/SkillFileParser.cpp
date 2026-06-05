#include "skills/SkillFileParser.h"

#include "support/AppLogger.h"

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

namespace {

// YAML frontmatter 分隔符
const QString kFrontmatterDelim = QStringLiteral("---");

// 最大技能文件大小（字节）
constexpr qint64 kMaxFileBytes = 32 * 1024;

// 最小 Markdown 体字符数
constexpr int kMinBodyChars = 50;

// ============================================================================
// 行级状态机：解析 YAML frontmatter
// ============================================================================

enum class ParseState {
    BeforeFrontmatter,   // 等待第一个 ---
    InFrontmatter,       // 正在解析 YAML 键值对
    AfterFrontmatter     // YAML 结束，等待 Markdown 体
};

// 功能：解析单行 YAML key: value；使用模块：parseFrontmatter 内部。
void parseYamlLine(const QString &line, SkillMetadata *metadata)
{
    const int colonIndex = line.indexOf(QLatin1Char(':'));
    if (colonIndex < 0) {
        return; // 不是有效的 YAML 行
    }

    const QString key = line.left(colonIndex).trimmed();
    QString value = line.mid(colonIndex + 1).trimmed();

    // 去除引号
    if (value.size() >= 2) {
        const QChar first = value.at(0);
        const QChar last = value.at(value.size() - 1);
        if ((first == QLatin1Char('"') && last == QLatin1Char('"')) ||
            (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
            value = value.mid(1, value.size() - 2);
        }
    }

    if (key == QStringLiteral("name")) {
        metadata->name = value;
    } else if (key == QStringLiteral("description")) {
        metadata->description = value;
    } else if (key == QStringLiteral("version")) {
        metadata->version = value;
    } else if (key == QStringLiteral("priority")) {
        bool ok = false;
        const int p = value.toInt(&ok);
        if (ok) {
            metadata->priority = p;
        }
    } else if (key == QStringLiteral("enabled")) {
        metadata->enabled = (value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
    } else if (key == QStringLiteral("author")) {
        metadata->author = value;
    }
    // triggers 单独处理（列表语法），忽略未知字段
}

// 功能：解析 YAML triggers 列表行（- item 语法）；使用模块：parseFrontmatter 内部。
bool parseTriggerItem(const QString &line, QString *trigger)
{
    QString trimmed = line.trimmed();
    if (!trimmed.startsWith(QLatin1Char('-'))) {
        return false;
    }

    trimmed = trimmed.mid(1).trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    // 去除引号
    if (trimmed.size() >= 2) {
        const QChar first = trimmed.at(0);
        const QChar last = trimmed.at(trimmed.size() - 1);
        if ((first == QLatin1Char('"') && last == QLatin1Char('"')) ||
            (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
            trimmed = trimmed.mid(1, trimmed.size() - 2);
        }
    }

    *trigger = trimmed;
    return true;
}

// 功能：解析完整的 YAML frontmatter 文本；使用模块：parseContent 内部。
std::optional<SkillMetadata> parseFrontmatter(const QString &yamlText, QString *error)
{
    SkillMetadata metadata;
    const QStringList lines = yamlText.split(QLatin1Char('\n'));

    ParseState state = ParseState::BeforeFrontmatter;
    bool foundDelimiter = false;

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();

        switch (state) {
        case ParseState::BeforeFrontmatter:
            if (line == kFrontmatterDelim) {
                state = ParseState::InFrontmatter;
                foundDelimiter = true;
            }
            // 忽略第一个 --- 之前的空白行
            break;

        case ParseState::InFrontmatter:
            if (line == kFrontmatterDelim) {
                state = ParseState::AfterFrontmatter;
                break;
            }

            // triggers 是唯一使用列表语法的字段
            if (line.startsWith(QStringLiteral("triggers:"))) {
                // 可能是内联列表 triggers: [a, b] 或后续行 - a, - b
                const int colonIndex = line.indexOf(QLatin1Char(':'));
                const QString afterColon = line.mid(colonIndex + 1).trimmed();
                if (!afterColon.isEmpty() && afterColon.startsWith(QLatin1Char('['))) {
                    // 内联列表：triggers: [a, b, c]
                    QString inner = afterColon;
                    inner.remove(QLatin1Char('['));
                    inner.remove(QLatin1Char(']'));
                    const QStringList items = inner.split(QLatin1Char(','));
                    for (const QString &item : items) {
                        QString trimmed = item.trimmed();
                        if (trimmed.size() >= 2) {
                            const QChar first = trimmed.at(0);
                            const QChar last = trimmed.at(trimmed.size() - 1);
                            if ((first == QLatin1Char('"') && last == QLatin1Char('"')) ||
                                (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
                                trimmed = trimmed.mid(1, trimmed.size() - 2);
                            }
                        }
                        if (!trimmed.isEmpty()) {
                            metadata.triggers.append(trimmed);
                        }
                    }
                }
                // 否则后续行用 - item 语法解析
                break;
            }

            if (line.startsWith(QStringLiteral("- ")) || line.startsWith(QStringLiteral("-"))) {
                // triggers 列表项
                QString trigger;
                if (parseTriggerItem(line, &trigger)) {
                    metadata.triggers.append(trigger);
                }
                break;
            }

            // 普通键值对
            parseYamlLine(line, &metadata);
            break;

        case ParseState::AfterFrontmatter:
            // YAML 已结束，忽略后续行
            break;
        }
    }

    if (!foundDelimiter || state != ParseState::AfterFrontmatter) {
        if (error != nullptr) {
            *error = QStringLiteral("Missing or incomplete YAML frontmatter delimiters (---).");
        }
        return std::nullopt;
    }

    return metadata;
}

// 功能：提取 Markdown 体（第二个 --- 之后的所有内容）；使用模块：parseContent 内部。
QString extractMarkdownBody(const QString &rawContent)
{
    const int firstDelim = rawContent.indexOf(kFrontmatterDelim);
    if (firstDelim < 0) {
        return QString();
    }

    const int secondDelim = rawContent.indexOf(kFrontmatterDelim, firstDelim + kFrontmatterDelim.size());
    if (secondDelim < 0) {
        return QString();
    }

    return rawContent.mid(secondDelim + kFrontmatterDelim.size()).trimmed();
}

} // namespace

namespace SkillFileParser {

std::optional<SkillDefinition> parseFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: file not accessible").arg(filePath));
        return std::nullopt;
    }

    if (fileInfo.size() > kMaxFileBytes) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: exceeds max file size").arg(filePath));
        return std::nullopt;
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: %2").arg(filePath, file.errorString()));
        return std::nullopt;
    }

    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    return parseContent(content, filePath, QStringLiteral("project"));
}

std::optional<SkillDefinition> parseContent(const QString &rawContent,
                                            const QString &sourcePath,
                                            const QString &sourceType)
{
    if (rawContent.trimmed().isEmpty()) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: empty content").arg(sourcePath));
        return std::nullopt;
    }

    // 1. 提取 frontmatter 文本
    const int firstDelim = rawContent.indexOf(kFrontmatterDelim);
    if (firstDelim < 0) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: no frontmatter delimiter found").arg(sourcePath));
        return std::nullopt;
    }

    const int secondDelim = rawContent.indexOf(kFrontmatterDelim, firstDelim + kFrontmatterDelim.size());
    if (secondDelim < 0) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: missing closing frontmatter delimiter").arg(sourcePath));
        return std::nullopt;
    }

    const QString yamlText = rawContent.mid(firstDelim, secondDelim + kFrontmatterDelim.size() - firstDelim);

    // 2. 解析 frontmatter
    QString parseError;
    std::optional<SkillMetadata> metadata = parseFrontmatter(yamlText, &parseError);
    if (!metadata.has_value()) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: %2").arg(sourcePath, parseError));
        return std::nullopt;
    }

    // 3. 提取 Markdown 体
    const QString body = extractMarkdownBody(rawContent);
    if (body.size() < kMinBodyChars) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: body too short (%3 chars, min %4)")
                               .arg(sourcePath).arg(body.size()).arg(kMinBodyChars));
        return std::nullopt;
    }

    // 4. 校验必填字段
    if (!metadata->isValid()) {
        AppLogger::warning(QStringLiteral("SkillFileParser"),
                           QStringLiteral("parse failed: %1, reason: missing required fields").arg(sourcePath));
        return std::nullopt;
    }

    SkillDefinition def;
    def.metadata = metadata.value();
    def.instructions = body;
    def.sourcePath = sourcePath;
    def.sourceType = sourceType;

    return def;
}

} // namespace SkillFileParser
