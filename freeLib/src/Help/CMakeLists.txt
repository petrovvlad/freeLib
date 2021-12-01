set(HELP_SOURCE
    index.html
    params.html
    cmd_params.html
    changelog.html
    script.js
    style.css)

file(COPY ${HELP_SOURCE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY img DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

install(FILES ${HELP_SOURCE} DESTINATION ${DATA_INSTALL_DIR}/help)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/img/ DESTINATION ${DATA_INSTALL_DIR}/help/img)
