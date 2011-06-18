#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QList>
#include <QSet>
#include "historyentry.h"

class QSettings;

class HistoryManager
{
public:
    HistoryManager(QSettings *settings);
    bool load();
    bool save();
    void addEntry(int programmeId);
    void removeEntry(int programmeId);
    void clear();
    bool containsProgramme(int programmeId) const;

private:
    QSettings *m_settings;
    QList<HistoryEntry> m_entries;
    QSet<int> m_programmeSet;
};

#endif // HISTORYMANAGER_H
