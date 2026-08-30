---
title: Enemies
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Introduction

Just as with [characters](Characters.md), you can use the TCI to define enemies. This is most useful to tanks, since DPS and healing classes usually don't care what the boss does. Make sure to define enemies AFTER you've defined your character profiles. Otherwise some profileset simulations can potentially become funky.

# Built-in Enemies

Simulationcraft has some built-in enemies ready-made for you to use.

## Fluffy Pillow

If you don't specify an enemy, SimC will spawn Fluffy Pillow as your adversary. For DPS specs, Fluffy Pillow is just what he sounds like, and just stands there and takes it while you kill him. For a healing spec, he auto-attacks the [Healing Enemy](Enemies.md#Healing_Enemy.md) described below. However, for a tank spec, Fluffy Pillow becomes a monstrosity that throws every attack he has at you (see the section on [enemy action lists](Enemies.md#Action_Lists)).

## Healing Enemies

**Note: Healing is currently not properly supported** 

If you run a simulation with healers and don't specify a target for them to heal (see [Character Basics](Characters.md#Basics)), the simulation will spawn a "healing enemy" for your healer to heal. It will also spawn a Fluffy\_Pillow that attacks the healing target so that you won't overheal.

## Tank Raid Dummies

Simc now also has models for the tank dummies that are available in most major cities. These enemies have the same melee, Dummy Strike, and Uber Strike attacks that the tank dummies do. You can define one of these enemies with the **tank\_dummy** command and **tank\_dummy\_type** option:

```
  #Create a Weak Tank Dummy named Alice
  tank_dummy=Alice
  tank_dummy_type=weak
  
  #Create a Dungeon Tank Dummy named Bob
  tank_dummy=Bob
  tank_dummy_type=dungeon
  
  #The sim will automatically try and infer the appropriate dummy type from the name if omitted
  #Create a Raid Tank Dummy
  tank_dummy=Raid

  #As long as the substring "Mythic" is in the name, it will spawn a mythic dummy
  #Create a Mythic Tank Dummy
  tank_dummy=Bob_Mythic
```

## TMI Standard Bosses

**Note: Tanking and thus TMI is currently not properly supported** 

The default boss (`Fluffy_Pillow`) uses a mixture of abilities, but isn't regularly adjusted to content levels. For tanks, there are a set of "standard" bosses that are designed for generating reliable/repeatable TMI scores for tanks. These bosses use auto attacks and apply a ticking dot, and the damage values are roughly tuned to simulate specific content levels. The **tmi\_boss** command and **tmi\_boss\_type** option are used to define a TMI standard boss. The syntax is

```
   tmi_boss=Name
   tmi_boss_type=T<tier #><difficulty>
```

In Warlords of Draenor, `<difficulty>` levels will be L, N, H, and M for LFR, Normal, Heroic, and Mythic, respectively. So for example `T17N` for the tier 17 normal boss, `T18H` for the tier 18 heroic boss, and so on.
```
  #An LFR boss
  tmi_boss=Larry_LFR
  tmi_boss_type=T17L
  
  #A 15-man Normal Boss
  tmi_boss=Ned_Normal
  tmi_boss_type=T17N
  
  #A 25-man Heroic Boss
  tmi_boss=Harry_Heroic
  tmi_boss_type=T17H
 
  #A Mythic boss
  tmi_boss=Marty_Mythic
  tmi_boss_type=T17M

  #As with Tank Dummies, the sim will attempt to infer the boss type from the name string 
  #if a tmi_boss_type argument isn't provided. This will generate a T17M boss.
  tmi_boss=Surprise_T17M
```

# Custom Enemies

