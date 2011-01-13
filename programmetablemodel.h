#ifndef PROGRAMMETABLEMODEL_H
#define PROGRAMMETABLEMODEL_H

#include <QAbstractTableModel>
#include "programme.h"

class ProgrammeTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ProgrammeTableModel(QObject *parent = 0);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void setDetailsVisible(bool detailsVisible);
    bool isDetailsVisible() const;
    void setFormat(int format);
    int format() const;
    void setProgrammes(const QList<Programme> &programmes);
    void setInfoText(const QString &text);
    QString infoText() const;
    Programme programme(int index) const;
    int defaultProgrammeIndex() const;

private:
    QList<Programme> m_programmes;
    QString m_infoText;
    bool m_detailsVisible;
    int m_format;
    int m_flagMask;
};

#endif // PROGRAMMETABLEMODEL_H
