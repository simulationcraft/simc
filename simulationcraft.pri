ORG_NAME    = SimulationCraft
APP_NAME    = SimulationCraft

INCLUDEPATH = ../engine
DEPENDPATH  = ../engine
VPATH       = ..
CONFIG     += c++11

CONFIG(debug, debug|release): OBJECTS_DIR = build/debug
CONFIG(release, debug|release): OBJECTS_DIR = build/release

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

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

macx {
  contains(QMAKE_CXX, clang++) {
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override
  }
  LIBS += -framework Security
}

win32 {
  LIBS += -lwininet -lshell32
}
