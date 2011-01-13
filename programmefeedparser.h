#ifndef PROGRAMMEFEEDPARSER_H
#define PROGRAMMEFEEDPARSER_H

#include <QDateTime>
#include <QRegExp>
#include <QXmlStreamReader>
#include "programme.h"

class ProgrammeFeedParser
{
public:
    ProgrammeFeedParser();
    bool parse(QIODevice *device);
    QString lastError() const;
    QList<Programme> programmes() const;

private:
    void parseChannelElement();
    void parseItemElement();
    int parseProgrammeId(const QString &s);
    int parseChannelId(const QString &s);
    QDateTime parseDateTime(const QString &s);
    QXmlStreamReader m_reader;
    QString m_error;
    QList<Programme> m_programmes;
    QRegExp m_dateTimeRegexp;
};

#endif // PROGRAMMEFEEDPARSER_H
