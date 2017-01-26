# [SimulationCraft](http://www.simulationcraft.org/) [![Build Status](https://travis-ci.org/simulationcraft/simc.svg)](https://travis-ci.org/simulationcraft/simc) [![Build status](https://ci.appveyor.com/api/projects/status/4e7fxyik4cuu4gxf?svg=true)](https://ci.appveyor.com/project/scamille/simc)
## Overview

SimulationCraft is a tool to explore combat mechanics in the popular MMO RPG World of Warcraft (tm).

It is a multi-player event driven simulator written in C++ that models player character damage-per-second in various raiding scenarios.

Increasing class synergy and the prevalence of proc-based combat modifiers have eroded the accuracy of traditional calculators that rely upon closed-form approximations to model very complex mechanics. The goal of this simulator is to close the accuracy gap while maintaining a performance level high enough to calculate relative stat weights to aid gear selection.

SimulationCraft allows raid/party creation of arbitrary size, generating detailed charts and reports for both individual and raid performance.

A simple graphical interface is included with the tool, allowing players to download and analyze characters from the Armory. It can also be run from the command-line in which case the player profiles are specified via parameter files.

## How Can I Use It?

Go to the [downloads page](http://www.simulationcraft.org/download.html) and get the package for your particular platform. 
The Windows package offers both a formal install and a archive that can be extracted on to your desktop. There is no Linux release since it is so ridiculously easy to build it yourself on that platform. Releases occur quite frequently so be sure to check the release notes.

Two executables are shipped: *SimulationCraft* sports a simple graphical user interface whereas *simc* uses a command-line interface.

Launching SimulationCraft will present you with an explanation on how to use the tool.

There is also an excellent [starters guide](../../wiki/StartersGuide) on our wiki.


## How Can I Get Help?

For a simple overview, thoroughly read the Welcome page that is presented upon launching SimulationCraft. In addition, detailed documentation material can be found on our [wiki pages](../../wiki/). Here you will find a list of features, a starters guide, as well as answers to frequently asked questions.

If your question is not answered there, then see the [Community](#community) section below on how to reach other users via IRC. We periodically scan the WoW, MMO, and Wowhead forums as well, but we cannot promise swift responses in those arenas.

If you have detailed questions that need to be answered in real time then I recommend you visit the IRC channel detailed in the [Community](#community) section below. There are active SimulationCraft developers on that channel 24hrs a day.

If you believe that the reported analysis is incorrect please open an [issue](../../issues). If you are unable to download your character, please open an [issue](../../issues). If you feel that the tool is missing features/directives necessary for analysis, please open an [issue](../../issues). Opening an [issue](../../issues) (as opposed to an email, forum post, chat msg, etc) is by far the most effective method of getting a swift resolution to your problem.

Also make sure to check our [common issues wiki](../../wiki/CommonIssues).

## How Can I Make Changes to the Models?

So you found an error and want to make a quick correction? Perhaps there is an option or feature you wish to add? Time to get your hands dirty!

Each release includes all the source code used to build that release. The platform specific downloads will include both the source and the necessary build scripts. Alternatively, you can live on the bleeding edge of development and extract the very latest updates from the source code repository.

Platform specific building instructions can be found on the [How-To-Build wiki](../../wiki/HowToBuild).

## How Can I Contribute?

The SimulationCraft team is comprised of volunteer developers from all over the world. We are always looking for new contributors. While C++ expertise is certainly helpful, we have several key members with limited coding experience. Maintaining optimal talent, gear, and default action priority lists is a huge task and requires virtually zero C++ knowledge.

We are a very laid back group of developers. While certain project members have areas of expertise, there is little in the way of strict responsibility and ownership. Developers are expected to exercise their initiative and help out wherever needed. GitHub provides considerable utilities for oversight. The mantra is: Just check it in. Don't ask for permission. If the code needs to be changed for functional (or artistic!) reasons, someone will revert/modify as needed.

If you are interested in joining the team, contact us on IRC or send an email to natehieter@gmail.com with your contact info. We look forward to working with you!

Checkout the [Developers Corner wiki page](../../wiki/Participate) as well.

## Community

IRC: irc.gamers-irc.org (#simulationcraft)

## Important Notice

SimulationCraft is different from SimCraft. Please use the full name SimulationCraft (or SimC) to refer to this project. Visit SimCraft if you are looking for full-motion simulators for SimRacing and FlightSim. 

## Migrating from GoogleCode to GitHub
To switch the remote git repository from googlecode to github, execute the following in your simc development folder:
- git remote set-url origin https://github.com/simulationcraft/simc.git

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

The Qt Toolkit (http://qt.io)

Copyright (c) 2016 The Qt Company Ltd. and other contributors. All rights reserved.
GNU Lesser General Public License, version 3 (see LICENSE.LGPL for more information).

UTF-8 CPP (http://utfcpp.sourceforge.net)

Copyright (c) 2006 Nemanja Trifunovic. All rights reserved.
Boost Software License, Version 1.0 (see LICENSE.BOOST for more information).
