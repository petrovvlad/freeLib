#define QT_USE_QSTRINGBUILDER
#include "options.h"

#include <QFileInfo>
#include <QStringBuilder>

static const quint8 key[] = {1,65,245,245,235,2,34,61,0,32,54,12,66};
QString encodeStr(const QString& str)
{
    QByteArray arr(str.toUtf8());
    quint32 index=0;
    for(int i =0; i<arr.size(); i++)
    {
        arr[i] = arr[i] ^ key[index];
        index++;
        if(index >= sizeof(key) / sizeof(quint32))
            index = 0;
    }

    return QLatin1String("#-#") + QString::fromLatin1(arr.toBase64());
}

QString decodeStr(const QString &str)
{
    if(str.left(3) != QLatin1String("#-#"))
        return str;
    QByteArray arr = QByteArray::fromBase64(str.mid(3).toLatin1());
    quint32 index = 0;
    for(int i =0; i<arr.size(); i++)
    {
        arr[i] = arr[i] ^ key[index];
        index++;
        if(index >= sizeof(key) / sizeof(quint32))
            index = 0;
    }
    return QString::fromUtf8(arr);
}

void ExportOptions::Save(QSettings *pSettings, bool bSavePasswords)
{
    pSettings->setValue(QStringLiteral("ExportName"), sName);
    if(bSavePasswords)
    {
        pSettings->setValue(QStringLiteral("Default"), bDefault);
        pSettings->setValue(QStringLiteral("Email"), sEmail);
        pSettings->setValue(QStringLiteral("from_email"), sEmailFrom);
        pSettings->setValue(QStringLiteral("mail_subject"), sEmailSubject);
        pSettings->setValue(QStringLiteral("EmailServer"), sEmailServer);
        pSettings->setValue(QStringLiteral("EmailPort"), nEmailServerPort);
        pSettings->setValue(QStringLiteral("EmailUser"), sEmailUser);
        pSettings->setValue(QStringLiteral("EmailPassword"), encodeStr(sEmailPassword));
        pSettings->setValue(QStringLiteral("PostprocessingCopy"), bPostprocessingCopy);
        pSettings->setValue(QStringLiteral("DevicePath"), sDevicePath);
        pSettings->setValue(QStringLiteral("PauseMail"), nEmailPause);
        pSettings->setValue(QStringLiteral("ConnectionType"), nEmailConnectionType);
        pSettings->setValue(QStringLiteral("sendTo"), sSendTo);
        pSettings->setValue(QStringLiteral("current_tool"), sCurrentTool);
    }
    pSettings->setValue(QStringLiteral("askPath"), bAskPath);
    pSettings->setValue(QStringLiteral("originalFileName"), bOriginalFileName);
    pSettings->setValue(QStringLiteral("ExportFileName"), sExportFileName);
    pSettings->setValue(QStringLiteral("OutputFormat"), sOutputFormat);
    pSettings->setValue(QStringLiteral("dropcaps"), bDropCaps);
    pSettings->setValue(QStringLiteral("join_series"), bJoinSeries);
    pSettings->setValue(QStringLiteral("hyphenate"), nHyphenate);
    pSettings->setValue(QStringLiteral("Vignette"), nVignette);
    pSettings->setValue(QStringLiteral("userCSS"), bUserCSS);
    pSettings->setValue(QStringLiteral("UserCSStext"), sUserCSS);
    pSettings->setValue(QStringLiteral("split_file"), bSplitFile);
    pSettings->setValue(QStringLiteral("break_after_cupture"), bBreakAfterCupture);
    pSettings->setValue(QStringLiteral("annotation"), bAnnotation);
    pSettings->setValue(QStringLiteral("footnotes"), nFootNotes);
    pSettings->setValue(QStringLiteral("transliteration"), bTransliteration);
    pSettings->setValue(QStringLiteral("removePersonal"), bRemovePersonal);
    pSettings->setValue(QStringLiteral("repairCover"), bRepairCover);
    pSettings->setValue(QStringLiteral("ml_toc"), bMlToc);
    pSettings->setValue(QStringLiteral("MAXcaptionLevel"), nMaxCaptionLevel);
    pSettings->setValue(QStringLiteral("authorTranslit"), bAuthorTranslit);
    pSettings->setValue(QStringLiteral("seriaTranslit"), bSeriaTranslit);
    pSettings->setValue(QStringLiteral("bookseriestitle"), sBookSeriesTitle);
    pSettings->setValue(QStringLiteral("authorstring"), sAuthorSring);
    pSettings->setValue(QStringLiteral("createCover"), bCreateCover);
    pSettings->setValue(QStringLiteral("createCoverAlways"), bCreateCoverAlways);
    pSettings->setValue(QStringLiteral("addCoverLabel"), bAddCoverLabel);
    pSettings->setValue(QStringLiteral("coverLabel"), sCoverLabel);
    pSettings->setValue(QStringLiteral("content_placement"), nContentPlacement);

    QStringList fonts_list;
    pSettings->beginWriteArray(QStringLiteral("fonts"));
    int countFont = vFontExportOptions.count();
    for (int iFont = 0; iFont < countFont; ++iFont)
    {
        pSettings->setArrayIndex(iFont);
        FontExportOptions &fontExportOptions = vFontExportOptions[iFont];

        pSettings->setValue(QStringLiteral("use"), fontExportOptions.bUse);
        pSettings->setValue(QStringLiteral("tag"), fontExportOptions.nTag);
        if(bSavePasswords)
        {
            pSettings->setValue(QStringLiteral("font"), fontExportOptions.sFont);
            pSettings->setValue(QStringLiteral("font_b"), fontExportOptions.sFontB);
            pSettings->setValue(QStringLiteral("font_i"), fontExportOptions.sFontI);
            pSettings->setValue(QStringLiteral("font_bi"), fontExportOptions.sFontBI);
        }
        else
        {
            pSettings->setValue(QStringLiteral("font"), QFileInfo(fontExportOptions.sFont).fileName());
            pSettings->setValue(QStringLiteral("font_b"), QFileInfo(fontExportOptions.sFontB).fileName());
            pSettings->setValue(QStringLiteral("font_i"), QFileInfo(fontExportOptions.sFontI).fileName());
            pSettings->setValue(QStringLiteral("font_bi"), QFileInfo(fontExportOptions.sFontBI).fileName());
        }
        pSettings->setValue(QStringLiteral("fontSize"),fontExportOptions.nFontSize);
    }
    pSettings->endArray();
}

