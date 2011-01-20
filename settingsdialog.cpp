#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QUrl>
#include "mainwindow.h"
#include "texteditordialog.h"
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QSettings *settings, QWidget *parent) :
    QDialog(parent), ui(new Ui::SettingsDialog), m_settings(settings)
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), SLOT(acceptChanges()));
    connect(ui->downloadDirToolButton, SIGNAL(clicked()), SLOT(openDirectoryDialog()));
    connect(ui->recoverCommandLinkButton, SIGNAL(clicked()), SLOT(recoverPassword()));
    connect(ui->orderCommandLinkButton, SIGNAL(clicked()), SLOT(orderTVkaista()));
    connect(ui->advancedPushButton, SIGNAL(toggled(bool)), SLOT(toggleAdvancedSettings(bool)));
    connect(ui->filePlayerToolButton, SIGNAL(clicked()), SLOT(editFilePlayerPath()));
    connect(ui->streamPlayerToolButton, SIGNAL(clicked()), SLOT(editStreamPlayerPath()));
    connect(ui->flashPlayerToolButton, SIGNAL(clicked()), SLOT(editFlashPlayerPath()));
    QStringList fontSizeOptions;
    fontSizeOptions << trUtf8("Pieni") << trUtf8("Normaali") << trUtf8("Suuri") << trUtf8("Erittäin suuri");
    QStringList doubleClickOptions;
    doubleClickOptions << trUtf8("Suoratoistetaan ohjelma") << trUtf8("Ladataan ohjelma");
    ui->fontSizeComboBox->addItems(fontSizeOptions);
    ui->doubleClickComboBox->addItems(doubleClickOptions);
    toggleAdvancedSettings(false);
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

bool SettingsDialog::isUsernameChanged() const
{
    return m_usernameChanged;
}

void SettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SettingsDialog::acceptChanges()
{
    saveSettings();
    accept();
}

void SettingsDialog::openDirectoryDialog()
{
    QString dirPath = QDir::fromNativeSeparators(ui->downloadDirLineEdit->text());
    dirPath = QFileDialog::getExistingDirectory(this, trUtf8("Valitse hakemisto"), dirPath);

    if (!dirPath.isEmpty()) {
        ui->downloadDirLineEdit->setText(QDir::toNativeSeparators(dirPath));
    }
}

