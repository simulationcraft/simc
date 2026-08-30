---
title: Spell Data
---
## Spell Data

Broadly speaking, spells are composed of "spell data", "effect data", and an optional "power data". Spell data contains the basic information about the spell, such as its name, cast time, cooldown, and tooltip description. Each spell data is linked to one or more effect data. Effect data describes the actual "actionable" properties of the spell, such as healing, applying an aura (buffs and debuffs), damaging, creating an object, summoning a pet, etc. Finally, if the spell has a resource cost, an associated power data entry is given for the spell that includes the characteristics of the resource cost.

SimulationCraft extracts many aspects of spell, effect, and power data to use in the simulator. The extracted data is used in class modules and more general features of the simulator to automate some of the workload for developers, as basic attributes of most spells can be directly gotten from the data, instead of individual developers keeping track of their values through experimentation, or by scouring forums for information. SimulationCraft also allows users to override the spell, effect, and power data to perform simple value changes to explore "what if" scenarios, without having to perform actual code changes, and rebuild their simulator.

## Spell Data Override

Overriding spell, effect, and power data is done through the **override.spell\_data** option. The option overrides the **_global_** data of the simulator.  It takes the form `override.spell_data=<spell|effect|power>.<id>.<field>=value`, where the different parts are:
  * `spell`, `effect`, or `power` refers to the type of data that is to be changed
  * `id` is the numerical identifier of the data. The spell identifiers can be found through SpellQuery or third party websites, effect and power identifiers can be found through SpellQuery.
  * `field` is the name of the field in the data that is to be overridden with a new value. A list of valid field names for spells, effects, and powers are given below.

If instead you wish to override the spell data of a specific player, one may use `override.player.spell_data` in player scope.

Some things need to be noted about the override system.
  1. It is an "low level" feature, and as such is not meant to be used by typical SimulationCraft users
  1. It may have some un-intuitive results occasionally. This can be due to a multitude of things, including:
    * The class developer is not using spell data, but rather hardcoding values. This is very rare nowadays, but can still happen
    * The spell, effect, or power that is being overridden is not the correct one. This can occasionally happen for more complex abilities. SimulationCraft tries to replicate Blizzard's way of using spells in the client as much as possible, some of which are not visible to normal users.
    * Class modules can occasionally cache spell data locally, for optimization reasons, or due to how the spell functions. Due to this, **overriding spell data should always be done before the character definition in the simulator.**

## List of fields

### Spells 

The spell data can be overridden with the following fields. The list contains the name, value type, and description of the field.
  * `prj_speed` (_float_), The projectile speed of the spell in yards per second.
  * `school` (_integer_), School mask of the spell
  * `scaling_class` (_integer_), Internal value of the scaling class for the spell
  * `spell_level` (_integer_), Required level of the spell
  * `max_level` (_integer_), Maximum scaling level of the spell
  * `req_max_level` (_integer_), Required maximum level of caster of the spell
  * `max_scaling_level` (_integer_), When non-zero, scaling level capped at `min( player_level, max_scaling_level )`
  * `min_range` (_float_), Minimum range of the spell (currently unused)
  * `max_range` (_float_), Maximum range of the spell (currently unused)
  * `cooldown` (_integer_, _milliseconds_), The cooldown of the spell in milliseconds
  * `charge_cooldown` (_integer_, _milliseconds_), The charge cooldown of the spell in milliseconds
  * `internal_cooldown` (_integer_, _milliseconds_), The internal cooldown of the spell in milliseconds
  * `category_cooldown` (_integer_, _milliseconds_), The cooldown of the category in milliseconds
  * `charges` (_integer_), The number of cooldown charges
  * `gcd` (_integer_, _milliseconds_), The global cooldown of the spell in milliseconds
  * `duration` (_integer_, _milliseconds_), The duration of the aura in milliseconds
  * `rune_cost` (_integer_), The rune cost of the spell. The following values can be combined for the rune cost: `1`: blood, `4`: unholy, `16`: frost, `64`: death. **Obsoleted in Simulationcraft release 7**
  * `runic_power_gain` (_integer_), The runic power gain when the ability is used, multiplied by 10
  * `max_stack` (_integer_), The maximum stack of the aura
  * `proc_charges` (_integer_), The number of stacks per trigger for the aura
  * `proc_chance` (_integer_), The percent chance for the spell to trigger
  * `cast_min` (_integer_, _milliseconds_), The minimum cast time of the spell (used only for low level characters)
  * `cast_max` (_integer_, _milliseconds_), The maximum (normal) cast time of the spell
  * `rppm` (_float_), RPPM value of the proc. **Since Simulationcraft 5.4.8 release 5**
  * `class_flags_family` (_integer_), The spell family, generally used to divide spells by class with further groupings via class flags
  * `class_flags` (_integer_), Flag(s) which indicates the grouping(s) the spell belongs to, used in affected-by lists in spell effects. This is a bit array; a spell can have multiple flags which allow it to belong to multiple groups. A positive value will add the spell to the numbered group. A negative value will remove the spell from the numbered group. A spell **MUST** have a class_flags_family for these groupings to apply.
  * `attributes` (_integer_), Attribute flags bit array. A positive value will set the attribute, a negative value will unset the attribute.

