
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

=============================================================================================

What is in the Install?

Windows Platforms:
(1) simc.exe => Command-line simulation executable
(2) simcqt.exe => Alpha-level graphic user-interface.  Accepts drag-n-drop of .simc profiles.
(3) Win32OpenSSL_Light-0_9_8k.exe => Installs OpenSSL which may be required by the Qt GUI 
(4) Examples.simc => Reference documentation for the many parameters
(5) Legend.html => Glossary for simulation output
(6) mingw*.dll and Qt*.dll => dlls necessary for simcqt.exe to run

Posix Platforms: (Linux, OSX, etc)
(1) simc => Command-line simulation executable
(2) simcqt.exe => Alpha-level graphic user-interface.
(3) Examples.simc => Reference documentation for the many parameters
(4) Legend.html => Glossary for simulation output

=============================================================================================

How To Run?

SimulationCraft is a parameter-driven command-line tool.  What this means is that you cannot
simply double-click the simc executable.  Parameters are specified in a generic parm=value
format.

Consider typing the following at a command-line prompt:
simc.exe armoy=us,Llane,Segv iterations=10000 calculate_scale_factors=1 html=Segv.html
Here we invoke the executable and pass four parm=value pairs.
armory => us,Llane,Segv
iterations => 10000
calculate_scale_factors => 1
html => Segv.html

Since that is painful to type over and over again, it is convenient to put all of the 
parm=value pairs into a script file, one parm=value pair per line.

When using the armory= or wowhead= options to download a profile, the save= parm can be
used to generate a script: 
simc.exe armory=us,Llane,Segv save=Segv.simc
simc.exe wowhead=14320165 save=Paladin_T9_05_11_55.simc

Unix users will find that these generated script files can be marked as executable and
then simply be invoked directly via #! magic.

Windows users may take advantage of the SIMC.BAT and SCALE_FACTORS.BAT utiities by
drag-n-dropping script files onto these icons.  They will generate output in the
form "script.txt" and "script.html".

For example:
Step 1: simc.exe armory=us,Llane,Segv save=Segv.simc
Step 2: Drag-n-Drop the Segv.simc file onto SIMC.BAT to generate DPS info.
Step 3: Drag-n-Drop the Segv.simc file onto SCALE_FACTORS.BAT to get DPS and stat weights

=============================================================================================

Parameter Reference

Detailed information on the many parameters can be found in Examples.simc

=============================================================================================

Graphic User Interface

simcqt.exe is an alpha-level graphic user-interface using Qt.  It is a very light-weight
wrapper that aids profile downloads, script building, and simulation results management.

In order to run this on Windows, you may have install OpenSSL.  The OpenSSL installer was
included in the zip download: Win32OpenSSL_Light-0_9_8k.exe

