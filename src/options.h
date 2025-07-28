#ifndef OPTIONS_H
#define OPTIONS_H

#include <QApplication>
#include <QString>
#include <QStandardPaths>
#include <QSettings>

#include "utilites.h"


#if __has_include(<execution>) //checking to see if the <execution> header is there
#ifdef emit
#undef emit
#define NOQTEMIT
#endif
#include <execution>
#ifdef NOQTEMIT
#define emit
#endif
#endif //__has_include(<execution>)

enum SendType{ST_Device, ST_Mail};

struct ToolsOptions
{
    QString sPath; //приложение
    QString sArgs; //параметры
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

enum ExportFormat {
    asis = 0x01,
    fb2 = 0x02,
    epub = 0x04,
    azw3 = 0x08,
    mobi = 0x10,
    mobi7 = 0x20,
    pdf = 0x40
};

Q_DECLARE_METATYPE(ExportFormat)

struct ExportOptions
{
    void Save(QSharedPointer<QSettings> pSettings, bool bSavePasswords = true);
    void Load(QSharedPointer<QSettings> pSettings);
    void setDefault(const QString &_sName, ExportFormat _OtputFormat, bool _bDefault);

    inline static const QString sDefaultEexpFileName = u"%a/[[%s/][%n2] ]%b"_s;
    inline static const QString sDefaultDropcapsFont = u"sangha.ttf"_s;
    inline static const QString sDefaultAuthorName =  u"%nf %nm %nl"_s;
    inline static const QString sDefaultCoverLabel = u"%abbrs - %n2"_s;
    inline static const QString sDefaultBookTitle = u"(%abbrs %n2) %b"_s;

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
    void readPasswords();
    void savePasswords();

    static QByteArray passwordToHash(const QString& password, const QByteArray &salt);
    static QByteArray generateSalt();

    QString sAlphabetName;
    QString sUiLanguageName;
    QString sDatabasePath;

    qint64 nCacheSize;

    QString sSidebarFontFamaly;
    QString sBooksListFontFamaly;
    QString sAnnotationFontFamaly;
    quint8 nSidebarFontSize;
    quint8 nBooksListFontSize;
    quint8 nAnnotationFontSize;
    bool bUseSytemFonts;

    qint8 nIconTray;
    bool bTrayColor;
    bool bShowDeleted;
    bool bUseTag;
    bool bShowSplash;
    bool bStorePosition;
    bool bCloseDlgAfterExport;
    bool bUncheckAfterExport;
    bool bExtendedSymbols;

#ifdef USE_HTTSERVER
    const static ushort nDefaultHttpPort = 8080;
    const static ushort  nDefaultProxyPort = 8080;

    QString sOpdsUser;
    QByteArray baOpdsPasswordHash;
    QByteArray baOpdsPasswordSalt;
    QString sProxyHost;
    QString sProxyUser;
    QString sProxyPassword;
    QString sBaseUrl;
    quint16 nHttpPort;
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
    std::vector<QString> vFavoriteLanguges;

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
inline bool bTray;
inline bool bUseGui;


#ifdef __cpp_lib_execution
#ifdef USE_TBB
inline constexpr auto executionpolicy = std::execution::par;
#else
inline constexpr auto executionpolicy = std::execution::seq;
#endif //USE_TBB
#endif //__cpp_lib_execution
}



#endif // OPTIONS_H
