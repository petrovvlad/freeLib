#ifndef OPTIONS_H
#define OPTIONS_H

#include <QApplication>
#include <QString>
#include <QStandardPaths>
#include <QSettings>

#include "utilites.h"

#ifdef emit
#undef emit
#define NOQTEMIT
#endif
#include <execution>
#ifdef NOQTEMIT
#define emit
#endif

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

enum ExportFormat {asis=1, epub, azw3, mobi, mobi7, pdf};
Q_DECLARE_METATYPE(ExportFormat)

struct ExportOptions
{
    void Save(QSharedPointer<QSettings> pSettings, bool bSavePasswords = true);
    void Load(QSharedPointer<QSettings> pSettings);
    void setDefault(const QString &_sName, ExportFormat _OtputFormat, bool _bDefault);

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
    QString sBookSeriesTitle;
    QString sAuthorSring;
    QString sCoverLabel;
    QString sSendTo;

    ExportFormat format;
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
    bool bUseForHttp;
    std::vector<FontExportOptions> vFontExportOptions;
};

struct Options
{
    void setDefault();
    void setExportDefault();
    void Load(QSharedPointer<QSettings> pSettings);
    void Save(QSharedPointer<QSettings> pSettings);

    static QByteArray passwordToHash(const QString& password, const QByteArray &salt);
    static QByteArray generateSalt();

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

private:
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    static constexpr qsizetype nPasswordSaltSize_ = 24;
    static constexpr quint64 nPasswordHashSize_ = 48;
#else
    static constexpr int nPasswordSaltSize_ = 8;
    static constexpr quint64 nPasswordHashSize_ = 20;
#endif

};

namespace g {
inline Options options;
inline bool bVerbose;
#ifdef USE_TBB
inline constexpr auto executionpolicy = std::execution::par;
#else
inline constexpr auto executionpolicy = std::execution::seq;
#endif
}



#endif // OPTIONS_H
