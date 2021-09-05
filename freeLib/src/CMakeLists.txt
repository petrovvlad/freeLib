cmake_minimum_required(VERSION 3.5)

project(freeLib VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_COMPONENTS Core Network Widgets Gui WebEngineWidgets Sql Xml XmlPatterns LinguistTools)
find_package(QT NAMES Qt6 Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${QT_COMPONENTS} REQUIRED)
find_package(ZLIB REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        common.h
        build_number.h
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        library.cpp
        library.h
        addlibrary.cpp
        addlibrary.h
        addlibrary.ui
        settingsdlg.cpp
        settingsdlg.h
        settingsdlg.ui
        exportframe.cpp
        exportframe.h
        exportframe.ui
        fontframe.cpp
        fontframe.h
        fontframe.ui
        exportdlg.cpp
        exportdlg.h
        exportdlg.ui
        aboutdialog.cpp
        aboutdialog.h
        aboutdialog.ui
        helpdialog.cpp
        helpdialog.h
        helpdialog.ui
        tagdialog.cpp
        tagdialog.h
        tagdialog.ui
        dropform.cpp
        dropform.h
        dropform.ui
        bookeditdlg.cpp
        bookeditdlg.h
        bookeditdlg.ui
        importthread.cpp
        importthread.h
        exportthread.cpp
        exportthread.h
        opds_server.cpp
        opds_server.h
        webpage.cpp
        webpage.h
        treebookitem.cpp
        treebookitem.h
        genresortfilterproxymodel.cpp
        genresortfilterproxymodel.h      
        fb2mobi/hyphenations.cpp
        fb2mobi/fb2mobi.cpp
        fb2mobi/fb2mobi.h
        fb2mobi/hyphenations.h
        mobiEdit/mobiedit.cpp
        mobiEdit/mobiedit.h
        SmtpClient/src/SmtpMime
        SmtpClient/src/emailaddress.cpp
        SmtpClient/src/emailaddress.h
        SmtpClient/src/mimeattachment.cpp
        SmtpClient/src/mimeattachment.h
        SmtpClient/src/mimecontentformatter.cpp
        SmtpClient/src/mimecontentformatter.h
        SmtpClient/src/mimefile.cpp
        SmtpClient/src/mimefile.h
        SmtpClient/src/mimehtml.cpp
        SmtpClient/src/mimehtml.h
        SmtpClient/src/mimeinlinefile.cpp
        SmtpClient/src/mimeinlinefile.h
        SmtpClient/src/mimemessage.cpp
        SmtpClient/src/mimemessage.h
        SmtpClient/src/mimemultipart.cpp
        SmtpClient/src/mimemultipart.h
        SmtpClient/src/mimepart.cpp
        SmtpClient/src/mimepart.h
        SmtpClient/src/mimetext.cpp
        SmtpClient/src/mimetext.h
        SmtpClient/src/quotedprintable.cpp
        SmtpClient/src/quotedprintable.h
        SmtpClient/src/smtpclient.cpp
        SmtpClient/src/smtpclient.h
        SmtpClient/src/smtpexports.h
)
add_subdirectory(quazip)


FILE(GLOB TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ts")
qt5_add_translation(QM_FILES ${TS_FILES})

qt5_add_resources(PROJECT_SOURCES resource.qrc)
configure_file(translations.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

if(UNIX AND NOT APPLE)
    file(COPY xsl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ../../KindleGenLinux/kindlegen DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/xsl/kindlegen)
    file(COPY Help DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif(UNIX AND NOT APPLE)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(freelib
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(freelib SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(freelib
            ${PROJECT_SOURCES}
            ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
        )
    endif()
endif()

target_link_libraries(freelib PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::XmlPatterns
    QuaZip::QuaZip)


set_target_properties(freelib PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(freelib)
endif()
