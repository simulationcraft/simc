
Welcome to SimulationCraft!

There have been some changes to the interface, so please read carefully.

The following files are no longer part of the distribution:
raid_70.txt
raid_80.txt
melee.txt
CLICK_ME_TO_RUN.BAT

The standard config files have been unpacked into a hierarchy of individual files.
Raid_T7.simcraft
Globals_T7.simcraft
Caster_T7.simcraft
Melee_T7.simcraft

Each class/talents/actions combination now has its own config file.  

The support of "optimal_raid=1" now means that a single player can be simulated (using
optimal raid buffs/debufs) resulting in much faster performance.

In addition every variation is combined into class-specific config files that make use 
of the default_xxx gear notation for conveniently comparing multiple specs across many
gear levels.

Unix users will find that each config file is now an executable that can be simply invoked.

Windows users may take advantage of the SIMCRAFT.BAT and SCALE_FACTORS.BAT scripts by
drag-n-dropping config files onto these icons.  They will generate output of the
form: confg_name.txt and config_name.html

The following Wiki page offers a more detailed resource on how to use the tool:

http://code.google.com/p/simulationcraft/wiki/HowToRun


