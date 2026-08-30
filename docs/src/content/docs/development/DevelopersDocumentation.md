---
title: Developers Documentation
---


# Introduction
SimulationCraft never had much documentation about its code. While this might be a big setback for new developers, there is always the risk of information becoming outdated very quickly and documentation is nearly always badly maintained.

So the purpose of this wiki page isn't to create a extensive documentation, but to give you more of a broad and conceptual overview about the architecture of SimC. You can consider it more of a guide/how to help you getting started.

# The core structure
  * The top layer of SimC is the class sim\_t:
    * It parses all options passed to the program
    * It creates and initializes players and sets up everything for the simulation
    * It runs the actual simulation: It controls the event wheel, starts and ends combat, analyzes at the end.

  * player\_t represents the player layer.
    * It contains all information on how to create and initialize a player, what buffs and procs he has, manages his stats and resources and everything else which has to do with the player.

  * action\_t is the foundation of all actions/abilities
    * It contains all ability information ( base dmg, coefficient, cooldown, etc. )
    * It defines the functions to execute an action. Important ones are execute(), ready(), cost(), impact( target ), tick()

# Abilities
  * action\_t is the core foundation for all abilites. It contains all necessary specifications to execute a action.
    * spell\_base\_t inherits from action\_t and defines mechanics common to all spells, including heals.
      * spell\_t ( harmful spells ), heal\_t and absorb\_t inherit from spell\_base\_t
    * attack\_t inherits from action\_t and defines attack mechanics
      * melee\_attack\_t and ranged\_attack\_t inherit from attack\_t

## Action State - New stateless system
`*Todo*`

# The class modules
  * `<`class`>_`t inherits from player\_t and takes care of everything specific about that class. Let's look at priest\_t as an example:
    * Most functions in player\_t are of virtual nature: this allows class module to override, or append to them. Example:
```
virtual double priest_t::composite_spell_power_multiplier()
{
  double m = player_t::composite_spell_power_multiplier();

  m *= 1.0 + buffs.inner_fire -> up () * buffs.inner_fire -> data().effectN( 2 ).percent();

  return m;
}
```
> > This redefines the priest\_t::composite\_spell\_power\_multiplier() function by combining the value of the parent function ( player\_t::composite\_spell\_power\_multiplier() ) with the value of the inner fire buff.

## Targetdata
`*Todo*`

## Class abilities
  * `<`class`>_<`attack/spell/heal/absorb`>_`t is derived from `<`attack/spell/heal/absorb`>_`t and usually defines all class abilities of a certain attack-type.
  * Actual abilities inherit from this `<`class`>_<`attack\_type`>_`t and define everything specific to the ability, including how talents affect it.
  * New `<`class`>_action_t` templates define common behavior for all `<`class`>_<`attack/spell/heal/absorb`>_`t.

  * Let's look at mind\_blast\_t in sc\_priest.cpp for example. It's inheritance structure is:
```
 * action_t
  * spell_base_t
   * spell_t
    * priest_action_t<spell_t>
     * priest_spell_t
      * mind_blast_t
```

### Example

Let's take a very simple sample:
```

struct flamestrike_t : public mage_spell_t
{
  flamestrike_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "flamestrike", p, p -> find_class_spell( "Flamestrike" ) )
  {
    parse_options( NULL, options_str );

    aoe = -1;
  }
};
```
This creates a flamestrike\_t class object, inheriting from mage\_spell\_t. It demands a mage\_t pointer, and the options string reference. It passes the mage\_t pointer on to mage\_spell\_t, as well as a "flamestrike" token, and spell data for "Flamestrike", found in the spell database.

After that, the options _( ,if= string )_ are parsed, and the spell is set to aoe = -1; , meaning that it will do damage to all available enemies, as is the case for flamestrike in WoW.

# Damage Calculations

The damage calculation for direct damage, healing, or absorption from spells and effects is performed in `action_t::calculate_direct_amount()`. It uses the following pseudocode, consolidating stuff strewn about a bunch of different methods. There are several methods whose names change for healing and absorption - these are noted with italics and commented on at the end.

