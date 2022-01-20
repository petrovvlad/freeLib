#-------------------------------------------------
#
# Project created by QtCreator 2012-07-20T12:41:42
#
#-------------------------------------------------


#contains(sql-drivers,sqlite):include("$${_PRO_FILE_PWD_}/../../sqlite/qsql_sqlite.pri")
#QT       += core
QT += sql xml network widgets xmlpatterns concurrent printsupport gui webenginewidgets
TARGET = freeLib
TEMPLATE = app

LIBS += -lz

#LIBS += -lcrypto -lssl
#LIBS += /opt/local/lib/libssl.a
#LIBS += /opt/local/lib/libcrypto.a

#LIBS += -lsqlite3

CONFIG(release, debug|release) {
build_nr.commands = ../../freeLib/src/build_number.sh
build_nr.depends = FORCE
QMAKE_EXTRA_TARGETS += build_nr
PRE_TARGETDEPS += build_nr
}

#TRANSLATIONS += ru.ts
DEFINES+= QUAZIP_STATIC

TRANSLATIONS += language_ru.ts language_uk.ts language_en.ts

macx{
#LIBS += $$OUT_PWD/../zlib/libz.a
ICON = $${_PRO_FILE_PWD_}/library.icns
QMAKE_POST_LINK  = "cp -R $${_PRO_FILE_PWD_}/xsl $$OUT_PWD/$${TARGET}.$${TEMPLATE}/Contents/MacOS"
QMAKE_POST_LINK += " && cp -f $${_PRO_FILE_PWD_}/../../KindleGen_Mac/kindlegen $$OUT_PWD/$${TARGET}.$${TEMPLATE}/Contents/MacOS/xsl/kindlegen"
QMAKE_POST_LINK += " && cp -R $${_PRO_FILE_PWD_}/Help $$OUT_PWD/$${TARGET}.$${TEMPLATE}/Contents/MacOS"
QMAKE_POST_LINK += " && mkdir -p $$OUT_PWD/$${TARGET}.$${TEMPLATE}/Contents/MacOS/language"
QMAKE_POST_LINK += " && cp -R $${_PRO_FILE_PWD_}/*.qm $$OUT_PWD/$${TARGET}.$${TEMPLATE}/Contents/MacOS/language"
QMAKE_POST_LINK += " && cp -R $${_PRO_FILE_PWD_}/ABC/*.txt $$OUT_PWD/$${TARGET}.$${TEMPLATE}/Contents/MacOS/language"
#QMAKE_POST_LINK += " && rcc -binary $${_PRO_FILE_PWD_}/resource.qrc -o $$OUT_PWD/resource.rcc"

QMAKE_INFO_PLIST=Info.Plist

QMAKE_LFLAGS += -framework Cocoa
OBJECTIVE_SOURCES += myapplicationclass.mm
QMAKE_MAC_SDK = macosx10.11

}

linux-g++{
QMAKE_POST_LINK  = "cp -R $${_PRO_FILE_PWD_}/xsl $$OUT_PWD"
QMAKE_POST_LINK += " && cp -f $${_PRO_FILE_PWD_}/../../KindleGenLinux/kindlegen $$OUT_PWD/xsl/kindlegen"
QMAKE_POST_LINK += " && cp -R $${_PRO_FILE_PWD_}/Help $$OUT_PWD"
QMAKE_POST_LINK += " && mkdir -p $$OUT_PWD/language"
QMAKE_POST_LINK += " && cp -R $${_PRO_FILE_PWD_}/*.qm $$OUT_PWD/language"
QMAKE_POST_LINK += " && cp -R $${_PRO_FILE_PWD_}/ABC/*.txt $$OUT_PWD/language"
}

win32{
    RC_FILE = $${_PRO_FILE_PWD_}/win.rc
#    QMAKE_POST_LINK  = "xcopy /y /e /i \"$${_PRO_FILE_PWD_}/xsl\" \"$$OUT_PWD)/release\""
#    QMAKE_POST_LINK  = "xcopy /y /e /i \"$${_PRO_FILE_PWD_}/../../kindlegen_win32/kindlegen.exe\" \"$$OUT_PWD)/xls/kindlegen.exe\""
#    QMAKE_POST_LINK += " && xcopy /y /e /i \"$${_PRO_FILE_PWD_}/Help\" \"$$OUT_PWD)\""
#    QMAKE_POST_LINK += " && if not exist \"$$OUT_PWD/language\" mkdir \"$$OUT_PWD/language\""
#    QMAKE_POST_LINK += " && xcopy /y \"$${_PRO_FILE_PWD_}/*.qm\" \"$$OUT_PWD/language\""
#    QMAKE_POST_LINK += " && xcopy /y \"$${_PRO_FILE_PWD_}/ABC/*.txt\" \"$$OUT_PWD/language\""
}

