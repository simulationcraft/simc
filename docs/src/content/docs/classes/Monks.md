---
title: Monks
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Textual configuration interface
_This section is a part of the [TCI](TextualConfigurationInterface) reference._
## Storm, Earth, and Fire

Storm, Earth, and Fire is fully supported in the sim. The action `storm_earth_and_fire` will spawn an enemy on the current target of the action (typically, the primary target of the simulator). There is a single buff and a single debuff involved in determining how many pets the actor has out at any given time, and whether a target has a pet on it. The buff `storm_earth_and_fire` has a maximum stack of two, where the stack count indicates the number of pets out. The debuff `storm_earth_and_fire_target` indicates whether the target has a pet on it.

Executing Storm, Earth, and Fire on a target that already has a pet on it will despawn that pet. Executing Storm, Earth, and Fire on a target that does not have a pet on it, when both pets are out, will despawn both pets. If a target dies, while a pet is on it, the pet is automatically despawned.
```
 # Use Storm, Earth, and Fire on the second and third targets (while they are alive)
 actions+=/storm_earth_and_fire,target=2,if=debuff.storm_earth_and_fire_target.down
 actions+=/storm_earth_and_fire,target=3,if=debuff.storm_earth_and_fire_target.down
```

There are a total of three available pets to summon (storm, earth, and fire pets). Each time the ability is executed, one of the remaining dormant pets is selected. For this reason, the reporting may show all three pet types in use. **This does not mean that all three pets are active at the same time.**

## Resources

The _initial\_chi_ (scope: player, default: 1 for WW, cap: 6 for Ascension) option sets the amount of chi the monk actor has at the beginning of the combat. Please note that it defaults to 1 chi given sims assume you Expel Harm pre-pull to have 1 chi. Normally in a raid fight, chi will reset to 1 chi if the player has more than 1 chi.
```
# Set the initial chi of the monk actor to one
monk.initial_chi=1
```
## Spells implementation notes

Regular spells are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting).

## Buffs
Regular buffs for this class are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting.md). Also, don't forget that set bonuses are added as buffs to a character. Buffs can be used in conditional expressions for actions, see [ActionLists#Buffs\_and\_debuffs](ActionLists#Buffs_and_debuffs).

## Debuffs
Regular spells are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting).

### _Stagger_
Stagger is part of the Brewmaster active mitigation. Given how unique it is, there is a custom format for it.

**Light Stagger** - Returns true if current Stagger level is `light_stagger`.

  ```
 # Purify stagger if in Light Stagger or green stagger
 actions+=/purifying_brew,if=Stagger.light_stagger
  ```
**Moderate Stagger** - Returns true if current Stagger level is `moderate_stagger`.

  ```
 # Purify stagger if in Moderate Stagger or yellow stagger
 actions+=/purifying_brew,if=Stagger.moderate_stagger
  ```
**Heavy Stagger** - Returns true if current Stagger level is `heavy_stagger`.

  ```
 # Purify stagger if in Heavy Stagger or red stagger
 actions+=/purifying_brew,if=Stagger.heavy_stagger
  ```
**Stagger Tick Percent** - Returns `(Stagger tick / Current Maximum HP)`.

  ```
 # Purify stagger once stagger tick percent goes over 2.2%
 actions+=/purifying_brew,if=Stagger.pct>2.2
  ```
**Stagger Percent** - Returns `(Stagger pool / Current Maximum HP)`.

  ```
 # Purify stagger once stagger percent goes over 2.2%
 actions+=/purifying_brew,if=Stagger.amounttotalpct>44
  ```
**Stagger Tick Amount** - Returns `Stagger tick`.

  ```
 # Purify stagger once stagger tick goes over 10,000 damage
 actions+=/purifying_brew,if=Stagger.amount>10000
  ```
**Stagger Amount** - Returns `Stagger pool`.

  ```
 # Purify stagger once stagger pool goes over 10,000 damage
 actions+=/purifying_brew,if=Stagger.amount_remains>200000
  ```
**Stagger Remains** - Returns remaining debuff duration of active debuff.

  ```
 # Purify stagger only if stagger debuff duration is greater than 3 seconds
 actions+=/purifying_brew,if=Stagger.remains>3
  ```
**Stagger Ticking** - Returns true if any stagger debuff is active.

  ```
 # Touch of Death if any stagger debuff exists on player
 actions+=/touch_of_death,if=Stagger.ticking
  ```
## Options

**Initial Chi** - This sets the Chi at the start of each iteration. It is advised that this is set no higher than 1 chi due to in-game chi gets set to 1 chi if you have more than 1 chi going into a fight. This requires whole numbers. Defaults to 1.

```
# Initial Chi is set to 1 chi
monk.initial_chi=1
```

# Reports
We only document non-obvious entries.
