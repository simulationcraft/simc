ORG_NAME    = SimulationCraft
APP_NAME    = SimulationCraft

INCLUDEPATH = ../engine
INCLUDEPATH += ../engine/include
INCLUDEPATH += ../engine/lib
DEPENDPATH  = ../engine
VPATH       = ..
CONFIG      += c++17

CONFIG(debug, debug|release): OBJECTS_DIR = build/debug
CONFIG(release, debug|release): OBJECTS_DIR = build/release

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

isEmpty(SC_NO_NETWORKING) {
  SC_NO_NETWORKING=$$(SC_NO_NETWORKING)
}

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
  DEFINES       += SC_USE_OPENSSL

  !isEmpty(OPENSSL_INCLUDES) {
    INCLUDEPATH += $$OPENSSL_INCLUDES
  }

  !isEmpty(OPENSSL_LIBS) {
    LIBS        += -L$$OPENSSL_LIBS
  }

  win32 {
    LIBS        += -lssleay32
  }
}

!isEmpty(SC_NO_NETWORKING) {
  DEFINES += SC_NO_NETWORKING
  message(Building without networking support)
}

contains(QMAKE_CXX, .+/clang\+\+)|contains(QMAKE_CXX, .+/g\+\+) {
  QMAKE_CXXFLAGS += -Wextra
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE += -O3 -ffast-math -fomit-frame-pointer -Os -fPIE

  !isEmpty(LTO) {
    QMAKE_CXXFLAGS_RELEASE += -flto
  }
}

unix|macx {
  exists(.git):system(which -s git) {
    DEFINES += SC_GIT_REV="\\\"$$system(git rev-parse --short HEAD)\\\""
    DEFINES += SC_GIT_BRANCH="\\\"$$system(git rev-parse --abbrev-ref HEAD)\\\""
  }
}

macx {
  contains(QMAKE_CXX, .+/clang\+\+) {
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override
  }

  CONFIG(sanitize) {
    QMAKE_CXXFLAGS += -fsanitize=address
    QMAKE_LFLAGS += -fsanitize=address
  }
}

win32 {
  QMAKE_CXXFLAGS += /MP
  win32-msvc {
    QMAKE_CXXFLAGS_RELEASE += /O2 /GL
    QMAKE_CXXFLAGS_WARN_ON += /w44800 /w44100 /w44065
  }

  # Enable /permissive- on Visual Studio 2017+
  win32-msvc:greaterThan(QMAKE_MSC_VER, 1909) {
    QMAKE_CXXFLAGS += /permissive-
  }

  exists(.git):system(where /q git) {
    DEFINES += SC_GIT_REV="\\\"$$system(git rev-parse --short HEAD)\\\""
    DEFINES += SC_GIT_BRANCH="\\\"$$system(git rev-parse --abbrev-ref HEAD)\\\""
  }

  # Allow PGO builds on Visual Studio 2015+
  !isEmpty(PGO):win32-msvc:greaterThan(QMAKE_MSC_VER, 1899) {
    QMAKE_CXXFLAGS_RELEASE += /GL
    QMAKE_LFLAGS_RELEASE   += /LTCG /USEPROFILE /PGD:"..\SimulationCraft.pgd"
  }
}

# Curl is now required for everything, on MacOS use the default system curl (library with
# MacOS, headers with MacOS SDK in XCode), on other unixy systems use pkg-config to find
# it, on Windows, require CURL_ROOT to be defined (in an environment variable or
# compilation definition) and pointing to the base curl directory (dll found in
# CURL_ROOT/bin, includes in CURL_ROOT/include)
isEmpty(SC_NO_NETWORKING) {
  
  message(Adding networking support dependencies)
  !win32:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libcurl
  }

  macx {
    LIBS += -lcurl
  }

  win32 {
    LIBS += wininet.lib crypt32.lib
  }
}

