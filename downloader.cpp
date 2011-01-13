#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include "downloader.h"

Downloader::Downloader(QObject *parent) :
    QObject(parent), m_networkAccessManager(new QNetworkAccessManager(this)),
    m_reply(0), m_bytesReceived(0), m_bytesTotal(-1), m_finished(false)
{
    m_buf = new char[4096];
}

Downloader::~Downloader()
{
    delete m_buf;
}

void Downloader::start(const QUrl &url)
{
    abort();
    qDebug() << "GET" << url;
    QNetworkRequest request(url);
    m_reply = m_networkAccessManager->get(request);
    connect(m_reply, SIGNAL(readyRead()), SLOT(replyReadyRead()));
    connect(m_reply, SIGNAL(finished()), SLOT(replyFinished()));
    connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(replyDownloadProgress(qint64,qint64)));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyNetworkError(QNetworkReply::NetworkError)));
}

void Downloader::abort()
{
    if (m_reply == 0) {
        return;
    }

    m_reply->abort();
    m_reply->deleteLater();
    m_reply = 0;
}

QString Downloader::lastError() const
{
    return m_error;
}

qint64 Downloader::bytesReceived() const
{
    return m_bytesReceived;
}

qint64 Downloader::bytesTotal() const
{
    return m_bytesTotal;
}

bool Downloader::hasError() const
{
    return !m_error.isEmpty();
}

bool Downloader::isFinished() const
{
    return m_finished;
}

void Downloader::setFilename(const QString &filename)
{
    m_filename = filename;
}

QString Downloader::filename() const
{
    return m_filename;
}

void Downloader::setFilenameFromReply(bool filenameFromReply)
{
    m_filenameFromReply = filenameFromReply;
}

bool Downloader::isFilenameFromReply() const
{
    return m_filenameFromReply;
}

void Downloader::replyReadyRead()
{
    if (!m_file.isOpen()) {
        /* "Content-Disposition: inline; filename=Tv-uutiset_2010.12.30_YLE-TV1_8661167.ts" */
        QString dispositionHeader = m_reply->rawHeader("Content-Disposition");

        if (m_filenameFromReply && dispositionHeader.startsWith("inline; filename=")) {
            m_filename = QFileInfo(QFileInfo(m_filename).dir(), dispositionHeader.mid(17)).filePath();
        }

        appendSuffixToFilenameAndCreateDir();
        qDebug() << "WRITE" << m_filename;
        m_file.setFileName(m_filename);

        if (!m_file.open(QIODevice::WriteOnly)) {
            m_error = m_file.errorString();
            abort();
            return;
        }
    }

    int len = m_reply->read(m_buf, 4096);

    if (!m_file.write(m_buf, len) < 0) {
        m_error = m_file.errorString();
        abort();
        return;
    }
}

void Downloader::replyFinished()
{
    m_file.close();
    m_finished = true;
    emit finished();
}

void Downloader::replyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_bytesReceived = bytesReceived;
    m_bytesTotal = bytesTotal;
}

void Downloader::replyNetworkError(QNetworkReply::NetworkError error)
{
    if (error == QNetworkReply::OperationCanceledError) {
        return;
    }

    abort();
    m_error = networkErrorString(error);
    emit networkError();
}

QString Downloader::networkErrorString(QNetworkReply::NetworkError error)
{
    switch (error) {
    case QNetworkReply::ConnectionRefusedError:
        return "ConnectionRefused";
    case QNetworkReply::RemoteHostClosedError:
        return "RemoteHostClosed";
    case QNetworkReply::HostNotFoundError:
        return "HostNotFound";
    case QNetworkReply::TimeoutError:
        return "Timeout";
    case QNetworkReply::OperationCanceledError:
        return "OperationCanceled";
    case QNetworkReply::SslHandshakeFailedError:
        return "SslHandshakeFailed";
    case QNetworkReply::ContentAccessDenied:
        return "ContentAccessDenied";
    case QNetworkReply::ContentOperationNotPermittedError:
        return "ContentOperationNotPermitted";
    case QNetworkReply::ContentNotFoundError:
        return "ContentNotFound";
    case QNetworkReply::AuthenticationRequiredError:
        return "AuthenticationRequired";
    default:
        return "UnknownNetworkError";
    }
}

void Downloader::appendSuffixToFilenameAndCreateDir()
{
    QFileInfo fileInfo(m_filename);
    QString suffix = fileInfo.suffix();

    if (!suffix.isEmpty()) {
        suffix.prepend('.');
    }

    QString prefix = m_filename.left(m_filename.length() - suffix.length());
    int number = 0;

    while (fileInfo.exists()) {
        number++;
        m_filename = QString("%1-%2%3").arg(prefix).arg(number).arg(suffix);
        fileInfo = QFileInfo(m_filename);
    }

    QDir dir = fileInfo.dir();
    qDebug() << dir.path();

    if (!dir.exists()) {
        qDebug() << dir.path();
        dir.mkpath(dir.path());
    }
}
