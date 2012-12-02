TEMPLATE = app
TARGET = SimulationCraft
QT += core gui network webkit
#CONFIG += paperdoll
#CONFIG += openssl

exists( build.conf ) {
  include( build.conf )
}

QMAKE_CXXFLAGS_RELEASE += -DNDEBUG
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS += $$OPTS

win32 {
  LIBS += -lwininet
  RC_FILE += simcqt.rc

  # OpenSSL stuff:
  OPENSSL_INCLUDES = C:/OpenSSL-Win32/include
  OPENSSL_LIBS = C:/OpenSSL-Win32/lib
}

macx {
  QMAKE_INFO_PLIST = qt/Simulationcraft.plist
  ICON = qt/icon/Simcraft2.icns
  OBJECTIVE_SOURCES += qt/sc_mac_update.mm
  LIBS += -framework CoreFoundation -framework Sparkle -framework AppKit
}

COMPILER_CHECK_CXX = $$replace(QMAKE_CXX,'.*g\\+\\+'.*,'g++')

contains(COMPILER_CHECK_CXX,'g++') {
  QMAKE_CXXFLAGS += -ffast-math -Woverloaded-virtual
}

INCLUDEPATH += engine
DEPENDPATH += engine

HEADERS += engine/simulationcraft.hpp
HEADERS += engine/sc_generic.hpp
HEADERS += engine/sc_timespan.hpp
HEADERS += engine/sc_sample_data.hpp
HEADERS += engine/sc_rng.hpp
HEADERS += engine/dbc/data_enums.hh
HEADERS += engine/dbc/data_definitions.hh
HEADERS += engine/dbc/specialization.hpp
HEADERS += engine/dbc/dbc.hpp
HEADERS += engine/utf8.h
HEADERS += engine/utf8/core.h
HEADERS += engine/utf8/checked.h
HEADERS += engine/utf8/unchecked.h
HEADERS += qt/sc_autoupdate.h
HEADERS += qt/simulationcraftqt.hpp

SOURCES += engine/sc_action.cpp
SOURCES += engine/sc_action_state.cpp
SOURCES += engine/sc_attack.cpp
SOURCES += engine/sc_buff.cpp
SOURCES += engine/sc_consumable.cpp
SOURCES += engine/sc_cooldown.cpp
SOURCES += engine/sc_dot.cpp
SOURCES += engine/sc_enchant.cpp
SOURCES += engine/sc_event.cpp
SOURCES += engine/sc_expressions.cpp
SOURCES += engine/sc_gear_stats.cpp
SOURCES += engine/sc_heal.cpp
SOURCES += engine/sc_item.cpp
SOURCES += engine/sc_option.cpp
SOURCES += engine/sc_pet.cpp
SOURCES += engine/sc_player.cpp
SOURCES += engine/sc_plot.cpp
SOURCES += engine/sc_raid_event.cpp
SOURCES += engine/sc_rating.cpp
SOURCES += engine/sc_reforge_plot.cpp
SOURCES += engine/sc_rng.cpp
SOURCES += engine/sc_scaling.cpp
SOURCES += engine/sc_sequence.cpp
SOURCES += engine/sc_set_bonus.cpp
SOURCES += engine/sc_sim.cpp
SOURCES += engine/sc_spell_base.cpp
SOURCES += engine/sc_harmful_spell.cpp
SOURCES += engine/sc_absorb.cpp
SOURCES += engine/sc_stats.cpp
SOURCES += engine/sc_thread.cpp
SOURCES += engine/sc_unique_gear.cpp
SOURCES += engine/sc_util.cpp
SOURCES += engine/sc_weapon.cpp
SOURCES += engine/report/sc_report_html_player.cpp
SOURCES += engine/report/sc_report_html_sim.cpp
SOURCES += engine/report/sc_report_text.cpp
SOURCES += engine/report/sc_report_xml.cpp
SOURCES += engine/report/sc_report.cpp
SOURCES += engine/report/sc_chart.cpp
SOURCES += engine/dbc/sc_const_data.cpp
SOURCES += engine/dbc/sc_item_data.cpp
SOURCES += engine/dbc/sc_spell_data.cpp
SOURCES += engine/dbc/sc_spell_info.cpp
SOURCES += engine/dbc/sc_data.cpp
SOURCES += engine/interfaces/sc_bcp_api.cpp
SOURCES += engine/interfaces/sc_chardev.cpp
SOURCES += engine/interfaces/sc_http.cpp
SOURCES += engine/interfaces/sc_js.cpp
SOURCES += engine/interfaces/sc_rawr.cpp
SOURCES += engine/interfaces/sc_wowhead.cpp
SOURCES += engine/interfaces/sc_xml.cpp
SOURCES += engine/class_modules/sc_death_knight.cpp
SOURCES += engine/class_modules/sc_druid.cpp
SOURCES += engine/class_modules/sc_enemy.cpp
SOURCES += engine/class_modules/sc_hunter.cpp
SOURCES += engine/class_modules/sc_mage.cpp
SOURCES += engine/class_modules/sc_monk.cpp
SOURCES += engine/class_modules/sc_paladin.cpp
SOURCES += engine/class_modules/sc_priest.cpp
SOURCES += engine/class_modules/sc_rogue.cpp
SOURCES += engine/class_modules/sc_shaman.cpp
SOURCES += engine/class_modules/sc_warlock.cpp
SOURCES += engine/class_modules/sc_warrior.cpp
SOURCES += qt/main.cpp
SOURCES += qt/sc_window.cpp
SOURCES += qt/sc_import.cpp

CONFIG(paperdoll) {
  DEFINES += SC_PAPERDOLL
  HEADERS += qt/simcpaperdoll.hpp
  SOURCES += qt/simcpaperdoll.cc
}

CONFIG(openssl) {
  DEFINES += SC_USE_OPENSSL
  INCLUDEPATH += $$OPENSSL_INCLUDES
  LIBS += -L$$OPENSSL_LIBS -lssleay32
}


# deployment for linux
DISTFILES += CHANGES \
    COPYING
unix:!mac {
 
    isEmpty(PREFIX):PREFIX = ~/
    SIMCDIR = $$PREFIX/SimulationCraft
    SHAREDIR = ~/.local/share
    INSTALLS += target \
    profiles \
	data \
    desktop \
    icon \
	locale

target.path = $$SIMCDIR

profiles.path = $$SIMCDIR/profiles
profiles.files += profiles/*

data.path = $$SIMCDIR
data.files += Welcome.html
data.files += Welcome.png
data.files += Legend.html
data.files += READ_ME_FIRST.txt

# TODO: Create/modify the install path into the desktop file, then install it to 
# $$ShareDir/applications
desktop.path = $$SIMCDIR
desktop.files = debian/simulationcraft.desktop

icon.path = $$SIMCDIR
icon.files = debian/simulationcraft.xpm

locale.path = $$SIMCDIR/locale
locale.files += locale/*

}