void ExportOptions::Load(QSettings *pSettings)
{
    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir=QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);

    sName = pSettings->value(QStringLiteral("ExportName")).toString();
    bDefault = pSettings->value(QStringLiteral("Default"), false).toBool();
    sEmail = pSettings->value(QStringLiteral("Email")).toString();
    sEmailFrom = pSettings->value(QStringLiteral("from_email")).toString();
    sEmailSubject = pSettings->value(QStringLiteral("mail_subject")).toString();
    sEmailServer = pSettings->value(QStringLiteral("EmailServer")).toString();
    nEmailServerPort = pSettings->value(QStringLiteral("EmailPort"), 25).toUInt();
    sEmailUser = pSettings->value(QStringLiteral("EmailUser")).toString();
    sEmailPassword =  decodeStr(pSettings->value(QStringLiteral("EmailPassword")).toString());
    bPostprocessingCopy = pSettings->value(QStringLiteral("PostprocessingCopy"), false).toBool();
    sDevicePath = pSettings->value(QStringLiteral("DevicePath"),HomeDir).toString();
    bOriginalFileName = pSettings->value(QStringLiteral("originalFileName"), false).toBool();
    sExportFileName = pSettings->value(QStringLiteral("ExportFileName"), QLatin1String(sDefaultEexpFileName)).toString();
    nEmailPause = pSettings->value(QStringLiteral("PauseMail"), 5).toUInt();
    nEmailConnectionType = pSettings->value(QStringLiteral("ConnectionType"), 0).toUInt();
    bDropCaps = pSettings->value(QStringLiteral("dropcaps"), false).toBool();
    bJoinSeries = pSettings->value(QStringLiteral("join_series"), false).toBool();
    nHyphenate = pSettings->value(QStringLiteral("hyphenate"), 0).toUInt();
    nVignette = pSettings->value(QStringLiteral("Vignette"), 0).toUInt();
    bUserCSS = pSettings->value(QStringLiteral("userCSS"), false).toBool();
    sUserCSS = pSettings->value(QStringLiteral("UserCSStext"), "").toString();
    bSplitFile = pSettings->value(QStringLiteral("split_file"), true).toBool();
    sOutputFormat = pSettings->value(QStringLiteral("OutputFormat")).toString();
    bBreakAfterCupture = pSettings->value(QStringLiteral("break_after_cupture"), true).toBool();
    bAnnotation = pSettings->value(QStringLiteral("annotation"), false).toBool();
    nFootNotes = pSettings->value(QStringLiteral("footnotes"), 0).toUInt();
    bAskPath = pSettings->value(QStringLiteral("askPath"), true).toBool();
    bTransliteration = pSettings->value(QStringLiteral("transliteration"), false).toBool();
    bRemovePersonal = pSettings->value(QStringLiteral("removePersonal"), false).toBool();
    bRepairCover = pSettings->value(QStringLiteral("repairCover"), true).toBool();
    bMlToc = pSettings->value(QStringLiteral("ml_toc"), true).toBool();
    nMaxCaptionLevel = pSettings->value(QStringLiteral("MAXcaptionLevel"), 2).toUInt();
    bSeriaTranslit = pSettings->value(QStringLiteral("seriaTranslit"), false).toBool();
    bAuthorTranslit = pSettings->value(QStringLiteral("authorTranslit"), false).toBool();
    sBookSeriesTitle = pSettings->value(QStringLiteral("bookseriestitle")).toString();
    sAuthorSring = pSettings->value(QStringLiteral("authorstring")).toString();
    bCreateCover = pSettings->value(QStringLiteral("createCover"), false).toBool();
    bCreateCoverAlways = pSettings->value(QStringLiteral("createCoverAlways"), false).toBool();
    bAddCoverLabel = pSettings->value(QStringLiteral("addCoverLabel"), false).toBool();
    sCoverLabel = pSettings->value(QStringLiteral("coverLabel")).toString();
    sSendTo = pSettings->value(QStringLiteral("sendTo"), QStringLiteral("device")).toString();
    sCurrentTool = pSettings->value(QStringLiteral("current_tool")).toString();
    nContentPlacement = pSettings->value(QStringLiteral("content_placement"), 0).toUInt();
    int countFonts = pSettings->beginReadArray(QStringLiteral("fonts"));
    vFontExportOptions.resize(countFonts);
    for(int i=0; i<countFonts; i++)
    {
        pSettings->setArrayIndex(i);
        FontExportOptions &fontExport = vFontExportOptions[i];
        fontExport.bUse = pSettings->value(QStringLiteral("use")).toBool();
        fontExport.nTag = pSettings->value(QStringLiteral("tag")).toInt(),
        fontExport.sFont = pSettings->value(QStringLiteral("font")).toString(),
        fontExport.sFontB = pSettings->value(QStringLiteral("font_b")).toString(),
        fontExport.sFontI = pSettings->value(QStringLiteral("font_i")).toString(),
        fontExport.sFontBI = pSettings->value(QStringLiteral("font_bi")).toString(),
        fontExport.nFontSize = pSettings->value(QStringLiteral("fontSize"), 100).toInt();
    }
    pSettings->endArray();
}

