# -------------------------------------------------
# Project created by QtCreator 2009-09-15T10:38:16
# -------------------------------------------------
QT += webkit
TARGET = simucraftqt
TEMPLATE = app
SOURCES += \
  main.cpp \
  sc_window.cpp
HEADERS += simucraftqt.h sc_autoupdate.h
ENGINEPATH = ../libsimucraft.a
INCLUDEPATH += ../engine
LIBS += $$ENGINEPATH

win32 {
	LIBS += -lwsock32
	RC_FILE = simucraftqt.rc
}

macx {
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
	QMAKE_INFO_PLIST = SparkleInfo.plist
	CONFIG += x86 x86_64
	ICON = icon/simucraft.icns
	OBJECTIVE_SOURCES += sc_mac_update.mm
	LIBS += -framework Sparkle
}
