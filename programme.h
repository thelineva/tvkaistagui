#ifndef PROGRAMME_H
#define PROGRAMME_H

#include <QString>
#include <QDateTime>

class Programme
{
public:
    Programme();
    int id;
    QString title;
    QString description;
    QDateTime startDateTime;
    int channelId;
    int flags;
    int duration;
    int seasonPassId;
};

#endif // PROGRAMME_H