void ExportOptions::setDefault(const QString &_sName, const QString &_sOtputFormat, bool _bDefault)
{
    sName = _sName;
    sOutputFormat = _sOtputFormat;
    bDefault = _bDefault;
    sSendTo = QStringLiteral("device");
    nEmailServerPort = 25;
    bPostprocessingCopy = false;
    bOriginalFileName = false;
    sExportFileName = QLatin1String(sDefaultEexpFileName);
    nEmailPause = 5;
    nEmailConnectionType = 0;
    bDropCaps = false;
    bJoinSeries = false;
    nHyphenate = 0;
    nVignette = 0;
    bUserCSS = false;
    bSplitFile = true;
    bBreakAfterCupture = true;
    bAnnotation = false;
    nFootNotes = 0;
    bAskPath = true;
    bTransliteration = false;
    bRemovePersonal = false;
    bRepairCover = true;
    bMlToc = true;
    nMaxCaptionLevel = 2;
    bSeriaTranslit = false;
    bAuthorTranslit = false;
    bCreateCover = false;
    bCreateCoverAlways = false;
    bAddCoverLabel = false;
    nContentPlacement = 0;
    vFontExportOptions.resize(1);
    FontExportOptions *pFontExportOptions = &vFontExportOptions[0];
    pFontExportOptions->bUse = true;
    pFontExportOptions->nTag = 2;  //"Dropcaps"
    pFontExportOptions->sFont = QLatin1String(sDefaultDropcapsFont);
    pFontExportOptions->nFontSize = 300;
}

