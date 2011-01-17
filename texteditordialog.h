#ifndef TEXTEDITORDIALOG_H
#define TEXTEDITORDIALOG_H

#include <QDialog>

namespace Ui {
    class TextEditorDialog;
}

class TextEditorDialog : public QDialog {
    Q_OBJECT
public:
    TextEditorDialog(QWidget *parent = 0);
    ~TextEditorDialog();
    void setText(const QString &text);
    QString text() const;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::TextEditorDialog *ui;
};

#endif // TEXTEDITORDIALOG_H
