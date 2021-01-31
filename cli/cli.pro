include(../simulationcraft.pri)

TEMPLATE    = app
TARGET      = simc
CONFIG     -= qt app_bundle link_prl
LIBS       += -L../lib
QT         -= core gui

# Linux puts binaries to a different place (see simulationcraft.pri)
win32|macx {
  DESTDIR   = ..
}

CONFIG(release, debug|release): LIBS += -L../lib/release -lsimcengine
CONFIG(debug, debug|release): LIBS += -L../lib/debug -lsimcengine

win32 {
  CONFIG += console
  QMAKE_PROJECT_NAME = "Simulationcraft CLI"
}

unix {
  LIBS += -lpthread
}


# Deployment for Linux
unix:!macx {
  DISTFILES  += CHANGES COPYING
  INSTALLS   += target Profiles
  # Disable strip
  QMAKE_STRIP = echo

  target.path = $$DESTDIR$$PREFIX/bin/

  Profiles.files = $$files(../profiles/*, recursive=true)
  Profiles.path = $$SHAREPATH/profiles
  Profiles.commands = @echo Installing profiles to $$SHAREPATH/profiles
}

include(../source_files/QT_engine_main.pri)

SOURCES = $$replace(SOURCES, engine/, ../engine/)
