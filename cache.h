#ifndef CACHE_H
#define CACHE_H

#include <QDir>
#include <QImage>
#include <QList>
#include "channel.h"
#include "programme.h"

class Cache
{
public:
    Cache();
    void setDirectory(const QDir &dir);
    QDir directory() const;
    QString lastError() const;
    QList<Channel> loadChannels(bool &ok);
    bool saveChannels(const QList<Channel> &channels);
    QList<Programme> loadProgrammes(int channelId, const QDate &date, bool &ok, int &age);
    bool saveProgrammes(int channelId, const QDate &date, const QDateTime &updateDateTime,
                        const QDateTime &expireDateTime, const QList<Programme> programmes);
    QList<Programme> loadSeasonPasses(bool &ok, int &age);
    bool saveSeasonPasses(const QDateTime &updateDateTime, const QList<Programme> programmes);
    QImage loadPoster(const Programme &programme);
    bool savePoster(const Programme &programme, const QByteArray &data);

private:
    QString buildChannelsXmlFilename() const;
    QString buildProgrammesXmlFilename(int channelId, const QDate &date) const;
    QString buildSeasonPassesXmlFilename() const;
    QString buildPosterFilename(const Programme &programme) const;
    QList<Programme> readProgrammeFeed(QIODevice *device, int channelId, bool &ok, int &age);
    void writeProgrammeFeed(QIODevice *device, const QDateTime &updateDateTime,
                            const QDateTime &expireDateTime, const QList<Programme> programmes);
    QDir m_dir;
    QString m_lastError;
};

#endif // CACHE_H
