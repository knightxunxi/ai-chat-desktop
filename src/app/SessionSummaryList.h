#pragma once

#include "core/ChatSession.h"

#include <QVector>

namespace SessionSummaryList {

void upsert(QVector<ChatSession> *summaries, const ChatSession &session, bool moveToTop);

} // namespace SessionSummaryList
