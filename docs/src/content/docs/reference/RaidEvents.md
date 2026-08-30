---
title: Raid Events
---
_This documentation is a part of the [TCI](TextualConfigurationInterface.md) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Predefined events
**fight\_style** (scope: global; default: "") is a string to declare a predefined set of raid events. It acts as a shortcut for **raid\_events** and will change this setting. It means it will effectively clear all events declared so far (without affecting events declared after).
```
  fight_style=HelterSkelter
```

  Acceptable values are:
  * _Patchwerk_ will set up an empty raid events list. This is a perfect stand still, single-target DPS fight. The name comes from the iconic DPS check fight from Naxxramas.
  * _CastingPatchwerk_ will set up a fight similar to _Patchwerk_ but the master target will be casting instead. It is equivalent to:
    ```
      raid_events+=/casting,cooldown=500,duration=500
    ```
  * _LightMovement_ will set up a fight with infrequent movement. It is equivalent to:
    ```
      raid_events+=/movement,players_only=1,cooldown=40,cooldown_stddev=10,distance=15,move_distance_min=10,move_distance_max=20,first=15
    ```
  * _HeavyMovement_ will set up a fight with frequent movement. It is equivalent to:
    ```
      raid_events+=/movement,players_only=1,cooldown=20,cooldown_stddev=15,distance=25,move_distance_min=20,move_distance_max=30,first=15
      raid_events+=/movement,players_only=1,cooldown=45,cooldown_stddev=15,distance=45,move_distance_min=40,move_distance_max=50,first=30
    ```
  * _DungeonSlice_ approximates a "slice" of a M+ dungeon. A single boss mob followed by alternating then interleaving large/weak trash packs (4-6 mobs for 18 seconds) and small/strong trash packs (1-3 mobs for 30 seconds). Durations are randomized on a per-enemy basis within 2 standard deviations of the mean. Due to the offset cooldowns, all add waves beyond the first of each type can potentially overlap, leading to a semi-random pattern between 1-9 enemies at any given time, with an average target count across the entire duration (including as enemies "die") of 4. Fight length locked to 6 minutes. Events are equivalent to:
    ```
      raid_events+=/adds,name=Boss,count=1,cooldown=500,duration=135,type=add_boss,duration_stddev=1
      raid_events+=/adds,name=SmallAdd,count=5,count_range=1,first=140,cooldown=45,duration=18,duration_stddev=2
      raid_events+=/adds,name=BigAdd,count=2,count_range=1,first=160,cooldown=50,duration=30,duration_stddev=2
    ```
  * _DungeonRoute_ has no events of its own but allows for pull events to be defined to simulate a series of enemies spawned with lifetimes determined by their health pools.
  * _HecticAddCleave_ will set up a fight with regular add spawns and frequent movement. Similar to the Tier15 encounter Horridon (but without the vulnerability on the boss). The events scale with `max_time`, with 450 it is the same as:
    ```
      raid_events+=/adds,count=5,first=22,cooldown=33,duration=22,last=337
      raid_events+=/movement,distance=25,first=22,cooldown=33,last=337
      raid_events+=/movement,players_only=1,distance=8,first=13,cooldown=18
    ```
  * _HelterSkelter_ will set up a "crazy" fight. It is equivalent to:
    ```
      raid_events+=/casting,cooldown=30,duration=3,first=15
      raid_events+=/movement,cooldown=30,distance=20
      raid_events+=/stun,cooldown=60,duration=2
      raid_events+=/invulnerable,cooldown=120,duration=3
    ```
  * _CleaveAdd_ will set up a fight that regularly spawns an add the actor cleaves down. The event scales with your input `max_time`, with 450 it is the same as:
    ```
      raid_events+=/adds,count=1,first=22,cooldown=33,duration=22,last=405
    ```
  * _Beastlord_ will set up a fight similar to the Tier 17 encounter Beastlord Darmac. The events scale with `max_time`, with 450 it is the same as:
    ```
      raid_events+=/adds,name=Pack_Beast,count=6,first=15,duration=10,cooldown=30,angle_start=0,angle_end=360,distance=3
      raid_events+=/adds,name=Heavy_Spear,count=2,first=15,duration=15,cooldown=20,spawn_x=-15,spawn_y=0,distance=15
      raid_events+=/movement,first=13,distance=5,cooldown=20,players_only=1,player_chance=0.1
      raid_events+=/adds,name=Beast,count=1,first=10,duration_stddev=5,duration=67,cooldown=112,cooldown_stddev=0,last=292
    ```
  * _Ultraxion_ will set up a fight similar to the Tier 17 encounter Ultraxion. The events scale with `max_time`, with 450 it is the same as:
    ```
      raid_event=/flying,first=0,duration=500,cooldown=500
      raid_event+=/position_switch,first=0,duration=500,cooldown=500
      raid_event+=/stun,duration=1.0,first=45.0,period=45.0
      raid_event+=/stun,duration=1.0,first=57.0,period=57.0
      raid_event+=/damage,first=6.0,period=6.0,last=59.5,amount=44000,type=shadow
      raid_event+=/damage,first=60.0,period=5.0,last=119.5,amount=44855,type=shadow
      raid_event+=/damage,first=120.0,period=4.0,last=179.5,amount=44855,type=shadow
      raid_event+=/damage,first=180.0,period=3.0,last=239.5,amount=44855,type=shadow
      raid_event+=/damage,first=240.0,period=2.0,last=299.5,amount=44855,type=shadow
      raid_event+=/damage,first=300.0,period=1.0,amount=44855,type=shadow
    ```

