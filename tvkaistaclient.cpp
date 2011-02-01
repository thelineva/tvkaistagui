#include <QDebug>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QAuthenticator>
#include "cache.h"
#include "channelfeedparser.h"
#include "programmefeedparser.h"
#include "programmetableparser.h"
#include "tvkaistaclient.h"

TvkaistaClient::TvkaistaClient(QObject *parent) :
    QObject(parent), m_networkAccessManager(new QNetworkAccessManager(this)), m_reply(0),
    m_cache(0), m_programmeTableParser(new ProgrammeTableParser), m_requestType(-1)
{
    m_requestedProgramme.id = -1;
    m_requestedStream.id = -1;
    connect(m_networkAccessManager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), SLOT(requestAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
}

TvkaistaClient::~TvkaistaClient()
{
    delete m_programmeTableParser;
}

void TvkaistaClient::setCache(Cache *cache)
{
    m_cache = cache;
}

Cache* TvkaistaClient::cache() const
{
    return m_cache;
}

void TvkaistaClient::setUsername(const QString &username)
{
    m_username = username;
}

QString TvkaistaClient::username() const
{
    return m_username;
}

void TvkaistaClient::setPassword(const QString &password)
{
    m_password = password;
}

QString TvkaistaClient::password() const
{
    return m_password;
}

void TvkaistaClient::setCookies(const QByteArray &cookieString)
{
    /* Tyhjennetään evästeet. */
    m_networkAccessManager->setCookieJar(new QNetworkCookieJar());

    QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(cookieString);
    m_networkAccessManager->cookieJar()->setCookiesFromUrl(cookies, QUrl("http://www.tvkaista.fi/"));
}

QByteArray TvkaistaClient::cookies() const
{
    QList<QNetworkCookie> cookies = m_networkAccessManager->cookieJar()->cookiesForUrl(QUrl("http://www.tvkaista.fi/"));
    QByteArray cookieString;
    int count = cookies.size();

    for (int i = 0; i < count; i++) {
        if (cookies.at(i).name() == "preferred_servers") {
            continue;
        }

        if (!cookieString.isEmpty()) {
            cookieString.append('\n');
        }

        cookieString.append(cookies.at(i).toRawForm(QNetworkCookie::Full));
    }

    return cookieString;
}

void TvkaistaClient::setProxy(const QNetworkProxy &proxy)
{
    m_networkAccessManager->setProxy(proxy);
}

QNetworkProxy TvkaistaClient::proxy() const
{
    return m_networkAccessManager->proxy();
}

void TvkaistaClient::setFormat(int format)
{
    m_format = format;
}

int TvkaistaClient::format() const
{
    return m_format;
}

void TvkaistaClient::setServer(const QString &server)
{
    m_server = server;
}

QString TvkaistaClient::server() const
{
    return m_server;
}

QString TvkaistaClient::lastError() const
{
    return m_error;
}

bool TvkaistaClient::isValidUsernameAndPassword() const
{
    return !m_username.isEmpty() && !m_password.isEmpty();
}

bool TvkaistaClient::isRequestUnfinished() const
{
    return m_reply != 0;
}

void TvkaistaClient::sendLoginRequest()
{
    abortRequest();
    QString urlString = "http://www.tvkaista.fi/";
    qDebug() << "GET" << urlString;
    m_reply = m_networkAccessManager->get(QNetworkRequest(QUrl(urlString)));
    m_requestType = 1;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(frontPageRequestFinished()));
}

void TvkaistaClient::sendChannelRequest()
{
    abortRequest();
    QString urlString = "http://www.tvkaista.fi/feedbeta/channels/";
    qDebug() << "GET" << urlString;
    QUrl url(urlString);
    QNetworkRequest request(url);
    m_reply = m_networkAccessManager->get(request);
    m_requestType = 3;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(channelRequestFinished()));
}

