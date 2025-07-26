#include "options.h"

#include <QFileInfo>
#include <QStringBuilder>
#include <QSharedPointer>
#include <QFont>
#include <algorithm>
#include <unordered_set>
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <QPasswordDigestor>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

#ifdef USE_QTKEYCHAIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <qt6keychain/keychain.h>
#else
#include <qt5keychain/keychain.h>
#endif
#endif //USE_QTKEYCHAIN

#ifndef USE_QTKEYCHAIN
static const quint8 key[] = {1,65,245,245,235,2,34,61,0,32,54,12,66};
QString encodeStr(const QString& str)
{
    QByteArray arr(str.toUtf8());
    quint32 index = 0;
    for(int i = 0; i<arr.size(); i++)
    {
        arr[i] = arr[i] ^ key[index];
        index++;
        if(index >= sizeof(key) / sizeof(quint32))
            index = 0;
    }

    return QStringLiteral("#-#") + QString::fromLatin1(arr.toBase64());
}

QString decodeStr(const QString &str)
{
    if(str.left(3) != u"#-#")
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
#endif

QByteArray Options::generateSalt()
{
    QByteArray salt;
    salt.resize(nPasswordSaltSize_);  // Размер соли

    // Заполняем соль случайными данными
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    QRandomGenerator::global()->fillRange(reinterpret_cast<quint64*>(salt.data()), salt.size()/sizeof(quint64));
#else
    qsrand(QTime::currentTime().msec());
    for(int i=0; i<salt.size(); i++)
        salt[i] = qrand();
#endif
    return salt;
}

QByteArray Options::passwordToHash(const QString& password, const QByteArray &salt)
{
    const int iterations = 4046;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QByteArray derivedKey = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha512, password.toUtf8(), salt, iterations, nPasswordHashSize_);
#else
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(password.toUtf8());
    hash.addData( salt);
    QByteArray key = hash.result();
    for (int i = 1; i < iterations; i++) {
        hash.reset();
        hash.addData(
            key);
        key = hash.result();
    }
    QByteArray derivedKey = key.left(nPasswordHashSize_);
#endif
    return derivedKey;
}

void ExportOptions::Save(QSharedPointer<QSettings> pSettings, bool bSavePasswords)
{
    pSettings->setValue(QStringLiteral("ExportName"), sName);
    if(bSavePasswords)
    {
        pSettings->setValue(u"Default"_s, bDefault);
        pSettings->setValue(u"Email"_s, sEmail);
        pSettings->setValue(u"from_email"_s, sEmailFrom);
        pSettings->setValue(u"mail_subject"_s, sEmailSubject);
        pSettings->setValue(u"EmailServer"_s, sEmailServer);
        pSettings->setValue(u"EmailPort"_s, nEmailServerPort);
        pSettings->setValue(u"EmailUser"_s, sEmailUser);
        pSettings->setValue(u"PostprocessingCopy"_s, bPostprocessingCopy);
        pSettings->setValue(u"DevicePath"_s, sDevicePath);
        pSettings->setValue(u"PauseMail"_s, nEmailPause);
        pSettings->setValue(u"ConnectionType"_s, nEmailConnectionType);
        pSettings->setValue(u"sendTo"_s, sSendTo);
        pSettings->setValue(u"current_tool"_s, sCurrentTool);
    }
    pSettings->setValue(u"askPath"_s, bAskPath);
    pSettings->setValue(u"originalFileName"_s, bOriginalFileName);
    pSettings->setValue(u"ExportFileName"_s, sExportFileName);
    pSettings->setValue(u"OutputFormat"_s, format);
    pSettings->setValue(u"UseForHttpServer"_s, bUseForHttp);
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

    int countFont = vFontExportOptions.size();
    pSettings->beginWriteArray(u"fonts"_s, countFont);
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
        pSettings->setValue(QStringLiteral("fontSize"), fontExportOptions.nFontSize);
    }
    pSettings->endArray();
}

void ExportOptions::Load(QSharedPointer<QSettings> pSettings)
{
    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count()>0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);

    sName = pSettings->value(QStringLiteral("ExportName")).toString();
    bDefault = pSettings->value(QStringLiteral("Default"), false).toBool();
    sEmail = pSettings->value(QStringLiteral("Email")).toString();
    sEmailFrom = pSettings->value(QStringLiteral("from_email")).toString();
    sEmailSubject = pSettings->value(QStringLiteral("mail_subject")).toString();
    sEmailServer = pSettings->value(QStringLiteral("EmailServer")).toString();
    nEmailServerPort = pSettings->value(QStringLiteral("EmailPort"), 25).toUInt();
    sEmailUser = pSettings->value(QStringLiteral("EmailUser")).toString();
