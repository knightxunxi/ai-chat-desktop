#include "app/SessionSummaryList.h"

#include <cassert>

namespace {

ChatSession sessionWithTitle(const QString &id, const QString &title)
{
    ChatSession session = ChatSession::createDefault();
    session.id = id;
    session.title = title;
    return session;
}

} // namespace

int main()
{
    QVector<ChatSession> summaries;
    const ChatSession first = sessionWithTitle(QStringLiteral("first"), QStringLiteral("First"));
    const ChatSession second = sessionWithTitle(QStringLiteral("second"), QStringLiteral("Second"));

    summaries.append(first);
    summaries.append(second);

    ChatSession renamedSecond = second;
    renamedSecond.title = QStringLiteral("Renamed Second");
    SessionSummaryList::upsert(&summaries, renamedSecond, false);

    assert(summaries.size() == 2);
    assert(summaries[0].id == QStringLiteral("first"));
    assert(summaries[1].id == QStringLiteral("second"));
    assert(summaries[1].title == QStringLiteral("Renamed Second"));

    SessionSummaryList::upsert(&summaries, renamedSecond, true);

    assert(summaries.size() == 2);
    assert(summaries[0].id == QStringLiteral("second"));
    assert(summaries[1].id == QStringLiteral("first"));

    const ChatSession third = sessionWithTitle(QStringLiteral("third"), QStringLiteral("Third"));
    SessionSummaryList::upsert(&summaries, third, false);

    assert(summaries.size() == 3);
    assert(summaries[0].id == QStringLiteral("third"));
    assert(summaries[1].id == QStringLiteral("second"));
    assert(summaries[2].id == QStringLiteral("first"));

    return 0;
}
