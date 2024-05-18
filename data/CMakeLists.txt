
if(UNIX AND NOT APPLE AND NOT HAIKU)
    install(FILES freelib.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
            FILES_MATCHING PATTERN "freelib*.png" PATTERN "freelib*.svg"
            PATTERN "status" EXCLUDE PATTERN "actions" EXCLUDE PATTERN "categories" EXCLUDE)

     install(FILES freelib.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo COMPONENT freelib)
endif(UNIX AND NOT APPLE AND NOT HAIKU)
