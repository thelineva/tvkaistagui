#ifndef PROGRAMMEFEEDPARSER_H
#define PROGRAMMEFEEDPARSER_H

#include <QDateTime>
#include <QPair>
#include <QRegExp>
#include <QUrl>
#include <QXmlStreamReader>
#include "programme.h"
#include "thumbnail.h"

class ProgrammeFeedParser
{
public:
    ProgrammeFeedParser();
    bool parse(QIODevice *device);
    QString lastError() const;
    QList<Programme> programmes() const;
    QList<Thumbnail> thumbnails() const;

private:
    void parseChannelElement();
    void parseItemElement();
    void parseMediaGroupElement();
    int parseProgrammeId(const QString &s);
    int parseChannelId(const QString &s);
    QDateTime parseDateTime(const QString &s);
    QTime parseTime(const QString &s);
    QXmlStreamReader m_reader;
    QString m_error;
    QList<Programme> m_programmes;
    QList<Thumbnail> m_thumbnails;
    QRegExp m_dateTimeRegexp;
    QRegExp m_timeRegexp;
};

#endif // PROGRAMMEFEEDPARSER_H
