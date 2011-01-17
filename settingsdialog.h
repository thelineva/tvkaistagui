#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
    class SettingsDialog;
}

class QSettings;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QSettings *settings, QWidget *parent = 0);
    ~SettingsDialog();

protected:
    void changeEvent(QEvent *e);

private slots:
    void acceptChanges();
    void openDirectoryDialog();
    void recoverPassword();
    void orderTVkaista();
    void toggleAdvancedSettings(bool checked);

private:
    void loadSettings();
    void saveSettings();
    Ui::SettingsDialog *ui;
    QSettings *m_settings;
};

#endif // SETTINGSDIALOG_H
