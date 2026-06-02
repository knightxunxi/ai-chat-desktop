#pragma once

#include "core/ChatSession.h"
#include "core/SessionListFilter.h"

#include <optional>
#include <QVector>

class QSqlQuery;

// 学习注释：聊天历史 SQLite 存储，负责会话列表、消息详情、搜索和删除。
// 使用模块：ApplicationController 通过它读写 ChatSession，ChatSessionExporter 只负责导出文件。
class ChatHistoryStorage
{
public:
    // 功能：指定数据库路径并创建唯一连接名；使用模块：生产环境用默认 AppData，测试用临时数据库。
    explicit ChatHistoryStorage(const QString &databasePath = defaultDatabasePath());
    // 功能：关闭并移除 SQLite 连接；使用模块：对象销毁时自动释放数据库资源。
    ~ChatHistoryStorage();

    // 功能：返回默认聊天数据库路径；使用模块：构造函数默认参数和问题排查。
    static QString defaultDatabasePath();

    // 功能：打开数据库并创建表；使用模块：ApplicationController::initialize。
    bool initialize(QString *errorMessage = nullptr);
    // 功能：插入或更新会话元信息；使用模块：保存当前会话。
    bool saveSession(const ChatSession &session, QString *errorMessage = nullptr);
    // 功能：保存单条消息；使用模块：当前实现主要由测试覆盖，批量保存使用 replaceSessionMessages。
    bool saveMessage(const ChatMessage &message, QString *errorMessage = nullptr);
    // 功能：用当前会话消息完整替换数据库消息；使用模块：saveCurrentSession 保证历史与内存一致。
    bool replaceSessionMessages(const ChatSession &session, QString *errorMessage = nullptr);
    // 功能：读取会话摘要列表；使用模块：侧边栏会话列表。
    QVector<ChatSession> loadSessionSummaries(QString *errorMessage = nullptr) const;
    // 功能：按筛选条件读取会话摘要列表；使用模块：收藏/归档筛选。
    QVector<ChatSession> loadSessionSummaries(SessionListFilter filter, QString *errorMessage = nullptr) const;
    // 功能：按标题或消息内容搜索会话；使用模块：MainWindow 搜索框经 ApplicationController 调用。
    QVector<ChatSession> searchSessionSummaries(const QString &query, QString *errorMessage = nullptr) const;
    // 功能：按筛选条件搜索会话；使用模块：搜索框和收藏/归档筛选组合。
    QVector<ChatSession> searchSessionSummaries(const QString &query, SessionListFilter filter, QString *errorMessage = nullptr) const;
    // 功能：设置会话收藏状态；使用模块：ApplicationController::toggleCurrentSessionFavorite。
    bool setSessionFavorite(const QString &sessionId, bool favorite, QString *errorMessage = nullptr);
    // 功能：设置会话归档状态；使用模块：ApplicationController::toggleCurrentSessionArchived。
    bool setSessionArchived(const QString &sessionId, bool archived, QString *errorMessage = nullptr);
    // 功能：读取指定会话及其消息；使用模块：切换会话、启动恢复最近会话。
    std::optional<ChatSession> loadSession(const QString &sessionId, QString *errorMessage = nullptr) const;
    // 功能：读取最近更新的会话；使用模块：测试和未来恢复入口。
    std::optional<ChatSession> loadLatestSession(QString *errorMessage = nullptr) const;
    // 功能：删除会话及其消息；使用模块：ApplicationController::deleteCurrentSession。
    bool clearSession(const QString &sessionId, QString *errorMessage = nullptr);

private:
    // 功能：确保 SQLite 连接可用；使用模块：所有数据库读写函数内部调用。
    bool ensureOpen(QString *errorMessage = nullptr) const;
    // 功能：确保旧数据库补齐新增会话字段；使用模块：initialize。
    bool ensureSessionColumn(const QString &columnName, const QString &definition, QString *errorMessage = nullptr) const;
    // 功能：补齐某个会话的消息列表；使用模块：loadSession/loadLatestSession。
    bool loadMessages(ChatSession *session, QString *errorMessage = nullptr) const;
    // 功能：把 SQL 查询结果转换为会话摘要；使用模块：列表和搜索读取。
    QVector<ChatSession> readSessionSummaries(QSqlQuery *query, QString *errorMessage = nullptr) const;
    // 功能：统一写入错误信息；使用模块：数据库函数失败分支。
    static void setError(QString *errorMessage, const QString &message);

    QString m_databasePath;   // 功能：SQLite 文件路径；使用模块：ensureOpen。
    QString m_connectionName; // 功能：Qt SQL 连接名；使用模块：避免多个存储实例连接冲突。
};
