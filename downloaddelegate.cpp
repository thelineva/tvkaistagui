#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QStyle>
#include "downloaddelegate.h"

int DownloadDelegate::PADDING = 10;

DownloadDelegate::DownloadDelegate(QWidget *parent) :
        QStyledItemDelegate(parent), m_parentWidget(parent),
        m_pixmap(":/images/unfinished-16x16.png")
{
}

void DownloadDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QFont titleFont(option.font);
    titleFont.setBold(true);
    painter->save();

    if ((option.state & QStyle::State_Selected) > 0) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    painter->translate(option.rect.x() + PADDING, option.rect.y() + PADDING);
    painter->setFont(titleFont);
    QRect bounding;
    painter->setClipping(false);
    painter->drawText(QRect(0, 0, option.rect.width() - PADDING, INT_MAX),
                      Qt::AlignLeft | Qt::TextDontClip, index.data().toString(), &bounding);

    painter->setFont(option.font);
    painter->drawText(QRect(bounding.right() + 10, 0, option.rect.width() - PADDING, INT_MAX),
                      Qt::AlignLeft | Qt::TextDontClip, index.data(Qt::UserRole + 5).toString(), &bounding);

    painter->setPen(painter->pen().color().lighter(200));
    int y = bounding.bottom() + 3;

    painter->drawText(QRect(0, y, option.rect.width() - PADDING, INT_MAX),
                      Qt::AlignLeft | Qt::TextDontClip, index.data(Qt::UserRole + 2).toString(), &bounding);

    int status = index.data(Qt::UserRole + 1).toInt();

    if (status == 0) {
        painter->drawText(QRect(0, bounding.bottom() + 3, option.rect.width() - PADDING, INT_MAX),
                      Qt::AlignLeft | Qt::TextDontClip, index.data(Qt::UserRole + 3).toString(), &bounding);
    }
    else if (status == 2) {
        int x = option.rect.width() -  2 * PADDING - m_pixmap.width();

        if (x > bounding.right()) {
            painter->drawPixmap(x, y + (bounding.height() - 16) / 2, m_pixmap);
        }
    }

    if ((option.state & QStyle::State_Selected) == 0) {
        painter->setPen(painter->pen().color().lighter(160));
        painter->drawLine(-PADDING, bounding.bottom() + 10, option.rect.width() - PADDING, bounding.bottom() + 10);
    }

    painter->restore();
}

QSize DownloadDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    /* Kolmerivinen lataamisen aikana ja näytettäessä virheilmoitus */
    QFont titleFont(option.font);
    titleFont.setBold(true);
    QFontMetrics metrics(titleFont, 0);
    int width = metrics.width(index.data().toString()) + 2 * PADDING;
    width += option.fontMetrics.width(index.data(Qt::UserRole + 5).toString()) + 10;
    width = qMax(width, option.fontMetrics.width(index.data(Qt::UserRole + 1).toString()) + 2 * PADDING);
    int status = index.data(Qt::UserRole + 1).toInt();
    int lineCount = (status == 0 || status == 3) ? 3 : 2;
    return QSize(width, option.fontMetrics.height() * lineCount + 23);
}
