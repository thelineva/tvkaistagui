#include <QDebug>
#include <QFileInfo>
#include <QLocale>
#include <QSettings>
#include <QTimer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include "mainwindow.h"
#include "downloader.h"
#include "downloadtablemodel.h"

DownloadTableModel::DownloadTableModel(QSettings *settings, QObject *parent) :
    QAbstractTableModel(parent), m_settings(settings), m_timer(new QTimer(this))
{
    connect(m_timer, SIGNAL(timeout()), SLOT(updateDownloadProgress()));
}

int DownloadTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_downloads.size();
}

int DownloadTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant DownloadTableModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    if (row < 0 || row >= m_downloads.size()) {
        return QVariant();
    }

    FileDownload download = m_downloads.value(row);

    switch (role) {
    case Qt::DisplayRole:
        return download.title;

    case Qt::UserRole + 1:
        return download.dateTime.toString(tr("d.M.yyyy"));

    case Qt::UserRole + 2:
        return download.description;
    }

    return QVariant();
}

QVariant DownloadTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

Qt::ItemFlags DownloadTableModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int DownloadTableModel::download(const Programme &programme, int format, const QString &channelName, const QUrl &url)
{
    m_settings->beginGroup("downloads");
    QString dirPath = m_settings->value("directory").toString();
    QString filenameFormat = m_settings->value("filenameFormat").toString();
    m_settings->endGroup();
    bool filenameFromReply = false;

    if (dirPath.isEmpty()) {
        dirPath = MainWindow::defaultDownloadDirectory();
    }

    QString extension = "ts";

    if (format == 0 || format == 1) {
        extension = "mp4";
    }

    if (filenameFormat.isEmpty()) {
        filenameFromReply = true;
        filenameFormat = MainWindow::defaultFilenameFormat();
    }

    filenameFormat.replace("%A", removeInvalidCharacters(programme.title));
    filenameFormat.replace("%a", toAscii(programme.title));
    filenameFormat.replace("%C", removeInvalidCharacters(channelName));
    filenameFormat.replace("%c", toAscii(channelName));
    filenameFormat.replace("%d", programme.startDateTime.toString("dd"));
    filenameFormat.replace("%m", programme.startDateTime.toString("MM"));
    filenameFormat.replace("%Y", programme.startDateTime.toString("yyyy"));
    filenameFormat.replace("%H", programme.startDateTime.toString("hh"));
    filenameFormat.replace("%M", programme.startDateTime.toString("mm"));
    filenameFormat.replace("%S", programme.startDateTime.toString("ss"));
    filenameFormat.replace("%e", extension);

    Downloader *downloader = new Downloader(this);
    downloader->setFilename(QFileInfo(QString("%1/%2").arg(dirPath, filenameFormat)).absoluteFilePath());
    downloader->setFilenameFromReply(filenameFromReply);
    downloader->start(url);
    connect(downloader, SIGNAL(finished()), SLOT(downloadFinished()));
    connect(downloader, SIGNAL(networkError()), SLOT(downloadNetworkError()));

    FileDownload download;
    int index = m_downloads.size();
    beginInsertRows(QModelIndex(), index, index);
    download.title = programme.title;
    download.dateTime = programme.startDateTime;
    download.status = 0;
    download.description = trUtf8("Ladataan");
    download.downloader = downloader;
    m_downloads.append(download);
    endInsertRows();

    if (!m_timer->isActive()) {
        m_timer->start(1000);
    }

    return index;
}

void DownloadTableModel::abortDownload(int row)
{
    FileDownload download = m_downloads.at(row);

    if (download.downloader == 0) {
        return;
    }

    download.filename = download.downloader->filename();
    download.downloader->abort();
    download.downloader->deleteLater();
    download.downloader = 0;
    download.status = 2;
    download.description = trUtf8("Keskeytetty");
    m_downloads.replace(row, download);
    QModelIndex modelIndex = index(row, 0, QModelIndex());
    emit dataChanged(modelIndex, modelIndex);
}

void DownloadTableModel::abortAllDownloads()
{
    int count = m_downloads.size();

    for (int i = 0; i < count; i++) {
        if (m_downloads.at(i).downloader != 0) {
            abortDownload(i);
        }
    }
}

bool DownloadTableModel::hasUnfinishedDownloads() const
{
    int count = m_downloads.size();

    for (int i = 0; i < count; i++) {
        if (m_downloads.at(i).downloader != 0) {
            return true;
        }
    }

    return false;
}

void DownloadTableModel::removeDownload(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    FileDownload download = m_downloads.takeAt(index);
    endRemoveRows();

    if (download.downloader != 0) {
        download.downloader->abort();
        download.downloader->deleteLater();
    }
}

QString DownloadTableModel::filename(int index) const
{
    FileDownload download = m_downloads.at(index);

    if (download.downloader == 0) {
        return download.filename;
    }

    return download.downloader->filename();
}

int DownloadTableModel::status(int index) const
{
    return m_downloads.at(index).status;
}

