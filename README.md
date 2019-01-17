# [SimulationCraft](https://www.simulationcraft.org/) [![Build Status](https://travis-ci.org/simulationcraft/simc.svg)](https://travis-ci.org/simulationcraft/simc) [![Build status](https://ci.appveyor.com/api/projects/status/4e7fxyik4cuu4gxf?svg=true)](https://ci.appveyor.com/project/scamille/simc)
## Overview

SimulationCraft is a tool to explore combat mechanics in the popular MMO RPG World of Warcraft (tm).

It is a multi-player event driven simulator written in C++ that models player character damage-per-second in various raiding and dungeon scenarios.

Increasing class synergy and the prevalence of proc-based combat modifiers have eroded the accuracy of traditional calculators that rely upon closed-form approximations to model very complex mechanics. The goal of this simulator is to close the accuracy gap while maintaining a performance level high enough to calculate relative stat weights to aid gear selection.

SimulationCraft allows raid/party creation of arbitrary size, generating detailed charts and reports for both individual and raid performance.

A simple graphical interface is included with the tool, allowing players to download and analyze characters from the Armory. It can also be run from the command-line in which case the player profiles are specified via parameter files.

## How Can I Use It?

Go to the [downloads page](https://www.simulationcraft.org/download.html) and get the package for your particular platform.
The Windows package offers both a formal install and a archive that can be extracted on to your desktop. There is no Linux release since it is so ridiculously easy to build it yourself on that platform. Releases occur quite frequently so be sure to check the release notes.

Two executables are shipped: *SimulationCraft* sports a simple graphical user interface whereas *simc* uses a command-line interface.

Launching SimulationCraft will present you with an explanation on how to use the tool.

There is also an excellent [starters guide](../../wiki/StartersGuide) on our wiki.


## How Can I Get Help?

For a simple overview, thoroughly read the Welcome page that is presented upon launching SimulationCraft. In addition, detailed documentation material can be found on our [wiki pages](../../wiki/). Here you will find a list of features, a starters guide, as well as answers to frequently asked questions.

If your question is not answered there, then see the [Community](#community) section below on how to reach other users via Discord. We periodically scan the WoW, MMO, and Wowhead forums as well, but we cannot promise swift responses in those arenas.

If you have detailed questions that need to be answered in real time then I recommend you visit the Discord server detailed in the [Community](#community) section below. There are active SimulationCraft developers on the server 24hrs a day.

If you believe that the reported analysis is incorrect please open an [issue](../../issues). If you are unable to download your character, please open an [issue](../../issues). If you feel that the tool is missing features/directives necessary for analysis, please open an [issue](../../issues). Opening an [issue](../../issues) (as opposed to an email, forum post, chat msg, etc) is by far the most effective method of getting a swift resolution to your problem.

Also make sure to check our [common issues wiki](../../wiki/CommonIssues).


## How Can I Contribute?
See [Contributing](CONTRIBUTING.md).

## Community

Discord: [SimCMinMax](https://discord.gg/tFR2uvK) (#simulationcraft)

## Important Notice

SimulationCraft is different from SimCraft. Please use the full name SimulationCraft (or SimC) to refer to this project. Visit SimCraft if you are looking for full-motion simulators for SimRacing and FlightSim.

## External Libraries

This program uses the following external libraries.

RapidJSON (http://rapidjson.org)

Copyright (c) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All rights reserved.
MIT License (see LICENSE.MIT for more information).

RapidXML (http://rapidxml.sourceforge.net/index.htm)

Copyright (c) 2006, 2007 Marcin Kalicinski. All rights reserved.
MIT License (see LICENSE.MIT for more information).

The MSInttypes r29 (https://code.google.com/p/msinttypes/)

Copyright (c) Alexander Chemeris. All rights reserved.
BSD 3-Clause License (see LICENSE.BSD for more information).

The Qt Toolkit (https://www.qt.io/)

Copyright (c) 2016 The Qt Company Ltd. and other contributors. All rights reserved.
GNU Lesser General Public License, version 3 (see LICENSE.LGPL for more information).

UTF-8 CPP (http://utfcpp.sourceforge.net)

Copyright (c) 2006 Nemanja Trifunovic. All rights reserved.
Boost Software License, Version 1.0 (see LICENSE.BOOST for more information).

{fmt} (https://github.com/fmtlib/fmt)

Copyright (c) 2012 - 2016, Victor Zverovich. All rights reserved.
BSD 2-Clause "Simplified" License (see LICENSE.BSD2 for more information).
