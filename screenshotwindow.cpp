#include <QDebug>
#include <QLabel>
#include <QMovie>
#include <QComboBox>
#include <QSettings>
#include "programmefeedparser.h"
#include "screenshotwindow.h"
#include "tvkaistaclient.h"
#include "ui_screenshotwindow.h"

ScreenshotWindow::ScreenshotWindow(QSettings *settings, QWidget *parent) :
    QMainWindow(parent), ui(new Ui::ScreenshotWindow),
    m_settings(settings), m_reply(0), m_redirections(0)
{
    ui->setupUi(this);
    ui->toolBar->setStyleSheet("QLabel { padding-left: 10px; padding-right: 5px; }");
    m_loadMovie = new QMovie(this);
    m_loadMovie->setFileName(":/images/load-32x32.gif");
    m_loadLabel = new QLabel(this);
    m_loadLabel->setMinimumSize(47, 32);

    QStringList viewOptions;
    viewOptions << "5" << "10" << "15" << "20" << "30" << "50" << "Kaikki";
    m_numScreenshotsComboBox = new QComboBox(this);
    m_numScreenshotsComboBox->addItems(viewOptions);
    m_numScreenshotsComboBox->setCurrentIndex(1);

    QLabel *viewLabel = new QLabel(trUtf8("Näytä"), this);
    ui->toolBar->addWidget(viewLabel);
    ui->toolBar->addWidget(m_numScreenshotsComboBox);

    QLabel *spacerLabel = new QLabel(this);
    spacerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->toolBar->addWidget(spacerLabel);
    ui->toolBar->addWidget(m_loadLabel);

    ui->frame->setBackgroundRole(QPalette::Base);

    connect(ui->actionClose, SIGNAL(triggered()), SLOT(close()));
    connect(m_numScreenshotsComboBox, SIGNAL(currentIndexChanged(int)), SLOT(numScreenshotsChanged()));

    settings->beginGroup("screenshotWindow");
    restoreGeometry(settings->value("geometry").toByteArray());
    int numScreenshots = settings->value("numScreenshots", 0).toInt();
    settings->endGroup();

    int count = viewOptions.size();

    if (numScreenshots < 0) {
        m_numScreenshotsComboBox->setCurrentIndex(viewOptions.size() - 1);
    }
    else {
        QString s = QString::number(numScreenshots);

        for (int i = 0; i < count; i++) {
            if (viewOptions.at(i) == s) {
                m_numScreenshotsComboBox->setCurrentIndex(i);
                break;
            }
        }
    }
}

ScreenshotWindow::~ScreenshotWindow()
{
    delete ui;
}

void ScreenshotWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ScreenshotWindow::closeEvent(QCloseEvent*)
{
    m_settings->beginGroup("screenshotWindow");
    m_settings->setValue("geometry", saveGeometry());

    if (m_numScreenshotsComboBox->currentIndex() == m_numScreenshotsComboBox->count() - 1) {
        m_settings->setValue("numScreenshots", -1);
    }
    else {
        m_settings->setValue("numScreenshots", m_numScreenshotsComboBox->currentText());
    }

    m_settings->endGroup();
}

void ScreenshotWindow::setClient(TvkaistaClient *client)
{
    m_client = client;
}

TvkaistaClient* ScreenshotWindow::client() const
{
    return m_client;
}

void ScreenshotWindow::fetchScreenshots(const Programme &programme)
{
    m_programme = programme;
    ui->listWidget->clear();
    ui->statusLabel->setText(trUtf8("Haetaan kuvakaappauksia..."));
    ui->stackedWidget->setCurrentIndex(1);
    setWindowTitle(trUtf8("Kuvakaappaukset - %1").arg(programme.title));
    startLoadingAnimation();
    m_reply = m_client->sendDetailedFeedRequest(programme);
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(networkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(feedRequestFinished()));
}

void ScreenshotWindow::numScreenshotsChanged()
{
    if (!m_thumbnails.isEmpty()) {
        ui->listWidget->clear();
        thumbnailsToQueue();
        fetchNextScreenshot();
    }
}

void ScreenshotWindow::feedRequestFinished()
{
    ProgrammeFeedParser parser;

    if (!parser.parse(m_reply)) {
        qWarning() << parser.lastError();
    }

    m_thumbnails = parser.thumbnails();
    m_reply->deleteLater();
    thumbnailsToQueue();
    fetchNextScreenshot();
}

void ScreenshotWindow::thumbnailsToQueue()
{
    if (m_reply != 0) {
        m_reply->abort();
        m_reply = 0;
    }

    int count = m_thumbnails.size();
    int step = 1;
    int numScreenshots = INT_MAX;

    if (m_numScreenshotsComboBox->currentIndex() < m_numScreenshotsComboBox->count() - 1) {
        numScreenshots = m_numScreenshotsComboBox->currentText().toInt();
        step = qMax(1, count / numScreenshots);
    }

    int remaining = numScreenshots - 1;

    for (int i = 0; i < count - 1 && remaining > 0; i += step) {
        m_queue.append(m_thumbnails.at(i));
    }

    if (count > 1) {
        m_queue.append(m_thumbnails.at(count - 1));
    }
}

void ScreenshotWindow::thumbnailRequestFinished()
{
    if (m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 303) {
        m_redirections++;

        if (m_redirections > 3) {
            return;
        }

        QUrl url = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        qDebug() << "Redirected to" << url;
        m_reply = m_client->sendThumbnailRequest(url);
        connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(networkError(QNetworkReply::NetworkError)));
        connect(m_reply, SIGNAL(finished()), SLOT(thumbnailRequestFinished()));
        return;
    }

    QPixmap pixmap;
    Thumbnail thumbnail = m_queue.takeFirst();
    pixmap.loadFromData(m_reply->readAll());
    m_reply->deleteLater();
    m_reply = 0;

    if (!pixmap.isNull()) {
        QIcon icon(pixmap);

        if (ui->listWidget->count() == 0) {
            ui->listWidget->setIconSize(QSize(pixmap.width(), pixmap.height()));
            ui->listWidget->setGridSize(QSize(pixmap.width() + 10,
                                              pixmap.height() + ui->listWidget->fontMetrics().height() + 10));
            ui->stackedWidget->setCurrentIndex(0);
        }

        new QListWidgetItem(icon, thumbnail.time.toString("h:mm"), ui->listWidget);
    }

    fetchNextScreenshot();
}

void ScreenshotWindow::networkError(QNetworkReply::NetworkError error)
{
    QString errorString = TvkaistaClient::networkErrorString(error);
    qWarning() << errorString;
    ui->statusLabel->setText(trUtf8("Virhe: %1").arg(errorString));
}

void ScreenshotWindow::fetchNextScreenshot()
{
    if (m_queue.isEmpty()) {
        stopLoadingAnimation();
        return;
    }

    Thumbnail thumbnail = m_queue.first();
    m_redirections = 0;
    m_reply = m_client->sendThumbnailRequest(thumbnail.url);
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(networkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(finished()), SLOT(thumbnailRequestFinished()));
}

void ScreenshotWindow::startLoadingAnimation()
{
    m_loadLabel->setMovie(m_loadMovie);
    m_loadMovie->start();
}

void ScreenshotWindow::stopLoadingAnimation()
{
    m_loadMovie->stop();
    m_loadLabel->setMovie(0);
    m_loadLabel->setText(" ");
}
