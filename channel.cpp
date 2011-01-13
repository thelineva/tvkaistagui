#include "channel.h"

Channel::Channel()
{
    id = -1;
}

Channel::Channel(int cid, const QString &cname)
{
    id = cid;
    name = cname;
}
