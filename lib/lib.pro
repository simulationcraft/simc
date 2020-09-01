include(../simulationcraft.pri)

TEMPLATE    = lib
TARGET      = simcengine
CONFIG     += staticlib create_prl
QT         -= core gui

win32 {
  QMAKE_PROJECT_NAME = "Simulationcraft Engine"
}

# If apikey is in environment, use that
ENV_APIKEY=$$(SC_DEFAULT_APIKEY)
!isEmpty(ENV_APIKEY) {
  SC_DEFAULT_APIKEY=$$ENV_APIKEY
}

!isEmpty(SC_DEFAULT_APIKEY) {
  DEFINES += SC_DEFAULT_APIKEY=\\\"$${SC_DEFAULT_APIKEY}\\\"
}

# On Linux compilation, setup the profile search directory
unix:!macx {
  !isEmpty(PREFIX)|!isEmpty(DESTDIR) {
    DEFINES += SC_SHARED_DATA=\\\"$$PREFIX/share/$$ORG_NAME/$$APP_NAME\\\"
  }
}

# Win32/OS X will setup target directory to the source root instead. On linux, this is setup in
# simulationcraft.pri.
win32|macx {
  CONFIG(release, debug|release): DESTDIR = release
  CONFIG(debug, debug|release): DESTDIR = debug
}

include(../source_files/QT_engine.pri)

# Fix paths in SOURCES, HEADERS, PRECOMPILED_HEADER, as they need to 
# refer to parent directory for the respective subprojects.
HEADERS = $$replace(HEADERS, engine/, ../engine/)
SOURCES = $$replace(SOURCES, engine/, ../engine/)
