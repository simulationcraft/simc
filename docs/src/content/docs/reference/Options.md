---
title: Options
---
_This documentation is a part of the [TCI](TextualConfigurationInterface.md) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Public test realms
  * **ptr** (scope: ulterior characters; default: 0) allows you to target the ptr version. By default, Simulationcraft targets the live WoW version. Beware:  Simulationcraft may not be fully updated for the ptr version, you're advised to check the version changes on [SimC commits](https://github.com/simulationcraft/simc/commits).
```
 # This allows you to compare the live version of a character
 # with his evil twin on the ptr. 
 # We first create the live character with ptr=0, then we set ptr
 # to 1 and copy the current character to a new one named "EvilTwin".

 ptr=0
 #<Insert other options here>
 #<Insert character declarations here>

 ptr=1
 copy=EvilTwin
```

# Combat Length
There are various combat length settings and modes in SimC, both allowing various combat models and also increasing its complexity.
## Time-based models
There are two time-based combat length models:
  * **Fixed Time** _(default)_:
 The combat length is purely based on the configured time-based combat length parameters. Enemy health percentage is uniformly distributed over the combat length.
  This is the default setting in SimulationCraft.
  * **Enemy Health Estimation**:
 The enemies health gets adjusted so that the resulting combat length based on the enemies demise corresponds to the configured time-based combat length parameters. The reason for this is to allow a realistic enemy health flow depending on the simulated characters dps variation, especially execute effects.
  To use this model, set `fixed_time=0`


### Configuration

Both Time-based models have the following parameters:
 * **max\_time** (scope: global; default: 300) is the duration, in seconds, you desire for the average fight duration.

```
 // This example will make the fights durations converge to 400s.
 max_time=400
```
* **vary\_combat\_length** (scope: global; default: 0.2) will make the combat length artifically vary linearly across the iterations. It is expressed as a fraction of the **max\_time** setting, between 0 and 1.

```
 #This example will make the combat length's targeted value vary between 180s and 220s.
 max_time=200
 vary_combat_length=0.1
```

To enable the Fixed Time model use:
 * **fixed\_time** (scope: global; default: 1), when different from zero, will enable the Fixed Time model. Enabling this setting will ignore the **override.target\_health** setting.
```
 #This example will make the combat length being exactly 300s on all iterations.
 max_time=300
 vary_combat_length=0.0
 fixed_time=1
```

## Fixed Enemy Health model
Specifying the initial health pool of the target: Simulationcraft will use this value as an initial health for the target and just end the simulation once the target reaches 0 hp.
The **max\_time** setting is ignored, except for the Safeguard settings. ( See section below )
 * **override.target\_health** (scope: global; default: 0), when different from zero, is the initial target health pool and triggers the second mode. Sets vary\_combat\_length to 0.
```
 # This example will ignore the max_time duration and just have an initial health pool of 100M HP.
 override.target_health=100000000
```

## Safeguards
All configurations have a safeguard to end combat eventually, to avoid endless simulations and various problems like going out of memory. The safeguard is set to twice the expected combat time.
```
 simulation_end = 2 * max_time * ( 1 + vary_combat_length )
```
# Infinite resources

* **infinite\_rage, energy, mana, focus, runic, health** (scope: global; default: 0), when different from zero, provide infinite resources of the corresponding type to all concerned characters.
```
 infinite_mana=1
```


# Latency
 * **strict\_gcd\_queue** (scope: global; default: 0), when different from zero, forces the application to properly model the in-game gcd queue. In the future, this setting should be defaulted to 1. When this setting is left to zero, a player can still change his queued gcd action after the time his previously queued action should have been executed by the server. TOCHECK.
