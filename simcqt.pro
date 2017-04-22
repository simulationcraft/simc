TEMPLATE = app
ORG_NAME = SimulationCraft
APP_NAME = SimulationCraft

CONFIG += c++11

CONFIG(qt) {
  TARGET = SimulationCraft

  QT += core gui network
  
  lessThan( QT_MAJOR_VERSION, 5 ) {
    error( "SimulationCraft requires QT 5 or higher." )
  }
  contains ( QT_MAJOR_VERSION , 5 ) {
    greaterThan( QT_MINOR_VERSION, 3 ) {
      QT += webengine webenginewidgets
      DEFINES += SC_USE_WEBENGINE
      message("Using WebEngine")
    }
    lessThan( QT_MINOR_VERSION, 4 ) {
      QT += webkit webkitwidgets
      DEFINES += SC_USE_WEBKIT
      message("Using WebKit")
    }
    QT += widgets
    DEFINES += QT_VERSION_5
  }
  contains( QT_MAJOR_VERSION, 4 ) {
    QT += webkit
  }
  OBJECTS_DIR = qt
}

Release: DEFINES += QT_NO_DEBUG_OUTPUT

win32-mingw
{
  ! macx {
    # QT 5.4 for MinGW does not yet contain the new Web Engine
    contains ( QT_MAJOR_VERSION , 5 ) {
        greaterThan( QT_MINOR_VERSION, 3 ) {
            QT -= webengine webenginewidgets
            QT += webkit webkitwidgets
            DEFINES -= SC_USE_WEBENGINE
            DEFINES += SC_USE_WEBKIT
            message("MinGW WebKit only")
        }
    }
  }
}

CONFIG(console) {
  QT       -= gui
  CONFIG   -= app_bundle
  CONFIG   -= qt
  TARGET = simc
  CONFIG += static staticlib
  OBJECTS_DIR = engine
}

TRANSLATIONS = locale/sc_de.ts \
    locale/sc_zh.ts \
    locale/sc_it.ts

# OSX qt 5.1 is fubar and has double slashes, messing up things
QTDIR=$$[QT_INSTALL_PREFIX]
QTDIR=$$replace(QTDIR, //, /)
QTBINDIR=$$[QT_INSTALL_BINS]
QTBINDIR=$$replace(QTBINDIR, //, /)

RESOURCES += \
    qt/simcqt.qrc

QMAKE_CXXFLAGS_RELEASE += -DNDEBUG
QMAKE_CXXFLAGS += $$OPTS

! isEmpty( SC_DEFAULT_APIKEY ) {
  DEFINES += SC_DEFAULT_APIKEY=\\\"$${SC_DEFAULT_APIKEY}\\\"
}

win32 {
  LIBS += -lwininet -lshell32
  RC_FILE += simcqt.rc
}

macx {
  CONFIG(qt) {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
    CONFIG += to_install
    QMAKE_INFO_PLIST = qt/Simulationcraft.plist
    ICON = qt/icon/Simcraft2.icns
    LIBS += -framework CoreFoundation -framework AppKit -framework Security
    DEFINES += SIMC_NO_AUTOUPDATE

    # Silence annoying warning for now
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override

    Resources.files = Welcome.html Welcome.png Error.html
    Resources.path = Contents/Resources
    Profiles.files = profiles/Tier19P profiles/Tier19H profiles/Tier19M
    Profiles.path = Contents/Resources/profiles
    Localization.files = locale/sc_de.qm locale/sc_it.qm locale/sc_zh.qm
    Localization.path = Contents/Resources/locale
    QMAKE_BUNDLE_DATA += Profiles Resources Localization
    QMAKE_DISTCLEAN += simc *.dmg
  }

  SIMC_RELEASE_OPTS = OPTS=\"-fPIE -mmacosx-version-min=10.7 -std=c++0x -stdlib=libc++ -fomit-frame-pointer -arch i386 -arch x86_64 -O3 -msse2 -march=native -ffast-math -flto -Os -DNDEBUG\"
  ! isEmpty( SC_DEFAULT_APIKEY ) {
    SIMC_RELEASE_OPTS += SC_DEFAULT_APIKEY=\"$${SC_DEFAULT_APIKEY}\"
  }

  release_simc.target = release_simc
  release_simc.commands = make -C engine clean && make CXX=clang++ $${SIMC_RELEASE_OPTS} -C engine install

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
    DEFINES += SC_SHARED_DATA=\\\"$$PREFIX/share/$$ORG_NAME/$$APP_NAME\\\"
  }
}

# GUI files
CONFIG(qt) {
include(source_files/QT_gui.pri)
}

CONFIG(openssl) {
  DEFINES += SC_USE_OPENSSL
  !isEmpty(OPENSSL_INCLUDES) {
    INCLUDEPATH += $$OPENSSL_INCLUDES
  }

  !isEmpty(OPENSSL_LIBS) {
    LIBS += -L$$OPENSSL_LIBS
  }

  win32 {
    LIBS += -lssleay32
  }
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
  !isEmpty(DESTDIR): PREFIX=$$DESTDIR/$$PREFIX
  isEmpty(PREFIX): PREFIX=/usr/local
  isEmpty(DATADIR): DATADIR=$$PREFIX/share
  isEmpty(SEARCH): SEARCH=$$DATADIR/$$ORG_NAME/$$APP_NAME
  isEmpty(BINDIR): BINDIR=$$PREFIX/bin
  INSTALLS += target \
              profiles \
              data \
              icon \
              locale

  SHAREPATH = $$DESTDIR$$PREFIX/share/$$ORG_NAME/$$APP_NAME

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
  locale.files += locale/*.qm
  locale.commands = @echo Installing localizations to $$SHAREPATH/locale
}
  
CONFIG(paperdoll) {
  DEFINES += SC_PAPERDOLL
  HEADERS += qt/sc_PaperDoll.hpp
  SOURCES += qt/sc_PaperDoll.cpp
}
