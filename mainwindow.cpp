#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMovie>
#include <QNetworkProxy>
#include <QPainter>
#include <QProcess>
#include <QSignalMapper>
#include <QTimer>
#include "aboutdialog.h"
#include "cache.h"
#include "downloader.h"
#include "downloaddelegate.h"
#include "downloadtablemodel.h"
#include "programmefeedparser.h"
#include "programmetablemodel.h"
#include "tvkaistaclient.h"
#include "screenshotwindow.h"
#include "settingsdialog.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    m_settings(QSettings::IniFormat, QSettings::UserScope,
                   QCoreApplication::applicationName(),
                   QCoreApplication::applicationName()),
    m_client(new TvkaistaClient(this)),
    m_downloadTableModel(new DownloadTableModel(&m_settings, this)),
    m_programmeListTableModel(new ProgrammeTableModel(false, this)),
    m_searchResultsTableModel(new ProgrammeTableModel(true, this)),
    m_currentTableModel(m_programmeListTableModel),
    m_cache(new Cache), m_settingsDialog(0), m_screenshotWindow(0),
    m_currentChannelId(-1), m_searchIcon(":/images/list-22x22.png"),
    m_downloading(false), m_searchResultsVisible(false)
{
    ui->setupUi(this);
    m_client->setCache(m_cache);
    m_downloadTableModel->setClient(m_client);
    ui->calendarWidget->setFirstDayOfWeek(Qt::Monday);
    ui->downloadsTableView->setModel(m_downloadTableModel);
    ui->downloadsTableView->addAction(ui->actionPlayFile);
    ui->downloadsTableView->addAction(ui->actionOpenDirectory);
    ui->downloadsTableView->addAction(ui->actionAbortDownload);
    ui->downloadsTableView->addAction(ui->actionRemoveDownload);
    ui->downloadsTableView->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->downloadsTableView->setItemDelegateForColumn(0, new DownloadDelegate(this));
    ui->programmeTableView->setModel(m_programmeListTableModel);
    ui->channelListWidget->addAction(ui->actionRefreshChannels);
    ui->channelListWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->calendarWidget->addAction(ui->actionCurrentDay);
    ui->calendarWidget->addAction(ui->actionPreviousDay);
    ui->calendarWidget->addAction(ui->actionNextDay);
    ui->calendarWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_formatComboBox = new QComboBox(this);
    m_formatComboBox->setMinimumWidth(150);
    m_searchComboBox = new QComboBox(this);
    m_searchComboBox->setEditable(true);
    m_searchComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_loadMovie = new QMovie(this);
    m_loadMovie->setFileName(":/images/load-32x32.gif");
    m_loadLabel = new QLabel(this);
    m_loadLabel->setMinimumSize(47, 32);
    QAction *searchAction = new QAction(this);
    searchAction->setText(trUtf8("Hae"));
    m_searchToolButton = new QToolButton(this);
    m_searchToolButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_searchToolButton->setDefaultAction(searchAction);
    ui->toolBar->addWidget(m_formatComboBox);
    ui->toolBar->addWidget(new QLabel(tr("Haku"), this));
    ui->toolBar->addWidget(m_searchComboBox);
    ui->toolBar->addWidget(m_searchToolButton);
    ui->toolBar->addWidget(m_loadLabel);
    ui->toolBar->setStyleSheet("QLabel { padding-left: 10px; padding-right: 5px; }");
    ui->splitter->setSizes(QList<int>() << 200 << 150);

    QIcon icon;
    icon.addFile(":/images/tvkaista-16x16.png", QSize(16, 16));
    icon.addFile(":/images/tvkaista-32x32.png", QSize(32, 32));
    icon.addFile(":/images/tvkaista-48x48.png", QSize(48, 48));
    setWindowIcon(icon);

    updateFontSize();

    m_noPosterImage = QImage(104, 80, QImage::Format_RGB32);
    QPainter painter(&m_noPosterImage);
    painter.fillRect(m_noPosterImage.rect(), QBrush(ui->descriptionTextEdit->palette().color(QPalette::Base)));
    painter.end();

    m_formatComboBox->addItems(videoFormats());

    ui->actionProgrammeList->setChecked(true);
    ui->actionProgrammeListButton->setChecked(true);

    QTextCharFormat textFormat;
    textFormat.setForeground(QBrush(QColor(138, 8, 8), Qt::SolidPattern));
    ui->calendarWidget->setWeekdayTextFormat(Qt::Saturday, textFormat);
    ui->calendarWidget->setWeekdayTextFormat(Qt::Sunday, textFormat);

    QWidget *calendarNavBar = ui->calendarWidget->findChild<QWidget*>("qt_calendar_navigationbar");

    if (calendarNavBar) {
        QPalette pal = calendarNavBar->palette();
        pal.setColor(calendarNavBar->backgroundRole(), pal.color(QPalette::Normal, calendarNavBar->backgroundRole()));
        pal.setColor(calendarNavBar->foregroundRole(), pal.color(QPalette::Normal, calendarNavBar->foregroundRole()));
        calendarNavBar->setPalette(pal);
    }

    QPalette palette = ui->calendarWidget->palette();
    palette.setColor(QPalette::Highlight, QColor(164, 164, 164));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    ui->calendarWidget->setPalette(palette);

    connect(ui->calendarWidget, SIGNAL(clicked(QDate)), SLOT(dateClicked(QDate)));
    connect(ui->calendarWidget, SIGNAL(activated(QDate)), SLOT(dateClicked(QDate)));
    connect(ui->downloadsTableView, SIGNAL(activated(QModelIndex)), SLOT(playDownloadedFile()));
    connect(ui->downloadsTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(downloadSelectionChanged()));
    connect(ui->programmeTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(programmeSelectionChanged()));
    connect(ui->programmeTableView, SIGNAL(activated(QModelIndex)), SLOT(programmeDoubleClicked()));
    connect(ui->programmeTableView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(programmeMenuRequested(QPoint)));
    connect(ui->actionSortByTimeAsc, SIGNAL(triggered()), SLOT(sortByTimeAsc()));
    connect(ui->actionSortByTimeDesc, SIGNAL(triggered()), SLOT(sortByTimeDesc()));
    connect(ui->actionSortByTitleAsc, SIGNAL(triggered()), SLOT(sortByTitleAsc()));
    connect(ui->actionSortByTitleDesc, SIGNAL(triggered()), SLOT(sortByTitleDesc()));
    connect(ui->channelListWidget, SIGNAL(currentRowChanged(int)), SLOT(channelClicked(int)));
    connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));
    connect(ui->actionDownloads, SIGNAL(triggered()), SLOT(toggleDownloadsDockWidget()));
    connect(ui->actionShortcuts, SIGNAL(triggered()), SLOT(toggleShortcutsDockWidget()));
    connect(ui->actionRefresh, SIGNAL(triggered()), SLOT(refreshProgrammes()));
    connect(ui->actionRefreshButton, SIGNAL(triggered()), SLOT(refreshProgrammes()));
    connect(ui->actionCurrentDay, SIGNAL(triggered()), SLOT(goToCurrentDay()));
    connect(ui->currentDayPushButton, SIGNAL(clicked()), SLOT(goToCurrentDay()));
    connect(ui->actionPreviousDay, SIGNAL(triggered()), SLOT(goToPreviousDay()));
    connect(ui->actionNextDay, SIGNAL(triggered()), SLOT(goToNextDay()));
    connect(ui->actionWatch, SIGNAL(triggered()), SLOT(watchProgramme()));    
    connect(ui->actionDownload, SIGNAL(triggered()), SLOT(downloadProgramme()));
    connect(ui->actionScreenshots, SIGNAL(triggered()), SLOT(openScreenshotWindow()));
    connect(ui->actionProgrammeList, SIGNAL(triggered()), SLOT(showProgrammeList()));
    connect(ui->actionProgrammeListButton, SIGNAL(triggered()), SLOT(showProgrammeList()));
    connect(ui->actionSearchResults, SIGNAL(triggered()), SLOT(showSearchResults()));
    connect(ui->actionSearchResultsButton, SIGNAL(triggered()), SLOT(showSearchResults()));
    connect(ui->actionPlayFile, SIGNAL(triggered()), SLOT(playDownloadedFile()));
    connect(ui->watchPushButton, SIGNAL(clicked()), SLOT(watchProgramme()));
    connect(ui->downloadPushButton, SIGNAL(clicked()), SLOT(downloadProgramme()));
    connect(ui->screenshotsPushButton, SIGNAL(clicked()), SLOT(openScreenshotWindow()));
    connect(ui->playFilePushButton, SIGNAL(clicked()), SLOT(playDownloadedFile()));
    connect(ui->actionOpenDirectory, SIGNAL(triggered()), SLOT(openDirectory()));
    connect(ui->actionSettings, SIGNAL(triggered()), SLOT(openSettingsDialog()));
    connect(ui->actionContents, SIGNAL(triggered()), SLOT(openHelp()));
    connect(ui->actionAbout, SIGNAL(triggered()), SLOT(openAboutDialog()));
    connect(ui->actionAbortDownload, SIGNAL(triggered()), SLOT(abortDownload()));
    connect(ui->abortDownloadToolButton, SIGNAL(clicked()), SLOT(abortDownload()));
    connect(ui->actionRemoveDownload, SIGNAL(triggered()), SLOT(removeDownload()));
    connect(ui->removeDownloadToolButton, SIGNAL(clicked()), SLOT(removeDownload()));
    connect(ui->actionRefreshChannels, SIGNAL(triggered()), SLOT(refreshChannels()));
    connect(m_formatComboBox, SIGNAL(currentIndexChanged(int)), SLOT(formatChanged()));
    connect(m_searchComboBox, SIGNAL(activated(QString)), SLOT(search()));
    connect(m_searchToolButton, SIGNAL(clicked()), SLOT(search()));
    connect(m_client, SIGNAL(channelsFetched(QList<Channel>)), SLOT(channelsFetched(QList<Channel>)));
    connect(m_client, SIGNAL(programmesFetched(int,QDate,QList<Programme>)), SLOT(programmesFetched(int,QDate,QList<Programme>)));
    connect(m_client, SIGNAL(posterFetched(Programme,QImage)), SLOT(posterFetched(Programme,QImage)));
    connect(m_client, SIGNAL(streamUrlFetched(Programme,int,QUrl)), SLOT(streamUrlFetched(Programme,int,QUrl)));
    connect(m_client, SIGNAL(searchResultsFetched(QList<Programme>)), SLOT(searchResultsFetched(QList<Programme>)));
    connect(m_client, SIGNAL(networkError()), SLOT(networkError()));
    connect(m_client, SIGNAL(loginError()), SLOT(loginError()));
    connect(m_client, SIGNAL(streamNotFound()), SLOT(streamNotFound()));
    connect(m_downloadTableModel, SIGNAL(downloadFinished()), SLOT(downloadFinished()));

    QAction *action = new QAction(this);
    action->setShortcut(Qt::Key_F2);
    connect(action, SIGNAL(triggered()), ui->programmeTableView, SLOT(setFocus()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(Qt::Key_F3);
    connect(action, SIGNAL(triggered()), SLOT(setFocusToCalendar()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(Qt::Key_F4);
    connect(action, SIGNAL(triggered()), SLOT(setFocusToChannelList()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(Qt::Key_F5);
    connect(action, SIGNAL(triggered()), ui->downloadsTableView, SLOT(setFocus()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(Qt::Key_F6);
    connect(action, SIGNAL(triggered()), m_searchComboBox, SLOT(setFocus()));
    addAction(action);

    action = new QAction(this);
    action->setShortcutContext(Qt::WidgetShortcut);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Delete));
    connect(action, SIGNAL(triggered()), SLOT(clearSearchHistory()));
    m_searchComboBox->addAction(action);

    QSignalMapper *signalMapper = new QSignalMapper(this);
    connect(signalMapper, SIGNAL(mapped(int)), SLOT(selectChannel(int)));

    int keys[] = { Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5,
                   Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9, Qt::Key_0,
                   Qt::Key_Q, Qt::Key_W, Qt::Key_E, Qt::Key_R, Qt::Key_T,
                   Qt::Key_Y, Qt::Key_U, Qt::Key_I, Qt::Key_O, Qt::Key_P };

    for (int i = 0; i < 20; i++) {
        action = new QAction(this);
        action->setShortcut(QKeySequence(Qt::ALT | keys[i]));
        signalMapper->setMapping(action, i);
        connect(action, SIGNAL(triggered()), signalMapper, SLOT(map()));
        addAction(action);
    }

    m_settings.beginGroup("client");
    QString cacheDirPath = m_settings.value("cacheDir").toString();

    if (cacheDirPath.isEmpty()) {
        cacheDirPath = QString("%1/cache").arg(QFileInfo(m_settings.fileName()).path());
    }

    int format = qBound(0, m_settings.value("format", 1).toInt(), videoFormats().size() - 1);
    m_client->setCookies(m_settings.value("cookies").toByteArray());
    m_client->setFormat(format);
    m_settings.endGroup();

    m_cache->setDirectory(QDir(cacheDirPath));
    m_formatComboBox->setCurrentIndex(format);
    loadClientSettings();
    setFormat(format);

    m_settings.beginGroup("mainWindow");
    restoreGeometry(m_settings.value("geometry").toByteArray());
    restoreState(m_settings.value("state").toByteArray());

    if (!ui->splitter->restoreState(m_settings.value("splitter").toByteArray())) {
        ui->splitter->setSizes(QList<int>() << 450 << 150);
    }

    int cid = m_settings.value("channel").toInt();
    int phraseCount = m_settings.beginReadArray("searchHistory");

    for (int i = 0; i < phraseCount; i++) {
        m_settings.setArrayIndex(i);
        m_searchHistory.append(m_settings.value("phrase").toString());
    }

    m_settings.endArray();
    m_searchComboBox->addItems(m_searchHistory);
    m_searchComboBox->setCurrentIndex(-1);

    QString sortKey = m_settings.value("sortProgrammes").toString();
    if (sortKey == "timeAsc") sortByTimeAsc();
    else if (sortKey == "titleAsc") sortByTitleAsc();
    else if (sortKey == "titleDesc") sortByTitleDesc();
    else sortByTimeDesc();

    m_settings.endGroup();

    QString version = m_settings.value("version").toString();

    if (version.isEmpty() || version < "1.1.2") {
        ui->shortcutsDockWidget->setVisible(true);

        m_settings.beginGroup("mediaPlayer");
        m_settings.setValue("stream", addDefaultOptionsToVlcCommand(m_settings.value("stream").toString()));
        m_settings.setValue("file", addDefaultOptionsToVlcCommand(m_settings.value("file").toString()));
        m_settings.endGroup();
    }

    m_currentDate = QDate::currentDate();
    fetchChannels(false);
    int channelCount = m_channels.size();

    for (int i = 0; i < channelCount; i++) {
        if (m_channels.at(i).id == cid) {
            ui->channelListWidget->setCurrentIndex(ui->channelListWidget->model()->index(i, 0, QModelIndex()));
            m_currentChannelId = cid;
            break;
        }
    }

    if (m_currentChannelId < 0 && !m_channels.isEmpty()) {
        fetchProgrammes(m_channels.at(0).id, QDate::currentDate(), false);
    }

    m_downloadTableModel->load();
    int downloadCount = m_downloadTableModel->rowCount(QModelIndex());

    if (downloadCount > 0) {
        ui->downloadsTableView->scrollToBottom();
        ui->downloadsTableView->resizeColumnToContents(0);
        ui->downloadsTableView->resizeRowsToContents();
        ui->downloadsTableView->selectRow(downloadCount - 1);
    }

    downloadSelectionChanged();
    updateCalendar();

    if (!m_client->isValidUsernameAndPassword()) {
        QTimer::singleShot(0, this, SLOT(openSettingsDialog()));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (m_downloadTableModel->hasUnfinishedDownloads()) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(windowTitle());
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(trUtf8("Lataukset keskeytetään, kun ohjelma suljetaan. Haluatko kuitenkin sulkea ohjelman?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (msgBox.exec() == QMessageBox::No) {
            e->ignore();
            return;
        }
    }

    m_settings.setValue("version", APP_VERSION);

    m_settings.beginGroup("mainWindow");
    m_settings.setValue("geometry", saveGeometry());
    m_settings.setValue("state", saveState());
    m_settings.setValue("splitter", ui->splitter->saveState());
    m_settings.setValue("channel", m_currentChannelId);

    int phraseCount = m_searchHistory.size();
    m_settings.beginWriteArray("searchHistory", phraseCount);

    for (int i = 0; i < phraseCount; i++) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("phrase", m_searchHistory.at(i));
    }

    m_settings.endArray();

    int sortKey = m_searchResultsTableModel->sortKey();
    bool descending = m_searchResultsTableModel->isDescending();
    QString sortKeyString;

    if (sortKey == 1 && !descending) sortKeyString = "timeAsc";
    else if (sortKey == 1 && descending) sortKeyString = "timeDesc";
    else if (sortKey == 2 && !descending) sortKeyString = "titleAsc";
    else if (sortKey == 2 && descending) sortKeyString = "titleDesc";

    m_settings.setValue("sortProgrammes", sortKeyString);
    m_settings.endGroup();

    m_settings.beginGroup("client");
    m_settings.setValue("cookies", m_client->cookies());
    m_settings.setValue("format", m_formatComboBox->currentIndex());
    m_settings.endGroup();

    m_downloadTableModel->abortAllDownloads();
    m_downloadTableModel->save();
}

void MainWindow::dateClicked(const QDate &date)
{
    fetchProgrammes(m_currentChannelId, date, false);
}

void MainWindow::channelClicked(int index)
{
    if (index < 0) {
        return;
    }

    fetchProgrammes(m_channels.at(index).id, m_currentDate, false);
}

void MainWindow::programmeDoubleClicked()
{
    m_settings.beginGroup("mainWindow");
    bool download = m_settings.value("doubleClick").toString() == "download";
    m_settings.endGroup();

    if (download) {
        downloadProgramme();
    }
    else {
        watchProgramme();
    }
}

void MainWindow::programmeSelectionChanged()
{
    QModelIndexList rows = ui->programmeTableView->selectionModel()->selectedRows(0);

    if (rows.isEmpty() || m_currentTableModel->programmeCount() == 0) {
        return;
    }

    m_currentProgramme = m_currentTableModel->programme(rows.at(0).row());
    m_settings.beginGroup("mainWindow");
    bool posterVisible = m_settings.value("posterVisible", true).toBool();
    m_settings.endGroup();

    if (m_currentProgramme.id < 0 || (m_currentProgramme.flags & 0x08) > 0 || !posterVisible || !fetchPoster()) {
        m_posterImage = m_noPosterImage;
    }

    updateDescription();
}

void MainWindow::downloadSelectionChanged()
{
    QModelIndexList indexes = ui->downloadsTableView->selectionModel()->selectedRows(0);
    bool playEnabled = false;
    bool abortEnabled = false;
    bool removeEnabled = false;

    if (!indexes.isEmpty()) {
        playEnabled = (indexes.size() == 1);
        removeEnabled = true;

        for (int i = 0; i < indexes.size(); i++) {
            if (m_downloadTableModel->status(indexes.at(i).row()) == 0) {
                abortEnabled = true;
            }
        }
    }

    ui->actionPlayFile->setEnabled(playEnabled);
    ui->playFilePushButton->setEnabled(playEnabled);
    ui->actionAbortDownload->setEnabled(abortEnabled);
    ui->abortDownloadToolButton->setEnabled(abortEnabled);
    ui->actionRemoveDownload->setEnabled(removeEnabled);
    ui->removeDownloadToolButton->setEnabled(removeEnabled);
}

void MainWindow::formatChanged()
{
    setFormat(m_formatComboBox->currentIndex());
}

void MainWindow::programmeMenuRequested(const QPoint &point)
{
    QMenu menu(this);
    menu.addAction(ui->actionWatch);
    menu.addAction(ui->actionDownload);
    menu.addAction(ui->actionScreenshots);
    menu.addAction(ui->actionRefresh);

    if (m_searchResultsVisible) {
        int sortKey = m_searchResultsTableModel->sortKey();
        bool descending = m_searchResultsTableModel->isDescending();
        ui->actionSortByTimeAsc->setChecked(sortKey == 1 && !descending);
        ui->actionSortByTimeDesc->setChecked(sortKey == 1 && descending);
        ui->actionSortByTitleAsc->setChecked(sortKey == 2 && !descending);
        ui->actionSortByTitleDesc->setChecked(sortKey == 2 && descending);

        QMenu *sortMenu = menu.addMenu(trUtf8("Järjestä"));
        sortMenu->addAction(ui->actionSortByTimeAsc);
        sortMenu->addAction(ui->actionSortByTimeDesc);
        sortMenu->addAction(ui->actionSortByTitleAsc);
        sortMenu->addAction(ui->actionSortByTitleDesc);
    }

    menu.exec(ui->programmeTableView->mapToGlobal(point));
}

void MainWindow::refreshProgrammes()
{
    QDateTime now = QDateTime::currentDateTime();

    if (m_lastRefreshTime.secsTo(now) < 3) {
        return;
    }

    m_lastRefreshTime = now;

    if (m_searchResultsVisible) {
        fetchSearchResults(m_searchPhrase);
    }
    else {
        fetchProgrammes(m_currentChannelId, m_currentDate, true);
    }
}

void MainWindow::goToCurrentDay()
{
    fetchProgrammes(m_currentChannelId, QDate::currentDate(), false);
}

void MainWindow::goToPreviousDay()
{
    fetchProgrammes(m_currentChannelId, m_currentDate.addDays(-1), false);
}

void MainWindow::goToNextDay()
{
    fetchProgrammes(m_currentChannelId, m_currentDate.addDays(1), false);
}

void MainWindow::watchProgramme()
{
    if (m_currentProgramme.id < 0) {
        streamNotFound();
        return;
    }

    if (m_formatComboBox->currentIndex() == 1) { /* Flash-video */
        QString urlString = QString("http://www.tvkaista.fi/embed/%1")
                            .arg(m_currentProgramme.id);

        startFlashStream(QUrl(urlString));
        return;
    }

    m_downloading = false;
    m_client->sendStreamRequest(m_currentProgramme);
    startLoadingAnimation();
}

void MainWindow::downloadProgramme()
{
    if (m_currentProgramme.id < 0) {
        streamNotFound();
        return;
    }

    m_downloading = true;
    m_client->sendStreamRequest(m_currentProgramme);
    startLoadingAnimation();
}

void MainWindow::openScreenshotWindow()
{
    if (m_currentProgramme.id < 0) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(windowTitle());
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(trUtf8("Kuvakaappauksia ei ole saatavilla."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    if (m_screenshotWindow == 0) {
        m_screenshotWindow = new ScreenshotWindow(&m_settings, this);
        m_screenshotWindow->setClient(m_client);
    }
    else {
        m_screenshotWindow->activateWindow();
    }

    m_screenshotWindow->fetchScreenshots(m_currentProgramme);
    m_screenshotWindow->show();
}

void MainWindow::openSettingsDialog()
{
    if (m_settingsDialog != 0) {
        m_settingsDialog->show();
        m_settingsDialog->activateWindow();
        return;
    }

    m_settingsDialog = new SettingsDialog(&m_settings, this);
    connect(m_settingsDialog, SIGNAL(accepted()), SLOT(settingsAccepted()));
    m_settingsDialog->show();
}

void MainWindow::settingsAccepted()
{
    m_settingsDialog->deleteLater();
    m_settingsDialog = 0;
    loadClientSettings();
    updateFontSize();

    if (m_channels.isEmpty()) {
        fetchChannels(false);
    }
}

void MainWindow::openAboutDialog()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::openHelp()
{
    if (!QDesktopServices::openUrl(QUrl("http://helineva.net/tvkaistagui/ohjeet/"))) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(windowTitle());
        msgBox.setText(trUtf8("Web-sivun avaaminen epäonnistui. Katso ohjeet "
                              "osoitteesta http://helineva.net/tvkaistagui/ohjeet/"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
        return;
    }
}

void MainWindow::abortDownload()
{
    QModelIndexList indexes = ui->downloadsTableView->selectionModel()->selectedRows(0);

    for (int i = 0; i < indexes.size(); i++) {
        m_downloadTableModel->abortDownload(indexes.at(i).row());
    }

    ui->downloadsTableView->resizeRowsToContents();
    downloadSelectionChanged();
}

void MainWindow::removeDownload()
{
    QModelIndexList indexes = ui->downloadsTableView->selectionModel()->selectedRows(0);
    QList<int> rows;

    for (int i = 0; i < indexes.size(); i++) {
        rows.append(indexes.at(i).row());
    }

    qSort(rows);

    for (int i = rows.size() - 1; i >= 0; i--) {
        m_downloadTableModel->removeDownload(rows.at(i));
    }

    ui->downloadsTableView->resizeColumnToContents(0);
}

void MainWindow::refreshChannels()
{
    fetchChannels(true);
}

void MainWindow::playDownloadedFile()
{
    QModelIndexList indexes = ui->downloadsTableView->selectionModel()->selectedRows(0);

    if (indexes.isEmpty()) {
        return;
    }

    int row = indexes.at(0).row();
    QString filename = QDir::toNativeSeparators(m_downloadTableModel->filename(row));
    int format = m_downloadTableModel->videoFormat(row);
    m_settings.beginGroup("mediaPlayer");
    QString command = m_settings.value("file").toString();
    m_settings.endGroup();

    if (command.isEmpty()) {
        command = defaultFilePlayerCommand();
    }

    startMediaPlayer(command, filename, format);
}

void MainWindow::openDirectory()
{
    QModelIndexList indexes = ui->downloadsTableView->selectionModel()->selectedRows(0);

    if (indexes.isEmpty()) {
        return;
    }

    QString urlString = QFileInfo(m_downloadTableModel->filename(indexes.at(0).row())).absolutePath();

#ifdef Q_OS_WIN
    QProcess::startDetached("explorer.exe", QStringList() << QDir::toNativeSeparators(urlString));
#else
    urlString.prepend("file://");
    QDesktopServices::openUrl(QUrl(urlString));
#endif
}

void MainWindow::search()
{
    QString phrase = m_searchComboBox->currentText().trimmed();
    m_searchHistory.removeAll(phrase);
    m_searchHistory.prepend(phrase);

    while (m_searchHistory.size() > 10) {
        m_searchHistory.removeLast();
    }

    m_searchComboBox->clear();
    m_searchComboBox->addItems(m_searchHistory);

    if (phrase == m_searchPhrase) {
        if (!m_searchResultsVisible) {
            toggleSearchResults();
        }

        refreshProgrammes();
    }
    else {
        fetchSearchResults(phrase);
    }
}

void MainWindow::clearSearchHistory()
{
    m_searchHistory.clear();
    m_searchComboBox->clear();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(windowTitle());
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(trUtf8("Hakuhistoria on tyhjennetty."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::sortByTimeAsc()
{
    m_searchResultsTableModel->setSortKey(1, false);
}

void MainWindow::sortByTimeDesc()
{
    m_searchResultsTableModel->setSortKey(1, true);
}

void MainWindow::sortByTitleAsc()
{
    m_searchResultsTableModel->setSortKey(2, false);
}

void MainWindow::sortByTitleDesc()
{
    m_searchResultsTableModel->setSortKey(2, true);
}

void MainWindow::showProgrammeList()
{
    if (m_searchResultsVisible) {
        toggleSearchResults();
    }
    else {
        ui->actionProgrammeListButton->setChecked(true);
        ui->actionProgrammeList->setChecked(true);
    }
}

void MainWindow::showSearchResults()
{
    if (m_searchResultsVisible) {
        ui->actionSearchResultsButton->setChecked(true);
        ui->actionSearchResults->setChecked(true);
    }
    else {
        toggleSearchResults();
    }
}

void MainWindow::toggleSearchResults()
{
    m_searchResultsVisible = !m_searchResultsVisible;
    ui->calendarWidget->setVisible(!m_searchResultsVisible);
    ui->channelListWidget->setVisible(!m_searchResultsVisible);
    ui->currentDayPushButton->setVisible(!m_searchResultsVisible);
    ui->actionSearchResults->setChecked(m_searchResultsVisible);
    ui->actionSearchResultsButton->setChecked(m_searchResultsVisible);
    ui->actionProgrammeList->setChecked(!m_searchResultsVisible);
    ui->actionProgrammeListButton->setChecked(!m_searchResultsVisible);
    ui->descriptionTextEdit->setPlainText(QString());
    m_currentProgramme.id = -1;

    QItemSelectionModel *selectionModel = ui->programmeTableView->selectionModel();

    if (m_searchResultsVisible) {
        m_currentTableModel = m_searchResultsTableModel;
        ui->programmeTableView->setModel(m_searchResultsTableModel);
    }
    else {
        m_currentTableModel = m_programmeListTableModel;
        ui->programmeTableView->setModel(m_programmeListTableModel);
    }

    delete selectionModel;
    connect(ui->programmeTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(programmeSelectionChanged()));

    updateColumnSizes();
    updateWindowTitle();
    scrollProgrammes();
}

void MainWindow::toggleDownloadsDockWidget()
{
    ui->downloadsDockWidget->setVisible(!ui->downloadsDockWidget->isVisible());
}

void MainWindow::toggleShortcutsDockWidget()
{
    ui->shortcutsDockWidget->setVisible(!ui->shortcutsDockWidget->isVisible());
}

void MainWindow::selectChannel(int index)
{
    if (index < 0 || index >= m_channels.size()) {
        return;
    }

    fetchProgrammes(m_channels.at(index).id, m_currentDate, false);
    ui->channelListWidget->setCurrentIndex(ui->channelListWidget->model()->index(index, 0, QModelIndex()));
}

void MainWindow::setFocusToCalendar()
{
    if (m_searchResultsVisible) {
        toggleSearchResults();
    }

    ui->calendarWidget->setFocus();
}

void MainWindow::setFocusToChannelList()
{
    if (m_searchResultsVisible) {
        toggleSearchResults();
    }

    ui->channelListWidget->setFocus();
}

void MainWindow::channelsFetched(const QList<Channel> &channels)
{
    m_channels = channels;
    stopLoadingAnimation();
    updateChannelList();

    if (m_currentChannelId < 0 && !m_channels.isEmpty()) {
        ui->channelListWidget->setCurrentIndex(ui->channelListWidget->model()->index(0, 0, QModelIndex()));
    }
}

void MainWindow::programmesFetched(int channelId, const QDate &date, const QList<Programme> &programmes)
{
    m_currentChannelId = channelId;
    m_currentDate = date;

    if (m_searchResultsVisible) {
        toggleSearchResults();
    }

    if (programmes.isEmpty()) {
        m_programmeListTableModel->setInfoText(trUtf8("Ohjelmatietoja ei ole saatavilla valitulle päivälle."));
    }
    else {
        m_programmeListTableModel->setProgrammes(programmes);
    }

    stopLoadingAnimation();
    updateColumnSizes();
    updateWindowTitle();
    updateCalendar();
    scrollProgrammes();
}

void MainWindow::posterFetched(const Programme &programme, const QImage &poster)
{
    if (m_currentProgramme.id != programme.id) {
        return;
    }

    m_posterImage = poster;
    addBorderToPoster();
    updateDescription();
}

void MainWindow::streamUrlFetched(const Programme &programme, int format, const QUrl &url)
{
    stopLoadingAnimation();

    if (m_downloading) {
        int row = m_downloadTableModel->download(
                programme, format, m_channelMap.value(programme.channelId), url);
        ui->downloadsTableView->resizeColumnToContents(0);
        ui->downloadsTableView->resizeRowsToContents();
        ui->downloadsTableView->scrollTo(m_downloadTableModel->index(row, 0, QModelIndex()));
    }
    else {
        m_settings.beginGroup("mediaPlayer");
        QString command = m_settings.value("stream").toString();
        m_settings.endGroup();

        if (command.isEmpty()) {
            command = defaultStreamPlayerCommand();
        }

        startMediaPlayer(command, url.toString(), format);
    }
}

void MainWindow::searchResultsFetched(const QList<Programme> &programmes)
{
    if (programmes.isEmpty()) {
        m_searchResultsTableModel->setInfoText(trUtf8("Ei hakutuloksia"));
    }
    else {
        m_searchResultsTableModel->setProgrammes(programmes);
        updateColumnSizes();
        ui->programmeTableView->setFocus();
        scrollProgrammes();
    }

    stopLoadingAnimation();
}

void MainWindow::downloadFinished()
{
    ui->downloadsTableView->resizeRowsToContents();
}

void MainWindow::networkError()
{
    qWarning() << m_client->lastError();
    stopLoadingAnimation();
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(windowTitle());
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(trUtf8("Verkkovirhe: %1").arg(m_client->lastError()));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::loginError()
{
    stopLoadingAnimation();
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(windowTitle());
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(trUtf8("Virheellinen käyttäjätunnus tai salasana."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::streamNotFound()
{
    stopLoadingAnimation();
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(windowTitle());
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(trUtf8("Ohjelmaa ei ole saatavilla."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::fetchChannels(bool refresh)
{
    bool ok;
    QList<Channel> channels = m_cache->loadChannels(ok);

    if (ok && !refresh) {
        m_channels = channels;
        updateChannelList();
        return;
    }

    if (m_client->isValidUsernameAndPassword()) {
        m_client->sendChannelRequest();
        startLoadingAnimation();
    }
}

void MainWindow::fetchProgrammes(int channelId, const QDate &date, bool refresh)
{
    if (channelId < 0 || date.isNull()) {
        return;
    }

    bool ok;
    int age;
    QList<Programme> programmes = m_cache->loadProgrammes(channelId, date, ok, age);

    if (programmes.isEmpty()) {
        ok = false;
    }

    if (ok && (!refresh || age < 30)) {
        m_currentChannelId = channelId;
        m_currentDate = date;

        if (m_searchResultsVisible) {
            toggleSearchResults();
        }
        else {
            updateColumnSizes();
        }

        m_programmeListTableModel->setProgrammes(programmes);
        updateWindowTitle();
        updateCalendar();
        scrollProgrammes();
        return;
    }

    if (m_client->isValidUsernameAndPassword()) {
        m_client->sendProgrammeRequest(channelId, date);
        startLoadingAnimation();
    }
}

void MainWindow::fetchSearchResults(const QString &phrase)
{
    if (!m_client->isValidUsernameAndPassword()) {
        return;
    }

    m_client->sendSearchRequest(phrase);
    m_searchPhrase = phrase;
    startLoadingAnimation();

    if (!m_searchResultsVisible) {
        toggleSearchResults();
    }

    updateWindowTitle();
    updateCalendar();
}

bool MainWindow::fetchPoster()
{
    m_posterImage = m_cache->loadPoster(m_currentProgramme);

    if (m_posterImage.isNull()) {
        m_client->sendPosterRequest(m_currentProgramme);
        return false;
    }
    else {
        addBorderToPoster();
    }

    return true;
}

void MainWindow::loadClientSettings()
{
    m_settings.beginGroup("client");
    m_client->setUsername(m_settings.value("username").toString());
    m_client->setPassword(decodePassword(m_settings.value("password").toString()));

    m_settings.beginGroup("proxy");
    QNetworkProxy proxy;
    proxy.setHostName(m_settings.value("host").toString());
    proxy.setPort(qBound(0, m_settings.value("port", 8080).toInt(), 65535));

    if (proxy.hostName().isEmpty()) {
        qDebug() << "NoProxy";
        proxy.setType(QNetworkProxy::NoProxy);
    }
    else {
        qDebug() << "Proxy" << proxy.hostName() << proxy.port();
        proxy.setType(QNetworkProxy::HttpProxy);
    }

    m_client->setProxy(proxy);
    m_settings.endGroup();
    m_settings.endGroup();
}

void MainWindow::updateFontSize()
{
    m_settings.beginGroup("mainWindow");
    int size = m_settings.value("fontSize", 10).toInt();
    m_settings.endGroup();

    QFont font(ui->downloadsTableView->font());
    font.setPointSize(size);
    ui->downloadsTableView->setFont(font);
    ui->programmeTableView->setFont(font);
    ui->descriptionTextEdit->setFont(font);
    ui->calendarWidget->setFont(font);
    ui->channelListWidget->setFont(font);
    ui->watchPushButton->setFont(font);
    ui->downloadPushButton->setFont(font);
    ui->screenshotsPushButton->setFont(font);
    m_formatComboBox->setFont(font);
    m_searchComboBox->setFont(font);

    font = ui->titleLabel->font();
    font.setPointSize(size);
    ui->titleLabel->setFont(font);

    updateColumnSizes();

    int lineHeight = ui->programmeTableView->fontMetrics().height() + 4;
    ui->programmeTableView->verticalHeader()->setDefaultSectionSize(lineHeight);

    lineHeight = ui->downloadsTableView->fontMetrics().height() * 2 + 23;
    ui->downloadsTableView->verticalHeader()->setDefaultSectionSize(lineHeight);
    ui->downloadsTableView->resizeRowsToContents();

    QLabel* labels1[] = {ui->sh1Label, ui->sh2Label, ui->sh3Label, ui->sh4Label, ui->sh5Label,
                        ui->sh11Label, ui->sh12Label, ui->sh13Label, ui->sh14Label, ui->sh15label};

    QLabel* labels2[] = {ui->sh6Label, ui->sh7Label, ui->sh8Label, ui->sh9Label, ui->sh10Label,
                         ui->sh16Label, ui->sh17Label, ui->sh18Label, ui->sh19Label, ui->sh20Label};

    font = ui->sh1Label->font();
    font.setPointSize(size);

    for (int i = 0; i < 10; i++) {
        labels1[i]->setFont(font);
    }

    font = ui->sh6Label->font();
    font.setPointSize(size);

    for (int i = 0; i < 10; i++) {
        labels2[i]->setFont(font);
    }
}

void MainWindow::updateColumnSizes()
{
    QHeaderView *header = ui->programmeTableView->horizontalHeader();
    int timeWidth = ui->programmeTableView->fontMetrics().width(trUtf8("00.00AA"));

    if (m_searchResultsVisible) {
        int dateWidth = ui->programmeTableView->fontMetrics().width(trUtf8("AAA 00.00.0000AA"));
        int titleWidth = ui->programmeTableView->width() - dateWidth - timeWidth - 20;
        header->resizeSection(0, dateWidth);
        header->resizeSection(1, timeWidth);
        header->resizeSection(2, titleWidth);
    }
    else {
        header->resizeSection(0, timeWidth);
        ui->programmeTableView->resizeColumnToContents(1);
    }
}

void MainWindow::updateChannelList()
{
    ui->channelListWidget->clear();
    m_channelMap.clear();
    int count = m_channels.size();

    for (int i = 0; i < count; i++) {
        Channel channel = m_channels.at(i);
        m_channelMap.insert(channel.id, channel.name);
        new QListWidgetItem(channel.name, ui->channelListWidget);
    }
}

void MainWindow::updateDescription()
{
    QString html("<p><b>");
    html.append(Qt::escape(m_currentProgramme.title));
    html.append("</b> ");
    html.append(trUtf8("%1 kanavalta %2").arg(
            m_currentProgramme.startDateTime.toString(trUtf8("ddd d.M.yyyy 'klo' h.mm")),
            m_channelMap.value(m_currentProgramme.channelId)));
    html.append("</p>");
    html.append("</p><p>");
    html.append(Qt::escape(m_currentProgramme.description));
    html.append("</p>");

    if (m_currentProgramme.id >= 0 && (m_currentProgramme.flags & 0x08) == 0) {
        ui->descriptionTextEdit->document()->addResource(QTextDocument::ImageResource, QUrl("poster.jpg"), m_posterImage);
        html.prepend("<img style=\"float: right\" src=\"poster.jpg\" />");
    }

    ui->descriptionTextEdit->setHtml(html);
}

void MainWindow::updateWindowTitle()
{
    QListWidgetItem *item = ui->channelListWidget->currentItem();
    QString title;

    if (!m_searchResultsVisible && item != 0) {
        QString dateString = m_currentDate.toString("ddd d. MMMM yyyy");

        /* Ubuntu Linuxissa kuukauden lopussa -ta, Windowsissa ei. Lisätään, jos puuttuu */
        if (!dateString.contains("kuuta")) dateString.replace("kuu", "kuuta");

        title = QString("%2 %3").arg(item->text(), dateString);
    }
    else if (m_searchResultsVisible && !m_searchPhrase.isEmpty()) {
        title = trUtf8("Hakutulokset: %2").arg(m_searchPhrase);
    }

    ui->titleLabel->setText(title);
}

void MainWindow::updateCalendar()
{
    ui->calendarWidget->setSelectedDate(m_currentDate);
    QDate currentDate = QDate::currentDate();

    if (m_formattedDate != currentDate) {
        /* Poistetaan muotoilut */
        ui->calendarWidget->setDateTextFormat(QDate(), QTextCharFormat());

        /* Väritetään neljä edellistä viikkoa */
        QTextCharFormat textFormat;
        QDate date = currentDate;
        textFormat.setBackground(QColor(227, 246, 206));

        for (int i = 0; i < 4 * 7; i++) {
            date = date.addDays(-1);
            ui->calendarWidget->setDateTextFormat(date, textFormat);
        }

        /* Väritetään seuraava viikko */
        date = currentDate;
        textFormat.setBackground(QColor(248, 224, 224));

        for (int i = 0; i < 7; i++) {
            date = date.addDays(1);
            ui->calendarWidget->setDateTextFormat(date, textFormat);
        }

        /* Väritetään kuluva päivä */
        textFormat.setBackground(QColor(172, 250, 88));
        textFormat.setForeground(Qt::black);
        textFormat.setFontWeight(QFont::Bold);
        ui->calendarWidget->setDateTextFormat(currentDate, textFormat);

        m_formattedDate = currentDate;
    }
}

void MainWindow::setFormat(int format)
{
    m_client->setFormat(format);
    m_programmeListTableModel->setFormat(format);
    m_searchResultsTableModel->setFormat(format);
}

void MainWindow::scrollProgrammes()
{
    if (m_searchResultsVisible) {
        if (m_searchResultsTableModel->programmeCount() == 0) {
            return;
        }

        QModelIndex modelIndex = m_searchResultsTableModel->index(0, 0, QModelIndex());

        if (m_searchResultsTableModel->sortKey() == 1 && !m_searchResultsTableModel->isDescending()) {
            ui->programmeTableView->scrollToBottom();
            modelIndex = m_searchResultsTableModel->index(
                    m_currentTableModel->rowCount(QModelIndex()) - 1, 0, QModelIndex());
        }
        else {
            ui->programmeTableView->scrollToTop();
        }

        ui->programmeTableView->selectionModel()->select(modelIndex,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui->programmeTableView->setCurrentIndex(modelIndex);
    }
    else {
        int row = m_currentTableModel->defaultProgrammeIndex();

        if (row < 0) {
            ui->programmeTableView->selectionModel()->clear();
            ui->descriptionTextEdit->setPlainText(QString());
            m_currentProgramme.id = -1;
        }
        else {
            QModelIndex index = m_currentTableModel->index(row, 0, QModelIndex());
            ui->programmeTableView->scrollTo(index, QAbstractItemView::PositionAtCenter);
            ui->programmeTableView->selectionModel()->select(
                    index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            ui->programmeTableView->setCurrentIndex(index);
        }
    }
}

void MainWindow::startLoadingAnimation()
{
    m_loadLabel->setMovie(m_loadMovie);
    m_loadMovie->start();
}

void MainWindow::stopLoadingAnimation()
{
    m_loadMovie->stop();
    m_loadLabel->setMovie(0);
    m_loadLabel->setText(" ");
}

void MainWindow::addBorderToPoster()
{
    QImage image(m_posterImage.width() + 8, m_posterImage.height() + 8, QImage::Format_RGB32);
    QPainter painter(&image);
    painter.fillRect(m_noPosterImage.rect(), QBrush(ui->descriptionTextEdit->palette().color(QPalette::Base)));
    painter.drawImage(QPoint(4, 4), m_posterImage);
    m_posterImage = image;
}

void MainWindow::startFlashStream(const QUrl &url)
{
    m_settings.beginGroup("mediaPlayer");
    QString command = m_settings.value("flash").toString();
    m_settings.endGroup();

    if (command.isEmpty()) {
        if (!QDesktopServices::openUrl(url)) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(windowTitle());
            msgBox.setText(trUtf8("Web-selaimen avaaminen epäonnistui."));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
        }

        return;
    }

    QString urlString = url.toString();
    QStringList args = splitCommandLine(command);
    int count = args.size();

    for (int i = 0; i < count; i++) {
        QString arg = args.at(i);
        arg = arg.replace("%F", urlString);
        args.replace(i, arg);
    }

    QString program = QDir::fromNativeSeparators(args.takeFirst());
    qDebug() << program << args;

    if (!QProcess::startDetached(program, args)) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(windowTitle());
        msgBox.setText(trUtf8("Web-selaimen avaaminen epäonnistui. Tarkista asetukset."));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
}

void MainWindow::startMediaPlayer(const QString &command, const QString &filename, int format)
{
    m_settings.beginGroup("mediaPlayer");
    QString deinterlaceOptions = m_settings.value(
            "deinterlaceOptions", "--vout-filter=deinterlace --deinterlace-mode=linear").toString();
    m_settings.endGroup();

    if (format != 3) {
         /* Lomitus tarvitsee poistaa vain 8 Mbps videoformaatista */
         deinterlaceOptions = QString();
    }

    QString s = command;
    s.replace("%D", deinterlaceOptions);
    s.replace("%F", filename);

    QStringList args = splitCommandLine(s);
    QString program = QDir::fromNativeSeparators(args.takeFirst());
    qDebug() << program << args;

    if (!QProcess::startDetached(program, args)) {
        QString message = trUtf8("Soittimen käynnistys epäonnistui.");
        QUrl url;

#ifdef Q_OS_WIN
        message.append("\n\n");
        message.append(trUtf8("TV-ohjelmien katseluun tarvitaan VLC-mediasoitin. "
                              "Ohjelman voi ladata ilmaiseksi osoitteesta\n\n"
                              "http://www.videolan.org/vlc/download-windows.html\n"));
        url = QUrl("http://www.videolan.org/vlc/download-windows.html");
#endif

#ifdef Q_OS_MAC
        message.append("\n\n");
        message.append(trUtf8("TV-ohjelmien katseluun tarvitaan VLC-mediasoitin. "
                              "Ohjelman voi ladata ilmaiseksi osoitteesta\n\n"
                              "http://www.videolan.org/vlc/download-macosx.html\n"));
        url = QUrl("http://www.videolan.org/vlc/download-macosx.html");
#endif

#ifdef Q_OS_LINUX
        message.append("\n\n");
        message.append(trUtf8("TV-ohjelmien katseluun tarvitaan VLC-mediasoitin. "
                              "Ohjelma löytyy pakettienhallinnasta hakusanalla vlc."));
#endif

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(windowTitle());
        msgBox.setIcon(QMessageBox::Critical);

        if (url.isEmpty()) {
            msgBox.setText(message);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }
        else {
            message.append("\n");
            message.append(trUtf8("Siirrytäänkö lataussivulle?"));
            msgBox.setText(message);
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

            if (msgBox.exec() == QMessageBox::Yes) {
                QDesktopServices::openUrl(url);
            }
        }
    }
}

QStringList MainWindow::splitCommandLine(const QString &command)
{
    QStringList args;
    QString arg;
    int len = command.length();
    bool quotes = false;

    for (int i = 0; i < len; i++) {
        QChar c = command.at(i);

        if (c == '\'') {
            quotes = !quotes;
        }
        else if (!quotes && c == ' ') {
            if (!arg.isEmpty()) args.append(arg);
            arg.clear();
        }
        else {
            arg.append(c);
        }
    }

    if (!arg.isEmpty()) args.append(arg);
    return args;
}

QString MainWindow::addDefaultOptionsToVlcCommand(const QString &command)
{
    if (command.isEmpty()) {
        return command;
    }

    QString s = command;
    s.remove(" %F");
    s.remove(" %D");

    if (!s.contains("--sub-language")) {
        s.append(" --sub-language=fi");
    }

    if (!s.contains("--audio-language")) {
        s.append(" --audio-language=Finnish,Swedish,English");
    }

    s.append(" %D %F");
    return s;
}

QString MainWindow::encodePassword(const QString &password)
{
    return password.toUtf8().toBase64();
}

QString MainWindow::decodePassword(const QString &password)
{
    return QString::fromUtf8(QByteArray::fromBase64(password.toAscii()).data());
}

QString MainWindow::defaultStreamPlayerCommand()
{
    QStringList paths;
    paths << "/usr/bin/vlc"
          << "C:/Program Files/VideoLAN/VLC/vlc.exe"
          << "C:/Program Files (x86)/VideoLAN/VLC/vlc.exe"
          << "/Applications/VLC.app/Contents/MacOS/VLC";

    QString path = "vlc";

    for (QStringList::iterator iter = paths.begin(); iter != paths.end(); iter++) {
        QString p = *iter;

        if (QFileInfo(p).exists()) {
            path = p;
            break;
        }
    }

    path = QDir::toNativeSeparators(path);

    if (path.contains(' ')) {
        path.prepend('\'');
        path.append('\'');
    }

    path.append(" --fullscreen --sub-language=fi"
                " --audio-language=Finnish,Swedish,English"
                " %D %F");
    return path;
}

QString MainWindow::defaultFilePlayerCommand()
{
    return defaultStreamPlayerCommand();
}

QString MainWindow::defaultDownloadDirectory()
{
    QString dir = QDesktopServices::storageLocation(QDesktopServices::MoviesLocation);

    if (dir.isEmpty()) {
        dir = QDir::homePath();
    }

    return dir;
}

QString MainWindow::defaultFilenameFormat()
{
    return "%D_%A.%e";
}

QStringList MainWindow::videoFormats()
{
    QStringList formats;
    formats << "300 kbps MP4" << "1 Mbps Flash" << "2 Mbps MP4" << "8 Mbps TS";
    return formats;
}
