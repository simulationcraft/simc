---
title: Target Options
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Enemy Health Options

Note that all enemy options should only be specified after you declare an enemy.
```
 enemy=Fluffy_Pillow
 enemy_health=2700000
 _additional options_
```


## Fixed Health Percentage

Using **enemy\_fixed\_health\_percentage** sets the enemy's health to a fixed percentage that will not change over the course of the sim.
```
 enemy=Bob_the_Dinosaur
 # Simulate a fight with a permanent execute range
 enemy_fixed_health_percentage=20
```

## Initial Health Percentage

Using **enemy\_initial\_health\_percentage** sets the enemy's health to start at a certain percentage. The percentage should be entered as a whole number.
```
  enemy=Fluffy_Pillow
  enemy_initial_health_percentage=20
```


## Initial Health

Using **enemy\_health** will allow you to set the starting health of the enemy, instead of having the sim calculate the enemy's health automatically.
```
 enemy=Fluffy_Pillow
 enemy_health=27000000
```

## Enemy Death Percentage
Using **enemy\_death\_pct** you can tell the sim to kill the enemy early, like in the case of Ragnaros where he dies at 10% like a pansy.
```
 enemy=Ragnaros
 enemy_death_pct=10
```

## TMI Standard Bosses

The default boss (`Fluffy_Pillow`) uses a mixture of abilities, but isn't regularly adjusted to content levels. For tanks, there are a set of "standard" bosses that are designed for generating reliable/repeatable TMI scores for tanks.  These can be invoked by setting the **tmi\_boss** option.

The format of this option is

`T<tier #><difficulty><raid format>`,

so for example `T15N10` for the tier 15 10-man normal boss, `T15H25` for the tier 15 25-man heroic boss, and so on.  If the `<raid format>` is left blank, 25-man is assumed.
```
 enemy=Ted_the_Terrible
 tmi_boss=T15H10
```

# Assigning a tank to enemy

You can use **enemy\_tank** option to assign a target for this enemy.
```
 enemy=Mean_Fluffy_Pillow
 enemy_tank=Theck
```

# Multiple Targets (AoE)

To simulate an AoE fight, simply define multiple enemies.  This simple example simulates a fight with 4 targets:
```
 enemy=Fluffy_Pillow
 enemy=enemy2
 enemy=enemy3
 enemy=enemy4
```

# Sim-wide Target Level

The option **target\_level** allows you to specify what level all targets in the simulator should be. The default is three levels higher than the highest level character in the sim, normally 88.
```
 enemy=Bob_the_Dinosaur
 target_level=87
 # You can use multiple options at once
 # Bob starts at 90%  health and dies at 30%
 enemy_initial_health_percentage=90
 enemy_death_pct=30
```

# Sim-wide Target Race

The **target\_race** options allow you to specify what race all targets in the simulator are to kick in the effects of certain talents. (Exorcism for example.)
```
 enemy=Fred_the_Zombie
 target_race=undead
```
