#include <QDebug>
#include <QRegExp>
#include "programmetableparser.h"

ProgrammeTableParser::ProgrammeTableParser() : m_requestedChannelId(-1),
    m_x(0), m_tableDepth(0), m_dayOfWeek(-1), m_validResults(true)
{
    m_programmes = new QList<Programme>[7];
    m_codec = QTextCodec::codecForName("UTF-8");
}

ProgrammeTableParser::~ProgrammeTableParser()
{
    delete [] m_programmes;
}

QDate ProgrammeTableParser::requestedDate() const
{
    return m_requestedDate;
}

void ProgrammeTableParser::setRequestedDate(const QDate &date)
{
    m_requestedDate = date;
    m_firstDay = date.addDays(-3);
}

int ProgrammeTableParser::requestedChannelId() const
{
    return m_requestedChannelId;
}

void ProgrammeTableParser::setRequestedChannelId(int channelId)
{
    m_requestedChannelId = channelId;
}

void ProgrammeTableParser::clear()
{
    m_requestedChannelId = -1;
    m_dayOfWeek = -1;
    m_x = 0;
    m_tableDepth = 0;
    m_validResults = true;

    for (int i = 0; i < 7; i++) {
        m_programmes[i].clear();
    }
}

bool ProgrammeTableParser::isValidResults() const
{
    return m_validResults;
}

QDate ProgrammeTableParser::date(int dayOfWeek) const
{
    Q_ASSERT(dayOfWeek >= 0 && dayOfWeek < 7);
    return m_firstDay.addDays(dayOfWeek);
}

QList<Programme> ProgrammeTableParser::programmes(int dayOfWeek) const
{
    Q_ASSERT(dayOfWeek >= 0 && dayOfWeek < 7);
    return m_programmes[dayOfWeek];
}

QList<Programme> ProgrammeTableParser::requestedProgrammes() const
{
    return m_programmes[3];
}

void ProgrammeTableParser::startElementParsed(const QString &name)
{
    if (m_x == 0 && name == "div" && attribute("id") == "channelboard") {
        m_x = 1;
    }
    else if (m_x == 1 && name == "tr" && attribute("class") == "infobox") {
        m_x = 2;
        m_currentProgramme = Programme();
    }
    else if (m_x == 2 && name == "td" && attribute("class").startsWith("programtime")) {
        m_x = 3;
        m_parseContent = true;
    }
    else if (m_x == 2 && name == "span" && attribute("id").startsWith("pid")) {
        m_x = 4;
        parseProgrammeId(attribute("id"));
        parseFlags();
        m_parseContent = true;
    }
    else if (m_x == 4 && name == "span") {
        parseFlags();
    }
    else if (m_x == 2 && name == "span" && attribute("class") == "information") {
        m_x = 5;
        m_parseContent = true;
    }
    else if (m_x == 0 && name == "div" && attribute("id") == "toolbarcalendar") {
        m_x = 6;
        m_parseContent = true;
    }

    if (m_x > 0 && name == "table") {
        m_tableDepth++;
//        qDebug() << "<table>" << m_tableDepth;

        if (m_tableDepth == 2) {
            m_dayOfWeek = (m_dayOfWeek + 1) % 7;
        }
    }
}

void ProgrammeTableParser::endElementParsed(const QString &name)
{
    if (m_x == 2 && name == "tr") {
        m_x = 1;

        if (m_dayOfWeek >= 0 && m_currentProgramme.startDateTime.isValid() && !m_currentProgramme.title.isEmpty() && m_validResults) {
            m_currentProgramme.channelId = m_requestedChannelId;

            if (m_currentProgramme.description.startsWith("Suosittele:")) {
                m_currentProgramme.description = QString();
            }

//            qDebug() << m_dayOfWeek << m_currentProgramme.id << m_currentProgramme.startDateTime << m_currentProgramme.title;
            m_programmes[m_dayOfWeek].append(m_currentProgramme);
        }
    }
    else if (m_x == 3 && name == "td") {
        m_x = 2;
        m_parseContent = false;
    }
    else if (m_x == 4 && name == "span") {
        m_x = 2;
        m_parseContent = false;
    }
    else if (m_x == 5 && name == "span") {
        m_x = 2;
        m_parseContent = false;
    }
    else if (m_x == 6 && name == "div") {
        m_x = 0;
    }

    if (m_x > 0 && name == "table") {
        m_tableDepth--;
//        qDebug() << "</table>" << m_tableDepth;
    }
}

void ProgrammeTableParser::contentParsed(const QString &content)
{
    if (m_x == 3) {
        QString s = content.trimmed();

        if (!s.isEmpty()) {
            parseTime(s);
        }
    }
    else if (m_x == 4) {
        if (m_currentProgramme.title.isEmpty()) {
            m_currentProgramme.title = content.trimmed();
        }
    }
    else if (m_x == 5) {
        QString s = content.trimmed();

        if (!s.isEmpty() && m_currentProgramme.description.isEmpty()) {
            m_currentProgramme.description = s;
            m_parseContent = false;
        }
    }
    else if (m_x == 6) {
        QString s = content.trimmed();
        QRegExp regex("(\\d{1,2})\\.(\\d{1,2})");
//        qDebug() << s;

        if (regex.indexIn(s) >= 0) {
            int day = regex.cap(1).toInt();
            int month = regex.cap(2).toInt();

            if (day != m_requestedDate.day() || month != m_requestedDate.month()) {
                m_validResults = false;
            }
        }
    }
}

bool ProgrammeTableParser::parseProgrammeId(const QString &s)
{
    /* "pid8217946" -> 8217946 */

    if (!s.startsWith("pid")) {
        return false;
    }

    bool ok;
    int pid = s.mid(3).toInt(&ok);

    if (!ok) {
        return false;
    }

    m_currentProgramme.id = pid;
    return true;
}

bool ProgrammeTableParser::parseTime(const QString &s)
{
    bool ok;
    int pos = s.indexOf('.');

    if (pos < 0) {
        return false;
    }

    int hours = s.mid(0, pos).toInt(&ok);

    if (!ok) {
        return false;
    }

    int minutes = s.mid(pos + 1).toInt(&ok);

    if (!ok) {
        return false;
    }

    QDate date = m_firstDay.addDays(m_dayOfWeek);
    QTime time(hours, minutes);

    if (!m_programmes[m_dayOfWeek].isEmpty()) {
        Programme prevProgramme = m_programmes[m_dayOfWeek].last();
        date = prevProgramme.startDateTime.date();

        if (time < prevProgramme.startDateTime.time()) {
            date = date.addDays(1);
        }
    }

    m_currentProgramme.startDateTime = QDateTime(date, time);
    return true;
}

void ProgrammeTableParser::parseFlags()
{
    QString clazz = attribute("class");
    if (clazz.contains("upcoming")) m_currentProgramme.flags |= 0xF;
    if (clazz.contains("nof0")) m_currentProgramme.flags |= 0x01;
    if (clazz.contains("nof1")) m_currentProgramme.flags |= 0x02;
    if (clazz.contains("nof2")) m_currentProgramme.flags |= 0x04;
    if (clazz.contains("nof3")) m_currentProgramme.flags |= 0x08;
}
