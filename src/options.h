#ifndef OPTIONS_H
#define OPTIONS_H

#include <QApplication>
#include <QString>
#include <QStandardPaths>
#include <QSettings>

#include "utilites.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
constexpr qsizetype nPasswordSaltSize = 24;
constexpr quint64 nPasswordHashSize = 48;
#else
constexpr int nPasswordSaltSize = 8;
constexpr quint64 nPasswordHashSize = 20;
#endif
QByteArray generateSalt();
QByteArray passwordToHash(const QString& password,const  QByteArray& salt);

enum SendType{ST_Device, ST_Mail};

struct ToolsOptions
{
    QString sPath;
    QString sArgs;
    QString sExt;
};

struct FontExportOptions
{
    QString sFont;
    QString sFontB;
    QString sFontI;
    QString sFontBI;
    int nFontSize;
    bool bUse;
    quint8 nTag;
};

struct ExportOptions
{
    void Save(QSharedPointer<QSettings> pSettings, bool bSavePasswords = true);
    void Load(QSharedPointer<QSettings> pSettings);
    void setDefault(const QString &_sName, const QString &_sOtputFormat, bool _bDefault);

    constexpr const static char* sDefaultEexpFileName = "%a/%s/%n2 %b";
    constexpr const static char* sDefaultDropcapsFont = "sangha.ttf";
    constexpr const static char* sDefaultAuthorName =  "%nf %nm %nl";
    constexpr const static char* sDefaultCoverLabel = "%abbrs - %n2";
    constexpr const static char* sDefaultBookTitle = "(%abbrs %n2) %b";
    QString sName;
    QString sCurrentTool;

    QString sEmail;
    QString sEmailFrom;
    QString sEmailSubject;
    QString sEmailServer;
    QString sEmailUser;
    QString sEmailPassword;
    QString sDevicePath;
    QString sExportFileName;
    QString sUserCSS;
    QString sOutputFormat;
    QString sBookSeriesTitle;
    QString sAuthorSring;
    QString sCoverLabel;
    QString sSendTo;

    quint16 nEmailServerPort;
    quint8 nEmailPause;
    quint8 nMaxCaptionLevel;
    quint8 nContentPlacement;
    quint8 nFootNotes;
    quint8 nEmailConnectionType;
    quint8 nHyphenate;
    quint8 nVignette;
    bool bDefault;
    bool bOriginalFileName;
    bool bPostprocessingCopy;
    bool bDropCaps;
    bool bJoinSeries;
    bool bUserCSS;
    bool bSplitFile;
    bool bBreakAfterCupture;
    bool bAnnotation;
    bool bAskPath;
    bool bTransliteration;
    bool bRemovePersonal;
    bool bRepairCover;
    bool bMlToc;
    bool bSeriaTranslit;
    bool bAuthorTranslit;
    bool bCreateCover;
    bool bCreateCoverAlways;
    bool bAddCoverLabel;
    std::vector<FontExportOptions> vFontExportOptions;
};

struct Options
{
    void setDefault();
    void setExportDefault();
    void Load(QSharedPointer<QSettings> pSettings);
    void Save(QSharedPointer<QSettings> pSettings);


    QString sAlphabetName;
    QString sUiLanguageName;
    QString sDatabasePath;

    qint64 nCacheSize;

    qint8 nIconTray;
    qint8 nTrayColor;
    bool bShowDeleted;
    bool bUseTag;
    bool bShowSplash;
    bool bStorePosition;
    bool bCloseDlgAfterExport;
    bool bUncheckAfterExport;
    bool bExtendedSymbols;

#ifdef USE_HTTSERVER
    const static ushort nDefaultOpdsPort = 8080;
    const static ushort  nDefaultProxyPort = 8080;

    QString sOpdsUser;
    QByteArray baOpdsPasswordHash;
    QByteArray baOpdsPasswordSalt;
    QString sProxyHost;
    QString sProxyUser;
    QString sProxyPassword;
    QString sBaseUrl;
    int nHttpExport;
    quint16 nOpdsPort;
    quint16 nOpdsBooksPerPage;
    quint16 nProxyPort;
    quint8 nProxyType;
    bool bOpdsEnable;
    bool bOpdsShowCover;
    bool bOpdsShowAnotation;
    bool bOpdsNeedPassword;
#endif

    std::unordered_map<QString, QString> applications;
    std::unordered_map<QString, ToolsOptions> tools;
    std::vector<ExportOptions> vExportOptions;
};


extern Options options;

#endif // OPTIONS_H