#ifndef USE_QTKEYCHAIN
    sEmailPassword =  decodeStr(pSettings->value(QStringLiteral("EmailPassword")).toString()); //TO DO: удалить позже
#endif
    bPostprocessingCopy = pSettings->value(QStringLiteral("PostprocessingCopy"), false).toBool();
    sDevicePath = pSettings->value(QStringLiteral("DevicePath"),HomeDir).toString();
    bOriginalFileName = pSettings->value(QStringLiteral("originalFileName"), false).toBool();
    sExportFileName = pSettings->value(QStringLiteral("ExportFileName"), sDefaultEexpFileName).toString();
    nEmailPause = pSettings->value(QStringLiteral("PauseMail"), 5).toUInt();
    nEmailConnectionType = pSettings->value(QStringLiteral("ConnectionType"), 0).toUInt();
    bDropCaps = pSettings->value(QStringLiteral("dropcaps"), false).toBool();
    bJoinSeries = pSettings->value(QStringLiteral("join_series"), false).toBool();
    nHyphenate = pSettings->value(QStringLiteral("hyphenate"), 0).toUInt();
    nVignette = pSettings->value(QStringLiteral("Vignette"), 0).toUInt();
    bUserCSS = pSettings->value(QStringLiteral("userCSS"), false).toBool();
    sUserCSS = pSettings->value(QStringLiteral("UserCSStext"), "").toString();
    bSplitFile = pSettings->value(u"split_file"_s, true).toBool();
    QVariant varOutputFormat = pSettings->value(u"OutputFormat"_s);
    QString sFormat = varOutputFormat.toString();
    if(sFormat == u"EPUB")
        format = epub;
    else if(sFormat == u"AZW3")
        format = azw3;
    else if(sFormat == u"MOBI")
        format = mobi;
    else if(sFormat == u"MOBI7")
        format = mobi7;
    else if(sFormat == u"-")
        format = asis;
    else
        format = varOutputFormat.value<ExportFormat>();
    bUseForHttp = pSettings->value(u"UseForHttpServer"_s, true).toBool();
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

void ExportOptions::setDefault(const QString &_sName, ExportFormat _OtputFormat, bool _bDefault)
{
    sName = _sName;
    format = _OtputFormat;
    bDefault = _bDefault;
    sSendTo = u"device"_s;
    nEmailServerPort = 25;
    bPostprocessingCopy = false;
    bOriginalFileName = false;
    sExportFileName = sDefaultEexpFileName;
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
    bUseForHttp = true;
    nContentPlacement = 0;
    vFontExportOptions.resize(1);
    FontExportOptions *pFontExportOptions = &vFontExportOptions[0];
    pFontExportOptions->bUse = true;
    pFontExportOptions->nTag = 2;  //"Dropcaps"
    pFontExportOptions->sFont = sDefaultDropcapsFont;
    pFontExportOptions->nFontSize = 300;
}

void Options::setDefault(){
    bShowDeleted = false;
    bUseTag = true;
    bShowSplash = true;
    bStorePosition = true;
    sUiLanguageName = QLocale::system().name();
    sAlphabetName = sUiLanguageName.left(2);
    setLocale(sUiLanguageName);
    QString sAppDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    sDatabasePath = sAppDir + u"/freeLib.sqlite"_s;
    nIconTray = 0;
    bTrayColor = true;
    bCloseDlgAfterExport = true;
    bUncheckAfterExport = true;
    bExtendedSymbols = false;
#ifdef USE_HTTSERVER
    bOpdsEnable = false;
    bOpdsShowCover = true;
    bOpdsShowAnotation = true;
    bOpdsNeedPassword = false;
    sOpdsUser = u""_s;
    baOpdsPasswordSalt.clear();
    baOpdsPasswordHash.clear();
    nHttpPort = nDefaultHttpPort;
    nOpdsBooksPerPage = 15;
    nProxyType = 0;
    nProxyPort = nDefaultProxyPort;
    sProxyHost = u""_s;
    sProxyUser = u""_s;
    sProxyPassword = u""_s;
#endif
    setExportDefault();
    tools.emplace(u"zip"_s, ToolsOptions{u"zip"_s, u"-9 -mj %f.zip %f"_s});
    bUseSytemFonts = true;
    QFont fontApp = QGuiApplication::font();
    QString sFontFamaly = fontApp.family();
    auto nSize = fontApp.pointSize();
    sSidebarFontFamaly = sFontFamaly;
    nSidebarFontSize = nSize;
    sBooksListFontFamaly = sFontFamaly;
    nBooksListFontSize = nSize;
    sAnnotationFontFamaly = sFontFamaly;
    nAnnotationFontSize = nSize;
}

