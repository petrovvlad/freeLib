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
    SmtpClient/demos/demo1/demo1.cpp \
    SmtpClient/demos/demo2/demo2.cpp \
    SmtpClient/demos/demo2/sendemail.cpp \
    SmtpClient/demos/demo3/demo3.cpp \
    SmtpClient/demos/demo4/demo4.cpp \
    SmtpClient/src/emailaddress.cpp \
    SmtpClient/src/mimeattachment.cpp \
    SmtpClient/src/mimebase64encoder.cpp \
    SmtpClient/src/mimebase64formatter.cpp \
    SmtpClient/src/mimebytearrayattachment.cpp \
    SmtpClient/src/mimecontentencoder.cpp \
    SmtpClient/src/mimecontentformatter.cpp \
    SmtpClient/src/mimefile.cpp \
    SmtpClient/src/mimehtml.cpp \
    SmtpClient/src/mimeinlinefile.cpp \
    SmtpClient/src/mimemessage.cpp \
    SmtpClient/src/mimemultipart.cpp \
    SmtpClient/src/mimepart.cpp \
    SmtpClient/src/mimeqpencoder.cpp \
    SmtpClient/src/mimeqpformatter.cpp \
    SmtpClient/src/mimetext.cpp \
    SmtpClient/src/quotedprintable.cpp \
    SmtpClient/src/smtpclient.cpp \
	library.cpp \
        mainwindow.cpp \
    addlibrary.cpp \
    importthread.cpp \
    settingsdlg.cpp \
    exportthread.cpp \
    exportdlg.cpp \
    aboutdialog.cpp \
    helpdialog.cpp \
    fb2mobi/hyphenations.cpp \
    tagdialog.cpp \
    fb2mobi/fb2mobi.cpp \
    opds_server.cpp \
    fontframe.cpp \
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
    treebookitem.cpp \
    genresortfilterproxymodel.cpp

HEADERS  += mainwindow.h \
    SmtpClient/demos/demo2/sendemail.h \
    SmtpClient/src/SmtpMime \
    SmtpClient/src/emailaddress.h \
    SmtpClient/src/mimeattachment.h \
    SmtpClient/src/mimebase64encoder.h \
    SmtpClient/src/mimebase64formatter.h \
    SmtpClient/src/mimebytearrayattachment.h \
    SmtpClient/src/mimecontentencoder.h \
    SmtpClient/src/mimecontentformatter.h \
    SmtpClient/src/mimefile.h \
    SmtpClient/src/mimehtml.h \
    SmtpClient/src/mimeinlinefile.h \
    SmtpClient/src/mimemessage.h \
    SmtpClient/src/mimemultipart.h \
    SmtpClient/src/mimepart.h \
    SmtpClient/src/mimeqpencoder.h \
    SmtpClient/src/mimeqpformatter.h \
    SmtpClient/src/mimetext.h \
    SmtpClient/src/quotedprintable.h \
    SmtpClient/src/smtpclient.h \
    SmtpClient/src/smtpmime_global.h \
    SmtpClient/test/connectiontest.h \
    common.h \
    addlibrary.h \
    importthread.h \
    library.h \
    settingsdlg.h \
    exportthread.h \
    exportdlg.h \
    aboutdialog.h \
    helpdialog.h \
    fb2mobi/fb2mobi.h \
    fb2mobi/hyphenations.h \
    tagdialog.h \
    opds_server.h \
    fontframe.h \
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
    treebookitem.h \
    genresortfilterproxymodel.h \
    build_number.h

FORMS    += mainwindow.ui \
    SmtpClient/demos/demo2/sendemail.ui \
    addlibrary.ui \
    settingsdlg.ui \
    exportdlg.ui \
    aboutdialog.ui \
    helpdialog.ui \
    tagdialog.ui \
    fontframe.ui \
    exportframe.ui \
    bookeditdlg.ui

RESOURCES += \
    resource.qrc

OTHER_FILES +=

OBJECTIVE_SOURCES += \
    myapplicationclass.mm

