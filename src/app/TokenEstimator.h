#pragma once

#include "core/ChatMessage.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

// 功能：纯静态工具类，估算文本所需的 token 数量。
// 使用模块：ContextWindowManager 用于判断上下文窗口是否溢出。
class TokenEstimator {
public:
    // 功能：估算单段文本的 token 数量；使用模块：上下文窗口管理和预估。
    static size_t estimateTokens(const QString &text);
    // 功能：估算单条消息的 token 数（内容 + role 字段开销 4 token）；使用模块：逐条统计。
    static size_t estimateMessageTokens(const ChatMessage &msg);
    // 功能：估算消息列表的总 token 数；使用模块：上下文窗口检查和压缩。
    static size_t estimateTotalTokens(const QVector<ChatMessage> &messages);
    // 功能：估算系统提示词的 token 数（内容 + 4 token 开销）；使用模块：计入上下文总 token。
    static size_t estimateSystemPromptTokens(const QString &systemPrompt);

    // V19: 从 OpenAI API usage JSON 解析实际 token 数
    static size_t parseUsageTokens(const QJsonObject &usage);
    static size_t promptTokensFromUsage(const QJsonObject &usage);
    static size_t completionTokensFromUsage(const QJsonObject &usage);

private:
    TokenEstimator() = delete;

    // 功能：判断是否为中文字符（含 CJK 统一汉字、扩展 A 区、兼容汉字和中日韩标点）。
    static bool isChineseChar(QChar ch);
    // 功能：判断是否为可计数的英文/数字/标点字符；中文和空白字符不计入英文 token 算法。
    static bool isTokenCountableChar(QChar ch);
};