void Options::setExportDefault()
{
    vExportOptions.clear();
    vExportOptions.resize(4);

    vExportOptions[0].setDefault(QApplication::tr("Save as") + u" ..."_s, asis, true);
    vExportOptions[1].setDefault(QApplication::tr("Save as") + u" MOBI"_s, mobi, false);
    vExportOptions[2].setDefault(QApplication::tr("Save as") + u" EPUB"_s, epub, false);
    vExportOptions[3].setDefault(QApplication::tr("Save as") + u" AZW3"_s, azw3, false);
}

void Options::Load(QSharedPointer<QSettings> pSettings)
{
    bShowDeleted = pSettings->value(QStringLiteral("ShowDeleted"), false).toBool();
    bUseTag = pSettings->value(QStringLiteral("use_tag"), true).toBool();
    bShowSplash = !pSettings->value(QStringLiteral("no_splash"), false).toBool();
    bStorePosition = pSettings->value(QStringLiteral("store_position"), true).toBool();
    sAlphabetName = pSettings->value(QStringLiteral("localeABC"), QLocale::system().name().left(2)).toString();
    sUiLanguageName = pSettings->value(QStringLiteral("localeUI"), QLocale::system().name()).toString();
    QString sAppDir = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst();
    sDatabasePath = pSettings->value(QStringLiteral("database_path"), QString(sAppDir + QStringLiteral("/freeLib.sqlite"))).toString();
    nIconTray = pSettings->value(QStringLiteral("tray_icon"), 0).toInt();
    bTrayColor = pSettings->value(QStringLiteral("tray_color"), 0).toBool();
    bCloseDlgAfterExport = pSettings->value(QStringLiteral("CloseExpDlg"), true).toBool();
    bUncheckAfterExport = pSettings->value(QStringLiteral("uncheck_export"), true).toBool();
    bExtendedSymbols = pSettings->value(QStringLiteral("extended_symbols"), false).toBool();
#ifdef USE_HTTSERVER
    bOpdsEnable = pSettings->value(QStringLiteral("OPDS_enable"), false).toBool();
    bOpdsShowCover = pSettings->value(QStringLiteral("srv_covers"), true).toBool();
    bOpdsShowAnotation = pSettings->value(QStringLiteral("srv_annotation"), true).toBool();
    bOpdsNeedPassword = pSettings->value(QStringLiteral("HTTP_need_pasword"), false).toBool();
    sOpdsUser = pSettings->value(u"HTTP_user"_s).toString();
    QString sSaltHashPassword = pSettings->value(u"httpPassword"_s).toString();
    if(!sSaltHashPassword.isEmpty()){
        auto vPassword = sSaltHashPassword.split(u":"_s);
        if(vPassword.size() == 2){
            baOpdsPasswordSalt = QByteArray::fromBase64(vPassword[0].toLatin1());
            baOpdsPasswordHash = QByteArray::fromBase64(vPassword[1].toLatin1());
            if(baOpdsPasswordSalt.size() != nPasswordSaltSize_ || baOpdsPasswordHash.size() != nPasswordHashSize_){
                baOpdsPasswordSalt.clear();
                baOpdsPasswordHash.clear();
            }
        }
    }else{
        QString sHttpPassword = pSettings->value(u"HTTP_password"_s).toString();
        if(!sHttpPassword.isEmpty()){
            baOpdsPasswordSalt = generateSalt();
            baOpdsPasswordHash = passwordToHash(sHttpPassword, baOpdsPasswordSalt);
            pSettings->setValue(u"httpPassword"_s, QString(baOpdsPasswordSalt.toBase64()) + u":"_s + QString(baOpdsPasswordHash.toBase64()));
            pSettings->remove(u"HTTP_password"_s);
        }
    }
    sBaseUrl = pSettings->value(u"BaseUrl"_s).toString();
    if(sBaseUrl.endsWith(u"/"))
        sBaseUrl.chop(1);
    nHttpPort = pSettings->value(u"OPDS_port"_s, 0).toInt();
    if(nHttpPort>0){
        pSettings->setValue(u"portHttp"_s, nHttpPort);
        pSettings->remove(u"OPDS_port"_s);
    }else
        nHttpPort = pSettings->value(u"portHttp"_s, nDefaultHttpPort).toInt();
    nOpdsBooksPerPage = pSettings->value(QStringLiteral("books_per_page"), 15).toInt();
    nProxyType = pSettings->value(QStringLiteral("proxy_type"), 0).toInt();
    nProxyPort = pSettings->value(QStringLiteral("proxy_port"), nDefaultProxyPort).toInt();
    sProxyHost = pSettings->value(QStringLiteral("proxy_host")).toString();
    sProxyUser = pSettings->value(QStringLiteral("proxy_password")).toString(); //TO DO: удалить позже
    sProxyPassword = pSettings->value(QStringLiteral("proxy_user")).toString();
#endif
    nCacheSize = pSettings->value(u"CacheSize"_s, 10).toLongLong()*1024*1024;

    int count = pSettings->beginReadArray(QStringLiteral("application"));
    for(int i=0; i<count; i++)
    {
        pSettings->setArrayIndex(i);
        QString sExt = pSettings->value(QStringLiteral("ext")).toString();
        QString sApp = pSettings->value(QStringLiteral("app")).toString();
        applications[sExt] = sApp;
    }
    pSettings->endArray();

    count = pSettings->beginReadArray(u"tools"_s);
    for(int i=0; i<count; i++)
    {
        pSettings->setArrayIndex(i);
        QString sName = pSettings->value(u"name"_s).toString();
        QString sPath = pSettings->value(u"path"_s).toString();
        QString sArgs = pSettings->value(u"args"_s).toString();
        tools[sName].sPath = sPath;
        tools[sName].sArgs = sArgs;
    }
    pSettings->endArray();

    if(g::bUseGui){
        QFont fontApp = QGuiApplication::font();
        if(pSettings->contains(u"useSystemFonts"_s)){ //временный блок
            bUseSytemFonts = pSettings->value(u"useSystemFonts"_s, true).toBool();
            sBooksListFontFamaly = pSettings->value(u"fontList"_s, fontApp.family()).toString();
            nBooksListFontSize = pSettings->value(u"fontListSize"_s, fontApp.pointSize()).toUInt();
            sAnnotationFontFamaly = pSettings->value(u"fontAnnotation"_s, fontApp.family()).toString();
            nAnnotationFontSize = pSettings->value(u"fontAnnotationSize"_s, fontApp.pointSize()).toUInt();
            sSidebarFontFamaly = sBooksListFontFamaly;
            nSidebarFontSize = nBooksListFontSize;

            pSettings->setValue(u"ui.font.usesystem"_s, bUseSytemFonts);
            pSettings->setValue(u"ui.font.sidebar"_s, sSidebarFontFamaly);
            pSettings->setValue(u"ui.font.size.sidebar"_s, nSidebarFontSize);
            pSettings->setValue(u"ui.font.bookslist"_s, sBooksListFontFamaly);
            pSettings->setValue(u"ui.font.sisze.booklist"_s, nBooksListFontSize);
            pSettings->setValue(u"ui.font.annotation"_s, sBooksListFontFamaly);
            pSettings->setValue(u"ui.font.size.annotation"_s, nBooksListFontSize);

            pSettings->remove(u"useSystemFonts"_s);
            pSettings->remove(u"fontList"_s);
            pSettings->remove(u"fontListSize"_s);
            pSettings->remove(u"fontAnnotation"_s);
            pSettings->remove(u"fontAnnotationSize"_s);
        }else{
            bUseSytemFonts = pSettings->value(u"ui.font.usesystem"_s, true).toBool();
            sSidebarFontFamaly = pSettings->value(u"ui.font.sidebar"_s, fontApp.family()).toString();
            nSidebarFontSize = pSettings->value(u"ui.font.size.sidebar"_s, fontApp.pointSize()).toUInt();
            sBooksListFontFamaly = pSettings->value(u"ui.font.bookslist"_s, fontApp.family()).toString();
            nBooksListFontSize = pSettings->value(u"ui.font.size.booklist"_s, fontApp.pointSize()).toUInt();
            sAnnotationFontFamaly = pSettings->value(u"ui.font.annotation"_s, fontApp.family()).toString();
            nAnnotationFontSize = pSettings->value(u"ui.font.size.annotation"_s, fontApp.pointSize()).toUInt();
        }
    }

    count = pSettings->beginReadArray(u"export"_s);
    vExportOptions.resize(count);
    std::unordered_set<ExportFormat> httpFormats;
    for(int i=0; i<count; i++)
    {
        pSettings->setArrayIndex(i);
        auto &exportOptions = vExportOptions[i];
        exportOptions.Load(pSettings);
#ifdef USE_HTTSERVER
        if(httpFormats.contains(exportOptions.format) && exportOptions.bUseForHttp)
            exportOptions.bUseForHttp = false;
        else
            httpFormats.insert(exportOptions.format);
#endif
    }
    pSettings->endArray();
    auto listFavoriteLang = pSettings->value(u"favoriteLanguges"_s).toStringList();
    std::ranges::copy(listFavoriteLang, std::back_inserter(vFavoriteLanguges));
}

