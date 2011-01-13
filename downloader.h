#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QFile>
#include <QObject>
#include <QNetworkReply>
#include <QUrl>

class QNetworkAccessManager;

class Downloader : public QObject
{
Q_OBJECT
public:
    Downloader(QObject *parent = 0);
    ~Downloader();
    void start(const QUrl &url);
    void abort();
    QString lastError() const;
    qint64 bytesReceived() const;
    qint64 bytesTotal() const;
    bool hasError() const;
    bool isFinished() const;
    void setFilename(const QString &filename);
    QString filename() const;
    void setFilenameFromReply(bool filenameFromReply);
    bool isFilenameFromReply() const;

signals:
    void finished();
    void networkError();

private slots:
    void replyReadyRead();
    void replyFinished();
    void replyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void replyNetworkError(QNetworkReply::NetworkError error);

private:
    QString networkErrorString(QNetworkReply::NetworkError error);
    void appendSuffixToFilename();
    QNetworkAccessManager *m_networkAccessManager;
    QNetworkReply *m_reply;
    char *m_buf;
    QFile m_file;
    QString m_error;
    QString m_filename;
    bool m_filenameFromReply;
    qint64 m_bytesReceived;
    qint64 m_bytesTotal;
    bool m_finished;
};

#endif // DOWNLOADER_H
