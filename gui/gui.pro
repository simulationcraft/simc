include(../simulationcraft.pri)

TEMPLATE    = app
TARGET      = SimulationCraft
CONFIG     += link_prl
QT         += network webengine webenginewidgets
DEFINES    += SC_USE_WEBENGINE
LIBS       += -L../lib -lsimcengine
DESTDIR     = ..
MOC_DIR     = moc
RCC_DIR     = resources

CONFIG(release, debug|release): LIBS += -L../lib/release -lsimcengine
CONFIG(debug, debug|release): LIBS += -L../lib/debug -lsimcengine

CONFIG(to_install) {
  DEFINES += SC_TO_INSTALL
}

macx {
  ICON              = ../qt/icon/Simcraft2.icns
  LIBS             += -framework CoreFoundation -framework AppKit
  QMAKE_INFO_PLIST  = ../qt/Simulationcraft.plist
  DEFINES          += SIMC_NO_AUTOUPDATE

  contains(QMAKE_CXX, clang++) {
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override
  }

  Resources.files = ../Welcome.html ../Welcome.png ../Error.html
  Resources.path = Contents/Resources
  Profiles.files = ../profiles/Tier18N ../profiles/Tier18H ../profiles/Tier18M ../profiles/Tier19P ../profiles/Tier19H ../profiles/Tier19M
  Profiles.path = Contents/Resources/profiles
  Localization.files = ../locale/sc_de.qm ../locale/sc_it.qm ../locale/sc_zh.qm
  Localization.path = Contents/Resources/locale

  QMAKE_BUNDLE_DATA += Profiles Resources Localization
}

win32 {
  QMAKE_PROJECT_NAME = "Simulationcraft GUI"
  RC_FILE = ../qt/simcqt.rc
}

RESOURCES = \
  qt/simcqt.qrc

TRANSLATIONS = \
  locale/sc_de.ts \
  locale/sc_zh.ts \
  locale/sc_it.ts

include(../source_files/QT_gui.pri)

# Fix paths in SOURCES and HEADERS, as they need to  refer to parent
# directory for the respective subprojects.
HEADERS = $$replace(HEADERS, qt/, ../qt/)
SOURCES = $$replace(SOURCES, qt/, ../qt/)