```
  // Base damage calculation, mostly action_t and weapon properties
  direct_amount = average( base_dd_min, base_dd_max ) + base_dd_adder;

  if ( weapon_multiplier > 0 )
    direct_amount += average( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> bonus_damage + weapon_speed * attack_power / 3.5;
    direct_amount *= weapon_multiplier;
  }

  direct_amount += spell_power_mod.direct * spell_power;
  direct_amount += attack_power_mod.direct * attack_power;

  // this next section is all encapsulated in state -> composite_da_multiplier(). 
  // It has five components, each set via action_t::snapshot_internal()
  // Unless otherwise specified, all methods are members of action_t

  // action_state_t -> da_multiplier (Player-based direct damage multipliers, see discussion section)
  direct_amount *= action_multiplier()    // default: action_t -> base_multiplier, overridden in many class modules
  direct_amount *= action_da_multiplier() // default: action_t -> base_dd_multiplier, overridden in many class modules
  direct_amount *= player -> composite_player_multiplier()    // default: 1.0, overridden in many class modules
  direct_amount *= player -> composite_player_dd_multiplier() // default: 1.0, not overridden anywhere yet

  // action_state_t -> persistent_multiplier (Persistent modifiers that are snapshot at the start of the spell cast)
  direct_amount *= composite_persistent_multiplier() // default: 1.0, overridden in several class modules

  // action_state_t -> target_da_multiplier (direct amount multiplier due to debuffs on the target)
  direct_amount *= composite_target_da_multiplier() // default: 1.0 via target -> composite_player_vulnerability() call

  // action_state_t -> versatility (Versatility multiplier, method called depends on action type)
  direct_amount *= ( composite_versatility() + player -> composite_damage_versatility() )

  // action_state_t -> resolve (Tank Resolve multiplier)
  direct_amount *= 1.0 + player -> buffs.resolve -> current_value / 100.00; // default: 1.0, only activated for tanks' heals/absorbs

  // end state -> composite_da_multiplier()

  // If the result is a crit, multistrike, or crit-multistrike, those effects are applied here
  if ( crit ) { tick_amount *= 1.0 + total_crit_bonus(); }
  if ( multistrike ) { tick_amount *= composite_multistrike_multiplier(); }
```

> Notes:
  * For heals and absorbs, `composite_player_multiplier()` is replaced with `composite_player_heal_multiplier()` or `composite_player_absorb_multiplier()`, respectively.
  * For heals, `composite_player_dd_multiplier()` is replaced with `composite_player_dh_multiplier()`. Absorbs do not have this multiplier at all.
  * For heals and absorbs, `composite_damage_versatility()` is replaced with `composite_heal_versatility()`.

  * The da\_multiplier section includes action-specific multipliers that affect the whole action (`action_multiplier`), action-specific multipliers that affect only the direct-damage portion of the action (`action_da_multiplier`), player-specific multipliers that affect all damage (`composite_player_multiplier`), and player-specific multipliers that affect all player direct damage (`composite_player_dd_multiplier`).
  * Persistent multipliers are used for spells which snapshot at the beginning of a cast and don't update mid-channel/cast.
  * Target debuff modifiers are used in a few class modules. This is also where the `damage_taken` and `vulnerable debuff` effects are applied (in `player_t`).
  * Versatility allows for action-specific versatility (unused as of this writing) which is additive with the player's versatility bonus. The default of `action_t::composite_versatility` is 1.0, and the player method just returns the player's versatility percent in decimal format.
  * Resolve is only active for tanks (defaults to 1.0 otherwise), and applies only to heals and absorbs.
  * Methods that also handle effects like AoE damage splitting and caps, glancing, etc. are also handled in calculate\_direct\_amount between `composite_da_multiplier()` and the crit/multistrike calculations. These have been omitted for clarity, and are fairly simple to understand when looking at the code.

Periodic damage and healing effects have a similar process in `action_t::calculate_tick_amount()`. It is much simpler, and  many of the same comments from above apply:

