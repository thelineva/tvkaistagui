#ifndef DOWNLOADDELEGATE_H
#define DOWNLOADDELEGATE_H

#include <QStyledItemDelegate>

class DownloadDelegate : public QStyledItemDelegate
{
Q_OBJECT
public:
    DownloadDelegate(QWidget *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;

private:
    QWidget *m_parentWidget;
    QPixmap m_pixmap;
    QColor m_color1;
    QColor m_color2;
    QColor m_color3;
    QColor m_color4;
    static int PADDING;
};

#endif // DOWNLOADDELEGATE_H