Simulationcraft supports defining custom enemies so that you can try to model specific boss encounters. Several properties of the enemy may be set, and you can define custom [action lists](Enemies.md#Action_Lists.md) for the enemy. You can also code custom raid events (like a periodic raid-wide AoE) that function independently of enemies - see the [Raid Events](RaidEvents) page for more details.

Make sure to define enemies AFTER you've defined your character profiles. Otherwise some profileset simulations can potentially become funky.

Note that all enemy options should only be specified after you declare an enemy.
```
 enemy=Fluffy_Pillow
 _additional options_
```

All of the options in this section are "current enemy" scoped - in other words, they only apply to the enemy currently being defined. If you are defining [multiple enemies](Enemies.md#Multiple_Targets_(AoE)), these options need to be set for each enemy (see `enemy_tank` example below).

## Assigning a target to enemy

You can use **enemy\_tank** option to assign a target for this enemy.
```
  enemy=Mean_Fluffy_Pillow
  enemy_tank=Theck
  enemy=Meaner_Fluffier_Pillow
  enemy_tank=Collision
```

## Health-Related Options

  * **enemy\_health** (scope: enemy) Allows you to set the starting health of the enemy, instead of having the sim calculate the enemy's health automatically.
```
  enemy=Fluffy_Pillow
  enemy_health=27000000
```
  * **enemy\_fixed\_health\_percentage** (scope: enemy) Sets the enemy's health to a fixed percentage that will not change over the course of the sim.
```
  enemy=Bob_the_Dinosaur
  # Simulate a fight with a permanent execute range
  enemy_fixed_health_percentage=20
```
  * **enemy\_initial\_health\_percentage** (scope: enemy) Sets the enemy's health to start at a certain percentage. The percentage should be entered as a whole number.
```
   enemy=Fluffy_Pillow
   enemy_initial_health_percentage=20
```
  * **enemy\_death\_pct** (scope: global) You can tell the sim to kill the enemy early, like in the case of Ragnaros where he dies at 10% like a pansy. This option affects all enemies in the sim.
```
  enemy=Ragnaros
  enemy_death_pct=10
```
  * **health\_recalculation\_dampening\_exponent**
  * **enemy\_custom\_health\_timeline** (scope: enemy) Can be used to adjust the time spent in various health intervals. The user specifies health percentages at various points in the iteration and the sim interpolates the rest. The value is a list of pairs `pct:time` separated by `/` or `,`. `pct` is a value between 0 and 100, `time` is a value between 0 and 1 (0 is the beginning of an iteration, 1 is the end). Note that this option **must** be used with `fixed_time` and cannot be used with other health related options.
```
  enemy=Fluffy_Pillow
  # Spend only the last 20% of the fight under 35% hp
  enemy_custom_health_timeline=35:0.8
```
## Other Enemy Options

  * **apply\_debuff** (**Simulationcraft 6.0.1 release 1 and later**) (default: 0) allows you to specify the integer number of stacks of the `damage_taken` debuff the boss applies with every successful attack. Each stack causes the target to take 1% increased damage from all sources. This is extremely useful for setting up tank swaps (see [Simulationcraft for Tanks](../appendixes/SimcForTanks.md)). Note that you can also specify this option for each attack individually on the enemy's [action list](Enemies.md#Action_Lists), and the action list option takes precedence.
```
   enemy=Sammy_Stacker
   #Sammy applies two stacks with every attack
   apply_debuff=2
```

## Action Lists

The real power of defining custom enemies is the ability to customize their action priority lists. Enemies have a small but comprehensive set of spells you can use to mimic real boss encounters. The abilities all have default settings, but you can customize their exact behavior through various options.

### Abilities

These are the abilities that an enemy actor has at its disposal. Examples will be given after we describe the options.

  * **auto\_attack** specifies the enemy's main-hand auto-attack.
  * **auto\_attack\_off\_hand** (**Simulationcraft 6.0.1 release 1 and later**) specifies the enemy's off-hand auto-attack (for dual-wielding bosses)
  * **melee\_nuke** is a direct-damage spell that does physical damage
  * **spell\_nuke** is a direct-damage spell that does fire damage
  * **spell\_dot** is a damage-over-time spell that does fire damage
  * **spell\_aoe** is a direct-damage spell that does fire damage, and hits all players in the simulation.
  * **summon\_add** summons an add.

### Ability Options

These options apply to all enemy actions unless otherwise noted. The order of the options is irrelevant, so you can mix and match in whatever order you like.

  * **damage** specifies the amount of damage the attack does. For spell\_dot, it specifies the damage per tick. Default amount varies per attack.
  * **attack\_speed** (default: 1.5 for auto-attacks, 3.0 for others) specifies the cast time of the action, in seconds. For auto-attacks this is the swing timer.
  * **range** specifies the half-width of the attack's damage range, in points of damage. Attacks deal between (damage-range) and (damage+range) damage. Default is 10% of the specified damage.
  * **cooldown** specifies the cooldown of the action.
  * **aoe\_tanks** (default: 0) when different from zero, will cause the action to cleave to all tanks in the simulation. Damage is _not_ split between the tanks.
  * **apply\_debuff** (**Simulationcraft 6.0.1 release 1 and later**) (default: 0) specifies the number of stacks of the `damage_taken` debuff the action will apply every time it deals damage. This overrides the enemy-scope option, so you can make individual attacks grant more or fewer stacks than the enemy's default value.
  * **type** (**Simulationcraft 6.0.1 release 1 and later**) allows you to specify the damage type of the ability. For example, `type=holy` will change the damage type to holy.
  * **dot\_duration** (spell\_dot only, default: 10.0) specifies the duration in seconds of the spell\_dot damage-over-time effect.
  * **tick\_time** (spell\_dot only, default: 1.0) specifies the tick interval in seconds of the spell\_dot damage-over-time event.
  * **name** (summon\_add only) specifies the name of the add being summoned.
  * **duration** (summon\_add only) specifies the lifetime of the add being summoned. Adds will have a health percentage linked to their remaining duration (in other words, they're just damage sponges, you can't kill them early).

### Example Action List

Here is an example that illustrates the actions and their options:

```
   enemy=Razor_Sharp_Pillow
   # Make all attacks apply 1 stack of the damage_taken debuff, unless overriden in the attack options
   apply_debuff=1 
   # A main-hand auto attack that does 400-600 damage every 2.0 seconds
   actions=auto_attack,damage=500,range=100,attack_speed=2.0
   # An off-hand attack that does 250-350 damage every 1.5 seconds and hits all tanks
   actions+=/auto_attack_off_hand,damage=300,range=50,attack_speed=1.5,aoe_tanks=1
   # A DoT that ticks every 2 seconds for 20 seconds, re-applied if it's not active
   actions+=/spell_dot,dot_duration=20,tick_time=2,if=!ticking 
   # An instant melee strike that does exactly 5000 damage with a 5-second cooldown and applies 2 stacks of the debuff
   actions+=/melee_nuke,damage=5000,range=0,apply_debuff=2,cooldown=5,attack_speed=0
   # A spell attack that does 5000-15000 damage on a 10-second cooldown and applies 3 stacks of the debuff, 3-second cast time (default)
   actions+=/spell_nuke,damage=10000,range=5000,apply_debuff=3,cooldown=10
   # An AoE attack that hits the whole raid for 2000-3000 damage and does not apply the debuff, on a 20-second cooldown, 2-second cast time
   actions+=/spell_aoe,damage=2500,attack_speed=2.0,range=500,apply_debuff=0,cooldown=20
```

# Multiple Targets (AoE)
Multiple enemies can be added using `desired_targets=X` where X is the number of total targets.

Alternatively, simply define multiple enemies.  This simple example simulates a fight with 4 targets:
```
 enemy=Fluffy_Pillow
 enemy=enemy2
 enemy=enemy3
 enemy=enemy4
```

# Global (Sim-Wide) Enemy Options

## Sim-wide Target Level

The option **target\_level** allows you to specify what level all targets in the simulator should be. The default is three levels higher than the highest level character in the sim, normally 88.
```
  enemy=Bob_the_Dinosaur
  target_level=87
  # You can use multiple options at once
  # Bob starts at 90%  health and dies at 30%
  enemy_initial_health_percentage=90
  enemy_death_pct=30
```

## Sim-wide Target Race

The **target\_race** options allow you to specify what race all targets in the simulator are to kick in the effects of certain talents.
```
  enemy=Fred_the_Zombie
  target_race=undead
```
