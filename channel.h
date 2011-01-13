#ifndef CHANNEL_H
#define CHANNEL_H

#include <QString>

class Channel
{
public:
    Channel();
    Channel(int id, const QString &name);
    int id;
    QString name;
};

#endif // CHANNEL_H
