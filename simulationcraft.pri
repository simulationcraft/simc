ORG_NAME    = SimulationCraft
APP_NAME    = SimulationCraft

INCLUDEPATH = ../engine
DEPENDPATH  = ../engine
VPATH       = ..
CONFIG     += c++11

CONFIG(debug, debug|release): OBJECTS_DIR = build/debug
CONFIG(release, debug|release): OBJECTS_DIR = build/release

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

# Setup some paths if DESTDIR/PREFIX are defined for Linux stuff
unix:!macx {
  !isEmpty(DESTDIR): PREFIX=$$DESTDIR/$$PREFIX
  isEmpty(PREFIX):   PREFIX=/usr/local
  isEmpty(DATADIR):  DATADIR=$$PREFIX/share
  isEmpty(SEARCH):   SEARCH=$$DATADIR/$$ORG_NAME/$$APP_NAME
  isEmpty(BINDIR):   BINDIR=$$PREFIX/bin

  SHAREPATH = $$DESTDIR$$PREFIX/share/$$ORG_NAME/$$APP_NAME
}

CONFIG(release, debug|release) {
  DEFINES += NDEBUG
}

CONFIG(openssl) {
  DEFINES     += SC_USE_OPENSSL
  INCLUDEPATH += $$OPENSSL_INCLUDES
  LIBS        += -L$$OPENSSL_LIBS -lssleay32
}

contains(QMAKE_CXX, clang++)|contains(QMAKE_CXX, g++) {
  QMAKE_CXXFLAGS += -Wextra
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE += -O3 -ffast-math -fomit-frame-pointer -Os -fPIE

  !isEmpty(LTO) {
    QMAKE_CXXFLAGS_RELEASE += -flto
  }
}

unix|macx {
  system(which -s git) {
    DEFINES += SC_GIT_REV="\\\"$$system(git rev-parse --short HEAD)\\\""
  }
}

macx {
  contains(QMAKE_CXX, clang++) {
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override
  }
  LIBS += -framework Security

  CONFIG(sanitize) {
    QMAKE_CXXFLAGS += -fsanitize=address
    QMAKE_LFLAGS += -fsanitize=address
  }
}

win32 {
  LIBS += -lwininet -lshell32
  win32-msvc2013|win32-msvc2015 {
    QMAKE_CXXFLAGS_RELEASE += /Ot /MP
  }

  # TODO: Mingw might want something more unixy here?
  system(where /q git) {
    DEFINES += SC_GIT_REV="\\\"$$system(git rev-parse --short HEAD)\\\""
  }

  !isEmpty(PGO) {
    win32-msvc2013 {
      QMAKE_LFLAGS_RELEASE += /LTCG
      QMAKE_CXXFLAGS_RELEASE += /GL
    }

    win32-msvc2015 {
      QMAKE_CXXFLAGS_RELEASE += /GL
      QMAKE_LFLAGS_RELEASE   += /LTCG /USEPROFILE /PGD:"..\SimulationCraft.pgd"
    }
  }
}