void Options::Save(QSharedPointer<QSettings> pSettings)
{
    int countExport = vExportOptions.size();
    pSettings->beginWriteArray(u"export"_s, countExport);
    for(int iExport=0; iExport<countExport; iExport++){
        pSettings->setArrayIndex(iExport);
        vExportOptions[iExport].Save(pSettings);
    }
    pSettings->endArray();

    pSettings->setValue(u"localeUI"_s, sUiLanguageName);
    pSettings->setValue(u"localeABC"_s, sAlphabetName);
    pSettings->setValue(u"database_path"_s, sDatabasePath);
    pSettings->setValue(u"ShowDeleted"_s, bShowDeleted);
    pSettings->setValue(u"use_tag"_s, bUseTag);
    pSettings->setValue(u"no_splash"_s, !bShowSplash);
    pSettings->setValue(u"store_position"_s, bStorePosition);
    if(!g::bTray)
        pSettings->setValue(u"tray_icon"_s, nIconTray);
    pSettings->setValue(u"tray_color"_s, bTrayColor);
    pSettings->setValue(u"CloseExpDlg"_s,bCloseDlgAfterExport);
    pSettings->setValue(u"uncheck_export"_s,bUncheckAfterExport);
    pSettings->setValue(u"extended_symbols"_s, bExtendedSymbols);
#ifdef USE_HTTSERVER
    pSettings->setValue(u"OPDS_enable"_s, bOpdsEnable);
    pSettings->setValue(u"HTTP_need_pasword"_s, bOpdsNeedPassword);
    pSettings->setValue(u"HTTP_user"_s, sOpdsUser);
    pSettings->setValue(u"httpPassword"_s, QString(baOpdsPasswordSalt.toBase64()) + u":"_s + QString(baOpdsPasswordHash.toBase64()));
    pSettings->setValue(u"BaseUrl"_s, sBaseUrl);
    pSettings->setValue(u"srv_annotation"_s, bOpdsShowAnotation);
    pSettings->setValue(u"srv_covers"_s, bOpdsShowCover);
    pSettings->setValue(u"portHttp"_s, nHttpPort);
    pSettings->setValue(u"books_per_page"_s, nOpdsBooksPerPage);
    pSettings->setValue(u"proxy_type"_s, nProxyType);
    pSettings->setValue(u"proxy_port"_s, nProxyPort);
    pSettings->setValue(u"proxy_user"_s, sProxyUser);
    pSettings->setValue(u"proxy_host"_s, sProxyHost);
#endif

    pSettings->beginWriteArray(u"application"_s);
    int index = 0;
    for(const auto &iApp :applications)
    {
        pSettings->setArrayIndex(index);
        pSettings->setValue(u"ext"_s, iApp.first);
        pSettings->setValue(u"app"_s, iApp.second);
        ++index;
    }
    pSettings->endArray();

    pSettings->beginWriteArray(u"tools"_s);
    index = 0;
    for(const auto &iTool :tools){
        pSettings->setArrayIndex(index);
        pSettings->setValue(u"name"_s, iTool.first);
        pSettings->setValue(u"path"_s, iTool.second.sPath);
        pSettings->setValue(u"args"_s, iTool.second.sArgs);
        ++index;
    }
    pSettings->endArray();

    pSettings->setValue(u"ui.font.usesystem"_s, bUseSytemFonts);
    pSettings->setValue(u"ui.font.sidebar"_s, sSidebarFontFamaly);
    pSettings->setValue(u"ui.font.size.sidebar"_s, nSidebarFontSize);
    pSettings->setValue(u"ui.fonts.bookslist"_s, sBooksListFontFamaly);
    pSettings->setValue(u"ui.font.size.booklist"_s, nBooksListFontSize);
    pSettings->setValue(u"ui.font.annotation"_s, sAnnotationFontFamaly);
    pSettings->setValue(u"ui.font.size.annotation"_s, nAnnotationFontSize);
}

