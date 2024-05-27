set(HELP_SOURCE
    about.md
    params.md
    cmd_params.md
    changelog.md)

file(COPY ${HELP_SOURCE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

install(FILES ${HELP_SOURCE} DESTINATION ${DATA_INSTALL_DIR}/help)
