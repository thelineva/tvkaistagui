#ifndef HISTORYENTRY_H
#define HISTORYENTRY_H

#include <QDateTime>

class HistoryEntry
{
public:
    HistoryEntry();
    int programmeId;
    QDateTime dateTime;
};

#endif // HISTORYENTRY_H
