#ifndef HTMLPARSER_H
#define HTMLPARSER_H

#include <QMap>
#include <QString>
#include <QTextCodec>

class QIODevice;

class HtmlParser
{
public:
    HtmlParser();
    virtual ~HtmlParser();
    bool parse(QIODevice *device);

protected:
    virtual void startElementParsed(const QString &name);
    virtual void endElementParsed(const QString &name);
    virtual void contentParsed(const QString &content);
    QString attribute(const QString &name);
    bool m_parseContent;
    QTextCodec *m_codec;

private:
    void parseAttributes();
    char *m_buf;
    QByteArray m_s;
    QByteArray m_a;
    int m_len;
    int m_x;
    QMap<QString, QString> m_attrs;
    bool m_attrsParsed;
};

#endif // HTMLPARSER_H
