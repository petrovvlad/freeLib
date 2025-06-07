set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(config-freelib.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-freelib.h)
configure_file(git-info.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/git-info.h)
set(QT_COMPONENTS Core Network Widgets Gui Svg Sql Xml Concurrent)

find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Core)
message(STATUS "Found Qt ${QT_VERSION_MAJOR} VERSION")
if(NOT DEFINED FREELIB_QT_MAJOR_VERSION)
    set(FREELIB_QT_MAJOR_VERSION ${QT_VERSION_MAJOR} CACHE STRING "Qt version to use (5 or 6), defaults to ${QT_VERSION_MAJOR}")
endif()
message(STATUS "FREELIB_QT_MAJOR_VERSION: ${FREELIB_QT_MAJOR_VERSION}")

find_package(KF${FREELIB_QT_MAJOR_VERSION}KIO QUIET)
if(KF${FREELIB_QT_MAJOR_VERSION}KIO_FOUND)
    message(STATUS "Found KIO")
    add_compile_definitions(USE_KIO)
endif()

find_package(KF${FREELIB_QT_MAJOR_VERSION}StatusNotifierItem QUIET)
if(KF${FREELIB_QT_MAJOR_VERSION}StatusNotifierItem_FOUND)
  message(STATUS "Found KStatusNotifier")
  add_compile_definitions(USE_KStatusNotifier)
endif()

if(FREELIB_QT_MAJOR_VERSION EQUAL 6)
    find_package(QT NAMES Qt6 REQUIRED)
    find_package(Qt6 COMPONENTS ${QT_COMPONENTS} REQUIRED)
else()
    find_package(QT NAMES Qt5 REQUIRED)
    find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)
endif()

find_package(ZLIB REQUIRED)
find_package(TBB QUIET)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(DjVuLibre QUIET)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp      mainwindow.h    mainwindow.ui
        library.cpp         library.h
        bookfile.cpp        bookfile.h
        librariesdlg.cpp    librariesdlg.h  librariesdlg.ui
        settingsdlg.cpp     settingsdlg.h   settingsdlg.ui
        exportframe.cpp     exportframe.h   exportframe.ui
        conversionframe.cpp conversionframe.h conversionframe.ui
        fontframe.cpp       fontframe.h     fontframe.ui
        exportdlg.cpp       exportdlg.h     exportdlg.ui
        aboutdialog.cpp     aboutdialog.h   aboutdialog.ui
        helpdialog.cpp      helpdialog.h    helpdialog.ui
        tagdialog.cpp       tagdialog.h     tagdialog.ui
        bookeditdlg.cpp     bookeditdlg.h   bookeditdlg.ui
        statisticsdialog.h  statisticsdialog.cpp statisticsdialog.ui
        importthread.cpp    importthread.h
        exportthread.cpp    exportthread.h
        options.cpp         options.h
        treebookitem.cpp    treebookitem.h
        coverlabel.cpp      coverlabel.h
        starsdelegate.cpp   starsdelegate.h
        starscombobox.cpp   starscombobox.h
        utilites.cpp        utilites.h
        epubreader.cpp      epubreader.h
        passwordlineedit.cpp            passwordlineedit.h
        genresortfilterproxymodel.cpp   genresortfilterproxymodel.h
        fb2mobi/hyphenations.cpp            fb2mobi/hyphenations.h
        fb2mobi/fb2mobi.cpp                 fb2mobi/fb2mobi.h
        mobiEdit/mobiedit.cpp               mobiEdit/mobiedit.h
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

if((FREELIB_QT_MAJOR_VERSION EQUAL 6) AND (QT_VERSION_MINOR GREATER_EQUAL 4))
  find_package(Qt6 COMPONENTS HttpServer QUIET)
  if(Qt6HttpServer_FOUND)
    message(STATUS "Found HttpServer")
    set(PROJECT_SOURCES
        ${PROJECT_SOURCES}
        opds_server.cpp     opds_server.h
    )
    add_compile_definitions(USE_HTTSERVER)
   endif()
endif()

find_package(Qt${QT_VERSION_MAJOR}Keychain QUIET)
if(Qt${QT_VERSION_MAJOR}Keychain_FOUND)
    message(STATUS "Found QtKeychain")
    add_compile_definitions(USE_QTKEYCHAIN)
endif()

find_package(QuaZip-Qt${QT_VERSION_MAJOR} QUIET)
if(NOT QuaZip-Qt${QT_VERSION_MAJOR}_FOUND)
    set(QUAZIP_INSTALL OFF)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(quazip)
endif()

if(DJVULIBRE_FOUND)
    message(STATUS "Found DjVuLibre")
    set(PROJECT_SOURCES
        ${PROJECT_SOURCES}
        djvu.cpp            djvu.h
    )
    add_compile_definitions(USE_DEJVULIBRE)
endif()

find_package(TBB QUIET)
if(TBB_FOUND)
    message(STATUS "Found TBB")
    add_compile_definitions(USE_TBB)
endif()

#configure_file(translations.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
#file(COPY ${TS_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/translate)

if(UNIX AND NOT APPLE)
    file(COPY xsl/fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/xsl)
endif(UNIX AND NOT APPLE)

if(FREELIB_QT_MAJOR_VERSION EQUAL 6)
  if(WIN32)
    qt_add_executable(freelib
      WIN32
      MANUAL_FINALIZATION
      ${PROJECT_SOURCES})
  else()
    qt6_add_resources(PROJECT_SOURCES resource.qrc)
    qt_add_executable(freelib
      MANUAL_FINALIZATION
      ${PROJECT_SOURCES}
    )
  endif(WIN32)
else()
    qt5_add_resources(PROJECT_SOURCES resource.qrc)
    add_executable(freelib
        ${PROJECT_SOURCES}
    )
endif()

#FILE(GLOB TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ts")
#qt5_add_translation(QM_FILES ${TS_FILES})

file(GLOB QM_FILES "${CMAKE_CURRENT_SOURCE_DIR}/language_*.qm")
#file(COPY ${QM_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
install(FILES ${QM_FILES} DESTINATION ${DATA_INSTALL_DIR}/translations)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(
        TARGET freelib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/translations
        )
    foreach( file_i ${QM_FILES})
        add_custom_command(
            TARGET freelib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${file_i}
            ${CMAKE_CURRENT_BINARY_DIR}/translations/)
    endforeach( file_i )
endif()

target_link_libraries(freelib PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Concurrent
    QuaZip::QuaZip)

if(Qt6HttpServer_FOUND)
    target_link_libraries(freelib PRIVATE Qt${QT_VERSION_MAJOR}::HttpServer)
endif()

if(KF${FREELIB_QT_MAJOR_VERSION}KIO_FOUND)
    target_link_libraries(freelib PRIVATE KF${FREELIB_QT_MAJOR_VERSION}::KIOCore)
endif()

if(KF${FREELIB_QT_MAJOR_VERSION}StatusNotifierItem_FOUND)
  target_link_libraries(freelib PRIVATE KF${FREELIB_QT_MAJOR_VERSION}::StatusNotifierItem)
endif()

if(Qt${QT_VERSION_MAJOR}Keychain_FOUND)
    target_link_libraries(freelib PRIVATE qt${QT_VERSION_MAJOR}keychain)
endif()

if(TBB_FOUND)
    target_link_libraries(freelib PRIVATE TBB::tbb)
endif()

target_precompile_headers(freelib
  PRIVATE
    precompiled.h
)

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