void Options::setDefault(){
    bShowDeleted = false;
    bUseTag = true;
    bShowSplash = true;
    bStorePosition = true;
    sAlphabetName = QLocale::system().name().left(2);
    sUiLanguageName = QLocale::system().name();
    QString sAppDir = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst();
    sDatabasePath = sAppDir + QStringLiteral("/freeLib.sqlite");
    nIconTray = 0;
    nTrayColor = 0;
    bCloseDlgAfterExport = true;
    bUncheckAfterExport = true;
    bExtendedSymbols = false;
    bOpdsEnable = false;
    bBrowseDir = false;
    sDirForBrowsing = QLatin1String("");
    bOpdsShowCover = true;
    bOpdsShowAnotation = true;
    bOpdsNeedPassword = false;
    sOpdsUser = QLatin1String("");
    sOpdsPassword = QLatin1String("");
    nOpdsPort = nDefaultOpdsPort;
    nOpdsBooksPerPage = 15;
    nHttpExport = 0;
    nProxyType = 0;
    nProxyPort = nDefaultProxyPort;
    sProxyHost = QLatin1String("");
    sProxyUser = QLatin1String("");
    sProxyPassword = QLatin1String("");
    setExportDefault();
}

void Options::setExportDefault()
{
    vExportOptions.clear();
    vExportOptions.resize(4);

    vExportOptions[0].setDefault(QApplication::tr("Save as") + QStringLiteral(" ..."), QStringLiteral("-"), true);
    vExportOptions[1].setDefault(QApplication::tr("Save as") + QStringLiteral(" MOBI"), QStringLiteral("MOBI"), false);
    vExportOptions[2].setDefault(QApplication::tr("Save as") + QStringLiteral(" EPUB"), QStringLiteral("EPUB"), false);
    vExportOptions[3].setDefault(QApplication::tr("Save as") + QStringLiteral(" AZW3"), QStringLiteral("AZW3"), false);
}

