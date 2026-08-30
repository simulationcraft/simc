---
title: Mages
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Textual configuration interface
_This section is a part of the [TCI](TextualConfigurationInterface) reference._

Regular spells are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting).

## Actions

### Water Elemental

Water Elemental's Freeze and Water Jet can be controlled manually via the `freeze` and `water_jet` actions.

```
...
actions+=/freeze,if=ground_aoe.comet_storm.remains>0.6
...
```

## Expressions

### Mana Gem

The remaining charges of a Mana Gem can be obtained by using the `mana_gem_charges` expression.

### Incanter's Flow

`incanters_flow_dir` can be used to determine if Incanter's Flow is currently gaining or losing stacks.

```
Time                   | 1  2  3  4  5  6  7  8  9 10 11 12 ...
-----------------------+---------------------------------------
Incanter's Flow stacks | 1  2  3  4  5  5  4  3  2  1  1  2 ...
incanters_flow_dir     | 1  1  1  1  0 -1 -1 -1 -1  0  1  1 ...
```

`incanters_flow_time_to.<stack number>.<stack type>` evaluates to the remaining time (in seconds) until the next occurrence of the specified stack. When the buff is in the desired state, this expression evaluates to 0.

`stack_number` is an integer between 1 and 5, `stack_type` can be `up`, `down` or `any`. `up` checks only stacks on the raising part of the cycle, `down` checks stacks on the falling part of the cycle and `any` checks both types.

### Ground AoE

Ground AoE can be tracked by `ground_aoe.X.Y`.

`X` can be any of:
* `blizzard`
* `comet_storm`
* `flame_patch`
* `frozen_orb`
* `meteor_burn`

`Y` can be any of:
* `remains`

If the specified ground AoE is currently not active, this expression evaluates to 0.

For example:

```
...
actions+=/flurry,if=buff.brain_freeze.react&prev_gcd.1.frostbolt&ground_aoe.frozen_orb.remains=0
...
```

### Firestarter and Searing Touch

The expression `firestarter.active` can be used to check if Firestarter is active. Time until Firestarter becomes inactive is represented by `firestarter.remains`.

```
actions=fireball,target_if=firestarter.active
# Cast Fireball on any target that will make it crit.
```

The expression `searing_touch.active` functions in the same way. Time until Searing Touch becomes active is represented by `searing_touch.remains`.

### Winter's Chill

The expression `remaining_winters_chill` provides an approximation of the number of remaining Winter's Chill stacks, accounting for spells that are currently in flight.

### Hot Streak

The expression `hot_streak_spells_in_flight` provides the number of spells that are currently in flight (to any target) and are capable of triggering Hot Streak.

### Kindling
The expression `expected_kindling_reduction` returns the expected amount of time left until Combustion's cooldown is ready as a fraction of Combustion's cooldown based on the rate that Kindling has been reducing the cooldown.

## Mage options

### Freeze effects

Since all freeze effects available in simc break on damage and thus almost never last their full duration, we opted to use one shared duration for all of them. In Battle for Azeroth, all freeze effects are guaranteed to last at least 1 s, which is why simc uses 1 s as the default freeze duration.

`mage.frozen_duration=<time in seconds>` overrides this default duration with a user-specified value.

### Arcane Missiles

Sometimes, there can be a benefit from chaining Arcane Missiles quickly. `mage.arcane_missiles_chain_delay=<time in seconds>` and `mage.arcane_missiles_chain_relstddev=<fraction of interval>` control the average time after a tick when Arcane Missiles will be chained.

### Overriding APL variables

The default Mage APLs include several variables, which can be configured through the `apl_variable` option.

## Crowd control

Some abilities have a different effect depending on whether the target is susceptible to crowd control. For example, against targets that are immune to crowd control, Freeze will not apply the root effect.

Target is susceptible to crowd control in these situations:

* Target's level is strictly smaller than player level + 3
* Target is an add spawned by the `adds` raid event
