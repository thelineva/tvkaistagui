#ifndef PROGRAMMETABLEPARSER_H
#define PROGRAMMETABLEPARSER_H

#include <QList>
#include "htmlparser.h"
#include "programme.h"

class QIODevice;

class ProgrammeTableParser : public HtmlParser
{
public:
    ProgrammeTableParser();
    ~ProgrammeTableParser();
    QDate requestedDate() const;
    void setRequestedDate(const QDate &date);
    int requestedChannelId() const;
    void setRequestedChannelId(int channelId);
    void clear();
    bool isValidResults() const;
    bool isLoginForm() const;
    QDate date(int dayOfWeek) const;
    QList<Programme> programmes(int dayOfWeek) const;
    QList<Programme> requestedProgrammes() const;

protected:
    void startElementParsed(const QString &name);
    void endElementParsed(const QString &name);
    void contentParsed(const QString &content);

private:
    bool parseProgrammeId(const QString &s);
    bool parseTime(const QString &s);
    QList<Programme> *m_programmes;
    QDate m_firstDay;
    QDate m_requestedDate;
    int m_requestedChannelId;
    Programme m_currentProgramme;
    int m_x;
    int m_tableDepth;
    int m_dayOfWeek;
    bool m_validResults;
};

#endif // PROGRAMMETABLEPARSER_H
