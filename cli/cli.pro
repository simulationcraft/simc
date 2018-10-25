include(../simulationcraft.pri)

TEMPLATE    = app
TARGET      = simc
CONFIG     -= qt app_bundle link_prl c++14
LIBS       += -L../lib -lsimcengine
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
  INSTALLS   += target profiles
  # Disable strip
  QMAKE_STRIP = echo

  target.path = $$DESTDIR$$PREFIX/bin/

  profiles.path = $$SHAREPATH/profiles
  profiles.files += ../profiles/*
  profiles.commands = @echo Installing profiles to $$SHAREPATH/profiles
}

include(../source_files/QT_engine_main.pri)

SOURCES = $$replace(SOURCES, engine/, ../engine/)