# Individual event syntax
**raid\_events** (scope: global; default: "") is a string sequence specifying the events affecting the whole raid. See [TextualConfigurationInterface](TextualConfigurationInterface.md).
```
  raid_events+=/damage,amount=20000,cooldown=10
  raid_events+=/movement,cooldown=30,distance=40
```

  * All events are periodic. The following options are available to you:
    * _cooldown_ or _period_ (default: 0) specifies the periodicity of the event, in seconds.
    * _duration_ (default:0) specifies the duration of the event, in seconds.
    * _distance_ specifies the distance of the movement event, which will take raid/personal movement cooldowns into account.
    ```
      #This example will make the raid spend 15s moving every 30s.
      raid_events+=/movement,cooldown=30,duration=15
    ```
  * The duration and cooldown are always following a normal distribution (see [Wikipedia - Normal distribution](http://en.wikipedia.org/wiki/Normal_distribution)). The following settings help you adjust this:
    * _cooldown\_stddev_ (default: 0) is the _standard deviation_, in seconds, of the cooldown. When left to zero, it will be defaulted to 10% of the cooldown.
    * _duration\_stddev_ (default: 0) is the _standard deviation_, in seconds, of the duration. When left to zero, it will be defaulted to 10% of the duration.
    ```
      #This example will make the raid spend 10s moving (with a 5s standard deviation) every 30s (with a 10s standard duration).
      raid_events+=/movement,cooldown=30,cooldown_stddev=10,duration=10,duration_stddev=5
    ```
  * You can also specify bounds for duration and cooldown, using the "<=" and ">=" operators with those keywords (note that _periodic_ won't work for specifying bounds for the cooldown though). If you don't specify any bounds, SimulationCraft will use 50% and 150% of the base value as the lower and upper bounds.
    ```
      #This example will make the raid spend 15s moving every 30s. Both duration and cooldown follow a normal law but the 
      cooldown will always be greater than 28s and lesser than 32s (rather than 27s and 33s with the default settings).
      raid_events+=/movement,cooldown=30,cooldown>=28,cooldown<=32,duration=15

      #This example will make the raid spend 15s moving every 30s. Both duration and cooldown follow a normal law but the 
      duration will always be greater than 14s and lesser than 16s (rather than 13.5s and 16.5s with the default settings).
      raid_events+=/movement,cooldown=30,duration=15,duration>=14,duration<=16
    ```
  * The following settings allow you to force the events to only occur during a certain phase:
    * _first_ (default: 0) specifies the first time, in seconds, the event will occur. 
    * _last_ (default: 0) specifies the last time, in seconds, the event may occur. It will not force the event to occur at this time, though. When lesser than or equal to zero, this setting will be ignored.  
    With WoW 8.0 (BFA):
    * _first\_pct_  specifies the boss health pct when the event will first occur and be scheduled.
    * _last\_pct_  specifies the boss health pct when the event will last occur and no longer be scheduled.
    * _force\_stop_ (default: false) specifies if a raid event which is up when _last_ or _last\_pct_ occurs will be instantly canceled or not
    * _pull_ specifies which pull event this event is a child of, required for all events in a `DungeonRoute` fight style sim. `first/last` based scheduling is based on the spawn time of the pull.
    * _pull\_target_ specifies the enemy that `first_pct/last_pct` should be based on. The enemy name given here must match an enemy defined in a pull event matching the `pull` parameter of this event. Required if an event defines `first_pct/last_pct`.
    ```
    #This example will make the raid spend 15s moving every 30s. It will only happen after two minutes.
    raid_events+=/movement,cooldown=30,duration=15,first=120

    #This example will make the raid spend 15s moving every 30s. It will only happen during the first three minutes.
    raid_events+=/movement,cooldown=30,duration=15,last=180

    #This example will spawn 5 adds for 10s every 30s, starting when the enemy defined with the name "Boss_angry_giant" in pull 2 of a DungeonRoute sim reaches 50% health. It will only happen during the first two minutes of the pull or until the pull ends, whichever comes first.
    raid_events+=/adds,count=5,first_pct=50,cooldown=30,duration=10,last=120,pull=2,pull_target=BOSS_angry_giant
    ```
  * If you want the event to occur at specific times, you can use the `timestamp` setting.
    * _timestamp_ specify a `:`-delimited list of timestamps, in seconds, at which the event should occur. This will cause all cooldown related options to be ignored.
    ```
      #This example will apply vulnerable to enemies for 10s at 20s, 50s, and 120s into the fight
      raid_events+=/vulnerable,duration=10,timestamps=20:50:120
    ```

# Filtering affected players
  * You may also make raid events distinguish between players and pets:
    * _players\_only_ (default: 0) specifies whether or not the raid event should only target players. When set to '0' both players and pets are affected by the raid event and when set to '1' only players are affected. Note: The **distraction** event is the only event that enables players\_only by default.
    ```
      #This example will stun all players in the raid every 30s for 5s, but will ignore all pets.
      raid_events+=/stun,players_only=1,cooldown=30,duration=5
    ```
  * The following setting allows you to set a per player chance that the raid event will affect them:
    * _player\_chance_ (default: 1.0) specifies a % chance for the raid event to affect each eligible player and pet. By default, 100% (1.0) of eligible players and pets are affected by each raid event.
    ```
      #This example has a 25% chance of distracting players for 5s every 60s
      raid_events+=/distraction,player_chance=.25,duration=5,cooldown=60
    ```
  * You can use distance\_min and distance\_max conditions (the distance in yards, extending from the boss) so that only ranged or melee characters are affected. See also the **distance** setting for characters.
    ```
      #This example will make the raid spend 15s moving every 30s, only players closer than 10m from the boss will be affected.
      raid_events+=/movement,cooldown=30,duration=15,distance_max=10

      #This example will make the raid spend 15s moving every 30s, only players further than 20m from the boss will be affected.
      raid_events+=/movement,cooldown=30,duration=15,distance_min=20
    ```
  * (BFA only) Finally, you can use player\_if= expressions to filter the affected players. This allows you to leverage the might of the already available expressions system from the action-priority system.
    ```
      #This example will make the raid spend 15s moving every 30s, only players with health percentage below 50 will be affected.
      raid_events+=/movement,cooldown=30,duration=15,player_if=health.pct<50

      #This example will make the raid spend 15s moving every 30s, only players with role 'spell' and level of at least 100.
      raid_events+=/movement,cooldown=30,duration=15,player_if=role.spell&level>=100
    ```

# Adds
  See also **target\_adds** in the [target properties](#Target) section if you rather want to spawn adds who will live through the whole fight.

  The _adds_ keyword allows you to make adds periodically spawn. Default actions list may not include aoe actions but you can mention some of them, using conditions based on the number of targets, see [ActionLists](ActionLists.md).

  Specific options are:
  * The number of adds spawned per wave can be specified  
    * _count_ (default: 1) specifies the number of adds to be generated.
    ```
      #This example creates waves of 3 adds that exist for 15 seconds at a time every 60 seconds, starting at 5 seconds.
      raid_events+=/adds,count=3,first=5,duration=15,cooldown=60
    ```
    * _count\_range_ (default: 0) specifies if you want to generate up to _count_ +/- _count\_range_ adds, randomly choosen for each wave.
    ```
      #This example creates waves of 3 adds that exist for 15 seconds at a time every 60 seconds, starting at 5 seconds.
      raid_events+=/adds,count=3,first=5,duration=15,cooldown=60
      #This example creates waves of 5 adds with a count_range of 3, meaning between 2 and 8 adds will be generated for each wave.
      raid_events+=/adds,count=5,count_range=3,first=5,duration=15,cooldown=60
    ```
  * The following setting allows you to name the adds part of the wave being defined.
    * _name_ (default: Fluffy\_Pillow\_WaveXX\_AddY) grants a specific name to the set of adds spawned.
    ```
      #This example names the wave 'Hogger'
      raid_events+=/adds,count=3,name=Hogger,first=5,duration=15,cooldown=60
    ```
  * The health of the adds can also be manually specified if desired
    * _health_ (default: 100,000) determine's the starting health points of each add spawned in that wave.
    ```
      #This example gives all adds in the wave 75,000 health
      raid_events+=/adds,count=3,first=5,duration=15,cooldown=60,health=75000
    ```
  * You can set whether the adds all spawn with the same duration, or each with its own duration. Both settings use durations as determined via other options standard options such as `duration` and `duration_stddev`.
    * _same\_duration_ (default: false) when set true will make all adds spawn with the same duration.
    ```
      #This example spawns 4 adds, all with the same duration, the duration being 30s with a standard deviation of 5s
      raid_events+=/adds,duration=30,duration_sttdev=5,same_duration=true
    ```
  * The following options require **distance\_targeting\_enabled=1** in order to function. The location of an add defaults to 0,0 (stacked on top of the main target). This can be changed in a few ways:
    * _spawn\_x_ and _spawn\_y_ (default: 0) set the x,y coordinates that a wave of adds will spawn at.
    * _distance_ (default: 0) sets the distance from 0,0 that the adds will spawn. The exact location is randomly generated, but will be _distance_ yards away.
    * _spawn\_distance\_min_ and _spawn\_distance\_max_ (default: 0) sets a band of space away from 0,0 that the adds will be able to spawn in. The exact location is randomly generated, but will be between _spawn\_distance\_min_ and _spawn\_distance\_max_ yards away. If either is omitted, the for that will be considered the minimum and be set to 0.
    ```
      #This example spawns 2 adds at the location 5,10
      raid_events+=/adds,count=2,first=5,duration=15,cooldown=60,spawn_x=5,spawn_y=10
      #This example spawns 4 adds at a distance of 10 yards away, randomly placed in an cone
      raid_events+=/adds,count=4,first=5,duration=15,cooldown=60,distance=10
      #This example spawns 3 adds at a distance of 0 - 30 yards away, randomly placed
      raid_events+=/adds,count=3,first=5,duration=15,cooldown=60,max_distance=30
      #This example spawns 2 adds at the location 5,10
      raid_events+=/adds,count=2,first=5,duration=15,cooldown=60,spawn_x=5,spawn_y=10
    ```
  * The angle in which adds will spawn changes based on a few factors.
    * If _spawn\_x_ and _spawn\_y_ are omitted, so that the adds spawn centered at 0,0, then _angle\_start_ is (default: 90 degrees), and, _angle\_end_ is (default: 270 degrees). This creates a cone in which the adds will spawn that is on the near side / behind Fluffy Pillow.
    * If _spawn\_x_ or _spawn\_y_ are specified, so that the adds spawn centered somewhere other than at 0,0, then _angle\_start_ is (default: 0 degrees), and, _angle\_end_ is (default: 360 degrees). This creates a circle around the specified location in which the adds will spawn.
    * Valid range for _angle\_start_ and _angle\_end_ is 0 - 360 degrees. If more than 360 degrees is specified it will be modulused to a valid value. If a negative value is specified it will be assigned a default value.
    ```
      #This example spawns adds between 60 and 240 degrees at 0,0
      raid_events+=/adds,count=2,first=5,duration=15,cooldown=60,angle_start=60,angle_end=240
      #This example spawns adds between 180 and 90 (450; either will work) degrees at 0,0
      raid_events+=/adds,count=2,first=5,duration=15,cooldown=60,angle_start=180,angle_end=90
      #This example spawns adds between 90 and 270 degrees at 10,15
      raid_events+=/adds,count=2,first=5,duration=15,cooldown=60,angle_start=90,angle_end=270,spawn_x=10,spawn_y=15
    ```
  * When randomly selecting where adds will be spawned, there is also the option of having all adds of that wave be stacked on top of each other or spread out.
    * _stacked_ (default: 0) sets whether all adds from a wave are at the same or different coordinates.
    ```
      #This example spawns 5 adds at a distance of 15 - 35 yards stacked up
      raid_events+=/adds,count=5,first=5,duration=15,cooldown=60,spawn_distance_min=15,spawn_distance_max=35,stacked=1
    ```

# Pull
  The _pull_ raid event is used to spawn waves of adds sequentially with durations based on their specified health pools being depleted by the simmed character rather than a time period. These events are used together with the `DungeonRoute` fight style to simulate a full dungeon run.

  Specific options are:
  * _pull_ specifies the order of the pull within the sim.
  * _bloodlust_ forces bloodlust to be cast for the pull.
  * _delay_ time period in seconds to approximate travel time to the start of the pull from the end of the previous pull or beginning of the sim for the first pull
  * _enemies_ a string that describes the enemies that make up the pull. It should consist of a sequence of enemy specifiers delimited by `|`, each specifier having the format `"name":health[:CreatureType]` with CreatureType being optional, default Humanoid. Prepending `BOSS_` to the name will spawn that mob as a boss type actor, with possible implications dependent on class module support.

  ```
    # This example spawns 3 pulls of adds followed by one boss, starting at 20 seconds with 10 seconds between each, with Bloodlust being used on the boss pull.
    raid_events+=/pull,pull=01,bloodlust=0,delay=020,enemies="small_add_1":100000:Elemental|"small_add_2":100000:Elemental|"small_add_3":100000:Elemental|"small_add_4":100000:Elemental|"small_add_5":100000:Elemental
    raid_events+=/pull,pull=02,bloodlust=0,delay=010,enemies="medium_add_1":200000:Beast|"medium_add_2":200000:Beast|"medium_add_3":200000:Demon
    raid_events+=/pull,pull=03,bloodlust=0,delay=010,enemies="big_add":300000:Beast|"medium_add":200000:Dragonkin|"small_add":100000:Abberation
    raid_events+=/pull,pull=04,bloodlust=1,delay=010,enemies="BOSS_angry_giant":1000000:Giant
  ```
# Buff
  The _buff_ raid event allows you to trigger one or more stacks of a buff. If a _duration_ option is set, the buff will be active for the set druation. Otherwise, the buff will last for the default duration based on the buff's implementation.

  Specific options are:
  * _buff\_name_ specifies the tokenized name of the buff and is required.
  * _stacks_ (default: 1) specifies the number of stacks to be applied each time the event occurs.
  ```
    #This example will give all actors a stack of the S4 M+ affix buff "Bounty: Haste" every minute, starting 30s in.
    raid_events+=/buff,buff_name=bounty_haste,first=30,cooldown=60
  ```
# Casting
  The _casting_ keyword allows you to make a raid event that will make the target to cast a spell your players must interrupt. There is no action condition relative to the target's casting but off-gcd interrupts do not need to be present in the actions list: they will be automatically used by the Simulationcraft.

  There is no specific option for this keyword.
  ```
    #This example will make the boss incant a spell for 2s every 6s.
    raid_events+=/casting,cooldown=6,duration=2
  ```

# Distraction
  The _distraction_ keyword allows you to periodically lower your players' skill, simulating those times of a fight when your players are distracted and unable to focus on their dps rotation because they have to focus on their environment. By default, the **players\_only** option is enabled for the distraction event. Also, see the **skill** command for more information.

  Specific options are:
  1. _skill_ (default: 0.2) is the skill loss suffered by the players.
  ```
    #This example will make your ranged players heavily distracted for 10s every 1min.
    raid_events+=/distraction,cooldown=60,duration=10,skill=0.4
  ```

# Invulnerability
  The _invul_ and _invulnerable_ keywords can be used to make the target periodically invulnerable, clearing all dots and debuffs on it. There is currently no way to use actions list to switch on another target but you can still use actions conditions to detect whether your target is currently invulnerable or not, see [ActionLists](ActionLists.md).

  Specific options are:
  * _retarget_ (default: 0) wether the players should acquire a new target or not.
  * _target_ the name of the enemy to apply the invulnerability to. For `DungeonRoute` this must match an enemy defined by a pull event with a matching `pull` parameter, checked at the pull's spawn time. Otherwise must match a defined enemy at the start of a sim.
  ```
    #This example will make your target invulnerable for 10s every 1min.
    raid_events+=/invulnerable,cooldown=60,duration=10

    #This example will make your target invulnerable for 10s every 1min and search a new target during this time
    raid_events+=/invulnerable,cooldown=60,duration=10,retarget=1

    #This example will make the "BOSS_angry_giant" target in pull 2 invulnerable for 10s every 1min and search a new target during this time
    raid_events+=/invulnerable,cooldown=60,duration=10,target="BOSS_angry_giant",pull=2,retarget=1
  ```

# Incoming damage
  The _damage_ keyword allows you to periodically make your raid suffer a uniformly distributed amount of damage. Note that simulated raid members can die from this damage.

  Specific options are:
  * _amount_ (default: 1) is the average amount of damage suffered by every player on every occurrence of the event, before mitigation.
    ```
      #This example will make your players suffer 20k damages every 30s.
      raid_events+=/damage,cooldown=30,amount=20000
    ```
  * _amount\_range_ (default: 0) is the range of damage into one direction.
  * _type_ (default: holy) is the type of damage.

# Incoming heals
  The _heal_ keyword allows you to periodically heal your entire raid without going to the trouble of making healer profiles. This is useful to counteract the effects of the _damage_ event.

  Specific options are:
  * _amount_ (default: 1) is the average amount of healing to each player on every occurrence of the event.
    ```
      #This example will heal your players for 20k every 30s.
      raid_events+=/heal,cooldown=30,amount=20000
    ```
  * _amount\_range_ (default: 0) is the range of of the heal amount, used to add some randomization.
  * _to\_pct_ (default: 0) when greater than zero will heal every player in the raid up to that percentage of their health. Takes precedence over _amount_ and _amount\_range_ if those are also specified.
  * _to\_pct\_range_ (default: 0) is the range of the percent heal amount, used to add some randomization.
    ```
      #This example will heal every player to 70% of health every 5s.
      raid_events+=/heal,cooldown=5,to_pct=70
    ```

# Movement
  The _movement_ and _moving_ keywords can be used to force your raid to move, forcing the players and their pets to interrupt their casts and making them unable to use their spells for the duration of the move. Some spells will still be usable, through, depending on your actions list

  Specific options are:
  * _move\_distance_ (default: 0) is the distance, in yards, the players have to run on (staring from their current location: all players, whether they are 5 yards or 30 yards away from the boss, will run on the same distance) . When different from zero, it will prevail on _duration_ since the movement speed (taking into account possible speed bonuses) and distance will be used to evaluate the run duration. When equal to zero, this setting will be ignored.
  * _to_ (default: -2) is the player's distance to the boss after moving. Only a 0 or positive value is processed. -1 means a default distance based on the player's role. -2 or lower means the current distance (i.e. the player moves and then returns to her current position). Currently the distance change happens instantaneously at the end of the movement event; if you need to check distance while moving, divide the event into several.
  * _direction_ (default: omni) is the directionality flag for the movement event. Valid values are _omni_, _away_, and _towards_. Currently only affects what kind of actions can be used during the movement event.
  * move\_distance\_min (default: 0) sets a minimum distance for the movement event. When combined with max\_distance allows for a single movement event to have differing distances.
  * move\_distance\_min (default: 0) sets a maximum distance for a movement event.
  * _distance\_range_ (default: 0) Works similarly, gives a range to move\_distance. The move\_distance\_min/max options will set the upper/lower limit, and can shift the distribution.
  ```
    #This line, added to a warlock's actions list, will make him/her use life tap when moving.
    actions+=/life_tap,moving=1

    #This example will make your players move for 5s every 20s. Our warlock will still use life tap.
    raid_events+=/movement,cooldown=30,duration=5

    #This example will make melee players run on 20 yards.
    raid_events+=/movement,cooldown=30,move_distance_max=10,move_distance=20

    #That will create a 60 second cooldown movement event with a stddev of 10 seconds, and only affects the player
    #25% of the time. The distance will be 60 yards 50% of the time, and the other 50%  will be between 40-60 yards.
    # raid_events+=/movement,cooldown=60,cooldown_stddev=10,distance=60,move_distance_min=40,move_distance_max=60,distance_range=20,player_chance=0.25
  ```

# Stuns
  The _stun_ keyword can be used to make your raid periodically stunned and unable to do anything.

  There is no specific option for this keyword.
  ```
    #This example will make your players stunned for 10s every 1min.
    raid_events+=/stun,cooldown=60,duration=10
  ```

# Vulnerability
  The _vulnerable_ keyword can be used to make the target periodically vulnerable, causing you to do more damage to the target. It is possible to change your actions list to keep your best cooldowns for those moments, see [ActionLists](ActionLists.md).

  * _multiplier_ (default: 1.0) sets the additional damage multiplier according to the original damage amount. A multiplier of 1.0 adds an extra 100% of original damage, resulting in 2x damage taken, etc.
  * _target_ the name of the enemy to apply the vulnerability to. For `DungeonRoute` this must match an enemy defined by a pull event with a matching `pull` parameter, checked at the pull's spawn time. Otherwise must match a defined enemy at the start of a sim.
  ```
    #This example will make your target vulnerable, taking twice as much damage for 20s every 80s.
    raid_events+=/vulnerable,cooldown=80,duration=20

    #This example will make your target vulnerable, taking 3x the normal damage for 10s every 120s.
    raid_events+=/vulnerable,cooldown=120,duration=10,multiplier=2.0

    #This line will disable automatic bloodlust 
    override.bloodlust=0

    #This line, when added to a shaman's action list, will make bloodlust cast on the first vulnerable moment.
    actions+=/bloodlust,if=target.debuff.vulnerable.react,time_to_die<=100
  ```
