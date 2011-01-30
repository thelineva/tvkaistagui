#include <QDebug>
#include <QXmlStreamWriter>
#include "cache.h"

Cache::Cache()
{
}

void Cache::setDirectory(const QDir &dir)
{
    m_dir = dir;
}

QDir Cache::directory() const
{
    return m_dir;
}

QString Cache::lastError() const
{
    return m_lastError;
}

QList<Channel> Cache::loadChannels(bool &ok)
{
    QList<Channel> channels;
    QString filename = buildChannelsXmlFilename();
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        ok = false;
        return channels;
    }

    qDebug() << "READ" << filename;
    QXmlStreamReader reader(&file);

    if (!reader.readNextStartElement() || reader.name() != "channels") {
        ok = false;
        file.close();
        return channels;
    }

    while (reader.readNextStartElement()) {
        if (reader.name() != "channel") {
            reader.skipCurrentElement();
        }

        QXmlStreamAttributes attrs = reader.attributes();
        Channel channel;
        channel.id = attrs.value("id").toString().toInt(&ok);

        if (!ok) {
            continue;
        }

        channel.name = attrs.value("name").toString();
        channels.append(channel);
        reader.skipCurrentElement();
    }

    ok = true;
    return channels;
}

bool Cache::saveChannels(const QList<Channel> &channels)
{
    QString filename = buildChannelsXmlFilename();
    QDir dir(QFileInfo(filename).absolutePath());

    if (!dir.exists()) {
        dir.mkpath(dir.path());
    }

    qDebug() << "WRITE" << filename;
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = file.errorString();
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("channels");
    int count = channels.size();

    for (int i = 0; i < count; i++) {
        Channel channel = channels.at(i);
        writer.writeStartElement("channel");
        writer.writeAttribute("id", QString::number(channel.id));
        writer.writeAttribute("name", channel.name);
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();
    return true;
}

QList<Programme> Cache::loadProgrammes(int channelId, const QDate &date, bool &ok, int &age)
{
    QList<Programme> programmes;
    QString filename = buildProgrammesXmlFilename(channelId, date);
    QFile file(filename);
    age = INT_MAX;

    if (!file.open(QIODevice::ReadOnly)) {
        ok = false;
        return programmes;
    }

    qDebug() << "READ" << filename;
    programmes = readProgrammeFeed(&file, channelId, ok, age);
    file.close();
    return programmes;
}

bool Cache::saveProgrammes(int channelId, const QDate &date, const QDateTime &updateDateTime,
                           const QDateTime &expireDateTime, const QList<Programme> programmes)
{
    QString filename = buildProgrammesXmlFilename(channelId, date);
    QDir dir(QFileInfo(filename).absolutePath());

    if (!dir.exists()) {
        dir.mkpath(dir.path());
    }

    qDebug() << "WRITE" << filename;
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = file.errorString();
        return false;
    }

    writeProgrammeFeed(&file, updateDateTime, expireDateTime, programmes);
    file.close();
    return true;
}

QList<Programme> Cache::loadSeasonPasses(bool &ok, int &age)
{
    QList<Programme> programmes;
    QString filename = buildSeasonPassesXmlFilename();
    QFile file(filename);
    age = INT_MAX;

    if (!file.open(QIODevice::ReadOnly)) {
        ok = false;
        return programmes;
    }

    qDebug() << "READ" << filename;
    programmes = readProgrammeFeed(&file, -1, ok, age);
    file.close();
    return programmes;
}

bool Cache::saveSeasonPasses(const QDateTime &updateDateTime, QList<Programme>programmes)
{
    QString filename = buildSeasonPassesXmlFilename();
    QDir dir(QFileInfo(filename).absolutePath());

    if (!dir.exists()) {
        dir.mkpath(dir.path());
    }

    qDebug() << "WRITE" << filename;
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = file.errorString();
        return false;
    }

    writeProgrammeFeed(&file, updateDateTime, QDateTime(), programmes);
    file.close();
    return true;
}

QImage Cache::loadPoster(const Programme &programme)
{
    QString filename = buildPosterFilename(programme);

    if (!QFileInfo(filename).exists()) {
        return QImage();
    }

    qDebug() << "READ" << filename;
    return QImage(filename);
}

bool Cache::savePoster(const Programme &programme, const QByteArray &data)
{
    QString filename = buildPosterFilename(programme);
    QDir dir(QFileInfo(filename).absolutePath());

    if (!dir.exists()) {
        dir.mkpath(dir.path());
    }

    qDebug() << "WRITE" << filename;
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(data);
    return true;
}

