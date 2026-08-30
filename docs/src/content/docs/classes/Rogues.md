---
title: Rogues
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**

# Textual configuration interface
_This section is a part of the [TCI](TextualConfigurationInterface) reference._

Regular spells are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting).

## Synchronizing weapons
The _sync\_weapons_ (default: 0) option on the _auto\_attack_ action can be used to force the synchronization of weapons at the beginning of the fight. When zero, the offhand will be desynchronized by half of its swing time. In game, you always start with your weapons synchronized but target switching and parry rushes often lead you to go unsynchronized.
```
 # Ensure the player will start with synched weapons.
 actions+=/auto_attack,sync_weapons=1
```

## Combo points
Combo points can be used in conditional expressions for actions (see [ActionLists](ActionLists)) through the _combo\_points_ character property.
```
 # Use eviscerate when you have 5 combo points
 actions+=/eviscerate,if=combo_points=5
```

The **initial\_combo\_points** setting (scope: current character; default: 0) can be used to start the fight with the given number of combo points. Only values within `[0; 5]` will be accepted.
```
 # Start the fight with two combo points.
 initial_combo_points=2
```

The Anticipation talent allows Rogues to store up to 5 Combo Points in a buff called "Anticipation". The `anticipation_charges` expression allows you to return the current number of Anticipation stacks. This is a shorthand for `buff.anticipation.stack`.
```
 # Dispatch if we have room to generate Anticipation charges
 actions+=/dispatch,if=(combo_points<5(talent.anticipation.enabled&anticipation_charges<4)
```

### Combo point changes in Simulationcraft 7.0.3, release 1

Anticipation now increases your maximum Combo Points to 8. `anticipation_charges` expression is removed, as is the Anticipation buff.

You can get the guaranteed number of combo points an ability is generating with the new `cp_gain` expression.
```
  # Backstab, if there would be no CP waste.
  actions+=/backstab,if=combo_points+cp_gain<=combo_points.max
```

You can get the maximum number of combo points spent with the new `cp_max_spend` expression.
```
  # Eviscerate at maximum combo point spendage.
  actions+=/eviscerate,if=combo_points>=cp_max_spend
```

## Poisons
  * _apply\_poison_ instantly changes the poisons on the player's weapons. The action won't be performed if the poisons already match your specifications or if you specified inactive poisons for both weapons.
    1. _main\_hand_ and _off\_hand_ (default: none, optional) are used to specify the poison to use on every weapon. Acceptable values are: "deadly", "instant" or "wound". Other values will result in an inactive poison being applied. **Since Simulationcraft 7.0.3, release 1** If no option is given, `apply_poison` will apply the "best" lethal poison for the actor (Deadly Poison or Agonizing Poison).
```
 actions+=/apply_poison,main_hand=instant,off_hand=deadly
```

**Since Simulationcraft 6.1.2, release 2** The `poisoned_enemies` expression evaluates to the number of enemy actors currently poisoned with either lethal or non-lethal poison.

## Honor among thieves
For single actor simulations, Honor Among Thieves can be approximated through the use of the `honor_among_thieves` action. The action allows the user to generate HAT combo points at a specific interval. Defaults to 2.3 seconds, with a standard deviation (of a normal distribution) of 100 milliseconds.
```
 # Generate some HAT combo points
 actions.precombat+=/honor_among_thieves,cooldown=2.3,cooldown_stddev=0.1
```
Note that this proxy Honor Among Thieves action is disabled if the Subtlety Rogue is simulated in an environment with other player profiles.

**Since Simulationcraft 7.0.3, release 1** Honor Among Thieves is no longer in the simulator.

## Miscellaneous
  * An expression called `stealthed.rogue` evaluates to 1 when any of the stealth-like effect crating buffs is up (Stealth, Vanish, Shadow Dance or Subterfuge). Notably, Shadowmeld is not in the list as it is not a true stealth-like effect, but you can check it using `stealthed.all`. Also, you can use `stealthed.mantle` to evaluates if we're currently stealthed into something that maintain Mantle of the Master Assassin aura or not.

## Weapon Swapping