DISTFILES += \
    Help/CMakeLists.txt \
    Help/changelog.html \
    Help/cmd_params.html \
    Help/help.komodoproject \
    Help/img/inter1.png \
    Help/img/inter2.png \
    Help/img/lib.png \
    Help/img/search.png \
    Help/img/set1.png \
    Help/img/set2.png \
    Help/index.html \
    Help/params.html \
    Help/script.js \
    Help/style.css \
    SmtpClient/.github/FUNDING.yml \
    SmtpClient/.github/workflows/main.yml \
    SmtpClient/.gitignore \
    SmtpClient/CMakeLists.txt \
    SmtpClient/LICENSE \
    SmtpClient/README.md \
    build_number.sh \
    img/icons/Streamline.png \
    img/icons/Streamline@2x.png \
    img/icons/Streamline_big.png \
    img/icons/Streamline_big@2x.png \
    img/icons/arrow_sans_down.png \
    img/icons/arrow_sans_down@2x.png \
    img/icons/arrow_sans_up.png \
    img/icons/arrow_sans_up@2x.png \
    img/icons/book.png \
    img/icons/book@2x.png \
    img/icons/checkbox.png \
    img/icons/checkbox@2x.png \
    img/icons/clear.png \
    img/icons/clear@2x.png \
    img/icons/close.png \
    img/icons/close@2x.png \
    img/icons/contact.png \
    img/icons/contact@2x.png \
    img/icons/contact_big.png \
    img/icons/contact_big@2x.png \
    img/icons/dark/book.svg \
    img/icons/dark/checkbox.svg \
    img/icons/dark/close.svg \
    img/icons/dark/down.svg \
    img/icons/dark/library-tmp.svg \
    img/icons/dark/library.svg \
    img/icons/dark/minus.svg \
    img/icons/dark/multitags.svg \
    img/icons/dark/pen.svg \
    img/icons/dark/plus.svg \
    img/icons/dark/settings.svg \
    img/icons/dark/streamline.svg \
    img/icons/dark/up.svg \
    img/icons/icon_150x150.png \
    img/icons/icon_30x30.png \
    img/icons/icon_48x48.png \
    img/icons/icon_50x50.png \
    img/icons/library.png \
    img/icons/library@2x.png \
    img/icons/light/book.svg \
    img/icons/light/checkbox.svg \
    img/icons/light/close.svg \
    img/icons/light/down.svg \
    img/icons/light/library-tmp.svg \
    img/icons/light/library.svg \
    img/icons/light/minus.svg \
    img/icons/light/multitags.svg \
    img/icons/light/pen.svg \
    img/icons/light/plus.svg \
    img/icons/light/settings.svg \
    img/icons/light/streamline.svg \
    img/icons/light/up.svg \
    img/icons/listview.png \
    img/icons/listview@2x.png \
    img/icons/minus.png \
    img/icons/minus@2x.png \
    img/icons/open.png \
    img/icons/open@2x.png \
    img/icons/pen_alt_fill.png \
    img/icons/pen_alt_fill@2x.png \
    img/icons/plus.png \
    img/icons/plus@2x.png \
    img/icons/save.png \
    img/icons/save@2x.png \
    img/icons/settings.png \
    img/icons/settings@2x.png \
    img/icons/stars/0star.svg \
    img/icons/stars/1star.svg \
    img/icons/stars/2star.svg \
    img/icons/stars/3star.svg \
    img/icons/stars/4star.svg \
    img/icons/stars/5star.svg \
    img/icons/treeview.png \
    img/icons/treeview@2x.png \
    img/site_logo.jpg \
    img/site_logo.png \
    img/sp.png \
    img/splash.png \
    img/splash@2x.png \
    img/splash@2x_old.png \
    img/splash_old.png \
    img/splash_opera.png \
    img/tray0.png \
    img/tray1.png \
    img/tray2.png \
    library.iconset/icon_128x128.png \
    library.iconset/icon_128x128@2x.png \
    library.iconset/icon_16x16.png \
    library.iconset/icon_16x16@2x.png \
    library.iconset/icon_256x256.png \
    library.iconset/icon_256x256@2x.png \
    library.iconset/icon_32x32.png \
    library.iconset/icon_32x32@2x.png \
    library.iconset/icon_512x512.png \
    library.iconset/icon_512x512@2x.png \
    xsl/css/style.css \
    xsl/fonts/PTF55F.ttf \
    xsl/fonts/PTF56F.ttf \
    xsl/fonts/PTF75F.ttf \
    xsl/fonts/PTF76F.ttf \
    xsl/fonts/sangha.ttf \
    xsl/hyphenations/de.txt \
    xsl/hyphenations/en.txt \
    xsl/hyphenations/ru.txt \
    xsl/hyphenations/uk.txt \
    xsl/img/cover.jpg \
    xsl/img/h0ta.png \
    xsl/img/h0ta.txt \
    xsl/img/h0tb.png \
    xsl/img/h0tb.txt \
    xsl/img/h1ta.png \
    xsl/img/h1ta.txt \
    xsl/img/h1tb.png \
    xsl/img/h1tb.txt \
    xsl/img/te.png \
    xsl/img/te.txt \
    xsl/opds/arrow_left.png \
    xsl/opds/arrow_right.png \
    xsl/opds/favicon.ico \
    xsl/opds/home.png \
    xsl/opds/icon_256x256.png \
    xsl/opds/splash_opera.png