SOURCES += main.cpp\
	library.cpp \
        mainwindow.cpp \
    addlibrary.cpp \
    importthread.cpp \
    SmtpClient/smtpclient.cpp \
    SmtpClient/quotedprintable.cpp \
    SmtpClient/mimetext.cpp \
    SmtpClient/mimepart.cpp \
    SmtpClient/mimemultipart.cpp \
    SmtpClient/mimemessage.cpp \
    SmtpClient/mimeinlinefile.cpp \
    SmtpClient/mimehtml.cpp \
    SmtpClient/mimefile.cpp \
    SmtpClient/mimecontentformatter.cpp \
    SmtpClient/mimeattachment.cpp \
    SmtpClient/emailaddress.cpp \
    settingsdlg.cpp \
    exportthread.cpp \
    exportdlg.cpp \
    aboutdialog.cpp \
    helpdialog.cpp \
    fb2mobi/hyphenations.cpp \
    tagdialog.cpp \
    fb2mobi/fb2mobi.cpp \
    dropform.cpp \
    opds_server.cpp \
    fontframe.cpp \
    libwizard.cpp \
    exportframe.cpp \
    mobiEdit/mobiedit.cpp \
    quazip/quazip/quazipnewinfo.cpp \
    quazip/quazip/quazip.cpp \
    quazip/quazip/qioapi.cpp \
    quazip/quazip/quazipfile.cpp \
    quazip/quazip/quazipfileinfo.cpp \
    quazip/quazip/JlCompress.cpp \
    quazip/quazip/quaadler32.cpp \
    quazip/quazip/quacrc32.cpp \
    quazip/quazip/quagzipfile.cpp \
    quazip/quazip/quaziodevice.cpp \
    quazip/quazip/quazipdir.cpp \
    quazip/quazip/unzip.c \
    quazip/quazip/zip.c \
    bookeditdlg.cpp \
    webpage.cpp \
    treebookitem.cpp \
    genresortfilterproxymodel.cpp

HEADERS  += mainwindow.h \
    common.h \
    addlibrary.h \
    importthread.h \
    SmtpClient/smtpclient.h \
    SmtpClient/quotedprintable.h \
    SmtpClient/mimetext.h \
    SmtpClient/mimepart.h \
    SmtpClient/mimemultipart.h \
    SmtpClient/mimemessage.h \
    SmtpClient/mimeinlinefile.h \
    SmtpClient/mimehtml.h \
    SmtpClient/mimefile.h \
    SmtpClient/mimecontentformatter.h \
    SmtpClient/mimeattachment.h \
    SmtpClient/emailaddress.h \
    library.h \
    settingsdlg.h \
    exportthread.h \
    exportdlg.h \
    aboutdialog.h \
    helpdialog.h \
    fb2mobi/fb2mobi.h \
    fb2mobi/hyphenations.h \
    tagdialog.h \
    dropform.h \
    opds_server.h \
    fontframe.h \
    libwizard.h \
    exportframe.h \
    mobiEdit/mobiedit.h \
    quazip/quazip/quazip.h \
    quazip/quazip/ioapi.h \
    quazip/quazip/quazipnewinfo.h \
    quazip/quazip/quazip_global.h \
    quazip/quazip/quazipfileinfo.h \
    quazip/quazip/JlCompress.h \
    quazip/quazip/quaadler32.h \
    quazip/quazip/quachecksum32.h \
    quazip/quazip/quacrc32.h \
    quazip/quazip/quagzipfile.h \
    quazip/quazip/quaziodevice.h \
    quazip/quazip/quazipdir.h \
    quazip/quazip/quazipfile.h \
    quazip/quazip/crypt.h \
    bookeditdlg.h \
    webpage.h \
    treebookitem.h \
    genresortfilterproxymodel.h \
    build_number.h

FORMS    += mainwindow.ui \
    addlibrary.ui \
    settingsdlg.ui \
    exportdlg.ui \
    aboutdialog.ui \
    helpdialog.ui \
    tagdialog.ui \
    dropform.ui \
    fontframe.ui \
    libwizard.ui \
    exportframe.ui \
    bookeditdlg.ui

RESOURCES += \
    resource.qrc

OTHER_FILES +=

OBJECTIVE_SOURCES += \
    myapplicationclass.mm

DISTFILES += \
    build_number.sh
