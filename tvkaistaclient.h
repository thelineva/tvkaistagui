#ifndef TVKAISTACLIENT_H
#define TVKAISTACLIENT_H

#include <QDate>
#include <QNetworkReply>
#include <QObject>
#include <QXmlStreamReader>
#include "channel.h"
#include "programme.h"

class QNetworkAccessManager;
class QNetworkRequest;
class Cache;
class ProgrammeFeedParser;
class ProgrammeTableParser;

class TvkaistaClient : public QObject
{
    Q_OBJECT
public:
    TvkaistaClient(QObject *parent = 0);
    ~TvkaistaClient();
    void setCache(Cache *cache);
    Cache* cache() const;
    void setUsername(const QString &username);
    QString username() const;
    void setPassword(const QString &password);
    QString password() const;
    void setCookies(const QByteArray &cookieString);
    QByteArray cookies() const;
    void setProxy(const QNetworkProxy &proxy);
    QNetworkProxy proxy() const;
    void setFormat(int format);
    int format() const;
    QString lastError() const;
    bool isValidUsernameAndPassword() const;
    void sendLoginRequest();
    void sendChannelRequest();
    void sendProgrammeRequest(int channelId, const QDate &date);
    void sendPosterRequest(const Programme &programme);
    void sendStreamRequest(const Programme &programme);
    void sendSearchRequest(const QString &phrase);
    QNetworkReply* sendDetailedFeedRequest(const Programme &programme);
    QNetworkReply* sendRequest(const QUrl &url);
    QNetworkReply* sendRequestWithAuthHeader(const QUrl &url);
    static QString networkErrorString(QNetworkReply::NetworkError error);

signals:
    void loggedIn();
    void channelsFetched(const QList<Channel> &channels);
    void programmesFetched(int channelId, const QDate &date, const QList<Programme> &programmes);
    void posterFetched(const Programme &programme, const QImage &poster);
    void streamUrlFetched(const Programme &programme, int format, const QUrl &url);
    void searchResultsFetched(const QList<Programme> &programmes);
    void streamNotFound();
    void loginError();
    void networkError();

private slots:
    void frontPageRequestFinished();
    void loginRequestFinished();
    void channelRequestFinished();
    void programmeRequestReadyRead();
    void programmeRequestFinished();
    void posterRequestFinished();
    void streamRequestFinished();
    void searchRequestFinished();
    void requestNetworkError(QNetworkReply::NetworkError error);
    void handleNetworkError();

private:
    void addAuthHeaderToRequest(QNetworkRequest &request);
    void abortRequest();
    bool checkResponse();
    QNetworkAccessManager *m_networkAccessManager;
    QNetworkReply *m_reply;
    Cache *m_cache;
    ProgrammeTableParser *m_programmeTableParser;
    Programme m_requestedProgramme;
    Programme m_requestedStream;
    int m_requestedFormat;
    QDateTime m_lastLogin;
    QString m_username;
    QString m_password;
    QString m_error;
    int m_networkError;
    int m_format;
};

#endif // TVKAISTACLIENT_H
