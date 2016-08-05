TEMPLATE = subdirs
SUBDIRS = lib cli gui

cli.depends = lib
gui.depends = lib
