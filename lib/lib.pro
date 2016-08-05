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
  !isEmpty(PGO) {
    win32-msvc2015 {
      QMAKE_LFLAGS_RELEASE += /LTCG /USEPROFILE
    }

    win32-msvc2013 {
      QMAKE_CXXFLAGS_RELEASE += /GL
      QMAKE_LFLAGS_RELEASE   += /LTCG
    }
  }
}

!isEmpty(SC_DEFAULT_APIKEY) {
  DEFINES += SC_DEFAULT_APIKEY=\"$${SC_DEFAULT_APIKEY}\"
}

!isEmpty(PREFIX)|!isEmpty(DESTDIR) {
  DEFINES += SC_SHARED_DATA=\\\"$$PREFIX/share/$$ORG_NAME/$$APP_NAME\\\"
}

include(../source_files/QT_engine.pri)
