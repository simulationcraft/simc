---
title: simc_gui.simc
---
simc_gui.simc
==============

Introduction
-------------
The *simc_gui.simc* file contains **all** settings of the last simulation performed by the SimulationCraft GUI, including all options selected, overrides and the profiles simulated.

It serves as a documentation of the input used for the simulation, which can be use to replicate the exact same simulation with the command line client (CLI).

More importantly, it can help to replicate errors/bugs encountered by users if they share the *simc_gui.simc* file with developers in a bug report.

Location
---------
The default location is returned by QStandardPaths::CacheLocation (http://doc.qt.io/qt-5/qstandardpaths.html#StandardLocation-enum)

OS     |  Path
-------- | ---------
Windows | "`<`executable-dir`>`/simc_gui.simc"
OsX | "~/Library/Application Support/SimulationCraft/SimulationCraft/simc_gui.simc"
Linux | "~/.cache/SimulationCraft/SimulationCraft/simc_gui.simc" or "~/.local/share/SimulationCraft/SimulationCraft/simc_gui.simc"



Description
-----------
The file contains the following sections:

* GUI options: All options specified in the Options tab.
* simulateText: Profiles to simulate found in the simulate tab active when pressing "Simulate".
* overrides: All manual options from the override tab.
* final options: special debug options.
