---
title: Death Knights
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Textual configuration interface
_This section is a part of the [TCI](TextualConfigurationInterface) reference._

Regular spells are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting).

## Rune expression system

**Since Simulationcraft 7.0.3, release 1** Runes are treated as a standard resource in the simulator. Normal resource expressions work on runes, with the exception of the `regen` and `time_to_max` resource expressions.

You can also use rune.time_to_X to return the time until X runes are available.

For example you can use this line to only execute obliterate if the actor has 4 or more runes available, and frost strike if the time until 3 runes are available is superior to the length of the global cooldown.
```
actions+=/obliterate,if=rune>=4
actions+=/frost_strike,if=rune.time_to_3>gcd
```


## Incoming damage
The expression `incoming_damage_5s` returns the amount of damage the Death Knight has taken in the previous five seconds.
```
 # Death Strike if the actor has taken 25% of maxhp damage in the previous 5 seconds.
 actions+=/death_strike,if=incoming_damage_5s>=health.max*0.25
```

## Synchronizing weapons
The _sync\_weapons_ (default: 1) option on the _auto\_attack_ action can be used to force the synchronization of weapons at the beginning of the fight. When zero, the offhand will be desynchronized by half of its swing time. In game, you always start with your weapons synchronized but target switching and parry rushes often lead you to go unsynchronized.
```
 # Ensure the player will start with synched weapons.
 actions+=/auto_attack,sync_weapons=1
```

## Anti-Magic Shell
The _antimagic\_shell_ action allows a Death Knight to simulate the Runic Power gain on incoming damage. The action contains three options: **interval**, **interval\_stddev**, and **damage**. The **interval** option sets the mean of the interval between two consecutive Anti-Magic Shell executions in seconds, and is required to be at minimum the cooldown of the spell. The **interval\_stddev** option sets the standard deviation of the interval between Anti-Magic Shell executions. If specified as less than 1, it is interpreted as a percent of the mean, otherwise it is interpreted as seconds. Finally, the **damage** option sets the amount of incoming damage. The default values for **interval**, and **interval\_stddev** are 60 and 5% respectively. The **damage** option is always required.
```
 # Simulate Runic Power gain on using Anti-Magic Shell to absorb 100000 magic damage every 60 seconds on average.
 actions+=/antimagic_shell,damage=100000
```

## Death Knight Runeforge Expressions

With Shadowlands increasing the variety of available runeforges that can be applied to Death Knight weapons, a new expression type has been added to the shadowlands branch to evaluate from the APL whether a given runeforge is equipped or not by a character.
It follows the death_knight.runeforge.name format and returns 1 if the runeforge is applied to one of the character's weapon and 0 otherwise.

The following names can be used: razorice, razorice_mh, razorice_oh, fallen_crusader, stoneskin_gargoyle, apocalypse, hysteria, sanguination, spellwarding, unending_thirst.
```
# Use Frostscythe only if razorice is equipped on the main hand
actions+=/frostscythe,if=death_knight.runeforge.razorice_mh
```
Note: the simpler runeforge.name format is also supported, but since it is intended to be used with Shadowlands Runeforge Legendary effects, the simulation will output a warning telling the user to use death_knight.runeforge.name instead.

## Dynamic actions

Some action names can be used in the death knight APLs to generate different spells depending on talent choice.
``wound_spender`` will be replaced by ``scourge_strike``, or ``clawing_shadows`` if it is talented.
``dnd_any`` and ``any_dnd`` will be replaced by ``death_and_decay`` or ``defile`` if it is talented.
At the moment, Death's Due isn't implemented in simc yet and no decision has been made on whether it will be included in ``any_dnd`` or not yet.


## Miscellanous

Army of the dead has an option to set the time at which it is used before pull. It only works when army of the dead is used in the precombat APL and won't have any effect otherwise. It takes a value between 1.5 (a full gcd before pull) and 10. Default value: 6
```
# Simulate the cast of army of the dead 4s before combat begins
actions.precombat+=/army_of_the_dead,precombat_time=4
```

Army of the Dead can be entirely disabled from a death knight profile with disable_aotd. (scope: player, default: 0)
This is made to make the process of running shorter fights that may not use army of the dead without fiddling with the Action Priority List.
```
# Run a simulation that will not use Army of the Dead
deathknight.disable_aotd=1
```

The state of the option can also be checked from the APL with death_knight.disable_aotd.
```
# Use Festering Strike unless army of the dead will be available soon and it's not disabled
actions+=/festering_strike,if=cooldown.army_of_the_dead.remains>6|death_knight.disable_aotd
```

An expression is available for the APL to count the number of unique enemies currently affected by Festering Wounds (not how many wounds there are).
It can be called with death_knight.fwounded_targets
```
# Use Scourge Strike during Death and Decay if at least 4 enemies are affected by Festering Wounds
actions+=/scourge_strike,if=death_and_decay.ticking&death_knight.fwounded_targets>=4
```

An option is available to specify use timings for Anti-Magic Zone, as if you were assigned specific timings for a boss fight
It can be utilized with deathknight.amz_use_time=x/y/z as a player scope option.
```
# Use AMZ at 15s, 135s and 255s
deathknight.amz_use_time=15/135/255
```

AMS has multiple custom options that can be used to examine the effects of Runic Power gains.
```
# A 0-1 value that sets how much of AMS is used per cast, default is 0
deathknight.ams_absorb_percent=.8
# How many seconds the sim should wait before using AMS for the first time, default is 20
deathknight.first_ams_cast=20
```
