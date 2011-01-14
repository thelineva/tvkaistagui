#include "thumbnail.h"

Thumbnail::Thumbnail()
{
}

Thumbnail::Thumbnail(const QUrl &u, const QTime &t)
{
    url = u;
    time = t;
}