void TvkaistaClient::sendProgrammeRequest(int channelId, const QDate &date)
{
    abortRequest();
    QString urlString = QString("http://www.tvkaista.fi/recordings/date/%1/%2/")
                        .arg(date.toString("dd/MM/yyyy")).arg(channelId);
    qDebug() << "GET" << urlString;
    m_reply = m_networkAccessManager->get(QNetworkRequest(QUrl(urlString)));
    m_requestType = 4;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(readyRead()), SLOT(programmeRequestReadyRead()));
    connect(m_reply, SIGNAL(finished()), SLOT(programmeRequestFinished()));
    m_programmeTableParser->setRequestedDate(date);
    m_programmeTableParser->setRequestedChannelId(channelId);
}

void TvkaistaClient::sendPosterRequest(const Programme &programme)
{
    abortRequest();
    QString urlString = QString("http://www.tvkaista.fi/resources/recordings/screengrabs/%1.jpg").arg(programme.id);
    qDebug() << "GET" << urlString;
    m_reply = m_networkAccessManager->get(QNetworkRequest(QUrl(urlString)));
    m_requestType = 5;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(posterRequestFinished()));
    m_requestedProgramme = programme;
}

QNetworkReply* TvkaistaClient::sendDetailedFeedRequest(const Programme &programme)
{
    QString urlString = QString("http://www.tvkaista.fi/feedbeta/programs/%1/detailed.mediarss").arg(programme.id);
    qDebug() << "GET" << urlString;
    QUrl url(urlString);
    QNetworkRequest request(url);
    return m_networkAccessManager->get(request);
}

QNetworkReply* TvkaistaClient::sendRequest(const QUrl &url)
{
    setServerCookie();
    qDebug() << "GET" << url.toString();
    QNetworkRequest request(url);
    return m_networkAccessManager->get(request);
}

QNetworkReply* TvkaistaClient::sendRequestWithAuthHeader(const QUrl &url)
{
    setServerCookie();
    qDebug() << "GET" << url.toString();
    QNetworkRequest request(url);
    return m_networkAccessManager->get(request);
}

void TvkaistaClient::sendStreamRequest(const Programme &programme)
{
    abortRequest();
    QString urlString = QString("http://www.tvkaista.fi/recordings/download/%1/").arg(programme.id);

    switch (m_format) {
    case 0:
        urlString.append("3/300000/");
        break;

    case 1:
        urlString.append("4/1000000/");
        break;

    case 2:
        urlString.append("3/2000000/");
        break;

    default:
        urlString.append("0/8000000/");
    }

    setServerCookie();
    qDebug() << "Server" << m_server;
    qDebug() << "GET" << urlString;
    m_reply = m_networkAccessManager->get(QNetworkRequest(QUrl(urlString)));
    m_requestType = 6;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(streamRequestFinished()));
    m_requestedStream = programme;
    m_requestedFormat = m_format;
}

void TvkaistaClient::sendSearchRequest(const QString &phrase)
{
    abortRequest();
    QString urlString = QString("http://services.tvkaista.fi/feedbeta/search/title/%1/flv.mediarss").arg(phrase);
    qDebug() << "GET" << urlString;
    QNetworkRequest request = QNetworkRequest(QUrl(urlString));
    m_reply = m_networkAccessManager->get(request);
    m_requestType = 7;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(searchRequestFinished()));
}

void TvkaistaClient::sendPlaylistRequest()
{
    abortRequest();
    QString urlString = "http://services.tvkaista.fi/feedbeta/playlist/standard.mediarss";
    qDebug() << "GET" << urlString;
    QNetworkRequest request = QNetworkRequest(QUrl(urlString));
    m_reply = m_networkAccessManager->get(request);
    m_requestType = 8;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(playlistRequestFinished()));
}

void TvkaistaClient::sendPlaylistAddRequest(int programmeId)
{
    abortRequest();
    QByteArray data("id=");
    data.append(QString::number(programmeId));
    QString urlString = "http://services.tvkaista.fi/feedbeta/playlist/";
    qDebug() << "POST" << urlString << data;
    m_reply = m_networkAccessManager->post(QNetworkRequest(QUrl(urlString)), data);
    m_requestType = 9;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(playlistAddRequestFinished()));
}

