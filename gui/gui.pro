include(../simulationcraft.pri)

TEMPLATE    = app
TARGET      = SimulationCraft
CONFIG     += link_prl c++14
QT         += network widgets
LIBS       += -L../lib -lsimcengine
MOC_DIR     = moc
RCC_DIR     = resources

# Linux puts binaries to a different place (see simulationcraft.pri)
win32|macx {
  DESTDIR   = ..
}

CONFIG(release, debug|release): LIBS += -L../lib/release -lsimcengine
CONFIG(debug, debug|release): LIBS += -L../lib/debug -lsimcengine

CONFIG(to_install) {
  DEFINES += SC_TO_INSTALL
}

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

# QT 5.4 for MinGW does not yet contain the new Web Engine
win32-mingw {
  greaterThan( QT_MINOR_VERSION, 3 ) {
      QT -= webengine webenginewidgets
      QT += webkit webkitwidgets
      DEFINES -= SC_USE_WEBENGINE
      DEFINES += SC_USE_WEBKIT
      message("MinGW WebKit only")
  }
}

macx {
  ICON              = ../qt/icon/Simcraft2.icns
  LIBS             += -framework CoreFoundation -framework AppKit
  QMAKE_INFO_PLIST  = ../qt/Simulationcraft.plist
  DEFINES          += SIMC_NO_AUTOUPDATE

  contains(QMAKE_CXX, .+/clang\+\+) {
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override
  }

  Resources.files = ../Welcome.html ../Welcome.png ../Error.html
  Resources.path = Contents/Resources
  Profiles.files =  ../profiles/PreRaids ../profiles/Tier22 ../profiles/Tier23
  Profiles.path = Contents/Resources/profiles
  Localization.files = ../locale/sc_de.qm ../locale/sc_it.qm ../locale/sc_zh.qm
  Localization.path = Contents/Resources/locale

  QMAKE_BUNDLE_DATA += Profiles Resources Localization
}

win32 {
  QMAKE_PROJECT_NAME = "Simulationcraft GUI"
  #RC_FILE = ../qt/simcqt.rc
  RC_ICONS = ../qt/icon/Simcraft2.ico
  DEFINES += VS_NEW_BUILD_SYSTEM
  CONFIG(debug, debug|release) {
    CONFIG += console
  }
}

# Deplopyment for Linux, note, the cli project also copies profiles
unix:!macx {

  !qtHaveModule(webengine) {
    QT -= webengine webenginewidgets
    QT += webkit webkitwidgets
    DEFINES -= SC_USE_WEBENGINE
    DEFINES += SC_USE_WEBKIT
    message("Linux WebKit default")
  }

  DISTFILES  += CHANGES COPYING
  INSTALLS   += target profiles data icon locale
  # Disable strip
  QMAKE_STRIP = echo

  target.path = $$DESTDIR$$PREFIX/bin/

  profiles.path = $$SHAREPATH/profiles
  profiles.files += ../profiles/*
  profiles.commands = @echo Installing profiles to $$SHAREPATH/profiles

  data.path = $$SHAREPATH
  data.files += ../Welcome.html
  data.files += ../Welcome.png
  data.files += ../READ_ME_FIRST.txt
  data.files += ../Error.html
  data.commands = @echo Installing global files to $$SHAREPATH

  icon.path = $$SHAREPATH
  icon.files = ../debian/simulationcraft.xpm
  icon.commands = @echo Installing icon to $$SHAREPATH

  locale.path = $$SHAREPATH/locale
  locale.files += ../locale/*.qm
  locale.commands = @echo Installing localizations to $$SHAREPATH/locale
}

RESOURCES = \
  qt/simcqt.qrc

TRANSLATIONS = \
  ../locale/sc_de.ts \
  ../locale/sc_zh.ts \
  ../locale/sc_it.ts

include(../source_files/QT_gui.pri)

# Fix paths in SOURCES and HEADERS, as they need to  refer to parent
# directory for the respective subprojects.
HEADERS = $$replace(HEADERS, qt/, ../qt/)
SOURCES = $$replace(SOURCES, qt/, ../qt/)
