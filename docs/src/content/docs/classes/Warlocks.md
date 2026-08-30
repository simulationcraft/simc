---
title: Warlocks
---


# Options
  * **soul\_shards** : Specify initial Soul Shard amount
  * **default\_pet** : Specify default main pet (_imp_, _voidwalker_, _incubus_, _succubus_, _sayaad_, _felhunter_, Demo only: _felguard_)
  * **disable_felstorm** : Disables automatic usage of Felstorm for Demonology's main Felguard
  * **normalize_destruction_mastery** : If true, will force Chaotic Energies to always return its average value (default value: false)

# Actions
  * _interrupt_ : Will use an appropriate pet interrupt (Spell Lock/Axe Toss) if target is casting and a valid pet is currently active

# Expressions
  * _last\_cast\_imps_ : Number of Wild Imps with one cast remaining
  * _two\_cast\_imps_ : Number of Wild Imps with two casts remaining
  * _igb\_ratio_ : Calculates number of Imp Gang Bosses divided by total Wild Imps (incl. IGBs)
  * _time\_to\_shard_ : Estimated time until Agony next generates a Soul Shard
  * _pet\_count_ : Total number of (Warlock) pets currently active
  * _havoc\_active_ : Boolean checking if Havoc is currently applied to _any_ target whatsoever
  * _havoc\_remains_ : Time remaining on the Havoc debuff, wherever it is
  * _incoming\_imps_ : Number of Wild Imps which will be spawning from Hand of Gul'dan but have not yet spawned
  * _can\_seed_ : Returns true if there is a valid target for Seed of Corruption which does not have a Seed DoT or Seed in-flight already
  * _time\_to\_imps.N.remains_ : Returns the time to having N total Wild Imps, including those scheduled to be spawned. If N is greater than the total expected, returns time to last imp. "all" can be used in place of a value as well.
  * _diabolic_ritual_ : Returns true if any Diabolic Ritual buff is active on the player
  * _demonic_art_ : Returns true if any Demonic Art buff is active on the player

# RNG Control
These are advanced player-scoped options that can be used to override a number of proc chance values that are not in spell data. Use caution when adjusting these values. All values should be between 0 and 1 unless otherwise specified. Default values are not listed on this page.
  * _rng_cunning_cruelty_sb_ : Cunning Cruelty (Shadow Bolt)
  * _rng_cunning_cruelty_ds_ : Cunning Cruelty (Drain Soul)
  * _rng_agony_ : Maximum increment value per tick of Agony for soul shard generation
  * _rng_nightfall_ : Maximum increment value per tick of Corruption/Wither for Nightfall
  * _rng_pact_of_the_eredruin_
  * _rng_shadow_invocation_
  * _rng_spiteful_reconstitution_
  * _rng_decimation_
  * _rng_dimension_ripper_
  * _rng_blackened_soul_ : Collapse chance upon gaining a stack of Wither
  * _rng_bleakheart_tactics_ : Chance to gain another stack of Wither
  * _rng_seeds_of_their_demise_ : Chance to proc Tormented Crescendo (Affliction) or Flashpoint (Destruction)
  * _rng_mark_of_perotharn_ : Chance to gain a stack of Wither on critical strikes
  * _rng_succulent_soul_aff_ : Chance to proc Succulent Soul (rolls once per soul shard gained)
  * _rng_succulent_soul_demo_
  * _rng_feast_of_souls_
  * _rng_umbral_lattice_ : Affliction Nerub-ar Palace tier set proc
  * _rng_empowered_legion_strike_ : Demonology Nerub-ar Palace tier set proc
