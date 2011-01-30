#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDate>
#include <QMainWindow>
#include <QSettings>
#include "channel.h"
#include "programme.h"

namespace Ui {
    class MainWindow;
}

class QComboBox;
class QLabel;
class QToolButton;
class QSignalMapper;
class Cache;
class DownloadTableModel;
class ProgrammeFeedParser;
class ProgrammeTableModel;
class ScreenshotWindow;
class SettingsDialog;
class TvkaistaClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    static QString encodePassword(const QString &password);
    static QString decodePassword(const QString &password);
    static QString defaultStreamPlayerCommand();
    static QString defaultFilePlayerCommand();
    static QString defaultDownloadDirectory();
    static QString defaultFilenameFormat();
    static QStringList videoFormats();

protected:
    void closeEvent(QCloseEvent *e);

private slots:
    void dateClicked(const QDate &date);
    void channelClicked(int index);
    void programmeDoubleClicked();
    void programmeSelectionChanged();
    void downloadSelectionChanged();
    void formatChanged();
    void programmeMenuRequested(const QPoint &point);
    void refreshProgrammes();
    void goToCurrentDay();
    void goToPreviousDay();
    void goToNextDay();
    void watchProgramme();
    void downloadProgramme();
    void openScreenshotWindow();
    void openSettingsDialog();
    void settingsAccepted();
    void openAboutDialog();
    void openHelp();
    void abortDownload();
    void removeDownload();
    void refreshChannels();
    void playDownloadedFile();
    void openDirectory();
    void search();
    void clearSearchHistory();
    void sortByTimeAsc();
    void sortByTimeDesc();
    void sortByTitleAsc();
    void sortByTitleDesc();
    void showProgrammeList();
    void showSearchResults();
    void showSeasonPasses();
    bool setCurrentView(int view);
    void toggleDownloadsDockWidget();
    void toggleShortcutsDockWidget();
    void selectChannel(int index);
    void setFocusToCalendar();
    void setFocusToChannelList();
    void addToSeasonPass();
    void channelsFetched(const QList<Channel> &channels);
    void programmesFetched(int channelId, const QDate &date, const QList<Programme> &programmes);
    void posterFetched(const Programme &programme, const QImage &poster);
    void streamUrlFetched(const Programme &programme, int format, const QUrl &url);
    void searchResultsFetched(const QList<Programme> &programmes);
    void seasonPassListFetched(const QList<Programme> &programmes);
    void addedToSeasonPass(bool ok);
    void removedFromSeasonPass(bool ok);
    void posterTimeout();
    void downloadFinished();
    void networkError();
    void loginError();
    void streamNotFound();

private:
    void fetchChannels(bool refresh);
    void fetchProgrammes(int channelId, const QDate &date, bool refresh);
    void fetchSearchResults(const QString &phrase);
    void fetchSeasonPasses(bool refresh);
    bool fetchPoster();
    void loadClientSettings();
    void updateFontSize();
    void updateColumnSizes();
    void updateChannelList();
    void updateDescription();
    void updateWindowTitle();
    void updateCalendar();
    void setFormat(int format);
    void scrollProgrammes();
    void startLoadingAnimation();
    void stopLoadingAnimation();
    void addBorderToPoster();
    void startFlashStream(const QUrl &url);
    void startMediaPlayer(const QString &command, const QString &filename, int format);
    QStringList splitCommandLine(const QString &command);
    static QString addDefaultOptionsToVlcCommand(const QString &command);
    Ui::MainWindow *ui;
    QComboBox *m_formatComboBox;
    QComboBox *m_searchComboBox;
    QLabel *m_loadLabel;
    QMovie *m_loadMovie;
    QTimer *m_posterTimer;
    QToolButton *m_searchToolButton;
    QSettings m_settings;
    TvkaistaClient *m_client;
    DownloadTableModel *m_downloadTableModel;
    ProgrammeTableModel *m_programmeListTableModel;
    ProgrammeTableModel *m_searchResultsTableModel;
    ProgrammeTableModel *m_seasonPassesTableModel;
    ProgrammeTableModel *m_currentTableModel;
    Cache *m_cache;
    SettingsDialog *m_settingsDialog;
    ScreenshotWindow *m_screenshotWindow;
    QList<Channel> m_channels;
    QMap<int, QString> m_channelMap;
    QStringList m_searchHistory;
    QString m_searchPhrase;
    QDateTime m_lastRefreshTime;
    int m_currentChannelId;
    QDate m_currentDate;
    Programme m_currentProgramme;
    QImage m_posterImage;
    QImage m_noPosterImage;
    QIcon m_searchIcon;
    QDate m_formattedDate;
    bool m_downloading;
    int m_currentView;
};

#endif // MAINWINDOW_H