void Options::Load(QSettings *pSettings)
{
    bShowDeleted =pSettings->value(QStringLiteral("ShowDeleted"), false).toBool();
    bUseTag=pSettings->value(QStringLiteral("use_tag"), true).toBool();
    bShowSplash = !pSettings->value(QStringLiteral("no_splash"), false).toBool();
    bStorePosition = pSettings->value(QStringLiteral("store_position"), true).toBool();
    sAlphabetName = pSettings->value(QStringLiteral("localeABC"), QLocale::system().name().left(2)).toString();
    sUiLanguageName = pSettings->value(QStringLiteral("localeUI"), QLocale::system().name()).toString();
    QString sAppDir = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst();
    sDatabasePath = pSettings->value(QStringLiteral("database_path"), QString(sAppDir + QLatin1String("/freeLib.sqlite"))).toString();
    nIconTray = pSettings->value(QStringLiteral("tray_icon"), 0).toInt();
    nTrayColor = pSettings->value(QStringLiteral("tray_color"), 0).toInt();
    bCloseDlgAfterExport = pSettings->value(QStringLiteral("CloseExpDlg"), true).toBool();
    bUncheckAfterExport = pSettings->value(QStringLiteral("uncheck_export"), true).toBool();
    bExtendedSymbols = pSettings->value(QStringLiteral("extended_symbols"), false).toBool();
    bOpdsEnable = pSettings->value(QStringLiteral("OPDS_enable"), false).toBool();
    bBrowseDir = pSettings->value(QStringLiteral("browseDir"), false).toBool();
    sDirForBrowsing = pSettings->value(QStringLiteral("dirForBrowsing")).toString();
    bOpdsShowCover = pSettings->value(QStringLiteral("srv_covers"), true).toBool();
    bOpdsShowAnotation = pSettings->value(QStringLiteral("srv_annotation"), true).toBool();
    bOpdsNeedPassword = pSettings->value(QStringLiteral("HTTP_need_pasword"), false).toBool();
    sOpdsUser = pSettings->value(QStringLiteral("HTTP_user")).toString();
    sOpdsPassword = pSettings->value(QStringLiteral("HTTP_password")).toString();
    nOpdsPort = pSettings->value(QStringLiteral("OPDS_port"), nDefaultOpdsPort).toInt();
    nOpdsBooksPerPage = pSettings->value(QStringLiteral("books_per_page"), 15).toInt();
    nHttpExport = pSettings->value(QStringLiteral("httpExport"), 0).toInt();
    nProxyType = pSettings->value(QStringLiteral("proxy_type"), 0).toInt();
    nProxyPort = pSettings->value(QStringLiteral("proxy_port"), nDefaultProxyPort).toInt();
    sProxyHost = pSettings->value(QStringLiteral("proxy_host")).toString();
    sProxyUser = pSettings->value(QStringLiteral("proxy_password")).toString();
    sProxyPassword = pSettings->value(QStringLiteral("proxy_user")).toString();

    int count = pSettings->beginReadArray(QStringLiteral("application"));
    for(int i=0;i<count;i++)
    {
        pSettings->setArrayIndex(i);
        QString sExt = pSettings->value(QStringLiteral("ext")).toString();
        QString sApp = pSettings->value(QStringLiteral("app")).toString();
        applications[sExt] = sApp;
    }
    pSettings->endArray();

    count=pSettings->beginReadArray(QStringLiteral("tools"));
    for(int i=0;i<count;i++)
    {
        pSettings->setArrayIndex(i);
        QString sName = pSettings->value(QStringLiteral("name")).toString();
        QString sPath = pSettings->value(QStringLiteral("path")).toString();
        QString sArgs = pSettings->value(QStringLiteral("args")).toString();
        QString sExt = pSettings->value(QStringLiteral("ext")).toString();
        tools[sName].sPath = sPath;
        tools[sName].sArgs = sArgs;
        tools[sName].sExt = sExt;
    }
    pSettings->endArray();

    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    count = pSettings->beginReadArray(QStringLiteral("export"));
    vExportOptions.resize(count);
    for(int i=0; i<count; i++)
    {
        pSettings->setArrayIndex(i);
        vExportOptions[i].Load(pSettings);
    }
    pSettings->endArray();
}

