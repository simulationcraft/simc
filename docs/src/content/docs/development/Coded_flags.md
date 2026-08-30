---
title: Coded Flags
---
# Introduction

One of the sources of confusion in simc for someone who is newer to coding, or even to coding veterans, is that when adding flags to a proc/ability/action, there's not a lot of documentation about what the flag itself does.

This is not (currently) an exhaustive list, but with time we will try and make sure it includes every flag in code.

Eventually someone will organize this. =P

Work in Progress: Unify all this with in-code documentation and creating doxygen docs. See http://simulationcraft.org/doc/structaction__t.html#pub-attribs

# Action Flags

These flags determine what an ability does during a simulation.

  * **school** - enum - default: SCHOOL\_NONE - What type of damage this spell does.
```cpp
   school = SCHOOL_PHYSICAL;
```

  * **resource\_current** - enum - What resource does this ability use.
```cpp
   resource_current = RESOURCE_RAGE;
```

  * **aoe** - int - The amount of targets that an ability impacts on. -1 will hit all targets.
```cpp
   aoe = 2; // Ability deals damage to 2 targets.
```

  * **may\_multistrike** - int - If the ability can proc multistrikes. 0 disables multistrikes, 1 enables, and -1 enables/disables multistrikes based on if the ability can crit or not.
```cpp
   may_multistrike = 1;
```

  * **dual** - bool - If set to true, this action will not be counted toward total amount of executes in reporting. Useful for abilities with parent/children attacks.

  * **callbacks** - bool - enables/disables proc callback system on the action.

  * **special** - bool - Whether or not the spell uses the yellow attack hit table.

  * **channeled** - bool - Tells the sim to not perform any other actions, as the ability is channeled.

  * **background** - bool - Enables/Disables direct execution of the ability in an action list. Abilities with background = true can still be called upon by other actions, example being deep wounds and how it is activated by devastate.

  * **use\_off\_gcd** - bool - Only useful for warrior/tanks at the moment, forces the sim to check conditions every 0.1 seconds to see if these abilities should be performed. Slows simulation down significantly.

  * **quiet** - bool - Disables/enables reporting of this action.

  * **direct\_tick**, **direct\_tick\_callbacks** Not used in WoD (?)

  * **periodic\_hit** - bool -

  * **repeating** - bool - Used for abilities that repeat themselves without user interaction, only used on autoattacks.

  * **harmful** - bool - Simplified: Will the ability pull the boss if used. Also determines whether ability can be used precombat without counting towards the 1 harmful spell limit

  * **proc** - bool - Whether or not this ability is a proc.

  * **initialized** - bool -

  * **may\_hit**, **may\_miss**, **may\_dodge**, **may\_parry**, **may\_glance**, **may\_block**, **may\_crush**, **may\_crit**, **tick\_may\_crit** - bool - Self explanatory.

  * **tick\_zero** - bool - Whether or not the ability/dot ticks immediately on usage.

  * **hasted\_ticks** - bool - Whether or not ticks scale with haste, generally only used for bleeds that do not scale with haste, or with ability drivers that deal damage every x number of seconds.

  * **split\_aoe\_damage** - bool - Splits damage evenly on aoe.

  * **base\_add\_multiplier** - double - Used with abilities that decay in damage with each target. Example: Revenge, main target takes 100% damage, 2nd target takes 66%, 3rd target 33%.

  * **dot\_behavior** - Behavior of dot. Acceptable inputs are DOT\_CLIP, DOT\_REFRESH, and DOT\_EXTEND.

  * **ability\_lag**, **ability\_lag\_stddev** - timespan\_t - Not used anymore. (??)

  * **rp\_gain** - double - Deathknight specific, how much runic power is gained.

  * **min\_gcd** - timespan\_t - The minimum gcd triggered no matter the haste.

  * **trigger\_gcd** - timespan\_t - Length of gcd triggered when used.

  * **range** - double - Distance that the ability may be used from.

  * **attack\_power\_mod.direct**, **attack\_power\_mod.tick** - double - Attack power scaling of the ability.

  * **spell\_power\_mod.direct**, **spell\_power\_mod.tick** - double - Spell power scaling of the ability.

  * **base\_execute\_time** - timespan\_t - Amount of time the ability uses to execute before modifiers.

  * **base\_tick\_time** - timespan\_t - Amount of time the ability uses between ticks.

  * **dot\_duration** - timespan\_t - Default duration of dot.

  * **base\_costs** - double - Cost of using the ability
```cpp
   base_costs[RESOURCE_RAGE] = 20; // Uses 20 rage on execute.
```

  * **costs\_per\_second** - double - Cost of using ability per second
```cpp
   costs_per_second[RESOURCE_RUNIC_POWER] = 10; // Uses 10 runic power per second.
```

  * **weapon\_multiplier** - double - Weapon damage for the ability.

  * **cooldown** - Used to manipulate cooldown duration and charges.
```cpp
   cooldown -> duration = timespan_t::from_seconds( 20 ); //Ability has a cooldown of 20 seconds.
   cooldown -> charges = 3; // Ability has 3 charges.
```

  * **movement\_directionality**
```cpp
   movement_directionality = MOVEMENT_OMNI; // Can move in any direction, ex: Heroic Leap, Blink. Generally set movement skills to this.
   movement_directionality = MOVEMENT_TOWARDS; // Can only be used towards enemy target. ex: Charge
   movement_directionality = MOVEMENT_AWAY; // Can only be used away from target. Ex: ????
```

  * **base\_teleport\_distance** - double - Maximum distance that the ability can travel. Used on abilities that instantly move you, or nearly instantly move you to a location.
```cpp
   base_teleport_distance = 40;
```
