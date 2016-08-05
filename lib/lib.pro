include(../simulationcraft.pri)

TEMPLATE    = lib
TARGET      = simcengine
CONFIG     += staticlib create_prl
QT         -= core gui

CONFIG(release, debug|release): DESTDIR = release
CONFIG(debug, debug|release): DESTDIR = debug

win32 {
  QMAKE_PROJECT_NAME = "Simulationcraft Engine"
}

!isEmpty(SC_DEFAULT_APIKEY) {
  DEFINES += SC_DEFAULT_APIKEY=\"$${SC_DEFAULT_APIKEY}\"
}

!isEmpty(PREFIX)|!isEmpty(DESTDIR) {
  DEFINES += SC_SHARED_DATA=\\\"$$PREFIX/share/$$ORG_NAME/$$APP_NAME\\\"
}

include(../source_files/QT_engine.pri)

# Fix paths in SOURCES, HEADERS, PRECOMPILED_HEADER, as they need to 
# refer to parent directory for the respective subprojects. Additionally, 
# simulationcraft.hpp must only be defined in PRECOMPILED_HEADER.
HEADERS -= engine/simulationcraft.hpp
HEADERS = $$replace(HEADERS, engine/, ../engine/)
SOURCES = $$replace(SOURCES, engine/, ../engine/)
PRECOMPILED_HEADER = $$replace(PRECOMPILED_HEADER, engine/, ../engine/)