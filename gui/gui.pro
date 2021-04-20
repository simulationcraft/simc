include(../simulationcraft.pri)

TEMPLATE    = app
TARGET      = SimulationCraft
CONFIG     += link_prl
QT         += network widgets webengine webenginewidgets
LIBS       += -L../lib -lsimcengine
INCLUDEPATH += ../qt
MOC_DIR     = moc
RCC_DIR     = resources
DEFINES += SC_USE_WEBENGINE
TRANSLATIONS = \
  ../locale/sc_de.ts \
  ../locale/sc_cn.ts \
  ../locale/sc_it.ts \
  ../locale/sc_ko.ts

# Linux puts binaries to a different place (see simulationcraft.pri)
win32|macx {
  DESTDIR   = ..
}

CONFIG(release, debug|release): LIBS += -L../lib/release -lsimcengine
CONFIG(debug, debug|release): LIBS += -L../lib/debug -lsimcengine

CONFIG(to_install) {
  DEFINES += SC_TO_INSTALL
}

Resources.files = ../qt/Welcome.html ../qt/Welcome.png ../qt/Error.html
Localization.files = $$files(../locale/*.qm)
Profiles.files = $$files(../profiles/*, recursive=true)


macx {
  ICON              = ../qt/icon/Simcraft2.icns
  LIBS             += -framework CoreFoundation -framework AppKit
  QMAKE_INFO_PLIST  = ../qt/Simulationcraft.plist
  DEFINES          += SIMC_NO_AUTOUPDATE

  contains(QMAKE_CXX, .+/clang\+\+) {
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override
  }

  Resources.path = Contents/Resources
  Profiles.path = Contents/Resources/profiles
  Localization.path = Contents/Resources/locale

  QMAKE_BUNDLE_DATA += Profiles Resources Localization
}

win32 {
  QMAKE_PROJECT_NAME = "Simulationcraft GUI"
  #RC_FILE = ../qt/simcqt.rc
  RC_ICONS = ../qt/icon/Simcraft2.ico
  CONFIG(debug, debug|release) {
    CONFIG += console
  }
}

# Deplopyment for Linux, note, the cli project also copies profiles
unix:!macx {

  DISTFILES  += CHANGES COPYING
  INSTALLS   += target Profiles Resources icon Localization
  # Disable strip
  QMAKE_STRIP = echo

  target.path = $$DESTDIR$$PREFIX/bin/

  Profiles.path = $$SHAREPATH/profiles
  profiles.commands = @echo Installing profiles to $$SHAREPATH/profiles

  Resources.path = $$SHAREPATH
  data.commands = @echo Installing global files to $$SHAREPATH

  icon.path = $$SHAREPATH
  icon.files = ../qt/icon/SimulationCraft.xpm
  icon.commands = @echo Installing icon to $$SHAREPATH

  Localization.path = $$SHAREPATH/locale
  Localization.commands = @echo Installing localizations to $$SHAREPATH/locale
}


include(../source_files/QT_gui.pri)

# Fix paths in SOURCES and HEADERS, as they need to  refer to parent
# directory for the respective subprojects.
HEADERS = $$replace(HEADERS, qt/, ../qt/)
SOURCES = $$replace(SOURCES, qt/, ../qt/)
