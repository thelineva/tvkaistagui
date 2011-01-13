#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QStyle>
#include "downloaddelegate.h"

int DownloadDelegate::PADDING = 10;

DownloadDelegate::DownloadDelegate(QWidget *parent) :
    QStyledItemDelegate(parent), m_parentWidget(parent)
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
    painter->drawText(QRect(bounding.right() + 10, 0, option.rect.width() - bounding.right() - 10, INT_MAX),
                      Qt::AlignLeft | Qt::TextDontClip, index.data(Qt::UserRole + 1).toString(), &bounding);

    painter->setPen(painter->pen().color().lighter(200));
    painter->drawText(QRect(0, bounding.bottom() + 3, option.rect.width() - PADDING, INT_MAX),
                      Qt::AlignLeft | Qt::TextDontClip, index.data(Qt::UserRole + 2).toString(), &bounding);

    if ((option.state & QStyle::State_Selected) == 0) {
        painter->setPen(painter->pen().color().lighter(160));
        painter->drawLine(-PADDING, bounding.bottom() + 10, option.rect.width(), bounding.bottom() + 10);
    }

    painter->restore();
}

QSize DownloadDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect bounding1 = option.fontMetrics.boundingRect(QRect(option.rect.x() + PADDING, PADDING, option.rect.width() - PADDING, INT_MAX),
                                                      Qt::AlignLeft, index.data().toString());
    QRect bounding2 = option.fontMetrics.boundingRect(QRect(bounding1.right() + 10, PADDING, option.rect.width() - bounding1.right() - 10, INT_MAX),
                                                      Qt::AlignLeft, index.data(Qt::UserRole + 1).toString());
    bounding1 = bounding1 | bounding2;
    bounding2 = option.fontMetrics.boundingRect(QRect(option.rect.x() + PADDING, bounding1.bottom() + 3, option.rect.width() - PADDING, INT_MAX),
                                                      Qt::AlignLeft, index.data(Qt::UserRole + 2).toString());
    bounding1 = bounding1 | bounding2;
    return QSize(bounding1.width() + 2 * PADDING, bounding1.height() + PADDING);
}