void Options::readPasswords()
{
#ifdef USE_QTKEYCHAIN
    QKeychain::ReadPasswordJob job(u"freelib"_s);
    job.setAutoDelete(false);
    QEventLoop loop;
    job.connect( &job, &QKeychain::ReadPasswordJob::finished, &loop, &QEventLoop::quit );
#ifdef USE_HTTSERVER
    job.setKey(u"proxy"_s);
    job.start();
    loop.exec();
    switch(job.error()){
    case QKeychain::NoError:
        sProxyPassword = job.textData();
        break;
    case QKeychain::EntryNotFound:
        break;
    default:
        LogWarning << "Error read password " <<  job.errorString();
    }
#endif //USE_HTTSERVER
    int countExport = vExportOptions.size();
    for(int iExport=0; iExport<countExport; iExport++){
        if(vExportOptions[iExport].sSendTo == u"e-mail"){
            job.setKey(u"smtp_"_s + QString::number(iExport+1));
            job.start();
            loop.exec();
            if (!job.error())
                vExportOptions[iExport].sEmailPassword = job.textData();
        }
    }
#else //USE_QTKEYCHAIN
    auto settings = GetSettings();
#ifdef USE_HTTSERVER
    settings->setValue(u"proxy_password"_s, sProxyPassword);
#endif //USE_HTTSERVER
    settings->beginReadArray(u"export"_s);
    int countExport = vExportOptions.size();
    for(int iExport=0; iExport<countExport; iExport++){
        if(vExportOptions[iExport].sSendTo == u"e-mail"){
            settings->setArrayIndex(iExport);
            vExportOptions[iExport].sEmailPassword =  decodeStr(settings->value(QStringLiteral("EmailPassword")).toString());
        }
    }
    settings->endArray();
#endif //USE_QTKEYCHAIN
}

