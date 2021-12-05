configure_file(config-freelib.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-freelib.h)
#set(QT_COMPONENTS Core Network Widgets Gui WebEngineWidgets Sql Xml XmlPatterns LinguistTools)
set(QT_COMPONENTS Core Network Widgets Gui WebEngineWidgets Sql Xml XmlPatterns)

find_package(QT NAMES Qt6 Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${QT_COMPONENTS} REQUIRED)
find_package(ZLIB REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        common.h
        mainwindow.cpp      mainwindow.h    mainwindow.ui
        library.cpp         library.h
        addlibrary.cpp      addlibrary.h    addlibrary.ui
        settingsdlg.cpp     settingsdlg.h   settingsdlg.ui
        exportframe.cpp     exportframe.h   exportframe.ui
        fontframe.cpp       fontframe.h     fontframe.ui
        exportdlg.cpp       exportdlg.h     exportdlg.ui
        aboutdialog.cpp     aboutdialog.h   aboutdialog.ui
        helpdialog.cpp      helpdialog.h    helpdialog.ui
        tagdialog.cpp       tagdialog.h     tagdialog.ui
        bookeditdlg.cpp     bookeditdlg.h   bookeditdlg.ui
        importthread.cpp    importthread.h
        exportthread.cpp    exportthread.h
        opds_server.cpp     opds_server.h
        options.cpp         options.h
        treebookitem.cpp    treebookitem.h
        coverlabel.cpp      coverlabel.h
        starsdelegate.cpp   starsdelegate.h
        utilites.cpp        utilites.h
        genresortfilterproxymodel.cpp   genresortfilterproxymodel.h
        fb2mobi/hyphenations.cpp        fb2mobi/hyphenations.h
        fb2mobi/fb2mobi.cpp             fb2mobi/fb2mobi.h
        mobiEdit/mobiedit.cpp           mobiEdit/mobiedit.h
        SmtpClient/src/emailaddress.cpp         SmtpClient/src/emailaddress.h
        SmtpClient/src/mimeattachment.cpp       SmtpClient/src/mimeattachment.h
        SmtpClient/src/mimecontentformatter.cpp SmtpClient/src/mimecontentformatter.h
        SmtpClient/src/mimefile.cpp             SmtpClient/src/mimefile.h
        SmtpClient/src/mimemessage.cpp          SmtpClient/src/mimemessage.h
        SmtpClient/src/mimemultipart.cpp        SmtpClient/src/mimemultipart.h
        SmtpClient/src/mimepart.cpp             SmtpClient/src/mimepart.h
        SmtpClient/src/mimetext.cpp             SmtpClient/src/mimetext.h
        SmtpClient/src/quotedprintable.cpp      SmtpClient/src/quotedprintable.h
        SmtpClient/src/smtpclient.cpp           SmtpClient/src/smtpclient.h
        SmtpClient/src/mimebase64encoder.cpp    SmtpClient/src/mimebase64encoder.h
        SmtpClient/src/mimebase64formatter.cpp  SmtpClient/src/mimebase64formatter.h
        SmtpClient/src/mimeqpformatter.cpp      SmtpClient/src/mimeqpformatter.h
        SmtpClient/src/mimeqpencoder.cpp        SmtpClient/src/mimeqpencoder.h
        SmtpClient/src/mimecontentencoder.cpp   SmtpClient/src/mimecontentencoder.h
)

add_subdirectory(Help)

find_package(QuaZip-Qt5 QUIET)
if(NOT QuaZip-Qt5_FOUND)
    set(QUAZIP_INSTALL OFF)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(quazip)
endif()


file(GLOB QM_FILES "${CMAKE_CURRENT_SOURCE_DIR}/language_*.qm")
file(COPY ${QM_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
install(FILES ${QM_FILES} DESTINATION ${DATA_INSTALL_DIR}/translations)
#FILE(GLOB TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ts")
#qt5_add_translation(QM_FILES ${TS_FILES})

qt5_add_resources(PROJECT_SOURCES resource.qrc)
#configure_file(translations.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
#file(COPY ${TS_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/translate)


if(UNIX AND NOT APPLE)
    file(COPY xsl/fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/xsl)
endif(UNIX AND NOT APPLE)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(freelib
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(freelib
        ${PROJECT_SOURCES}
        #${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
    )
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

if(UNIX AND NOT APPLE)
    install(TARGETS freelib DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(UNIX AND NOT APPLE)