void TvkaistaClient::sendPlaylistRemoveRequest(int programmeId)
{
    abortRequest();
    QString urlString = QString("http://services.tvkaista.fi/feedbeta/playlist/%1/").arg(programmeId);
    qDebug() << "DELETE" << urlString;
    QNetworkRequest request = QNetworkRequest(QUrl(urlString));
    m_reply = m_networkAccessManager->deleteResource(request);
    m_requestType = 10;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(playlistRemoveRequestFinished()));
}

void TvkaistaClient::sendSeasonPassListRequest()
{
    abortRequest();
    QString urlString = "http://services.tvkaista.fi/feedbeta/seasonpasses/*/standard.mediarss";
    qDebug() << "GET" << urlString;
    QNetworkRequest request = QNetworkRequest(QUrl(urlString));
    m_reply = m_networkAccessManager->get(request);
    m_requestType = 11;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(seasonPassListRequestFinished()));
}

void TvkaistaClient::sendSeasonPassIndexRequest()
{
    abortRequest();
    QString urlString = "http://services.tvkaista.fi/feedbeta/seasonpasses/";
    qDebug() << "GET" << urlString;
    QNetworkRequest request = QNetworkRequest(QUrl(urlString));
    m_reply = m_networkAccessManager->get(request);
    m_requestType = 12;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(seasonPassIndexRequestFinished()));
}

void TvkaistaClient::sendSeasonPassAddRequest(int programmeId)
{
    abortRequest();
    QByteArray data("id=");
    data.append(QString::number(programmeId));
    QString urlString = "http://services.tvkaista.fi/feedbeta/seasonpasses/";
    qDebug() << "POST" << urlString << data;
    m_reply = m_networkAccessManager->post(QNetworkRequest(QUrl(urlString)), data);
    m_requestType = 13;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(seasonPassAddRequestFinished()));
}

void TvkaistaClient::sendSeasonPassRemoveRequest(int seasonPassId)
{
    abortRequest();
    QString urlString = QString("http://services.tvkaista.fi/feedbeta/seasonpasses/%1/").arg(seasonPassId);
    qDebug() << "DELETE" << urlString;
    QNetworkRequest request = QNetworkRequest(QUrl(urlString));
    m_reply = m_networkAccessManager->deleteResource(request);
    m_requestType = 14;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(seasonPassRemoveRequestFinished()));
}

void TvkaistaClient::frontPageRequestFinished()
{
    QByteArray data;
    data.append("username=");
    data.append(m_username.toUtf8().toPercentEncoding());
    data.append("&password=");
    data.append(m_password.toUtf8().toPercentEncoding());
    data.append("&rememberme=unlessnot&action=login");
    QString urlString = "http://www.tvkaista.fi/login/";
    qDebug() << "POST" << urlString;
    m_reply = m_networkAccessManager->post(QNetworkRequest(QUrl(urlString)), data);
    m_requestType = 2;
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(requestNetworkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(loginRequestFinished()));
}

void TvkaistaClient::loginRequestFinished()
{
    QByteArray data = m_reply->readAll();
    bool invalidPassword = data.contains("<form");
    m_lastLogin = QDateTime::currentDateTime();
    m_reply->deleteLater();
    m_reply = 0;

    if (invalidPassword) {
        emit loginError();
    }
    else if (m_programmeTableParser->requestedChannelId() >= 0) {
        sendProgrammeRequest(m_programmeTableParser->requestedChannelId(),
                             m_programmeTableParser->requestedDate());
    }
    else if (m_requestedStream.id >= 0) {
        sendStreamRequest(m_requestedStream);
    }
    else {
        emit loggedIn();
    }
}

void TvkaistaClient::channelRequestFinished()
{
    ChannelFeedParser parser;

    if (!parser.parse(m_reply)) {
        qDebug() << parser.lastError();
        m_reply->deleteLater();
        m_reply = 0;
    }
    else {
        QList<Channel> channels = parser.channels();
        m_cache->saveChannels(channels);
        m_reply->deleteLater();
        m_reply = 0;
        emit channelsFetched(channels);
    }
}

