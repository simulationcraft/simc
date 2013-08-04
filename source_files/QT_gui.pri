HEADERS += qt/sc_autoupdate.h
HEADERS += qt/simulationcraftqt.hpp
SOURCES += qt/main.cpp
SOURCES += qt/sc_window.cpp
SOURCES += qt/sc_import.cpp
SOURCES += qt/sc_options_tab.cpp
  
  
CONFIG(paperdoll) {
  DEFINES += SC_PAPERDOLL
  HEADERS += qt/simcpaperdoll.hpp
  SOURCES += qt/simcpaperdoll.cc
}