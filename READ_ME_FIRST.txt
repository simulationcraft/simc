
Welcome to SimulationCraft!

=============================================================================================

Overview

SimulationCraft is a tool to explore combat mechanics in the popular MMO RPG World of Warcraft.

It is a multi-player event driven simulator written in C++ that models raid damage and threat 
generation.  Increasing class synergy and the prevalence of proc-based combat modifiers have 
eroded the accuracy of traditional calculators that rely upon closed-form approximations to 
model very complex mechanics. The goal of this simulator is to close the accuracy gap while 
maintaining a performance level high enough to calculate relative stat weights to aid gear 
selection.

SimulationCraft allows raid/party creation of arbitrary size, generating detailed charts and
reports for both individual and raid performance. Currently, it is a command-line tool in 
which the player profiles are specified via parameter files. 

Documentation can be found here: http://code.google.com/p/simulationcraft/wiki/StartersGuide

=============================================================================================

What is in the Archive?

Windows Platforms:
(1) READ_ME_FIRST.txt => This file
(2) simc.exe => Command-line simulation executable
(3) SimulationCraft.exe => Basic graphic user-interface.  Accepts drag-n-drop of .simc profiles.
(4) Win32OpenSSL_Light-*.exe => OpenSSL for Qt (some Windows platforms already have this). Install this if you're getting errors when using the browser in the GUI.
(5) Legend.html => Glossary for simulation output
(6) Welcome.html => Welcome screen for the GUI
(7) phonon4.dll and Qt*.dll => dlls necessary for simcqt.exe to run
(8) /profiles => All of the DPS profiles you will find in the GUI
(9) /profiles_heal => Experimental healing and tanking profiles

Posix Platforms: (Linux, OSX, etc)
(1) simc => Command-line simulation executable
(2) simcqt.exe => Alpha-level graphic user-interface.
(3) Legend.html => Glossary for simulation output

=============================================================================================

How To Run the Command-Line Tool?

SimulationCraft is a parameter-driven command-line tool.  What this means is that you cannot
simply double-click the simc executable.  Parameters are specified in a generic parm=value
format.

A starter's guide can be found here: http://code.google.com/p/simulationcraft/wiki/StartersGuide

Consider typing the following at a command-line prompt:
simc.exe armory=us,Llane,Segv iterations=10000 calculate_scale_factors=1 html=Segv.html
Here we invoke the executable and pass four parm=value pairs.
armory => us,Llane,Segv
iterations => 10000
calculate_scale_factors => 1
html => Segv.html

Since that is painful to type over and over again, it is convenient to put all of the 
parm=value pairs into a script file, one parm=value pair per line.

When using the armory=, wowhead=, etc. options to download a profile, the save= parm can be
used to generate a script: 
simc.exe armory=us,Llane,Segv save=Segv.simc
simc.exe wowhead=14320165 save=Paladin_T9_05_11_55.simc
simc.exe chardev=83984325 save=Bob.simc

Unix users will find that these generated script files can be marked as executable and
then simply be invoked directly via #! magic.

=============================================================================================

What about a Graphic User Interface?

There is a basic GUI built using Qt that is now being bundled with the CLI version.
If you associate the .simc extension with the simcqt.exe executable you will be able to 
simply double-click your config files.  Alternatively, you can simply drag-and-drop a 
config file on top of SimulationCraft.exe.  It will load the config and allow you to set up 
different options, such as calculating scale factors, after which you press "Simulate!" 
to start it running.

In order to run this on Windows, you may have to install OpenSSL.  The OpenSSL installer was
included in the zip download: Win32OpenSSL_Light-*.exe

If you get an error of "MSVCP100.dll is not found." Please install VC++ redistributable package from Microsoft.
http://www.microsoft.com/download/en/details.aspx?displaylang=en&id=5555
MSVCP100.dll should be linked automatically when building the GUI, but in certain cases it silently fails.

=============================================================================================

Parameter Reference

Detailed information on the many parameters can be found in the online documentation.
http://code.google.com/p/simulationcraft/wiki/TextualConfigurationInterface

=============================================================================================