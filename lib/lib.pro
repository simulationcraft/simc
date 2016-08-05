include(../simulationcraft.pri)

TEMPLATE    = lib
TARGET      = simcengine
CONFIG     += staticlib create_prl
QT         -= core gui

CONFIG(release, debug|release): DESTDIR = release
CONFIG(debug, debug|release): DESTDIR = debug

PRECOMPILED_HEADER = ../engine/simulationcraft.hpp

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
