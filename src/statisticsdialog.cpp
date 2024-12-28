#include <QLocale>
#include <QStringBuilder>

#include "statisticsdialog.h"
#include "ui_statisticsdialog.h"
#include "library.h"
#include "utilites.h"

StatisticsDialog::StatisticsDialog(QWidget *parent) :
    QDialog(parent, Qt::Dialog | Qt::WindowTitleHint),
    ui(new Ui::StatisticsDialog)
{
    ui->setupUi(this);

    QLocale locale;
    SLib& lib = g::libs[g::idCurrentLib];
    QString sText;
    sText = u"<table>"_s
            u"<tr><td width=\"50%\">"_s % tr("Library name") % u"</td><td>"_s % lib.name % u"</td></tr>"_s
            u"<tr><td>"_s % tr("Version") % u"</td><td>"_s % lib.sVersion % u"</td></tr>"_s
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
            u"<tr><td>"_s % tr("Book count") % u"</td><td>"_s % locale.toString(lib.books.size()) % u"</td></tr>"_s
            u"<tr><td>"_s % tr("Author count") % u"</td><td>"_s % locale.toString(lib.authors.size() - 1) % u"</td></tr>"_s
            u"<tr><td>"_s % tr("Seria count") % u"</td><td>"_s % locale.toString(lib.serials.size()) % u"</td></tr>"_s
#else
            u"<tr><td>"_s % tr("Book count") % u"</td><td>"_s % locale.toString((uint)lib.books.size()) % u"</td></tr>"_s
            u"<tr><td>"_s % tr("Author count") % u"</td><td>"_s % locale.toString((uint)(lib.authors.size() - 1)) % u"</td></tr>"_s
            u"<tr><td>"_s % tr("Seria count") % u"</td><td>"_s % locale.toString((uint)lib.serials.size()) % u"</td></tr>"_s
#endif
            u"</table>"_s;
    ui->textEdit->setText(sText);
}

StatisticsDialog::~StatisticsDialog()
{
    delete ui;
}
