---
title: Shaman
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Textual configuration interface
_This section is a part of the [TCI](TextualConfigurationInterface) reference._

Regular spells are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting).

## Synchronizing weapons
The _sync\_weaapons_ (default: 0) option on the _auto\_attack_ action can be used to force the synchronization of weapons at the beginning of the fight. When zero, the offhand will be desynchronized by half of its swing time. In game, you always start with your weapons synchronized but target switching and parry rushes often lead you to go unsynchronized.
```
 # Ensure the player will start with synched weapons.
 actions+=/auto_attack,sync_weapons=1
```

## Thorim's Invocation

The last Thorim's Invocation triggered spell the actor has cast can be checked with the following player-scope expressions.
* `ti_lightning_bolt` evaluates to 1 if the last actor-cast Thorim's Invocation triggered spell cast was Lightning Bolt, or if no Lightning Bolt or Chain Lightning has been cast during combat
* `ti_chain_lightning` evaluates to 1 if the last actor-cast Thorim's Invocation triggered spell cast was Chain Lightning

New in **Simulationcraft 10.0.0**.

## Alpha Wolf

The state of the Alpha Wolf buff on all wolves can be evaluated in the action lists with a custom expression. The player-scope expression `alpha_wolf_min_remains` evaluates to the minimum remaining time on all the Alpha Wolf buffs on wolves. Evaluates to 0 if there is no buff up, no Feral Spirits spawned, or the actor has no Alpha Wolf talent.

New in **Simulationcraft 10.1.0**.

## Deeply Rooted Elements

The talent proc behaviour was changed going into 10.1. Beforehand it was a fixed proc percent chance per cast. Afterwards it became an incrementing proc chance with two initial 0% chances. E.g.: 0%, 0%, 1%, 2%, 3%,...
To compare both models a switch was added to SimulationCraft.
- `shaman.dre_flat_chance=1` enables the old fixed proc chance per cast
- `shaman.dre_flat_chance=0` disables the fixed proc chance and enables the incrementing chance. This is the Default. 

The `shaman.dre_forced_failures` (default: 2) can be used to control the number of forced proc attempts that fail, before the incremental proc chance based on attempts starts to accumulate.

The current DRE proc chance can be evaluated the `dre_chance_pct` expression. Evaluates to 0..100, and is aware of the `shaman.dre_forced_failures` option.
