#pragma once

#include "core/ChatSession.h"

#include <QString>

namespace ChatSessionExporter {

QString toMarkdown(const ChatSession &session);
bool writeMarkdown(const ChatSession &session, const QString &filePath, QString *error = nullptr);

} // namespace ChatSessionExporter
