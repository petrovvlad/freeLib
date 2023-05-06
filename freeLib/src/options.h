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

    constexpr const static char* sDefaultEexpFileName = "%a/%s/%n3%b";
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
    QVector<FontExportOptions> vFontExportOptions;
};

struct Options
{
    void setDefault();
    void setExportDefault();
    void Load(QSharedPointer<QSettings> pSettings);
    void Save(QSharedPointer<QSettings> pSettings);

    const static ushort nDefaultOpdsPort = 8080;
    const static ushort  nDefaultProxyPort = 8080;

    QString sAlphabetName;
    QString sUiLanguageName;
    QString sDatabasePath;
    QString sOpdsUser;
    QString sOpdsPassword;
    QString sProxyHost;
    QString sProxyUser;
    QString sProxyPassword;
    QString sDirForBrowsing;

    int nHttpExport;
    quint16 nOpdsPort;
    quint16 nOpdsBooksPerPage;
    quint16 nProxyPort;
    quint8 nProxyType;

    qint8 nIconTray;
    qint8 nTrayColor;
    bool bShowDeleted;
    bool bUseTag;
    bool bShowSplash;
    bool bStorePosition;
    bool bCloseDlgAfterExport;
    bool bUncheckAfterExport;
    bool bExtendedSymbols;
    bool bBrowseDir;
    bool bOpdsEnable;
    bool bOpdsShowCover;
    bool bOpdsShowAnotation;
    bool bOpdsNeedPassword;

    QHash <QString, QString> applications;
    QHash <QString, ToolsOptions> tools;
    QVector<ExportOptions> vExportOptions;
};


extern Options options;

#endif // OPTIONS_H