```
 strict_gcd_queue=1
```

  * **gcd\_lag** (scope: global; default:0.150) represents the latency, in seconds, suffered by the client when notifying the server you queued up a new gcd action. It should be set to your in-game latency (unless it is too small, see warning below). TOCHECK.
  * **gcd\_lag\_stddev** (scope: global; default:0.0) is the standard deviation (see [Wikipedia - Normal distribution](http://en.wikipedia.org/wiki/Normal_distribution)) that will be used to make **gcd\_lag** vary across the simulation.
```
 #This example sets up a 50ms latency with a 10ms standard deviation.
 gcd_lag=0.05
 gcd_lag_stddev=0.01
```

  * **channel\_lag** (scope: global; default:0.250) represents the latency, in seconds, suffered by the client when notifying the server you're still channeling a spell and waiting for the notification that your channeling ticked. It should be set to twice your in-game latency (unless it is too small, see warning below). TOCHECK.
  * **channel\_lag\_stddev** (scope: global; default:0.0) is the standard deviation (see [Wikipedia - Normal distribution](http://en.wikipedia.org/wiki/Normal_distribution)) that will be used to make **channel\_lag** vary across the simulation.
```
 #This example sets up a 100ms latency with a 20ms standard deviation.
 channel_lag=0.10
 channel_lag_stddev=0.02
```

  * **queue\_lag** (scope: global; default:0.037) represents the duration, in seconds, it takes for the server to process your queued gcd-bound action. It is server-specific and does not depend on your in-game latency. TOCHECK.
  * **queue\_lag\_stddev** (scope: global; default:0.0) is the standard deviation (see [Wikipedia - Normal distribution](http://en.wikipedia.org/wiki/Normal_distribution)) that will be used to make **queue\_lag** vary across the simulation.
```
 #This example sets up a 10ms process time with a 2ms standard deviation.
 queue_lag=0.01
 queue_lag_stddev=0.002
```

  * **default\_world\_lag** (scope: global; default: 0.1) represents the network latency in seconds to your server. It is meant to be the equivalent of "World lag" shown in the World of Warcraft client. World lag is currently used to extend the duration of cooldowns for actions that have them to simulate the roundtrip required for the server to acknowledge the start of a cooldown. This is the default version of the option, which is set for every actor in the simulation. Latency may be overridden on a player level with the _world\_lag_ option.
```
 #This example sets up a sim-wide world latency of 300ms with 50ms standard deviation.
 default_world_lag=0.3
 default_world_lag_stddev=0.05
```

  * **default\_world\_lag\_stddev** (scope:global; default 10% of _default\_world\_lag_) is the standard deviation that will be used to make **default\_world\_lag** vary across the simulation.

  * **travel\_variance** (scope: global; default: 0.075) is the standard deviation (see [Wikipedia - Normal distribution](http://en.wikipedia.org/wiki/Normal_distribution)), in seconds, of the time a spell need to fly to its target when fired from a ranged distance.
```
 #This example sets up a 150ms standard deviation for the spells flight time.
 travel_variance=0.150
```

  1. You may want to use **gcd\_lag** and **channel\_lag** to simulate brain\_lag. It is a valid option, along with the use of **skill** (see the [skill](#skill.md) section for more information) and **reaction\_time** (see [ActionLists](ActionLists.md)).
  1. Warning! Making the lag values too small or setting them to zero can result in discontinuities in the haste plots and jumps in haste scale factors.... Latency helps smooth out the behavior.

# Multithreading

  * **threads** (scope: global; default: 0) is the number of threads to use to perform computations. A value of 0 or less will use as many threads as there are CPU threads available on your system.
Increasing this number will linearly decrease the computations times: maximum performances are reached with a value equal to the number of soft cores your CPU has. It may have a slight impact on other applications, though. Note that some features such as outputting combat logs are not available when using more than one thread! Besides, the memory consumption will also increase as data are duplicated across threads to simplify the conception and enhance performances. Finally, the report will display the number of iterations per thread rather then the total number.
```
 # An Intel Nehalem core i7 has 4 cores and, through the hyper-threading technology, 8 soft cores. Let's use 7 of them for Simulationcraft and leave one for foreground applications.
 threads=7
```

  * **process\_priority** (scope: global; default: below\_normal; choices: `[` low, below\_normal, normal, above\_normal, highest `]` ) defines the scheduling priority of SimC engine process.
```
 # You do not want a SimulationCraft simulation to degrade your WoW raid performance. You use eg. 
 process_priority=low
```

# Networking

## Http cache
  * **http\_clear\_cache** (scope: ulterior http calls; default: 0), when used with a non-zero value, will force the the flush of the http cache before the next lines of the file are interpreted and executed.
```
 # Since we'll force the use of the cache in this script, we first flush in cas it has been executed before.
 http_clear_cache=1

 # We first pull two characters who both want the same legendary weapon in order to figure out how they will compare.
 armory=us,illidan,john
 main_hand=pwn_weapon,ilevel=666,quality=legendary,stats=666crit_2000sta_666str,enchant=landslide,weapon=axe2h_3.80speed_9000min_9000max

 armory=us,illidan,bill
 main_hand=pwn_weapon,ilevel=666,quality=legendary,stats=666crit_2000sta_666str,enchant=landslide,weapon=axe2h_3.80speed_9000min_9000max

 # Now we pull the rest of the guild's raiders, enforcing the cache. Blii and John will be pulled out from the cache, saving us two queries. The rest of the guild will have to be pulled out from the armory since we flushed the cache at the beginning of the script.
 guild=willyoumarryme,region=us,server=illidan,cache=1,max_rank=5
```

## Proxy
  * **proxy** (scope: subsequent network operations; default "none,,0") can be used to specify a proxy. It will force the application to perform network operations (character and guilds importations, and items and spells queries) through this proxy.The syntax is `proxy=type,host,port`.
    1. _type_: only "http" is supported for now.
    1. _host_ is either an url ("myproxy.com" for example) on an IP address.
    1. _port_ is the port to use (80 is the default port for http requests but your proxy may use another one).
```
 # Here is a regular use: the proxy is specified BEFORE the character importation.
 proxy=http,proxy.example.com,3128 
 armory=us,illidan,John

 # In this second example, the proxy is specified after the character importation: the first armory request (and necessary item queries) won't go through the proxy.
 armory=us,illidan,John
 proxy=http,proxy.example.com,3128 
```

## Items importation sources
> See [Equipment#Items\_data\_importation](Equipment.md#Items_data_importation).

# Advanced options
**Options you should probably not mess up with.**

## Aura delay
  * **default_aura_delay** (scope: global; default: 0.15) is the delay, in seconds, the Blizzard servers need to process aura applications: it is the timespan between the action triggering an aura application and the actual application. It has nothing to do with latency, it is only related to the intricacies of Blizzard's code and their servers' performances. It is used through a normal distribution with a 25% standard deviation (see [Wikipedia - Normal distribution](http://en.wikipedia.org/wiki/Normal_distribution)). This setting affects the following spells: druids' eclipse procs, mages' ignites and warrior's deep wounds. For ignite and deep wounds, see also [MunchingAndRolling](MunchingAndRolling).
```
 default_aura_delay=0.25
```

## Timing wheel
Simulationcraft divides the time into very short slices, which are sequentially processed. By default, we use 32 slices per second (every one of them has a 31ms duration). The consequence is that all events within the same slice will occur at the end of the time slice rather than their original time: they are slightly delayed. Internally, the application uses a "wheel": old slices are reused for future slices.

  * **wheel\_granularity** (scope: global; default: 32) is the number of slices per second to use. Values lesser than or equal to zero will be defaulted to 32.
  * **wheel\_seconds** (scope: global; default: 1024) is the total length, in seconds, of the time wheel.  It should be large enough for the longest, non-infinite, buffs or debuffs. Values below 600s will be defaulted to 1024s.
```
 wheel_granularity=64
 wheel_seconds=1024
```

> For performances reasons, the total wheel time size will be rounded up to the closest superior power of 2.

## Resources regeneration frequency
  * **regen\_periodicity** (scope: global; default: 0.25) is the timespan, in seconds, between two regen ticks. Changing this setting does NOT change the regeneration speed, just how choppy or smoothy it is. Lower values tend to produce many unnecessary events, slowing down the computations. Only mana, focus and energy are affected by this setting.
```
 regen_periodicity=1.0
```

## Error Confidence
  * **confidence** (scope: global; default: 0.95) is the level of confidence for which the true dps lies within the error interval. For example with confidence=0.97, dps=10'000 and error=20, the true dps lies between 9'980 and 10'020 with 97% probability.
```
 confidence=0.97
```

## Allow experimental specialization
  * **allow_experimental_specializations** (scope: global; default: 0) if enabled allows circumventing the deactivation for not fully supported, experimental class specialization. This affects mostly healer specs which often have some basic, but no full support in SimulationCraft. Please note that this is a positional option and needs to come before the character creation.
```
allow_experimental_specializations=1
```
