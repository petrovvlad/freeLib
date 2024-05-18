#ifdef QUAZIP_STATIC
#include "quazip/quazip/quazip.h"
#include "quazip/quazip/quazipfile.h"
#else
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#endif

#ifdef USE_DEJVULIBRE
#include <libdjvu/ddjvuapi.h>
#endif

#include <QActionGroup>
#include <QApplication>
#include <QBuffer>
#include <QButtonGroup>
#include <QByteArray>
#include <QCloseEvent>
#include <QCollator>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QDomDocument>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIODevice>
#include <QItemDelegate>
#include <QLabel>
#include <QLibraryInfo>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMultiMap>
#include <QNetworkProxy>
#include <QObject>
#include <QPainter>
#include <QProcess>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSettings>
#include <QSharedPointer>
#include <QSortFilterProxyModel>
#include <QSplashScreen>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSslSocket>
#include <QStandardPaths>
#include <QString>
#include <QStringBuilder>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QSystemTrayIcon>
#include <QTcpServer>
#include <QTextBrowser>
#include <QTextCodec>
#include <QTextStream>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QToolButton>
#include <QTranslator>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QVariant>
#include <QVector>
#include <QWidget>
#include <QWidgetAction>
#include <QXmlStreamReader>
#include <QtAlgorithms>
#include <QtConcurrent>
#include <algorithm>
#include <iostream>
#include <typeinfo>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QByteArrayView>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
#include <QHttpServer>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <QPasswordDigestor>
#endif
