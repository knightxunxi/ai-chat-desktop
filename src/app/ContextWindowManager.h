#pragma once

#include "core/ChatMessage.h"

#include <QString>
#include <QVector>

// 功能：上下文窗口压缩结果。
struct ContextWindowResult {
    QVector<ChatMessage> processedMessages; // 压缩后的消息列表
    QString summary;                        // 被裁剪消息的摘要（可能为空）
    int trimmedRoundCount = 0;              // 被裁剪的对话轮数
    bool wasTrimmed = false;                // 是否实际发生了压缩
};

// 功能：上下文窗口管理器，负责检查 token 溢出并压缩历史对话。
// 使用模块：ApplicationController 在每次请求前检查并可能压缩上下文。
class ContextWindowManager {
public:
    // 功能：构造并设置最大上下文 token 数；使用模块：ApplicationController 初始化。
    explicit ContextWindowManager(size_t maxContextTokens = kDefaultContextWindow);

    // 功能：设置摘要生成客户端；使用模块：ApplicationController::initialize() 中注入。
    void setSummaryClient(class SummaryAPIClient *client);

    // 功能：处理消息列表，必要时压缩上下文；使用模块：ApplicationController 请求前调用。
    ContextWindowResult processMessages(
        const QVector<ChatMessage> &messages,
        const QString &systemPrompt);

    // 功能：根据模型名返回其上下文窗口大小；使用模块：配置加载后确定窗口上限。
    static size_t contextWindowForModel(const QString &modelName);

    // 功能：构建压缩提示文本，用于注入 system prompt；使用模块：ApplicationController 注入。
    static QString buildCompressionHint(int trimmedRounds, const QString &summary);

private:
    // 功能：判断总 token 是否超过压缩阈值；使用模块：processMessages 第一步。
    bool needsCompression(size_t totalTokens) const;

    // 功能：裁剪范围描述。
    struct TrimRange {
        int keepFromIndex = 0;  // 保留消息的起始索引（从此开始到末尾保留）
        int trimmedCount = 0;   // 被裁剪的消息数量
        int trimmedRounds = 0;  // 被裁剪的完整对话轮数
    };

    // 功能：识别需要裁剪的消息范围；使用模块：processMessages 压缩分支。
    TrimRange identifyTrimRange(const QVector<ChatMessage> &messages) const;

    // 功能：确保消息列表以 User 开头、User 结尾；使用模块：processMessages 压缩后兜底。
    void ensureRoleOrdering(QVector<ChatMessage> &messages) const;

    // 功能：统计指定范围内的完整 user→assistant 轮数。
    static int countCompleteRounds(const QVector<ChatMessage> &messages, int startIdx, int endIdx);

    size_t m_maxContextTokens;
    SummaryAPIClient *m_summaryClient = nullptr;

    static constexpr double kCompressionThreshold = 0.85;
    static constexpr int kKeepRecentRounds = 3;
    static constexpr int kDefaultContextWindow = 64000;
};