void Options::savePasswords()
{
#ifdef USE_QTKEYCHAIN
    QKeychain::WritePasswordJob job( u"freelib"_s );
    job.setAutoDelete(false);
    QEventLoop loop;
    job.connect( &job, &QKeychain::WritePasswordJob::finished, &loop, &QEventLoop::quit  );
#ifdef USE_HTTSERVER
    job.setKey(u"proxy"_s);
    job.setTextData( sProxyPassword );
    job.start();
    loop.exec();
    if (job.error()) [[unlikely]]
        LogWarning << "Error save password" << job.errorString();
#endif //USE_HTTSERVER
    int countExport = vExportOptions.size();
    for(int iExport=0; iExport<countExport; iExport++){
        if(vExportOptions[iExport].sSendTo == u"e-mail"){
            job.setKey(u"smtp_"_s + QString::number(iExport+1));
            job.setTextData(vExportOptions[iExport].sEmailPassword);
            job.start();
            loop.exec();
        }
    }
#else //USE_QTKEYCHAIN
    auto settings = GetSettings();
#ifdef USE_HTTSERVER
    sProxyPassword = settings->value(u"proxy_user"_s).toString();
#endif //USE_HTTSERVER
    int countExport = vExportOptions.size();
    settings->beginWriteArray(u"export"_s, countExport);
    for(int iExport=0; iExport<countExport; iExport++){
        if(vExportOptions[iExport].sSendTo == u"e-mail"){
            settings->setArrayIndex(iExport);
            settings->setValue(u"EmailPassword"_s, encodeStr(vExportOptions[iExport].sEmailPassword));
        }
    }
    settings->endArray();
#endif //USE_QTKEYCHAIN
}


