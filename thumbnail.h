#ifndef THUMBNAIL_H
#define THUMBNAIL_H

#include <QUrl>
#include <QTime>

class Thumbnail
{
public:
    Thumbnail();
    Thumbnail(const QUrl &u, const QTime &t);
    QUrl url;
    QTime time;
};

#endif // THUMBNAIL_H