QString Cache::buildChannelsXmlFilename() const
{
    return m_dir.filePath("channels.xml");
}

QString Cache::buildProgrammesXmlFilename(int channelId, const QDate &date) const
{
    QString path = QString("%1/%2/p%2-%3.xml").arg(
            date.toString("yyyy-MM")).arg(channelId).arg(date.toString("yyyy-MM-dd"));

    return m_dir.filePath(path);
}

QString Cache::buildSeasonPassesXmlFilename() const
{
    return m_dir.filePath("season-passes.xml");
}

QString Cache::buildPosterFilename(const Programme &programme) const
{
    QString path = QString("%1/%2/i%3.jpg").arg(
            programme.startDateTime.toString("yyyy-MM")).arg(programme.channelId).arg(programme.id);

    return m_dir.filePath(path);
}

QList<Programme> Cache::readProgrammeFeed(QIODevice *device, int channelId, bool &ok, int &age)
{
    QList<Programme> programmes;
    QXmlStreamReader reader(device);

    if (!reader.readNextStartElement() || reader.name() != "programmes") {
        ok = false;
        return programmes;
    }

    QXmlStreamAttributes attrs = reader.attributes();
    QString expireDateTimeString = attrs.value("expireDateTime").toString();

    if (!expireDateTimeString.isEmpty()) {
        QDateTime expireDateTime = QDateTime::fromString(expireDateTimeString, "yyyy-MM-dd'T'hh:mm:ss");

        if (expireDateTime < QDateTime::currentDateTime()) {
            ok = false;
            return programmes;
        }
    }

    QString updateDateTimeString = attrs.value("updateDateTime").toString();

    if (!updateDateTimeString.isEmpty()) {
        QDateTime updateDateTime = QDateTime::fromString(updateDateTimeString, "yyyy-MM-dd'T'hh:mm:ss");
        age = updateDateTime.secsTo(QDateTime::currentDateTime());
    }

    while (reader.readNextStartElement()) {
        if (reader.name() != "programme") {
            reader.skipCurrentElement();
        }

        attrs = reader.attributes();
        Programme programme;
        bool intOk;
        int id = attrs.value("id").toString().toInt(&intOk);

        if (intOk) {
            programme.id = id;
        }

        programme.startDateTime = QDateTime::fromString(attrs.value("dateTime").toString(),
                                                        "yyyy-MM-dd'T'hh:mm:ss");
        programme.flags = attrs.value("flags").toString().toInt();
        QString channelString = attrs.value("channel").toString();

        if (!channelString.isEmpty()) {
            int programmeChannelId = channelString.toInt(&intOk);

            if (intOk) {
                programme.channelId = programmeChannelId;
            }
        }

        if (programme.channelId < 0) {
            programme.channelId = channelId;
        }

        while (reader.readNextStartElement()) {
            if (reader.name() == "title") {
                programme.title = reader.readElementText();
            }
            else if (reader.name() == "description") {
                programme.description = reader.readElementText();
            }
        }

        programmes.append(programme);
    }

    ok = true;
    return programmes;
}

void Cache::writeProgrammeFeed(QIODevice *device, const QDateTime &updateDateTime,
                               const QDateTime &expireDateTime, QList<Programme>programmes)
{
    QXmlStreamWriter writer(device);
    writer.writeStartDocument();
    writer.writeStartElement("programmes");
    writer.writeAttribute("updateDateTime", updateDateTime.toString("yyyy-MM-dd'T'hh:mm:ss"));

    if (!expireDateTime.isNull()) {
        writer.writeAttribute("expireDateTime", expireDateTime.toString("yyyy-MM-dd'T'hh:mm:ss"));
    }

    int count = programmes.size();

    for (int i = 0; i < count; i++) {
        Programme programme = programmes.at(i);
        writer.writeStartElement("programme");

        if (programme.id >= 0) {
            writer.writeAttribute("id", QString::number(programme.id));
        }

        writer.writeAttribute("dateTime", programme.startDateTime.toString("yyyy-MM-dd'T'hh:mm:ss"));

        if (programme.flags > 0) {
            writer.writeAttribute("flags", QString::number(programme.flags));
        }

        if (programme.channelId >= 0) {
            writer.writeAttribute("channel", QString::number(programme.channelId));
        }

        writer.writeTextElement("title", programme.title);
        writer.writeTextElement("description", programme.description);
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();
}