**Since Simulationcraft 6.1.2-01** Rogues have an experimental weapon swapping mechanism in the simulator. Two new options, `main_hand_secondary` and `off_hand_secondary` allows a profile to specify a secondary weapon to main and off hand, respectively. Both options use the normal item option format.

In addition, there is a new action `swap_weapon` that performs a weapon swap for the profile. Currently, weapon swap incurs a GCD of 1.0 seconds, resets the swing timer(s) of the swapped weapons, and resets the RPPM-related timers. The action has two options, `slot` that specifies the slot to swap (valid values are _main_, _off_, or _both_, with default of _main_). The second option, `swap_to` specifies what set to swap to, with valid values of _primary_ and _secondary_ and a default of _secondary_.

```
# Specify secondary main/offhands
main_hand_secondary=oregorgers_acidetched_gutripper,id=113874,bonus_id=567,enchant=mark_of_the_thunderlord
off_hand_secondary=oregorgers_acidetched_gutripper,id=113874,bonus_id=567,enchant=mark_of_the_thunderlord

# .. and swap to them in certain situations (and swap back to primary when needed)
actions+=/swap_weapon,slot=both,swap_to=secondary,if=active_enemies>1
actions+=/swap_weapon,slot=both,swap_to=primary,if=active_enemies=1
```

## Shrouded Suffocation

**Since Simulationcraft 8.1.0 release 1** you can determine whether the Garrote has the damage bonus from Shrouded Suffocation by using the `ss_buffed` expression for actions.

## Exsanguinate

**Since Simulationcraft 7.0.3 release 1** Garrote and Rupture have an expression to determine whether the current target has an exsanguinated dot on them. `exsanguinated` expression on a `garrote` or `rupture` action will evaluate to 1 if the current dot (on the target) is exsanguinated.
```
  # Don't refresh Garrote if the current target's dot is Exsanguinated
  actions+=/garrote,if=!exsanguinated&remains<=5&combo_points<combo_points.max
```

In addition, you can use the more generic form `dot.X.exsanguinated` (where X is either `garrote` or `rupture`) on any action in an action priority list to check whether the current target's respective dot is exsanguinated.

## Roll the Bones

**Since Simulationcraft 7.0.3 release 1** You can check for the number of active Roll the Bones buffs on the actor with the new `rtb_buffs` expression.
```
  # Use Roll the Bones until you have 6 buffs up!
  actions+=/roll_the_bones,if=combo_points>=cp_max_spend&rtb_buffs<6
```

Additionally, you may use `rtb_list` expression to check for the presence of specific buffs on the actor. The expression takes the form `rtb_list.OP.LIST`, where OP is the operating mode and LIST is the list of buffs. The expression will return 0 if the evaluation fails, and 1 if it succeeds.
  * The `OP` accepts two values: `any`, meaning any of the buffs on `LIST` being up succeeds the evaluation, and `all`, meaning all of the buffs on `LIST` must be up to succeed the evaluation.
  * The `LIST` accepts a sequence of RTB buffs, represented by numerical values from 1 to 6. The numbers correspond to the six buffs in alphabetical order: 1: Broadsides, 2: Buried Treasure, 3: Grand Melee, 4: Jolly Roger, 5: Shark Infested Waters, 6: True Bearing. With **BfA** this changes to: 1: Broadside, 2: Buried Treasure, 3: Grand Melee, 4: Ruthless Precision, 5: Skull and Crossbones, 6: True Bearing.
```
  # Use Roll the Bones until you have at least Shark Infested Waters and Broadsides up
  actions+=/roll_the_bones,if=combo_points>=cp_max_spend&!rtb_list.all.15
```

If you want to check what effect different buffs or buff combinations have, you can use the `fixed_rtb` option to force the simulation to always give you the specified buff list. Like for the list above, use numbers for the buffs in alphabetical order.
```
  # Always roll Broadside
  fixed_rtb=1
  # Always roll 6 buffs
  fixed_rtb=123456
```

## Bleed effects

**Since Simulationcraft 7.0.3 release 1** A new expression `bleeds` evaluates to the number of bleeding effects on the target. Currently Rogue module defines Garrote and Rupture abilities as bleeds.

# Reports
We only document here non-obvious entries.

## Uptimes
  * energy\_cap : the percentage of the fight time spent with a full energy bar.
