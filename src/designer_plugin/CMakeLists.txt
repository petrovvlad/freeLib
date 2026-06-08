cmake_minimum_required(VERSION 3.16)
project(FileLineEditPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Designer)

set(WIDGET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)

add_library(FileLineEditPlugin MODULE)


target_sources(FileLineEditPlugin PRIVATE
    ${WIDGET_SOURCE_DIR}/filelineedit.cpp
    filelineedit.plugin.cpp
)

target_include_directories(FileLineEditPlugin PRIVATE
    ${WIDGET_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(FileLineEditPlugin Qt6::Designer Qt6::Core Qt6::Widgets)

set_target_properties(FileLineEditPlugin PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
)

set(CMAKE_VERBOSE_MAKEFILE ON)

install(TARGETS FileLineEditPlugin
    DESTINATION $$[QT_INSTALL_PREFIX]/plugins/designer
)

message(STATUS "Plugin will be installed to: $$[QT_INSTALL_PREFIX]/plugins/designer")