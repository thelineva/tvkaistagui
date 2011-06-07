#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QStyle>
#include "downloaddelegate.h"

int DownloadDelegate::PADDING = 10;

DownloadDelegate::DownloadDelegate(QWidget *parent) :
        QStyledItemDelegate(parent), m_parentWidget(parent),
        m_pixmap(":/images/unfinished-16x16.png"),
        m_color1(220, 239, 255), m_color2(247, 255, 240),
        m_color3(255, 240, 240), m_color4(230, 230, 230)
{
}

void DownloadDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QFont titleFont(option.font);
    titleFont.setBold(true);
    painter->save();

    int status = index.data(Qt::UserRole + 1).toInt();

    if ((option.state & QStyle::State_Selected) > 0) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(QPen(option.palette.highlightedText(), 1.0));
    }
    else {
        if (status == 1) {
            painter->fillRect(option.rect, m_color1);
        }
        else if (status == 2) {
            painter->fillRect(option.rect, m_color2);
        }
        else if (status == 3) {
            painter->fillRect(option.rect, m_color3);
        }
        else if (status == 4) {
            painter->fillRect(option.rect, m_color4);
        }

        painter->setPen(painter->pen().color().lighter(160));
        painter->drawLine(option.rect.x(), option.rect.y() + option.rect.height() - 1,
                          option.rect.width(), option.rect.y() + option.rect.height() - 1);
        painter->setPen(QPen(option.palette.text(), 1.0));
    }

    if (status == 0) {
        double progress = index.data(Qt::UserRole + 6).toDouble();
        int ry = option.rect.y();
        int width = qRound(option.rect.width() * progress);
        int height = option.rect.height() - 1;

        if ((option.state & QStyle::State_Selected) > 0) {
            ry += height - 5;
            height = 5;
        }

        painter->fillRect(option.rect.x(), ry, width, height, m_color1);
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

    if (status == 0 || status == 3) {
        painter->drawText(QRect(0, bounding.bottom() + 3, option.rect.width() - PADDING, INT_MAX),
                      Qt::AlignLeft | Qt::TextDontClip, index.data(Qt::UserRole + 3).toString(), &bounding);
    }
    else if (status == 2) {
        int x = option.rect.width() -  2 * PADDING - m_pixmap.width();

        if (x > bounding.right()) {
            painter->drawPixmap(x, y + (bounding.height() - 16) / 2, m_pixmap);
        }
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
    return QSize(width, (option.fontMetrics.height() + 3) * lineCount + 2 * PADDING);
}
