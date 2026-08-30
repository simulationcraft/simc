---
title: Evokers
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**

## APL expressions

* **`evoker.use_clipping`** Returns the value of the evoker.use_clipping option

* **`evoker.use_early_chaining`** Returns the value of the evoker.use_early_chainingoption

## Options
* **evoker.use_clipping** (default: 1) Set to let every Disintegrate in Dragonrage be clipped after the 3rd tick.
* **evoker.use_early_chaining** (default: 1) Set to chain Disintegrate in Dragonrage before the window where a tick can be carried without loss (3rd tick on 5 tick disintegrates)
* **evoker.scarlet_overheal** (default: 0.5) Overhealing assumed for Scarlet Adaptation. Currently applies to all healing events.
* **evoker.heal_eb_chance** (default: 0.9) Percentage chance that a healing leaping flame may trigger an essence burst.
* **evoker.prepull_deep_breath_delay** (default: 0.3) Mean delay that prepulling with deepbreath will incur on opener, used as input for Gaussian
* **evoker.prepull_deep_breath_delay_stddev** (default: 0.05) Standard Deviation for delay that prepulling with deep breath will incur on opener, used as input for Gaussian
* **evoker.naszuro_accurate_behaviour** (default: 0) Attempts to emulate accurate Naszuro Behaviour (Needs multi-actor sims)
* **evoker.naszuro_bounce_chance** (default: 0.85) Chance that Naszuro will randomly bounce from a DPS.
* **evoker.force_clutchmates** (default: "", valid: "yes"|"no") Allows the user to force clutchmates into a specific state. Clutchmates by default will automatically adapt to party size. Valid inputs: "yes" => Will force clutchmates on no matter what, even if you were you have >5 players in the sim. "no" => will always disable clutchmates even if you have <= 5 players in the sim.
* **evoker.make_simplified_if_alone** (default: 1) This will automatically spawn in some simplified DPS actors if the Augmentation Evoker is alone. If you wish to see personal DPS for augmentation sim with `evoker.make_simplified_if_alone=0`. This feature is currently very much in alpha.
* **evoker.simplified_actor_ilevel** (default: -1) This sets the Item Level of the Simplified Actors the Augmentation Evoker spawns in. -1 will use their default item level (This varies depending on time of season to match what is the roughly expected gear of the playerbase. This tends towards BiS).
* **evoker.simulate_bombardments** (default: 1) This will simulate bombardment triggers using RPPM instead of using allied damage events to attempt to trigger it.
* **evoker.simulate_bombardments_time_between_procs_mean** (default: 2s) Mean time between attempting to proc bombardments. Works as Gaussian with a 2 second minimum no matter the mean or standard deviation.
* **evoker.simulate_bombardments_time_between_procs_stddev** (default: 0.15s) Standard deviation on bombardment procs
* **evoker.allied_virtual_cd_time** (Default: 118s) A control for the Fake Cooldown Aug uses to try and coordinate to. This is because Breath of Eons gets a lot of CDR from other effects which made writing the APL hard. This cooldown is triggered every time the aug uses breath of eons.
