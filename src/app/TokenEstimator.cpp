#include "app/TokenEstimator.h"

// 功能：中文字符 Unicode 范围判定，每个中文计 1.5 token。
bool TokenEstimator::isChineseChar(QChar ch)
{
    const ushort code = ch.unicode();

    // CJK Unified Ideographs: U+4E00–U+9FFF
    if (code >= 0x4E00 && code <= 0x9FFF) {
        return true;
    }

    // CJK Unified Ideographs Extension A: U+3400–U+4DBF
    if (code >= 0x3400 && code <= 0x4DBF) {
        return true;
    }

    // CJK Compatibility Ideographs: U+F900–U+FAFF
    if (code >= 0xF900 && code <= 0xFAFF) {
        return true;
    }

    // CJK Symbols and Punctuation: U+3000–U+303F（中文标点）
    if (code >= 0x3000 && code <= 0x303F) {
        return true;
    }

    return false;
}

// 功能：英文/数字/标点字符判定，每 4 个计 1 token。
bool TokenEstimator::isTokenCountableChar(QChar ch)
{
    const ushort code = ch.unicode();

    // 字母 A-Z / a-z
    if ((code >= 0x0041 && code <= 0x005A) ||
        (code >= 0x0061 && code <= 0x007A)) {
        return true;
    }

    // 数字 0-9
    if (code >= 0x0030 && code <= 0x0039) {
        return true;
    }

    // 标点符号：!@#$%^&*()_+-={}[]|\:;"'<>,.?/~`'
    // clang-format off
    switch (code) {
    case 0x0021: // !
    case 0x0040: // @
    case 0x0023: // #
    case 0x0024: // $
    case 0x0025: // %
    case 0x005E: // ^
    case 0x0026: // &
    case 0x002A: // *
    case 0x0028: // (
    case 0x0029: // )
    case 0x005F: // _
    case 0x002B: // +
    case 0x002D: // -
    case 0x003D: // =
    case 0x007B: // {
    case 0x007D: // }
    case 0x005B: // [
    case 0x005D: // ]
    case 0x007C: // |
    case 0x005C: // backslash
    case 0x003A: // :
    case 0x003B: // ;
    case 0x0022: // "
    case 0x0027: // '
    case 0x003C: // <
    case 0x003E: // >
    case 0x002C: // ,
    case 0x002E: // .
    case 0x003F: // ?
    case 0x002F: // /
    case 0x007E: // ~
    case 0x0060: // `
        return true;
    }
    // clang-format on

    return false;
}

// 功能：核心 token 估算算法。
// 中文字符（含标点）每个计 1.5 token，英文/数字/符号每 4 个计 1 token，
// 空格、换行、制表符不计。最终结果四舍五入。
size_t TokenEstimator::estimateTokens(const QString &text)
{
    double total = 0.0;
    int englishChunkCount = 0;

    for (const QChar ch : text) {
        if (isChineseChar(ch)) {
            // 先将累积的英文 chunk 换算为 token
            if (englishChunkCount > 0) {
                total += static_cast<double>(englishChunkCount) / 4.0;
                englishChunkCount = 0;
            }
            total += 1.5;
        } else if (isTokenCountableChar(ch)) {
            ++englishChunkCount;
        }
        // 空格、换行、制表符等不计入任何 count
    }

    // 将剩余英文 chunk 换算
    if (englishChunkCount > 0) {
        total += static_cast<double>(englishChunkCount) / 4.0;
    }

    return static_cast<size_t>(total + 0.5);
}

size_t TokenEstimator::estimateMessageTokens(const ChatMessage &msg)
{
    // 消息内容 token + 4 token role 字段开销
    return estimateTokens(msg.content) + 4;
}

size_t TokenEstimator::estimateTotalTokens(const QVector<ChatMessage> &messages)
{
    size_t total = 0;
    for (const ChatMessage &msg : messages) {
        total += estimateMessageTokens(msg);
    }
    return total;
}

size_t TokenEstimator::estimateSystemPromptTokens(const QString &systemPrompt)
{
    return estimateTokens(systemPrompt) + 4;
}

// V19: 从 OpenAI API usage JSON 解析实际 token 数
size_t TokenEstimator::parseUsageTokens(const QJsonObject &usage)
{
    return static_cast<size_t>(usage.value(QStringLiteral("total_tokens")).toInt(0));
}

size_t TokenEstimator::promptTokensFromUsage(const QJsonObject &usage)
{
    return static_cast<size_t>(usage.value(QStringLiteral("prompt_tokens")).toInt(0));
}

size_t TokenEstimator::completionTokensFromUsage(const QJsonObject &usage)
{
    return static_cast<size_t>(usage.value(QStringLiteral("completion_tokens")).toInt(0));
}
