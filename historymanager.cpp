#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include "historymanager.h"

HistoryManager::HistoryManager(QSettings *settings) : m_settings(settings)
{
}

bool HistoryManager::load()
{
    m_entries.clear();
    m_programmeSet.clear();
    QString dirPath = QFileInfo(m_settings->fileName()).path();
    QString filename = QString("%1/history.xml").arg(dirPath);
    QFile file(filename);

    if (!file.exists()) {
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << file.errorString();
        return false;
    }

    qDebug() << "READ" << filename;
    QXmlStreamReader reader(&file);

    if (!reader.readNextStartElement() || reader.name() != "history") {
        file.close();
        return false;
    }

    while (reader.readNextStartElement()) {
        if (reader.name() != "programme") {
            reader.skipCurrentElement();
        }

        QXmlStreamAttributes attrs = reader.attributes();
        HistoryEntry entry;
        bool ok;
        entry.programmeId = attrs.value("id").toString().toInt(&ok);

        if (!ok) {
            continue;
        }

        entry.dateTime = QDateTime::fromString(attrs.value("dateTime").toString(),
                                               "yyyy-MM-dd'T'hh:mm:ss");
        m_entries.append(entry);
        m_programmeSet.insert(entry.programmeId);
        reader.skipCurrentElement();
    }

    return true;
}

bool HistoryManager::save()
{
    QString dirPath = QFileInfo(m_settings->fileName()).path();
    QString filename = QString("%1/history.xml").arg(dirPath);
    QDir dir(dirPath);

    if (!dir.exists()) {
        dir.mkpath(dirPath);
    }

    qDebug() << "WRITE" << filename;
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << file.errorString();
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("history");
    int count = m_entries.size();

    for (int i = 0; i < count; i++) {
        HistoryEntry entry = m_entries.at(i);
        writer.writeStartElement("programme");
        writer.writeAttribute("id", QString::number(entry.programmeId));
        writer.writeAttribute("dateTime", entry.dateTime.toString("yyyy-MM-dd'T'hh:mm:ss"));
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();
    return true;
}

void HistoryManager::addEntry(int programmeId)
{
    if (m_programmeSet.contains(programmeId)) {
        return;
    }

    HistoryEntry entry;
    entry.programmeId = programmeId;
    entry.dateTime = QDateTime::currentDateTime();
    m_entries.append(entry);
    m_programmeSet.insert(programmeId);
}

void HistoryManager::removeEntry(int programmeId)
{
    int count = m_entries.size();

    for (int i = 0; i < count; i++) {
        HistoryEntry entry = m_entries.at(i);

        if (entry.programmeId == programmeId) {
            m_entries.removeAt(i);
            count--;
            i--;
        }
    }

    m_programmeSet.remove(programmeId);
}

void HistoryManager::clear()
{
    m_entries.clear();
    m_programmeSet.clear();
}

bool HistoryManager::containsProgramme(int programmeId) const
{
    return m_programmeSet.contains(programmeId);
}