```
  // Base tick amount
  tick_amount = base_td + base_ta_adder;
  tick_amount += spell_power_mod.tick * spell_power;
  tick_amount += attack_power_mod.tick * attack_power;

  // this next section is all encapsulated in state -> composite_ta_multiplier(). 
  // It has five components, each set via action_t::snapshot_internal()
  // Unless otherwise specified, all methods are members of action_t
  
  // action_state_t -> ta_multiplier
  tick_amount *= action_multiplier();
  tick_amount *= action_ta_multiplier(); // specific multipliers for ticks only
  tick_amount *= player -> composite_player_multiplier();
  tick_amount *= player -> composite_player_td_multiplier(); // player DoT-only damage, rarely used

  // action_state_t -> persistent_multiplier
  tick_amount *= composite_persistent_multiplier()

  // action_state_t -> target_ta_multiplier
  tick_amount *= composite_target_ta_multiplier() // default: 1.0 via target -> composite_player_vulnerability() call

  // action_state_t -> versatility
  tick_amount *= ( composite_versatility() + player -> composite_damage_versatility() )

  // action_state_t -> resolve
  tick_amount *= 1.0 + player -> buffs.resolve -> current_value / 100.00; // default: 1.0, only activated for tanks' heals/absorbs

  // end state -> composite_ta_multiplier()

  // If the result is a crit, multistrike, or crit-multistrike, those effects are applied here
  if ( crit ) { tick_amount *= 1.0 + total_crit_bonus(); }
  if ( multistrike ) { tick_amount *= composite_multistrike_multiplier(); }

```
  * For heals and absorbs, `composite_player_multiplier()` is replaced with `composite_player_heal_multiplier()` or `composite_player_absorb_multiplier()`, respectively.
  * For heals, `composite_player_td_multiplier()` is replaced with `composite_player_th_multiplier()`. Absorbs do not have this multiplier at all.
  * For heals and absorbs, `composite_damage_versatility()` is replaced with `composite_heal_versatility()`.

Percent heals are handled similarly, though the base amount is obviously calculated differently. See `heal_t::calculate_tick_amount()` in `sc_spell.cpp` for details.


# Various helper functions

  * event\_t: Event class used for creating custom events. Defining the inherited execute() function is mandatory, defining what happens when the event is executed. To add the event to the timing wheel, use sim -> add\_event( event\_t`*`, timespan\_t delta\_time );

  * sample\_data\_t
  * stats\_t
  * cooldown\_t
  * dot\_t
  * gain\_t

# How to Localize the GUI
QT allows for very easy localization. All strings wrapped in tr() can get localized on runtime.

How to create and modify a localization file, using the German language as a example.

1. run
```
lupdate simcqt.pro -ts qt/locale/de_DE.ts 
```

2. open the created .ts file with QT Linguist. Translate a word, check it as translated and continue. After you're done, save the file.

3. In QT Linguist, call File -> Release. This will create a .qm file, which is a compressed/compiled version of the .ts file.

4. Run SimulationCraft.

# Special Gotchas
- Never compare a number to NaN. It will result in unexpected behaviour with certain fast-math settings.
## Buffs
- Never create more than one buff per player with the same name.

# External services (work in progress)

We run a [jenkins](http://jenkins-ci.org) service at [Simulationcraft.org](http://jenkins.simulationcraft.org) that automatically builds the command line client of SimulationCraft by periodically polling the git repository at google code. After building the command line binary, a set of tests are run on it.

The server scripts, tests, and the web content of simulationcraft.org are located at GitHub (http://github.com/simulationcraft). The testing system uses [BATS](http://github.com/sstephenson/bats) to invoke simulationcraft with a set of simple parameters.

Currently, testing is split into two separate categories: fight style testing, and class module testing. Fight style testing tests the highest ilevel raid simulation at the time against three separate fight styles (HeavyMovement, HelterSkelter, and HecticAddCleave). The class module testing tests each individual highest ilevel class/spec profile against all relevant talents for the class. The relevant talents are generated by the `talent_options` script, found in the `simc-tests` repository on GitHub.

We also offer IRC logs for viewing at [http://simulationcraft.org/irclogs/](http://simulationcraft.org/irclogs/).
