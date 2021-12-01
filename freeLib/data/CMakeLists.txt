
if(UNIX AND NOT APPLE AND NOT HAIKU)
    install(FILES freelib.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(DIRECTORY icons/application/ DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
            FILES_MATCHING PATTERN "freelib*.png" PATTERN "freelib*.svg"
            PATTERN "status" EXCLUDE PATTERN "actions" EXCLUDE PATTERN "categories" EXCLUDE)

     install(FILES freelib.appdata.xml DESTINATION share/metainfo COMPONENT freelib)

    install(CODE "
      execute_process(COMMAND sudo gtk-update-icon-cache ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons/hicolor)
      "
    )
endif(UNIX AND NOT APPLE AND NOT HAIKU)