### Effects

The effect data can be overridden with the following fields.
  * `coefficient` (_float_), The average amount scaling coefficient for the effect
  * `delta` (_float_), The delta amount scaling coefficient for the effect
  * `bonus` (_float_), The bonus (in essence, per combo point) amount scaling coefficient for the effect
  * `sp_coefficient` (_float_), The spell power coefficient for the effect
  * `ap_coefficient` (_float_), The attack power coefficient for the effect
  * `period` (_integer_, _milliseconds_), The base tick time of the effect in milliseconds
  * `base_value` (_integer_), The base value of the effect (more discussion on this below)
  * `misc_value1` (_integer_), The first "misc" value of the effect (use is effect type, or effect dependant)
  * `misc_value2` (_integer_), The second "misc" value of the effect (use is effect type, or effect dependant)
  * `chain_multiplier` (_float_), The multiplier to apply spells when they jump from target to target
  * `points_per_combo_points` (_float_), "Old style" version of the `bonus` field. Unused
  * `points_per_level` (_float_), "Old style" version of the `average` field. Unused
  * `die_sides` (_integer_), "Old style" version of the `delta` field. Unused
  * `class_flags` (_integer_), Group(s) of spells that this effect applies to. This is a bit array; an effect can apply to multiple groups. A positive value will add the numbered group to the list of spells to be affected. A negative value will remove the numbered group from the list of spells to be affected.

The `base_value` field defines the "base value" of the effect in most cases, where there is no (player level, or item level) based scaling applied to it. For example with many buffs that affect the actor with a percentage modifier (This is arguably the most typical use of the value), this field specifies the percent modifier as an integer. Unfortunately, the meaning of the field is always dependant on the effect type, and ultimately, what Blizzard intended it to be. For example, it can also indicate the number of units to summon for a certain type of spell.

### Powers (Added in Simulationcraft 7.2.0 release 2)

The power data can be overridden with the following fields.
  * `cost` (_integer_), The absolute resource cost of the spell
  * `cost_per_tick` (_integer_), The absolute resource cost of the spell per tick
  * `max_cost` (_integer_), The absolute maximum resource cost of the spell (in-game maximum is `cost + max_cost`)
  * `pct_cost` (_float_), The percent cost of the spell in terms of the maximum base resource (e.g. Mana). Values given in a range [0..1].
  * `pct_cost_per_tick` (_float_), The percent cost of the spell per tick in terms of the maximum base resource. Values given in a range [0..1].
  * `max_pct_cost` (_float_), The maximum percent cost of the spell in terms of the maximum base resource (e.g., Mana). Values given in a range [0..1].

There are some "quirks" in the power data that users should know. The quirks are due to Blizzard internal conventions, and because Simulationcraft exports the data as unmodified as possible, they also transfer to the simulator power data.
  * Some absolute resources are given as tenths of an unit. At the time of writing (Legion), such resources include: Rage, Runic Power, Burning Ember, Astral Power, Pain, and Demonic Fury. In other words, overriding these resources requires the value to be multiplied by 10, i.e., Death Coil costs 350 Runic Power (instead of 35 that is shown in SpellQuery). You can always check with SpellQuery how the resource behaves with the new override.
  * Spells with variable costs (max cost field) are such that the in game maximum cost is actually the cost plus the maximum cost field value. For example Earth Shock that costs 10 - 100 Maelstrom has a maximum cost field value of 90 in the data.
  * Do not attempt to add a completely new resource cost to a spell, it will not work.
 
## Examples

Below is a simple example to show how to implement one of the 1/6/2014 hot fixes for the Death Knight class. We begin by looking at the spell data of "Icy Talons", to figure out the effect that we need to change:

```
 $ simc spell_query=spec_spell.name=icy_talons
 Name             : Icy Talons (id=50887) [Off GCD]
 Class            : Frost Death Knight
 Spell Level      : 55
 Attributes       : ......x. x....... ........ ........   ........ ........ ........ ........ 
                  : ........ ........ ........ ........   ........ ........ ........ ..x...x. 
                  : ........ ........ ........ ........   ........ ........ ........ ........ 
                  : ........ ........ ........ ........   ........ ........ ........ ........ 
                  : ........ ........ ........ ........   ........ ........ ........ ........ 
                  : ........ ........ ........ ........   ........ ........ ........ ........ 
 Effects          :
 #1 (id=43156)    : Apply Aura (6) | Modify Melee Attack Speed% (319)
                    Base Value: 30 | Scaled Value: 30
 Description      : Your melee attack speed is increased by $s1%.
```

The only effect of the spell is to apply an aura that modifies the actor's melee attack speed% by 30 (the value in `base_value` field). We adjust the value to the new hotfixed value of 45.

```
 $ simc override.spell_data=effect.43156.base_value=45 spell_query=spec_spell.name=icy_talons
 ...
 Effects          :
 #1 (id=43156)    : Apply Aura (6) | Modify Melee Attack Speed% (319)
                    Base Value: 45 | Scaled Value: 45
 ...
```

Note that if the override had been put after the spell\_query option, it would not have applied it correctly for the spell query.
