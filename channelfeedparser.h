#ifndef CHANNELFEEDPARSER_H
#define CHANNELFEEDPARSER_H

#include <QXmlStreamReader>
#include "channel.h"

class QIODevice;

class ChannelFeedParser
{
public:
    bool parse(QIODevice *device);
    QString lastError() const;
    QList<Channel> channels() const;

private:
    void parseChannelElement();
    void parseItemElement();
    int parseChannelId(const QString &s);
    QXmlStreamReader m_reader;
    QString m_error;
    QList<Channel> m_channels;
};

#endif // CHANNELFEEDPARSER_H