void SettingsDialog::recoverPassword()
{
    if (!QDesktopServices::openUrl(QUrl("http://www.tvkaista.fi/login/recover/"))) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(windowTitle());
        msgBox.setText(trUtf8("Web-sivun avaaminen epäonnistui. Palauta salasana osoitteessa "
                              "http://wwww.tvkaista.fi/login/recover/"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
        return;
    }
}

void SettingsDialog::orderTVkaista()
{
    if (!QDesktopServices::openUrl(QUrl("http://www.tvkaista.fi/order/"))) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(windowTitle());
        msgBox.setText(trUtf8("Web-sivun avaaminen epäonnistui. Tilaa TV-kaista osoitteesta "
                              "http://wwww.tvkaista.fi/order/"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
        return;
    }
}

void SettingsDialog::toggleAdvancedSettings(bool checked)
{
    ui->advancedSettingsLine->setVisible(checked);
    ui->proxyLabel->setVisible(checked);
    ui->proxyHostLineEdit->setVisible(checked);
    ui->proxyPortLabel->setVisible(checked);
    ui->proxyPortSpinBox->setVisible(checked);
}

void SettingsDialog::editFilePlayerPath()
{
    TextEditorDialog dialog(this);
    dialog.setText(ui->filePlayerLineEdit->text());

    if (dialog.exec() == QDialog::Accepted) {
        ui->filePlayerLineEdit->setText(dialog.text());
    }
}

void SettingsDialog::editStreamPlayerPath()
{
    TextEditorDialog dialog(this);
    dialog.setText(ui->streamPlayerLineEdit->text());

    if (dialog.exec() == QDialog::Accepted) {
        ui->streamPlayerLineEdit->setText(dialog.text());
    }
}

void SettingsDialog::editFlashPlayerPath()
{
    TextEditorDialog dialog(this);
    dialog.setText(ui->flashPlayerLineEdit->text());

    if (dialog.exec() == QDialog::Accepted) {
        ui->flashPlayerLineEdit->setText(dialog.text());
    }
}

void SettingsDialog::loadSettings()
{
    m_settings->beginGroup("client");
    ui->usernameLineEdit->setText(m_settings->value("username").toString());
    ui->passwordLineEdit->setText(MainWindow::decodePassword(m_settings->value("password").toString()));

    m_settings->beginGroup("proxy");
    ui->proxyHostLineEdit->setText(m_settings->value("host").toString());
    ui->proxyPortSpinBox->setValue(m_settings->value("port", 8080).toInt());
    m_settings->endGroup();
    m_settings->endGroup();

    m_settings->beginGroup("mainWindow");
    int fontSize = m_settings->value("fontSize", 10).toInt();
    int fontIndex = 1;

    if (fontSize <= 8) fontIndex = 0;
    else if (fontSize <= 10) fontIndex = 1;
    else if (fontSize <= 16) fontIndex = 2;
    else if (fontSize <= 22) fontIndex = 3;

    ui->fontSizeComboBox->setCurrentIndex(fontIndex);

    if (m_settings->value("doubleClick").toString() == "download") {
        ui->doubleClickComboBox->setCurrentIndex(1);
    }
    else {
        ui->doubleClickComboBox->setCurrentIndex(0);
    }

    ui->posterCheckBox->setChecked(m_settings->value("posterVisible", true).toBool());
    m_settings->endGroup();

    m_settings->beginGroup("downloads");
    QString dir = m_settings->value("directory").toString();
    QString filenameFormat = m_settings->value("filenameFormat").toString();
    m_settings->endGroup();

    m_settings->beginGroup("mediaPlayer");
    QString streamCommand = m_settings->value("stream").toString();
    QString fileCommand = m_settings->value("file").toString();
    QString flashCommand = m_settings->value("flash").toString();
    m_settings->endGroup();

    if (dir.isEmpty()) {
        dir = MainWindow::defaultDownloadDirectory();
    }

    ui->downloadDirLineEdit->setText(dir);
    ui->filenameFormatLineEdit->setText(filenameFormat);

    if (streamCommand.isEmpty()) {
        streamCommand = MainWindow::defaultStreamPlayerCommand();
    }

    if (fileCommand.isEmpty()) {
        fileCommand = MainWindow::defaultFilePlayerCommand();
    }

    ui->streamPlayerLineEdit->setText(streamCommand);
    ui->streamPlayerLineEdit->setCursorPosition(0);
    ui->filePlayerLineEdit->setText(fileCommand);
    ui->filePlayerLineEdit->setCursorPosition(0);
    ui->flashPlayerLineEdit->setText(flashCommand);
    ui->flashPlayerLineEdit->setCursorPosition(0);
}

void SettingsDialog::saveSettings()
{
    m_settings->beginGroup("client");
    QString username = ui->usernameLineEdit->text();
    m_usernameChanged = m_settings->value("username") != username;
    m_settings->setValue("username", username);
    m_settings->setValue("password", MainWindow::encodePassword(ui->passwordLineEdit->text()));

    m_settings->beginGroup("proxy");
    m_settings->setValue("host", ui->proxyHostLineEdit->text());
    m_settings->setValue("port", ui->proxyPortSpinBox->value());
    m_settings->endGroup();
    m_settings->endGroup();

    m_settings->beginGroup("mainWindow");
    int fontIndex = ui->fontSizeComboBox->currentIndex();
    int fontSize = 10;

    if (fontIndex == 0) fontSize = 8;
    else if (fontIndex == 1) fontSize = 10;
    else if (fontIndex == 2) fontSize = 16;
    else if (fontIndex == 3) fontSize = 22;

    m_settings->setValue("fontSize", fontSize);

    if (ui->doubleClickComboBox->currentIndex() == 0) {
        m_settings->setValue("doubleClick", "stream");
    }
    else {
        m_settings->setValue("doubleClick", "download");
    }

    m_settings->setValue("posterVisible", ui->posterCheckBox->isChecked());
    m_settings->endGroup();

    m_settings->beginGroup("downloads");
    m_settings->setValue("directory", ui->downloadDirLineEdit->text());
    m_settings->setValue("filenameFormat", ui->filenameFormatLineEdit->text());
    m_settings->endGroup();

    m_settings->beginGroup("mediaPlayer");
    m_settings->setValue("stream", ui->streamPlayerLineEdit->text());
    m_settings->setValue("file", ui->filePlayerLineEdit->text());
    m_settings->setValue("flash", ui->flashPlayerLineEdit->text());
    m_settings->endGroup();
}