bool DownloadTableModel::load()
{
    m_downloads.clear();
    QString dirPath = QFileInfo(m_settings->fileName()).path();
    QString filename = QString("%1/downloads.xml").arg(dirPath);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QXmlStreamReader reader(&file);

    if (!reader.readNextStartElement() || reader.name() != "downloads") {
        file.close();
        return false;
    }

    while (reader.readNextStartElement()) {
        if (reader.name() != "programme") {
            reader.skipCurrentElement();
        }

        QXmlStreamAttributes attrs = reader.attributes();
        FileDownload download;
        download.downloader = 0;
        download.status = attrs.value("status").toString().toInt();
        download.dateTime = QDateTime::fromString(attrs.value("dateTime").toString(), "yyyy-MM-dd'T'hh:mm:ss");

        if (download.status == 3) {
            download.description = trUtf8("Epäonnistunut");
        }
        else if (download.status == 2) {
            download.description = trUtf8("Keskeytetty");
        }
        else if (download.status == 1) {
            download.description = trUtf8("Valmis");
        }

        while (reader.readNextStartElement()) {
            if (reader.name() == "title") {
                download.title = reader.readElementText();
            }
            else if (reader.name() == "filename") {
                download.filename = reader.readElementText();
            }
        }

        int index = m_downloads.size();
        beginInsertRows(QModelIndex(), index, index);
        m_downloads.append(download);
        endInsertRows();
    }

    file.close();
    return true;
}

bool DownloadTableModel::save()
{
    QString dirPath = QFileInfo(m_settings->fileName()).path();
    QString filename = QString("%1/downloads.xml").arg(dirPath);
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("downloads");
    int count = m_downloads.size();

    for (int i = 0; i < count; i++) {
        FileDownload download = m_downloads.at(i);
        writer.writeStartElement("programme");
        writer.writeAttribute("status", QString::number(download.status));
        writer.writeAttribute("dateTime", download.dateTime.toString("yyyy-MM-dd'T'hh:mm:ss"));
        writer.writeTextElement("title", download.title);
        writer.writeTextElement("filename", download.filename);
        writer.writeEndElement(); // programme
    }

    writer.writeEndElement(); // downloads
    writer.writeEndDocument();
    file.close();
    return true;
}

void DownloadTableModel::updateDownloadProgress()
{
    int count = m_downloads.size();

    for (int i = 0; i < count; i++) {
        FileDownload download = m_downloads.at(i);

        if (download.downloader == 0) {
            continue;
        }

        Downloader *downloader = download.downloader;
        qint64 received = downloader->bytesReceived();
        qint64 total = downloader->bytesTotal();

        if (total <= 0) {
            download.description = formatBytes(received);
        }
        else {
            download.description = trUtf8("%1 / %2 (%3 %)").arg(formatBytes(received), formatBytes(total)).arg(
                                                                 qRound(received / (double)total * 100));
        }

        m_downloads.replace(i, download);
        QModelIndex modelIndex = index(i, 0, QModelIndex());
        emit dataChanged(modelIndex, modelIndex);
    }
}

void DownloadTableModel::downloadFinished()
{
    int count = m_downloads.size();
    int running = 0;

    for (int i = 0; i < count; i++) {
        FileDownload download = m_downloads.at(i);

        if (download.downloader != 0) {
            if (download.downloader->isFinished()) {
                download.filename = download.downloader->filename();
                download.downloader->deleteLater();
                download.downloader = 0;
                download.status = 1;
                download.description = trUtf8("Valmis");
                m_downloads.replace(i, download);
                QModelIndex modelIndex = index(i, 0, QModelIndex());
                emit dataChanged(modelIndex, modelIndex);
            }
            else {
                running++;
            }
        }
    }

    if (running == 0) {
        m_timer->stop();
    }
}

void DownloadTableModel::downloadNetworkError()
{
    int count = m_downloads.size();

    for (int i = 0; i < count; i++) {
        FileDownload download = m_downloads.at(i);

        if (download.downloader != 0 && download.downloader->hasError()) {
            download.downloader->deleteLater();
            download.downloader = 0;
            download.description = download.downloader->lastError();
            download.status = 3;
            m_downloads.replace(i, download);
            QModelIndex modelIndex = index(i, 2, QModelIndex());
            emit dataChanged(modelIndex, modelIndex);
        }
    }
}

QString DownloadTableModel::formatBytes(qint64 bytes) const
{
    if (bytes < 1024) {
        return trUtf8("%1 t").arg(bytes);
    }

    if (bytes < 1024 * 1024) {
        return trUtf8("%1 kt").arg(bytes / 1024);
    }

    if (bytes < 1024 * 1024 * 1024.0) {
        return trUtf8("%1 Mt").arg(QLocale::system().toString(bytes / (double)(1024 * 1024), 'f', 2));
    }

    return trUtf8("%1 Gt").arg(QLocale::system().toString(bytes / (double)(1024 * 1024 * 1024), 'f', 2));
}

QString DownloadTableModel::toAscii(const QString &s)
{
    QString norm = s.normalized(QString::NormalizationForm_D);
    QString r;
    int len = norm.length();
    QChar p = '-';

    for (int i = 0; i < len; i++) {
        QChar c = norm.at(i);

        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-' || c.isDigit()) {
            r.append(c);
            p = c;
        }
        else if (p != '_' && (c == ' ' || c == '_')) {
            r.append('_');
            p = '_';
        }
    }

    if (r.endsWith('_')) {
        r.chop(1);
    }

    return r;
}

QString DownloadTableModel::removeInvalidCharacters(const QString &s)
{
    QString invalid = "|\\!?*<\":>+[]/";
    QString r;
    int len = s.length();

    for (int i = 0; i < len; i++) {
        QChar c = s.at(i);

        if (!invalid.contains(c)) {
            r.append(c);
        }
    }

    return r;
}
