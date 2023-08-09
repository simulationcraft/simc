TEMPLATE = subdirs
SUBDIRS = lib cli

cli.depends = lib

# Windows Arm64 does not support QtWebEngine; Do not build GUI
!win32-arm64-msvc {
  SUBDIRS += gui
  gui.depends = lib
}

lessThan( QT_MAJOR_VERSION, 5 ) {
  error( "SimulationCraft requires QT 5 or higher." )
}

# OS X release target
macx {
  create_release.target   = create_release
  create_release.depends  = all
  create_release.commands = $$dirname(QMAKE_QMAKE)/macdeployqt Simulationcraft.app && qt/osx_release.sh

  QMAKE_EXTRA_TARGETS += create_release
}