void TvkaistaClient::programmeRequestReadyRead()
{
    m_programmeTableParser->parse(m_reply);
}

void TvkaistaClient::programmeRequestFinished()
{
    if (!checkResponse()) {
        return;
    }

    if (m_programmeTableParser->isValidResults()) {
        QDateTime now = QDateTime::currentDateTime();
        QDate today = now.date();

        for (int i = 0; i < 7; i++) {
            QList<Programme> programmes = m_programmeTableParser->programmes(i);

            if (programmes.isEmpty()) {
                continue;
            }

            QDateTime expireDateTime;

            if (m_programmeTableParser->date(i) == today) {
                expireDateTime = now.addSecs(300);
            }
            else if (m_programmeTableParser->date(i) > today) {
                expireDateTime = QDateTime(m_programmeTableParser->date(i), QTime(0, 0));
            }

            m_cache->saveProgrammes(m_programmeTableParser->requestedChannelId(),
                                    m_programmeTableParser->date(i), now,
                                    expireDateTime, programmes);
        }
    }

    m_reply->deleteLater();
    m_reply = 0;
    emit programmesFetched(m_programmeTableParser->requestedChannelId(),
                           m_programmeTableParser->requestedDate(),
                           m_programmeTableParser->requestedProgrammes());
    m_programmeTableParser->clear();
}

void TvkaistaClient::posterRequestFinished()
{
    if (!checkResponse()) {
        return;
    }

    QByteArray data = m_reply->readAll();
    QImage poster = QImage::fromData(data, "JPEG");

    if (!poster.isNull()) {
        m_cache->savePoster(m_requestedProgramme, data);
        emit posterFetched(m_requestedProgramme, poster);
    }

    m_requestedProgramme.id = -1;
    m_reply->deleteLater();
    m_reply = 0;
}

