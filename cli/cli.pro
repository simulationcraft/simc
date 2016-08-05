include(../simulationcraft.pri)

TEMPLATE    = app
TARGET      = simc
CONFIG     -= qt app_bundle link_prl
QT         -= core gui
DESTDIR     = ..

CONFIG(release, debug|release): LIBS += -L../lib/release -lsimcengine
CONFIG(debug, debug|release): LIBS += -L../lib/debug -lsimcengine

win32 {
  CONFIG += console
  QMAKE_PROJECT_NAME = "Simulationcraft CLI"
  !isEmpty(PGO) {
    QMAKE_LFLAGS_RELEASE += /LTCG
    QMAKE_CXXFLAGS_RELEASE += /GL
  }
}

include(../source_files/QT_engine_main.pri)
