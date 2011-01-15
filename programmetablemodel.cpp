#include <QDebug>
#include "programmetablemodel.h"

ProgrammeTableModel::ProgrammeTableModel(bool detailsVisible, QObject *parent) :
    QAbstractTableModel(parent), m_detailsVisible(detailsVisible),
    m_format(3), m_flagMask(0x08), m_sortKey(0), m_descending(false)
{
}

int ProgrammeTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_infoText.isEmpty() ? m_programmes.size() : 1;
}

int ProgrammeTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_detailsVisible ? 3 : 2;
}

QVariant ProgrammeTableModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    if (!m_infoText.isEmpty() && role == Qt::DisplayRole && row == 0) {
        if ((m_detailsVisible && index.column() == 2) || (!m_detailsVisible && index.column() == 1)) {
            return m_infoText;
        }
    }

    if (row < 0 || row >= m_programmes.size()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        Programme programme = m_programmes.value(row);

        if (m_detailsVisible) {
            switch (index.column()) {
            case 0:
                return programme.startDateTime.toString(tr("ddd dd.MM.yyyy "));

            case 1:
                return programme.startDateTime.toString(tr("h.mm "));

            case 2:
                return programme.title;

            default:
                return QVariant();
            }
        }
        else {
            switch (index.column()) {
            case 0:
                return programme.startDateTime.toString(tr("h.mm"));

            case 1:
                return programme.title;

            default:
                return QVariant();
            }
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        if (m_detailsVisible && index.column() < 2) {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        else {
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }
    else if (role == Qt::ForegroundRole) {
        Programme programme = m_programmes.value(row);

        if ((programme.flags & m_flagMask) > 0) {
            return Qt::darkGray;
        }
    }

    return QVariant();
}

QVariant ProgrammeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

Qt::ItemFlags ProgrammeTableModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void ProgrammeTableModel::setFormat(int format)
{
    m_format = format;

    if (format == 0) {
        m_flagMask = 0x01;
    }
    else if (format == 1) {
        m_flagMask = 0x02;
    }
    else if (format == 2) {
        m_flagMask = 0x04;
    }
    else {
        m_flagMask = 0x08;
    }

    emit dataChanged(index(0, 0, QModelIndex()), index(0, columnCount(QModelIndex()) - 1, QModelIndex()));
}

int ProgrammeTableModel::format() const
{
    return m_format;
}

void ProgrammeTableModel::setSortKey(int key, bool descending)
{
    m_sortKey = key;
    m_descending = descending;

    if (!m_programmes.isEmpty()) {
        QList<Programme> copy = m_programmes;
        setProgrammes(copy);
    }
}

int ProgrammeTableModel::sortKey() const
{
    return m_sortKey;
}

bool ProgrammeTableModel::isDescending() const
{
    return m_descending;
}

void ProgrammeTableModel::setProgrammes(const QList<Programme> &programmes)
{
    setInfoText(QString());

    if (!m_programmes.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_programmes.size() - 1);
        m_programmes.clear();
        endRemoveRows();
    }

    if (!programmes.isEmpty()) {
        beginInsertRows(QModelIndex(), 0, programmes.size() - 1);
        QList<Programme> tmp;
        int count = programmes.size();

        if (m_sortKey == 1) {
            QMap<QPair<QDateTime, QString>, Programme> sortMap;

            for (int i = 0; i < count; i++) {
                Programme programme = programmes.at(i);
                sortMap.insertMulti(QPair<QDateTime, QString>(programme.startDateTime, programme.title), programme);
            }

            tmp = sortMap.values();
        }
        else if (m_sortKey == 2) {
            QMap<QPair<QString, QDateTime>, Programme> sortMap;

            for (int i = 0; i < count; i++) {
                Programme programme = programmes.at(i);
                sortMap.insertMulti(QPair<QString, QDateTime>(programme.title, programme.startDateTime), programme);
            }

            tmp = sortMap.values();
        }
        else {
            tmp = programmes;
        }

        if (m_descending) {
            for (int i = count - 1; i >= 0; i--) {
                m_programmes.append(tmp.at(i));
            }
        }
        else {
            m_programmes = tmp;
        }

        endInsertRows();
    }
}

int ProgrammeTableModel::programmeCount() const
{
    return m_programmes.size();
}

void ProgrammeTableModel::setInfoText(const QString &text)
{
    if (text.isEmpty() && !m_infoText.isEmpty()) { /* Jos teksti poistettu */
        beginRemoveRows(QModelIndex(), 0, 0);
        m_infoText = text;
        endRemoveRows();
    }
    else if (!text.isEmpty() && m_infoText.isEmpty()) { /* Jos teksti lisätty */
        setProgrammes(QList<Programme>());
        beginInsertRows(QModelIndex(), 0, 0);
        m_infoText = text;
        endInsertRows();
    }
    else if (!text.isEmpty()) { /* Jos teksti vain muuttunut */
        QModelIndex modelIndex = index(0, 1, QModelIndex());
        emit dataChanged(modelIndex, modelIndex);
    }
}

QString ProgrammeTableModel::infoText() const
{
    return m_infoText;
}

Programme ProgrammeTableModel::programme(int index) const
{
    return m_programmes.value(index);
}

int ProgrammeTableModel::defaultProgrammeIndex() const
{
    int count = m_programmes.size();
    int index = -1;

    for (int i = 0; i < count; i++) {
        if ((m_programmes.at(i).flags & 0x08) > 0) {
            continue;
        }

        index = i;

        if (m_programmes.at(i).startDateTime.time().hour() >= 18) {
            return i;
        }
    }

    return index;
}
