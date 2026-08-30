---
title: Unique Gear Actions
---
# Unique Gear Actions
Unique gear actions are actions meant to specify how to handle special effects that do something outside of a simple use or equip effect. Usually, these are utilized to specify when to collect orbs spawned by specific gear effects but may be utilized for other effects. 

## The War Within
| Action Name                                                 | Notes                                                 | Example                                                                                     |
| ----------------------------------------------------------- | ----------------------------------------------------- | ------------------------------------------------------------- |
| pickup_entropic_skardyn_core                                | When to pick up orbs spawned by Entropic Skardyn Core | actions+=/pickup_entropic_skardyn_core,use_off_gcd=1
| do_treacherous_transmitter_task                             | When to complete the given Treacherous Transmitter task, potential buffs provided by this trinket are `errant_manaforge_emission`, `cryptic_instructions` and `realigning_nexus_convergence_divergence` | actions+=/do_treacherous_transmitter_task,use_off_gcd=1
| pickup_nerubian_pheromone                                   | When to pick up orbs spawned by Nerubian Pheromone Secreter | actions+=/pickup_nerubian_phearomone,use_off_gcd=1
| pickup_cinderbee_orb                                        | When to pick up orbs spawned by the Embrace of the Cinderbee set | actions+=/pickup_cinderbee_orb,use_off_gcd=1