void TvkaistaClient::streamRequestFinished()
{
    if (m_reply == 0) {
        return;
    }

    if (m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302) {
        emit streamUrlFetched(m_requestedStream, m_requestedFormat,
                              m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
    }
}

void TvkaistaClient::searchRequestFinished()
{
    if (m_reply == 0) {
        return;
    }

    ProgrammeFeedParser parser;

    if (!parser.parse(m_reply)) {
        qWarning() << parser.lastError();
    }

    m_reply->deleteLater();
    m_reply = 0;
    emit searchResultsFetched(parser.programmes());
}

void TvkaistaClient::playlistRequestFinished()
{
    ProgrammeFeedParser parser;

    if (!parser.parse(m_reply)) {
        qWarning() << parser.lastError();
    }

    m_reply->deleteLater();
    m_reply = 0;
    emit playlistFetched(parser.programmes());
}

void TvkaistaClient::playlistAddRequestFinished()
{
    if (m_reply == 0) {
        return;
    }

    qDebug() << "REPLY" << m_reply->readAll();
    m_reply->deleteLater();
    m_reply = 0;
    emit editRequestFinished(1, true);
}

void TvkaistaClient::playlistRemoveRequestFinished()
{
    if (m_reply == 0) {
        return;
    }

    qDebug() << "REPLY" << m_reply->readAll();
    m_reply->deleteLater();
    m_reply = 0;
    emit editRequestFinished(2, true);
}

void TvkaistaClient::seasonPassListRequestFinished()
{
    ProgrammeFeedParser parser;

    if (!parser.parse(m_reply)) {
        qWarning() << parser.lastError();
    }

    m_reply->deleteLater();
    m_reply = 0;
    m_cache->saveSeasonPasses(QDateTime::currentDateTime(), parser.programmes());
    emit seasonPassListFetched(parser.programmes());
}

void TvkaistaClient::seasonPassIndexRequestFinished()
{
    if (m_reply == 0) {
        return;
    }

    ProgrammeFeedParser parser;

    if (!parser.parse(m_reply)) {
        qWarning() << parser.lastError();
    }

    QMap<QString, int> seasonPassMap;
    QList<Programme> seasonPasses = parser.programmes();
    int count = seasonPasses.size();

    for (int i = 0; i < count; i++) {
        Programme seasonPass = seasonPasses.at(i);
        seasonPassMap.insert(seasonPass.title, seasonPass.id);
    }

    m_reply->deleteLater();
    m_reply = 0;
    emit seasonPassIndexFetched(seasonPassMap);
}

void TvkaistaClient::seasonPassAddRequestFinished()
{
    if (m_reply == 0) {
        return;
    }

    qDebug() << "REPLY" << m_reply->readAll();
    m_reply->deleteLater();
    m_reply = 0;
    emit editRequestFinished(3, true);
}

void TvkaistaClient::seasonPassRemoveRequestFinished()
{
    if (m_reply == 0) {
        return;
    }

    qDebug() << "REPLY" << m_reply->readAll();
    m_reply->deleteLater();
    m_reply = 0;
    emit editRequestFinished(4, true);
}

void TvkaistaClient::requestNetworkError(QNetworkReply::NetworkError error)
{
    qDebug() << "ERROR" << error;

    if (error == QNetworkReply::OperationCanceledError || m_reply == 0) {
        return;
    }

    /* Ei virheilmoituksia kuvakaappausten hakemisesta. */
    if (m_requestType == 5) {
        return;
    }

    if (error == QNetworkReply::AuthenticationRequiredError) {
        m_networkError = 1;
    }
    else if (m_requestedStream.id >= 0 && error == QNetworkReply::ContentNotFoundError) {
        m_networkError = 2;
    }
    else if (m_requestType == 9 && error == QNetworkReply::UnknownContentError) {
        /* Ohjelma on jo lisätty listaan. */
        m_networkError = 4;
    }
    else if (m_requestType == 13 && error == QNetworkReply::UnknownContentError) {
        /* Ohjelma on jo lisätty sarjoihin. */
        m_networkError = 4;
    }
    else if (m_requestType >= 0) {
        m_networkError = 0;
        m_error = networkErrorString(error);
    }

    abortRequest();

    /* http://bugreports.qt.nokia.com/browse/QTBUG-16333 */
    QTimer::singleShot(0, this, SLOT(handleNetworkError()));
}

void TvkaistaClient::handleNetworkError()
{
    if (m_networkError == 1) {
        emit loginError();
    }
    else if (m_networkError == 2) {
        emit streamNotFound();
    }
    else if (m_networkError == 3) {
        emit editRequestFinished(1, false);
    }
    else if (m_networkError == 4) {
        emit editRequestFinished(3, false);
    }
    else {
        emit networkError();
    }
}

void TvkaistaClient::requestAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    qDebug() << "AUTH" << m_username;

    if (authenticator->user() != m_username || authenticator->password() != m_password) {
        authenticator->setUser(m_username);
        authenticator->setPassword(m_password);
    }
}

void TvkaistaClient::abortRequest()
{
    QNetworkReply *reply = m_reply;

    if (reply != 0) {
        m_reply = 0;
        reply->abort();
        reply->deleteLater();
    }

    m_requestedProgramme.id = -1;
    m_requestedStream.id = -1;
    m_requestType = -1;
}

bool TvkaistaClient::checkResponse()
{
    if (m_reply == 0) {
        return false;
    }

    if (m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302) {
        QDateTime now = QDateTime::currentDateTime();

        if (m_lastLogin.isNull() || m_lastLogin < now.addSecs(-5)) {
            sendLoginRequest();
            return false;
        }
    }

    return true;
}

void TvkaistaClient::setServerCookie()
{
    QNetworkCookie serverCookie("preferred_servers", m_server.toAscii());
    serverCookie.setDomain("www.tvkaista.fi");
    m_networkAccessManager->cookieJar()->setCookiesFromUrl(QList<QNetworkCookie>() << serverCookie, QUrl("http://www.tvkaista.fi/"));
}

QString TvkaistaClient::networkErrorString(QNetworkReply::NetworkError error)
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
