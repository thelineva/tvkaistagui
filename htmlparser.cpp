#include <QDebug>
#include <QIODevice>
#include "htmlparser.h"

HtmlParser::HtmlParser() : m_parseContent(false),
    m_codec(QTextCodec::codecForLocale()), m_buf(new char[4096]), m_x(0)
{
}

HtmlParser::~HtmlParser()
{
    delete m_buf;
}

bool HtmlParser::parse(QIODevice *device)
{
    m_len = device->read(m_buf, 4096);

    while (m_len > 0) {
        for (int i = 0; i < m_len; i++) {
            char c = m_buf[i];

            if (c == '<') {
                if (m_x == 0 && m_parseContent) {
                    contentParsed(m_codec->toUnicode(m_s));
                }

                m_x = 1;
                m_s.clear();
                m_a.clear();
                m_attrsParsed = false;
                m_attrs.clear();
            }
            else if (c == '>') {
                if (m_x > 0 && !m_s.isEmpty()) {
                    if (m_s.startsWith("/")) {
                        m_s.remove(0, 1);
                        endElementParsed(m_codec->toUnicode(m_s));
                    }
                    else {
                        startElementParsed(m_codec->toUnicode(m_s));
                    }
                }

                m_x = 0;
                m_s.clear();
                m_a.clear();
            }
            else if (m_x == 1) {
                if (c == ' ') {
                    m_x = 2;
                }
                else {
                    m_s.append(c);
                }
            }
            else if (m_x == 2) {
                m_a.append(c);
            }
            else if (m_x == 0 && m_parseContent) {
                m_s.append(c);
            }
        }

        m_len = device->read(m_buf, 4096);
    }

    return true;
}

void HtmlParser::startElementParsed(const QString&)
{
}

void HtmlParser::endElementParsed(const QString&)
{
}

void HtmlParser::contentParsed(const QString&)
{
}

QString HtmlParser::attribute(const QString &name)
{
    if (!m_attrsParsed) {
        parseAttributes();
    }

    return m_attrs.value(name);
}

void HtmlParser::parseAttributes()
{
    QByteArray name;
    QByteArray value;
    bool quotes = false;
    bool y = false;
    int len = m_a.length();

    for (int i = 0; i < len; i++) {
        char c = m_a.at(i);

        if (c == '"') {
            quotes = !quotes;
        }
        else if (y) {
            if (!quotes && c == ' ') {
                m_attrs.insert(m_codec->toUnicode(name), m_codec->toUnicode(value));
                name.clear();
                value.clear();
                y = false;
            }
            else {
                value.append(c);
            }
        }
        else {
            if (c == '=') {
                y = true;
            }
            else {
                name.append(c);
            }
        }
    }

    if (!value.isEmpty()) {
        m_attrs.insert(m_codec->toUnicode(name), m_codec->toUnicode(value));
    }

    m_attrsParsed = true;
}
