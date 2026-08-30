---
title: Priests
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to reach out on Discord to the [Priest team](https://github.com/orgs/simulationcraft/teams/priest/members).**

# Sim Module notes
The module is broken up into the following parts:
- Main Priest file - Holds base spells or other things available to more than one spec. ([sc_priest.cpp](https://github.com/simulationcraft/simc/blob/dragonflight/engine/class_modules/priest/sc_priest.cpp) and [sc_priest.hpp](https://github.com/simulationcraft/simc/blob/dragonflight/engine/class_modules/priest/sc_priest.hpp))
- Spec Files - Specialization specific implementations
  - [Discipline](https://github.com/simulationcraft/simc/blob/dragonflight/engine/class_modules/priest/sc_priest_discipline.cpp)
  - [Holy](https://github.com/simulationcraft/simc/blob/dragonflight/engine/class_modules/priest/sc_priest_holy.cpp)
  - [Shadow](https://github.com/simulationcraft/simc/blob/dragonflight/engine/class_modules/priest/sc_priest_shadow.cpp)
- [APL File](https://github.com/simulationcraft/simc/blob/dragonflight/engine/class_modules/apl/apl_priest.cpp) - Stores the Action Priority List for each spec.
- [Pet File](https://github.com/simulationcraft/simc/blob/dragonflight/engine/class_modules/priest/sc_priest_pets.cpp) - Handles all pets for Priest's (Shadowfiend, Mindbender, etc).

# Insanity
By default the Shadow Priest sim starts with Insanity based on current talents. 

Ordered in terms of priority:
1. Shadow Crash x2 + Divine Star x2 = 24 Insanity
2. Divine Star x4 = 24 Insanity
3. Shadow Crash x2 + Halo x1 = 22 Insanity
4. Shadow Crash x2 = 12 Insanity
5. Halo x1 = 10 Insanity

If you would like to turn off the initial insanity completely you can use this Boolean option to start with 0 Insanity (True by default):
```
priest.init_insanity=0
```

If you would like to change this to a non-zero value you can use this option:
```
initial_resource=insanity=X
```

# Spell implementation notes
The following spells are mentioned due to oddities with the implementation that are not straightforward or require more clarification. If a spell is not mentioned here you can find it in the correct base or spec file with a typical implementation. Feel free to reach out in discord if you have questions.

## Shadow Word: Death
This section is specifically covering interactions with Shadow Word: Death and the T31 Set Bonus from 10.2.

- (2) Set Bonus: Shadow Word: Death triggers 2 additional times at 15% effectiveness. Triggers again 1 additional time at 15% effectiveness if Deathspeaker is active or your target is below 20% health.
- (4) Set Bonus: Each time you take damage from Shadow Word: Death, gain a stack of Death’s Torment, increasing the initial damage of your next cast of Shadow Word: Pain by 250% or the damage of your next Shadow Crash by 50%, stacking up to 12 times.

The chained Shadow Word: Deaths have several interactions with the kit. These are listed below:
- Extra Chain from 2pc
  - Deathspeaker? ✔️  
  - Execute? ✔️ 
  - Does not snapshot multipliers (except for Deathspeaker)
- Death and Madness
  - Reset: Chain casts do not count as the free reset.
  - Debuff: Refreshed for All casts.
- Deathspeaker
  - Only consumed by original SW:D cast, not Chains.
  - If the original consumed Deathspeaker all chains get the Deathspeaker bonus damage.
- Insanity Generation
  - All casts generate full Insanity.
- Inescapable Torment
  - All casts trigger Inescapable Torment.
- Psychic Link
  - All casts trigger Psychic Link
- Self-Damage
  - All casts trigger the Self-Damage.
  - Chained casts only hit for 10% of what the original does.

## Shadow Weaving (Shadow Priest Mastery)
The Shadow Priest Mastery is not currently applied automatically by SimC due to improper spelldata. To get around this we apply this benefit manually to each priest spell with the `affected_by_shadow_weaving` option. This is turned off by default, so any spells that _are_ affected by mastery should have `affected_by_shadow_weaving = true;` added into the constructor. This should be done for priest or pet spells.

## Power Infusion
Power Infusion assumes that the actor is using the spell for its self-use only. To change this behavior, you need to set the option `priest.self_power_infusion=0`. Doing so will change the uptime of PI of the actor to 0%.

## Power Word: Shield
By default Power Word: Shield will debuff the target with the Weakened Soul debuff and prevent PWS from being cast on that target until it expires. To simulate PWS spam over an entire raid, without having to simulate an entire raid, you can use the 'ignore\_debuff' option.
```ini
  # Chain spam PWS
  actions+=/power_word_shield,ignore_debuff=1
```

# Bugs
By activating `bugs=1` inside your character's sim you will get access to the following bugs:
- [Tormented Spirits get the crit value on Shadowy Apparitions based on previous action](https://github.com/SimCMinMax/WoW-BugTracker/issues/1097)
- Void Lasher Mind Sear does not scale with most shadow priest auras

# Custom Options
## Idol of Y'Shaarj
To test various cases with Idol of Y'Shaarj there is an option created which will tell SimC to force this to always be a certain buff. In practice this would require some sort of setup but the idea is to simulate if an add or something had this condition and you could easily swap to it.
- `priest.forced_yshaarj_type="default"` (default: `default`, options: `pride`, `anger`, `despair`, `violence`)

## Self Power Infusion
To simulate giving away Power Infusion to someone else you can use `priest.self_power_infusion` (`default=1`). This option controls if the actor casts and gives themselves Power Infusion. See the section above on [Power Infusion](Priests.md#power-infusion) implementation details for more info.

## Mindgames (Class Talent)
This ability has two parts, one to reverse damage and one to reverse healing. By default the healing reversal is disabled. You can easily overwrite this with the following options:

- `priest.mindgames_damage_reversal=1` (default: `1`)
- `priest.mindgames_healing_reversal=0` (default: `0`)

These options also control the extra healing or damage that actors get when these are flipped on. So with `priest.mindgames_damage_reversal=1` the actor will generate a healing event in SimC by default (unless `priest.ignore_healing` is turned on). Additionally if `priest.mindgames_healing_reversal=1` the sim will create the damage reversal event as a child action to Mindgames. Both of these reversals assume the ENTIRE shield is broken, rather than a partial shield.

**Note:** to see the report output for healing actions you'll want to enable `enable_dps_healing=1`.

***

# Healing implementation
Healing sims are not currently supported in SimC. The following section is leftover from the last time this was worked on.

## Limitations
Fights in Simulationcraft are unidimensional: all players, pets and targets are on the same axis, only the distance from the main target changes. besides, although players can lose health (because they're tanking or suffering from damages on the whole raid), they will continue to perform actions after their death (and there may be some fancy bugs here). As a result, some spells exhibit a behaviour slightly different from their in-game counterparts.

Regarding the reports, only the total healing done will be reported, not the actual healing done or the overhealing.

## Setting up a simulation
To use healing features in Simulationcraft, import or declare a discipline or holy priest and ensure his role is set to "heal" (should be the default for those specs). We also suggest you add other players, even dummy ones (add single lines such as "warrior=john"). And maybe a tank (use "role=tank").

## Targets
Regarding the targets, by default it is always the caster himself. Although the default target cannot be changed, you can explicitly specify on every action the target of your choice. You can also add a condition to check the target's health, see [ActionLists](ActionLists).
```ini
 # Cast flash heal on John when he has less than 50% health.
 actions+=/flash_heal,target=John,if=target.health_pct<50
```

Spells with many targets are detailed in the next section; eligible targets will be friendly, non-pet, targets in range (when relevant) and they will be sequentially chosen (following the declaration or importation order) unless specified otherwise.
