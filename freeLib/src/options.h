#ifndef OPTIONS_H
#define OPTIONS_H

#include <QApplication>
#include <QString>
#include <QStandardPaths>
#include <QSettings>
#include <QVector>

enum SendType{ST_Device, ST_Mail};

struct ToolsOptions
{
    QString sPath;
    QString sArgs;
    QString sExt;
};

struct FontExportOptions
{
    bool bUse;
    int nTag;
    QString sFont;
    QString sFontB;
    QString sFontI;
    QString sFontBI;
    int nFontSize;
};

struct ExportOptions
{
    void Save(QSettings *pSettings, bool bSavePasswords=true);
    void Load(QSettings *pSettings);
    void setDefault(const QString &_sName, const QString &_sOtputFormat, bool _bDefault);

    constexpr const static char* sDefaultEexpFileName = "%a/%s/%n3%b";
    constexpr const static char* sDefaultDropcapsFont = "sangha.ttf";
    constexpr const static char* sDefaultAuthorName =  "%nf %nm %nl";
    constexpr const static char* sDefaultCoverLabel = "%abbrs - %n2";
    constexpr const static char* sDefaultBookTitle = "(%abbrs %n2) %b";
    QString sName;
    bool bDefault;
    QString sCurrentTool;

    QString sEmail;
    QString sEmailFrom;
    QString sEmailSubject;
    QString sEmailServer;
    quint16 nEmailServerPort;
    QString sEmailUser;
    QString sEmailPassword;
    quint32 nEmailPause;
    quint8  nEmailConnectionType;

    bool bPostprocessingCopy;
    QString sDevicePath;
    bool bOriginalFileName;
    QString sExportFileName;
    bool bDropCaps;
    bool bJoinSeries;
    int nHyphenate;
    int nVignette;
    bool bUserCSS;
    QString sUserCSS;
    bool bSplitFile;
    QString sOutputFormat;
    bool bBreakAfterCupture;
    bool bAnnotation;
    int nFootNotes;
    bool bAskPath;
    bool bTransliteration;
    bool bRemovePersonal;
    bool bRepairCover;
    bool bMlToc;
    int nMaxCaptionLevel;
    bool bSeriaTranslit;
    bool bAuthorTranslit;
    QString sBookSeriesTitle;
    QString sAuthorSring;
    bool bCreateCover;
    bool bCreateCoverAlways;
    bool bAddCoverLabel;
    QString sCoverLabel;
    QString sSendTo;
    int nContentPlacement;
    QVector<FontExportOptions> vFontExportOptions;
};

struct Options
{
    void setDefault();
    void setExportDefault();
    void Load(QSettings *pSettings);
    void Save(QSettings *pSettings);

    const static ushort nDefaultOpdsPort = 8080;
    const static ushort  nDefaultProxyPort = 8080;
    bool bShowDeleted;
    bool bUseTag;
    bool bShowSplash;
    bool bStorePosition;

    QString sAlphabetName;
    QString sUiLanguageName;
    QString sDatabasePath;

    bool bCloseDlgAfterExport;
    bool bUncheckAfterExport;
    bool bExtendedSymbols;

    QString sOpdsUser;
    QString sOpdsPassword;
    quint16 nOpdsPort;
    quint16 nOpdsBooksPerPage;
    bool bOpdsEnable;
    bool bOpdsShowCover;
    bool bOpdsShowAnotation;
    bool bOpdsNeedPassword;

    QString sProxyHost;
    QString sProxyUser;
    QString sProxyPassword;
    quint16 nProxyPort;
    quint8 nProxyType;
    int nHttpExport;
    bool bBrowseDir;
    QString sDirForBrowsing;

    qint8 nIconTray;
    qint8 nTrayColor;

    QHash <QString,QString> applications;
    QHash <QString,ToolsOptions> tools;
    QVector<ExportOptions> vExportOptions;
};


extern Options options;

#endif // OPTIONS_H
