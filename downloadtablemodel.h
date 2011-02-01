#ifndef DOWNLOADTABLEMODEL_H
#define DOWNLOADTABLEMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QUrl>
#include "programme.h"

class Downloader;
class TvkaistaClient;
class QSettings;
class QTimer;

struct FileDownload
{
    QString title;
    QDateTime dateTime;
    QString description;
    QString filename;
    QString format;
    QString channelName;
    int programmeId;
    int status;
    Downloader *downloader;
};

class DownloadTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DownloadTableModel(QSettings *settings, QObject *parent = 0);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QString descriptionString(const FileDownload &download) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void setClient(TvkaistaClient *client);
    TvkaistaClient* client() const;
    int download(const Programme &programme, int format, const QString &channelName, const QUrl &url);
    void abortDownload(int index);
    void abortAllDownloads();
    bool hasUnfinishedDownloads() const;
    void removeDownload(int index);
    QString filename(int index) const;
    int status(int index) const;
    int videoFormat(int index) const;
    int programmeId(int index) const;
    bool load();
    bool save();

signals:
    void downloadFinished();

private slots:
    void updateDownloadProgress();
    void downloaderFinished();
    void networkError();

private:
    int tryResumeDownload(int programmeId, const QUrl &url);
    QString formatBytes(qint64 bytes) const;
    QString toAscii(const QString &s);
    QString removeInvalidCharacters(const QString &s);
    QSettings *m_settings;
    TvkaistaClient *m_client;
    QList<FileDownload> m_downloads;
    QTimer *m_timer;
};

#endif // DOWNLOADTABLEMODEL_H
