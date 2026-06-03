#include "app/ContextWindowManager.h"

#include "app/SummaryAPIClient.h"
#include "app/TokenEstimator.h"
#include "support/AppLogger.h"

#include <QHash>

// --- 构造 ---

ContextWindowManager::ContextWindowManager(size_t maxContextTokens)
    : m_maxContextTokens(maxContextTokens)
{
}

void ContextWindowManager::setSummaryClient(SummaryAPIClient *client)
{
    m_summaryClient = client;
}

// --- 静态：模型名 → 上下文窗口大小 ---

size_t ContextWindowManager::contextWindowForModel(const QString &modelName)
{
    static const QHash<QString, size_t> modelMap = {
        {QStringLiteral("deepseek-v4-flash"), 128000},
        {QStringLiteral("deepseek-chat"), 64000},
        {QStringLiteral("gpt-4"), 8192},
        {QStringLiteral("gpt-4-turbo"), 128000},
        {QStringLiteral("gpt-3.5-turbo"), 4096},
    };

    for (auto it = modelMap.begin(); it != modelMap.end(); ++it) {
        if (modelName.contains(it.key(), Qt::CaseInsensitive)) {
            return it.value();
        }
    }

    return kDefaultContextWindow;
}

// --- 压缩判断 ---

bool ContextWindowManager::needsCompression(size_t totalTokens) const
{
    return totalTokens > static_cast<size_t>(static_cast<double>(m_maxContextTokens) * kCompressionThreshold);
}

// --- 识别裁剪范围 ---

ContextWindowManager::TrimRange ContextWindowManager::identifyTrimRange(
    const QVector<ChatMessage> &messages) const
{
    TrimRange range;

    // 从末尾向前扫描，找到第 kKeepRecentRounds 轮用户消息的索引
    int roundCount = 0;
    int keepFromIndex = messages.size();

    for (int i = messages.size() - 1; i >= 0; --i) {
        if (messages[i].role == MessageRole::User) {
            ++roundCount;
            if (roundCount >= kKeepRecentRounds) {
                keepFromIndex = i;
                break;
            }
        }
    }

    // 边界：消息不足 kKeepRecentRounds 轮，不裁剪
    if (roundCount < kKeepRecentRounds) {
        range.keepFromIndex = 0;
        range.trimmedCount = 0;
        range.trimmedRounds = 0;
        return range;
    }

    range.keepFromIndex = keepFromIndex;
    range.trimmedCount = keepFromIndex;
    range.trimmedRounds = countCompleteRounds(messages, 0, keepFromIndex);

    return range;
}

// --- 统计完整轮数 ---

int ContextWindowManager::countCompleteRounds(
    const QVector<ChatMessage> &messages, int startIdx, int endIdx)
{
    int rounds = 0;
    int i = startIdx;

    while (i < endIdx) {
        // 找一组 user→assistant
        if (i < endIdx && messages[i].role == MessageRole::User) {
            ++i;
            // 跳过紧随其后的 Assistant 消息
            if (i < endIdx && messages[i].role == MessageRole::Assistant) {
                ++rounds;
                ++i;
            }
            // 如果 User 后没有 Assistant，这轮不完整，但已计 User，继续
        } else {
            ++i;
        }
    }

    return rounds;
}

// --- 确保消息以 User 开头、User 结尾 ---

void ContextWindowManager::ensureRoleOrdering(QVector<ChatMessage> &messages) const
{
    if (messages.isEmpty()) {
        return;
    }

    // 首条不是 User → 找到第一个 User 消息，删除之前的所有消息
    if (messages.first().role != MessageRole::User) {
        int firstUserIdx = -1;
        for (int i = 0; i < messages.size(); ++i) {
            if (messages[i].role == MessageRole::User) {
                firstUserIdx = i;
                break;
            }
        }

        if (firstUserIdx > 0) {
            messages.erase(messages.begin(), messages.begin() + firstUserIdx);
        } else if (firstUserIdx < 0) {
            // 没有 User 消息，清空（防御）
            messages.clear();
            return;
        }
    }

    if (messages.isEmpty()) {
        return;
    }

    // 末条不是 User → 找到最后一个 User 消息，删除之后的所有消息
    if (messages.last().role != MessageRole::User) {
        int lastUserIdx = -1;
        for (int i = messages.size() - 1; i >= 0; --i) {
            if (messages[i].role == MessageRole::User) {
                lastUserIdx = i;
                break;
            }
        }

        if (lastUserIdx >= 0 && lastUserIdx < messages.size() - 1) {
            messages.erase(messages.begin() + lastUserIdx + 1, messages.end());
        }
    }
}

// --- 生成压缩提示 ---

QString ContextWindowManager::buildCompressionHint(int trimmedRounds, const QString &summary)
{
    if (summary.isEmpty()) {
        return QStringLiteral("已压缩 %1 轮历史对话").arg(trimmedRounds);
    }

    return QStringLiteral("已压缩 %1 轮历史对话为摘要：%2").arg(trimmedRounds).arg(summary);
}

// --- 核心处理逻辑 ---

ContextWindowResult ContextWindowManager::processMessages(
    const QVector<ChatMessage> &messages,
    const QString &systemPrompt)
{
    const size_t totalTokens = TokenEstimator::estimateTotalTokens(messages)
                               + TokenEstimator::estimateSystemPromptTokens(systemPrompt);

    if (!needsCompression(totalTokens)) {
        ContextWindowResult result;
        result.processedMessages = messages;
        result.summary = QString();
        result.trimmedRoundCount = 0;
        result.wasTrimmed = false;
        return result;
    }

    const TrimRange trim = identifyTrimRange(messages);

    // 构建裁剪后保留的消息
    QVector<ChatMessage> trimmedMessages;
    if (trim.keepFromIndex > 0 && trim.keepFromIndex < messages.size()) {
        trimmedMessages = messages.mid(trim.keepFromIndex);
    } else if (trim.keepFromIndex == 0) {
        // 不足 3 轮，不裁剪但标记超阈值
        ContextWindowResult result;
        result.processedMessages = messages;
        result.summary = QString();
        result.trimmedRoundCount = 0;
        result.wasTrimmed = true;
        AppLogger::info(QStringLiteral("ContextWindow"),
                        QStringLiteral("Context over threshold but fewer than %1 rounds; no trim. "
                                       "tokens=%2 of %3")
                            .arg(kKeepRecentRounds)
                            .arg(totalTokens)
                            .arg(m_maxContextTokens));
        return result;
    } else {
        trimmedMessages = messages;
    }

    // 生成摘要
    QString summary;
    if (m_summaryClient != nullptr && trim.trimmedCount > 0) {
        const QVector<ChatMessage> croppedMessages = messages.mid(0, trim.trimmedCount);
        summary = m_summaryClient->generateSummary(croppedMessages);
    }

    // 日志：压缩发生
    AppLogger::info(QStringLiteral("ContextWindow"),
                    QStringLiteral("ContextWindow compressed %1 rounds (%2 messages kept, %3 trimmed, "
                                   "tokens: %4 of %5)")
                        .arg(trim.trimmedRounds)
                        .arg(trimmedMessages.size())
                        .arg(trim.trimmedCount)
                        .arg(totalTokens)
                        .arg(m_maxContextTokens));

    // 兜底：确保消息列表以 User 开头、User 结尾
    ensureRoleOrdering(trimmedMessages);

    ContextWindowResult result;
    result.processedMessages = trimmedMessages;
    result.summary = summary;
    result.trimmedRoundCount = trim.trimmedRounds;
    result.wasTrimmed = true;
    return result;
}