void Options::Save(QSettings *pSettings)
{
    int countExport = vExportOptions.count();
    pSettings->beginWriteArray(QStringLiteral("export"));
    for(int iExport=0; iExport<countExport; iExport++){
        pSettings->setArrayIndex(iExport);
        vExportOptions[iExport].Save(pSettings);
    }
    pSettings->endArray();

    pSettings->setValue(QStringLiteral("localeUI"), options.sUiLanguageName);
    pSettings->setValue(QStringLiteral("localeABC"), options.sAlphabetName);
    pSettings->setValue(QStringLiteral("database_path"), options.sDatabasePath);
    pSettings->setValue(QStringLiteral("ShowDeleted"), options.bShowDeleted);
    pSettings->setValue(QStringLiteral("use_tag"), options.bUseTag);
    pSettings->setValue(QStringLiteral("no_splash"), !options.bShowSplash);
    pSettings->setValue(QStringLiteral("store_position"), options.bStorePosition);
    pSettings->setValue(QStringLiteral("tray_icon"), options.nIconTray);
    pSettings->setValue(QStringLiteral("CloseExpDlg"),options.bCloseDlgAfterExport);
    pSettings->setValue(QStringLiteral("uncheck_export"),options.bUncheckAfterExport);
    pSettings->setValue(QStringLiteral("extended_symbols"), options.bExtendedSymbols);
    pSettings->setValue(QStringLiteral("OPDS_enable"), options.bOpdsEnable);
    pSettings->setValue(QStringLiteral("httpExport"), options.nHttpExport);
    pSettings->setValue(QStringLiteral("browseDir"), options.bBrowseDir);
    pSettings->setValue(QStringLiteral("dirForBrowsing"), options.sDirForBrowsing);
    pSettings->setValue(QStringLiteral("HTTP_need_pasword"), options.bOpdsNeedPassword);
    pSettings->setValue(QStringLiteral("HTTP_user"), options.sOpdsUser);
    pSettings->setValue(QStringLiteral("HTTP_password"), options.sOpdsPassword);
    pSettings->setValue(QStringLiteral("srv_annotation"), options.bOpdsShowAnotation);
    pSettings->setValue(QStringLiteral("srv_covers"), options.bOpdsShowCover);
    pSettings->setValue(QStringLiteral("OPDS_port"), options.nOpdsPort);
    pSettings->setValue(QStringLiteral("books_per_page"), options.nOpdsBooksPerPage);
    pSettings->setValue(QStringLiteral("proxy_type"), options.nProxyType);
    pSettings->setValue(QStringLiteral("proxy_port"), options.nProxyPort);
    pSettings->setValue(QStringLiteral("proxy_user"), options.sProxyUser);
    pSettings->setValue(QStringLiteral("proxy_host"), options.sProxyHost);
    pSettings->setValue(QStringLiteral("proxy_password"), options.sProxyPassword);

    pSettings->beginWriteArray(QStringLiteral("application"));
    auto iApp = options.applications.constBegin();
    int index = 0;
    while(iApp != options.applications.constEnd())
    {
        pSettings->setArrayIndex(index);
        pSettings->setValue(QStringLiteral("ext"), iApp.key());
        pSettings->setValue(QStringLiteral("app"), iApp.value());
        ++iApp;
        ++index;
    }
    pSettings->endArray();

    pSettings->beginWriteArray(QStringLiteral("tools"));
    auto iTool = options.tools.constBegin();
    index=0;
    while(iTool != options.tools.constEnd()){
        pSettings->setArrayIndex(index);
        pSettings->setValue(QStringLiteral("name"), iTool.key());
        pSettings->setValue(QStringLiteral("path"), iTool->sPath);
        pSettings->setValue(QStringLiteral("args"), iTool->sArgs);
        pSettings->setValue(QStringLiteral("ext"), iTool->sExt);
        ++index;
        ++iTool;
    }
    pSettings->endArray();
}


