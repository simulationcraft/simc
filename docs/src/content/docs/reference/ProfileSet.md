---
title: Profile Set
---
# Profile Sets

**_(Added in Simulationcraft 725-01)_**

Profile sets are a new mechanism of batch-simulating actors. Simulationcraft currently has two ways to do large (multi-actor) simulations. First, inputting multiple actors without any additional options will simulate all actors in the same environment (i.e., a raid). Adding `single_actor_batch=1` option will segregate the actors to simulating separately in individual phases.

Profile sets add a third simulation mode. Instead of simulating multiple profiles under the same environment, profile sets first simulate a baseline set of profiles, and then each profile set individually in a separate simulation environment.

This has several benefits, the largest being that profile sets only require memory during run time for two simultaneous actors (i.e., the baseline which is kept in memory during the whole simulation run, and the individual profile set environment that is destroyed after it finishes). This should essentially remove the memory-related issues of conventional simulation modes (i.e., multi-actor or `single_actor_batch=1`), where the simulator allocates memory for all actors at the initialization phase of the simulator.

Note that profile sets only output summary information about the baseline set of profiles. This includes at most the minimum, first quartile, median, mean, third quartile, and maximum metric values, as well as the standard deviation and the number of iterations used. The full information content is only available in JSON reports, the textual report only outputs the median value, and the HTML report includes all but standard deviation and number of iterations.

**Note, profileset functionality is only available on the command-line Simulationcraft program.**

## Usage

Profile sets can be created in the simulator with the following format:
```
<baseline profile definition>

profileset.<profileset name>=<option-1>
profileset.<profileset name>+=<option-2>
...
profileset.<profileset name>+=<option-N>
```

You can only include a single baseline profile in the profile set simulation. Profile set names must be unique. In reporting, the profile set name is what differentiates the results. If you want to use whitespaces in the profile set names, enclose the name in double quotes (```profileset."white space"=...```). You cannot use the `.` character in a profileset name.

You can also control which metric is used to collect from profile sets with the ```profileset_metric``` option (default `dps`). Note that output may be odd for metrics that do not support extended data collection (e.g., percentiles).

**Using `armory` option with profile sets is not recommended as it will significantly slow down the initialization process. Save your base profile to a file (using `save`command), or use an externally imported Blizzard armory data with the `local_json` option.**

### Parallel processing for profilesets 

**_(Supported in Simulationcraft 735-01 or newer)_**

The default behavior of Simulationcraft profilesets iterates over all of the profilesets in (an unordered) sequence. Each profileset simulation uses the same number of threads as the baseline simulation has defined (by default the number of hardware concurrent threads).

In addition to the sequential mode, Simulationcraft can also process profilesets in parallel. With parallel processing, each profileset will create a worker thread (up to a maximum number of workers), responsible for simulating the profile. A simulation-scope option `profileset_work_threads` specifies the number of threads each worker is allowed to use. The maximum number of workers in the simulator is defined as `floor( threads / profileset_work_threads )`. For example, with `threads=8` and `profileset_work_threads=2` the simulator would run four concurrent profileset workers.

The parallel profileset mode will also change the progress bar of the simulator to no longer report iteration-level details of each simulated profileset, but rather express the progress in terms of finished profilesets.

### Supported options for profile sets

Profile sets support the vast majority of simulation and player scope options. You can control the number of threads, override spell data, define different `raid_events`, or use `target_error` or `iterations` in an individual profile set without it leaking to other profile sets. You can also specify output options for individual profile sets, in which case the corresponding report is generated. Note that the profile set simulations are currently run with `report_details=0`, so detailed reporting of actions and buffs is unavailable.

Options that do not work in profile sets include (but are not limited to): various output-only options such as ```spell_query```, scale factor calculation, any kind of plotting, or adding additional players (e.g., ```armory```, ```copy```, or ```class_name``` options).

### Profile sets with multiple actors in the baseline

**_(Added during Simulationcraft 1015-01)_**

When the baseline simulation includes multiple actors, there are several additional options that can be used to configure which actor will be modified in the profile sets.

* **profileset_main_actor_index** (scope: global, default: 0) The actor-creation index in the input text after which options will be overridden and new options will be appended. This is not limited to player actors and can be used to override other actors (pets, enemies, etc.).
* **profileset_report_player_index** (scope: global, default: 0) The actor index in the simulation for which player metrics will be reported. Note that this index is not the same as `profileset_main_actor_index` because this option only includes players.
* **profileset_multiactor_base_name** (scope: global, default: "Baseline") The name for the baseline simulation that will be displayed in the HTML report if a multi-actor `profileset_metric` such as `raid_dps` or `time` is selected.
