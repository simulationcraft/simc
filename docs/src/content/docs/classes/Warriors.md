---
title: Warriors
---
Warrior specific options:

Note that Protection simulations are currently disabled as the module hasn't been fully updated for Battle for Azeroth yet.

into_the_fray_friends (prot)
  - Default value: -1
  - This tells the sim how many friends you will have within 10 yards of you, which will increase the amount of haste that you get from the talent. "-1" (or any negative value) will assume max stacks at full uptime.
  - Could be helpful for finding breakpoints of when the talent should be used.
  - Takes enemies into account on top of it, so if you set it to 1 and run patchwerk simulations, it will grant you 2 stacks from the haste buff.

never_surrender_percentage (prot)
  - Default value : 70
  - Takes an integer value between -1 and 100 that will be used to determine the hp percentage of the player when casting Ignore Pain with the Never Surrender talent. Setting the option to -1 will have the simulation use the current hp of the warrior, rather than a set specified amount.

warrior_fixed_time
  - Default Value: 1
  - This is enabled by default so that <20% bosses live longer in a simulation with only warriors. 

non_dps_mechanics
  - Default value: 0
  - Enabling this will enable non dps stuff like healing from bloodthirst. 
