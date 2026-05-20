#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

struct StreamParseResult {
    QVector<QString> textDeltas;
    bool done = false;
};

class StreamParser
{
public:
    StreamParseResult consume(const QByteArray &data);
    StreamParseResult finish();
    void reset();

private:
    void parseLine(const QByteArray &line, StreamParseResult &result);

    QByteArray m_buffer;
};
