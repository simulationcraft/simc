TEMPLATE = app

CONFIG(qt) {
  TARGET = SimulationCraft

  QT += core gui network webkit

  contains ( QT_MAJOR_VERSION , 5 ) {
    QT += widgets webkitwidgets
    DEFINES += QT_VERSION_5
  }
  OBJECTS_DIR = qt
}

CONFIG(console) {
  QT       -= gui
  CONFIG   -= app_bundle
  CONFIG   -= qt
  TARGET = simc
  CONFIG += static staticlib
  OBJECTS_DIR = engine
}

# OSX qt 5.1 is fubar and has double slashes, messing up things
QTDIR=$$[QT_INSTALL_PREFIX]
QTDIR=$$replace(QTDIR, //, /)
QTBINDIR=$$[QT_INSTALL_BINS]
QTBINDIR=$$replace(QTBINDIR, //, /)

RESOURCES += \
    qt/simcqt.qrc

QMAKE_CXXFLAGS_RELEASE += -DNDEBUG
QMAKE_CXXFLAGS += $$OPTS

win32 {
  LIBS += -lwininet -lshell32
  RC_FILE += simcqt.rc
}

macx {
  CONFIG(qt) {
    QMAKE_CXXFLAGS += -stdlib=libstdc++
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
    CONFIG += to_install
    QMAKE_INFO_PLIST = qt/Simulationcraft.plist
    ICON = qt/icon/Simcraft2.icns
    LIBS += -framework CoreFoundation -framework AppKit
    DEFINES += SIMC_NO_AUTOUPDATE

    Resources.files = Welcome.html Welcome.png Error.html
    Resources.path = Contents/Resources
    Profiles.files = profiles/Tier16H profiles/Tier17N
    Profiles.path = Contents/Resources/profiles
    QMAKE_BUNDLE_DATA += Profiles Resources
    QMAKE_DISTCLEAN += simc *.dmg
  }

  release_simc.target = release_simc
  release_simc.commands = make -C engine clean && make CXX=clang++ OPTS=\"-fPIE -mmacosx-version-min=10.6 -std=c++0x -fomit-frame-pointer -arch i386 -arch x86_64 -O3 -msse2 -march=native -ffast-math -flto -Os -DNDEBUG\" -C engine install

  create_release.target   = create_release
  create_release.depends  = all release_simc
  create_release.commands = $$QTBINDIR/macdeployqt $(QMAKE_TARGET).app && \
                            qt/osx_release.sh

  QMAKE_EXTRA_TARGETS += release_simc create_release
}

# This will match both 'g++' and 'clang++'
COMPILER_CHECK_CXX = $$replace(QMAKE_CXX,'.*g\\+\\+'.*,'g++')
COMPILER_CXX = $$basename(QMAKE_CXX)

contains(COMPILER_CHECK_CXX,'g++') {
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE += -O3 -fomit-frame-pointer
  QMAKE_CXXFLAGS += -ffast-math -Woverloaded-virtual
  equals(COMPILER_CXX,'clang++') {
    QMAKE_CXXFLAGS_RELEASE += -flto -Os
  }
}

win32-msvc2010 {
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE += -Ox -GL -GS-
  QMAKE_LFLAGS_RELEASE += /LTCG
  QMAKE_CXXFLAGS += -fp:fast -GF -arch:SSE2
}

INCLUDEPATH += engine
DEPENDPATH += engine

PRECOMPILED_HEADER = engine/simulationcraft.hpp

# engine files
include(source_files/QT_engine.pri)

# engine Main file
CONFIG(console) {
include(source_files/QT_engine_main.pri)
  !isEmpty(PREFIX)|!isEmpty(DESTDIR) {
    DEFINES += SC_SHARED_DATA=\\\"$$PREFIX/share/SimulationCraft\\\"
  }
}

# GUI files
CONFIG(qt) {
include(source_files/QT_gui.pri)
}

CONFIG(openssl) {
  DEFINES += SC_USE_OPENSSL
  INCLUDEPATH += $$OPENSSL_INCLUDES
  LIBS += -L$$OPENSSL_LIBS -lssleay32
}

CONFIG(to_install) {
  DEFINES += SC_TO_INSTALL
}

# deployment for linux
unix:!mac {
  CONFIG(console)
  {
  LIBS += -pthread
  }
  DISTFILES += CHANGES \
               COPYING

  # Disable strip
  QMAKE_STRIP=echo

  isEmpty(PREFIX): PREFIX=~/SimulationCraft
  isEmpty(DESTDIR): DESTDIR=
  INSTALLS += target \
              profiles \
              data \
              icon \
              locale

  SHAREPATH = $$DESTDIR$$PREFIX/share/SimulationCraft

  CONFIG(linux_packaging) {
    DEFINES += SC_LINUX_PACKAGING=\\\"$$PREFIX/share/SimulationCraft\\\"
  }

  target.path = $$DESTDIR$$PREFIX/bin/

  profiles.path = $$SHAREPATH/profiles
  profiles.files += profiles/*
  profiles.commands = @echo Installing profiles to $$SHAREPATH/profiles

  data.path = $$SHAREPATH
  data.files += Welcome.html
  data.files += Welcome.png
  data.files += READ_ME_FIRST.txt
  data.files += Error.html
  data.commands = @echo Installing global files to $$SHAREPATH

  icon.path = $$SHAREPATH
  icon.files = debian/simulationcraft.xpm
  icon.commands = @echo Installing icon to $$SHAREPATH

  locale.path = $$SHAREPATH/locale
  locale.files += locale/*
  locale.commands = @echo Installing localizations to $$SHAREPATH/locale
}
  
CONFIG(paperdoll) {
  DEFINES += SC_PAPERDOLL
  HEADERS += qt/simcpaperdoll.hpp
  SOURCES += qt/simcpaperdoll.cc
}
