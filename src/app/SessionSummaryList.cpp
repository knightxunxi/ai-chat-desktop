#include "app/SessionSummaryList.h"

namespace SessionSummaryList {

void upsert(QVector<ChatSession> *summaries, const ChatSession &session, bool moveToTop)
{
    if (summaries == nullptr) {
        return;
    }

    int existingIndex = -1;
    for (int index = 0; index < summaries->size(); ++index) {
        if ((*summaries)[index].id == session.id) {
            existingIndex = index;
            break;
        }
    }

    if (existingIndex >= 0) {
        summaries->removeAt(existingIndex);
    }

    if (moveToTop || existingIndex < 0) {
        summaries->prepend(session);
    } else {
        summaries->insert(existingIndex, session);
    }
}

} // namespace SessionSummaryList
