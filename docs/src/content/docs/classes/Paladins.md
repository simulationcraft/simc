---
title: Paladins
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Textual configuration interface
_This section is a part of the [TCI](TextualConfigurationInterface) reference._

Regular spells are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting).

## Buffs
Regular buffs for this class are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting.md). Also, don't forget that set bonuses are added as buffs to a character. Buffs can be used in conditional expressions for actions, see [ActionLists#Buffs\_and\_debuffs](ActionLists#Buffs_and_debuffs).

## Custom Paladin expressions
### Time Until Next HPG (Retribution only)
`time_to_hpg` will return the time until the next holy power generator is available, in seconds. It will also enforce a minimum time equal to the current GCD (i.e. if there's a holy power generator that is off cooldown, but the GCD isn't up for another 350 ms, this will evaluate to 0.350).
```
  #cast TV if we're at 5 holy power and a holy power generator is available next GCD
  actions+=/templars_verdict,if=holy_power>=5&time_to_hpg<=gcd.max
```
Note that this does not do any fancy calculus to figure out if the next GCD actually _will_ be a HPG. It _only_ returns the time until the next HPG is available. If your action list prioritizes other spells over HPGs, then the time until the next HPG is actually cast could be longer than `time_to_hpg`. It should never be shorter, however.

Note#2: This is only supported for Retribution Paladin at the moment.

### Time to next CSAA HoPo (Retribution only)
Only works with the Talent `Crusading Strikes`. `time_to_next_csaa_hopo` returns the time in seconds until you get your next Holy Power from your auto attacks.

### Holy Power Generators until two Dawn Stacks
Use `hpg_to_2dawn` to get the currently needed amount of Holy Power Generators to reach two Stacks of Dawn. This is a variable that can be between 6 (6 Holy Power Generators needed until 2 Dawn Stacks are reached) and -2 (Two Dawn Stacks are already reached and you would need one more Holy Power Generator to refresh the Dawn-Buff)
```
# Use Shield of the Righteous when we're about to reach the next Dawn Stack to use it with Hammer of Light
actions.hammer_of_light+=/shield_of_the_righteous,if=hpg_to_2dawn=4
```

`next_armament` returns either 0 or 1, depending on which Holy Armament comes next

`holy_bulwark` returns 0

`sacred_weapon` returns 1

`avenging_wrath` returns `sentinel`, if talented into Sentinel, otherwise `avenging_wrath`

`judgment_holy_power` (Protection only) returns the current amount of Holy Power Judgment would give, depending on available Buffs (Avenging Wrath with Sanctified Wrath, Bastion of Light)

## Consecration Precombat time
When Consecration is included in the precombat APL, the spell will be used with a special behaviour based on the precombat_time option given to it (default: 2s). The ground aoe's start time will be delayed by 1s to simulate the time the boss takes to run into it. Its duration and cooldown will also be adjusted accordingly.
>Negative values will be handled the same way as some users may find it more intuitive (-3 is 3s before combat starts). The absolute value has to be lower than consecration's total duration, and higher than the player's based gcd duration (1.5s).
```
# Use consecration 3s before combat starts
actions.precombat=consecration,precombat_time=3
```

## Lightsmith specifics

### Lightsmith's Divine Guidance
```
For each Holy Power ability cast, your next Consecration deals x damage or healing immediately, split across all enemies and allies.
```
**As of 12.1, Divine Guidance has been updated to no longer prefer to heal. Will keep this here to maybe show how weird it had been.**

Since this ability shares its value between damage and healing, the default implementation for SimCraft is to always heal the Paladin to have a more realistic scenario. The actual implementation tries to mirror what is happening ingame. It will always prefer to heal injured targets before it tries to deal damage. If it heals a total amount of 5 targets, it will deal 0 damage. Only if there is no injured target available will Divine Guidance deal full damage, otherwise it will always try to heal the injured targets first (Which also include pets)

With 0 targets to heal and 1 enemy, the ability will deal 100% damage.

With 1 target to heal and 1 enemy, the ability will heal 50% (1 target, 2 targets total) and deal 50% damage.

With 4 targets to heal and 1 enemy, the ability will heal 80% and deal 20% damage.

With 1 target to heal and 3 enemies, the ability will heal 25% and deal 75% damage, split between 3 targets (25% each).

With 1 target to heal and 20 enemies, the ability will heal 20% and deal 80% damage, split between 20 targets (4% each).

With 5 targets to heal and 1 enemy, the ability will heal 100% and deal no damage.

### Holy Armament management
Three new variables have been introduced to handle Holy Armament handling. Holy Armaments can only be cast by using the action `holy_armaments`, which will always alternate between Holy Bulwark and Sacred Weapon, starting with Holy Bulwark)
See [#custom-paladin-expressions](https://github.com/simulationcraft/simc/wiki/Paladins#custom-paladin-expressions)

```
# Use Holy Armaments when the next Armament is Sacred Weapon, if the buff is not up or can pandemic, AW is not up or the remaining cooldown is less than 30s
actions.standard+=/holy_armaments,if=next_armament=sacred_weapon&(!buff.sacred_weapon.up|(buff.sacred_weapon.remains<6&!buff.avenging_wrath.up&cooldown.avenging_wrath.remains<=30))
```

### Solidarity and you
If you do not specify a target for the `holy_armaments` cast, it will always cast on the Paladin. Solidarity will then choose a "random" target (Which will be the closest DPS in game for Sacred Weapon, first and foremost), which will only be correctly reflected in Sims if you choose to do a multi actor sim with at least two actors. For fair comparisons, this actor should be a DPS role, so Solidarity will always choose the DPS, instead of maybe other Tanks. Solidarity will choose a "random" target once (picks the first DPS > Healer > Tank they find introduced into the Sim) and continue to "randomly" use their Sacred Weapon onto them.

## Player-scoped Options
`fake_solidarity` (0-1, default 1) - When set to 0, Solidarity will behave like usual (Try to give a DPS a Weapon and a Tank a Bulwark - If it's a single actor Sim, Solidarity will do nothing). When set to 1, Solidarity will no longer give another player an Armament, but will only give the Paladin (or the target, if specified) an Armament and give the Paladin a new buff, `fake_solidarity`, which stacks asynchronously and increases Sacred Weapon damage by 100% per stack, to approximate it's gain without a multi actor Sim. This does not affect Holy Bulwark Absorb.

`ror_bulwark_additional_proc_chance` (0-1, default 0.3) - Reflection of Radiance adheres to `fake_solidarity`, too, and triggers more often with fake armaments out. While Sacred Weapon will always have 100% more chance to trigger with each additional weapon out, Bulwark Absorbs will only partially increase in chance, since damage taken on the Tank will be more often than on the group.

`blessed_hammer_strikes` (1-3, default 2) - Blessed Hammer can hit multiple times depending on the size of the target in-game. To reproduce that behavior, this player-scoped option can be used to specify the number of time each cast will hit each target.
If the number has decimals, they will be used as a chance to generate an extra strike for every blessed hammer cast.

`starting_armament` (sacred_weapon, holy_bulwark) - Defines the Holy Armament with which Lightsmith starts the fight. If the option is not set, it defaults to Sacred Weapon. If it is set to something else that is undefined, it starts with a random weapon (50% Sacred Weapon, 50% Holy Bulwark)


# Reports
We only document here non-obvious entries.

## Procs
  * parry\_haste: the number of times your swings have been hasted after you parried an attack.
