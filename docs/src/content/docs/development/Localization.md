---
title: Localization
---


# Introduction
The SimulationCraft GUI can be localized to specific languages. This is a small how-to if you want to contribute to that effort.


# Requirements
  * SimulationCraft source code from git
  * A QT installation. See HowToBuild on how to install QT for your platform.


# Adding a new Language / Translation File (.ts)
  1. Open simulationcraft.pro with QT Creator
  1. Edit gui/gui.pro and add a new translation file to **TRANSLATIONS**. For example, to add a German translation we add a ´locale/sc\_de.ts´ file.
  1. In QT Creator, run Extras -> Extern -> Linguist -> update translation (lupdate). This creates or updates a language translation file (.ts).
  1. If the language is new, edit qt/sc\_OptionsTab.cpp and add the new language in the GUI Localization array, incrementing the index too
  * For example, if you're adding a french translation:
```
globalsLayout_left -> addRow( tr( "GUI Localization" ),     choice.gui_localization = createChoice( 5, "auto", "en", "de", "zh", "it" ) );
// Becomes:
globalsLayout_left -> addRow( tr( "GUI Localization" ),     choice.gui_localization = createChoice( 6, "auto", "en", "de", "zh", "it", "fr" ) );
// Notice the 5 turning to 6, to match the new array size 
```


# Doing the translation
  1. In your QT bin directory, start linguist.exe ( or similar ).
  1. Open the desired translation file from `<`simc-source-dir`>`/locale
  1. Add or update the translations. Save everything.

# Exporting a translation file
For a translation file to be used in the GUI, it needs to be exported from a .ts file to a .qm file.

This can be done again in QTCreator under Extras -> Extern -> Linguist -> release translation (lrelease )

# Testing your translation
When building SimC GUI in debug mode, you'll get debug messages in the console you are running it. You should get a line

`[Localization]: Trying to load local file from: <path>/sc_<lang>.qm `

Now just ensure that your exported .qm file is in the correct location and launch SimC again.

# Adjusting paths for packaged (nightly) builds
Adjust [Windows build script](https://github.com/simulationcraft/simc/blob/ff1682b77339d432f7bc104c071e4008578620fb/WinReleaseScripts/build-simc.bat#L112)
