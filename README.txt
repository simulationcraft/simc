
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
(1) simcraft.exe => Command-line simulation executable
(2) SIMCRAFT.BAT => Wrapper around simcraft.exe that will execute script files when they are 
"dropped" on top of its icon.  After completion, the output will be passed to notepad and a 
browser.
(3) SCALE_FACTORS.BAT => Similar to SIMCRAFT.BAT but will also generate scale factors, also
known as "stat weights", also known as "EP values".  In addition, these scale factors will be
used to generate http links to wowhead and lootrank for gear ranking.
(4) simcraftqt.exe => Alpha-level graphic user-interface.  See #5.
(5) Win32OpenSSL_Light-0_9_8k.exe => Installs OpenSSL which is required by the Qt GUI 
(6) Examples.simcraft => Reference documentation for the many parameters

Posix Platforms: (Linux, OSX, etc)
(1) simcraft => Command-line simulation executable
(2) simcraftqt.exe => Alpha-level graphic user-interface.
(3) Examples.simcraft => Reference documentation for the many parameters

=============================================================================================

How To Run?

SimulationCraft is a parameter-driven command-line tool.  What this means is that you cannot
simply double-click the simcraft executable.  Parameters are specified in a generic parm=value
format.

Consider typing the following at a command-line prompt:
simcraft.exe armoy=us,Llane,Segv iterations=10000 calculate_scale_factors=1 html=Segv.html
Here we invoke the executable and pass four parm=value pairs.
armory => us,Llane,Segv
iterations => 10000
calculate_scale_factors => 1
html => Segv.html

Since that is painful to type over and over again, it is convenient to put all of the 
parm=value pairs into a script file, one parm=value pair per line.

When using the armory= or wowhead= options to download a profile, the save= parm can be
used to generate a script: 
simcraft.exe armory=us,Llane,Segv save=Segv.simcraft

Unix users will find that these generated script file can be marked as executable and
then simply be invoked directly via #! magic.

Windows users may take advantage of the SIMCRAFT.BAT and SCALE_FACTORS.BAT utiities by
drag-n-dropping script files onto these icons.  They will generate output of the
form "script.txt" and "script.html".

For example:
Step 1: simcraft.exe armory=us,Llane,Segv save=Segv.simcraft
Step 2: Drag-n-Drop the Segv.simcraft file onto SIMCRAFT.BAT to generate DPS info.
Step 3: Drag-n-Drop the Segv.simcraft file onto SCALE_FACTORS.BAT to get DPS and stat weights

=============================================================================================

Parameter Reference

Detailed information on the many parameters can be found in Examples.simcraft

=============================================================================================

Graphic User Interface

simcraftqt.exe is an alpha-level graphic user-interface using Qt.  It is a very light-weight
wrapper that aids profile downloads, script building, and simulation results management.

