#include <QDebug>
#include "channelfeedparser.h"

bool ChannelFeedParser::parse(QIODevice *device)
{
    m_reader.setDevice(device);
    m_channels.clear();

    if (!m_reader.readNextStartElement()) {
        m_error = "Invalid channel feed";
        return false;
    }

    if (m_reader.name() != "rss") {
        m_error = "Channel feed does not contain rss element";
        return false;
    }

    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "channel") {
            parseChannelElement();
        }
        else {
            m_reader.skipCurrentElement();
        }
    }

    return true;
}

QString ChannelFeedParser::lastError() const
{
    return m_error;
}

QList<Channel> ChannelFeedParser::channels() const
{
    return m_channels;
}

void ChannelFeedParser::parseChannelElement()
{
    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "item") {
            parseItemElement();
        }
        else {
            m_reader.skipCurrentElement();
        }
    }
}

void ChannelFeedParser::parseItemElement()
{
    int channelId = -1;
    QString name;

    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "title") {
            name = m_reader.readElementText();
        }
        else if (m_reader.name() == "link") {
            channelId = parseChannelId(m_reader.readElementText());
        }
        else {
            m_reader.skipCurrentElement();
        }
    }

    if (channelId >= 0) {
        m_channels.append(Channel(channelId, name));
    }
}

int ChannelFeedParser::parseChannelId(const QString &s)
{
    /* "http://www.tvkaista.fi/feed/channels/1004" -> 1004 */
    int pos = s.lastIndexOf('/');

    if (pos < 0) {
        return -1;
    }

    bool ok;
    int channelId = s.mid(pos + 1).toInt(&ok);

    if (!ok) {
        return -1;
    }

    return channelId;
}
