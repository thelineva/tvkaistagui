#include "texteditordialog.h"
#include "ui_texteditordialog.h"

TextEditorDialog::TextEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TextEditorDialog)
{
    ui->setupUi(this);
}

TextEditorDialog::~TextEditorDialog()
{
    delete ui;
}

void TextEditorDialog::changeEvent(QEvent *e)
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

void TextEditorDialog::setText(const QString &text)
{
    ui->plainTextEdit->setPlainText(text);
}

QString TextEditorDialog::text() const
{
    QString s = ui->plainTextEdit->toPlainText().trimmed();
    s.replace("\n", "");
    s.replace("\r", "");
    return s;
}
