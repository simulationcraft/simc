#pragma once
#include "simulationcraft.hpp"

#include "player/pet_spawner.hpp"
#include "sc_warlock_pets.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/warlock.hpp"

namespace warlock
{

enum dimensional_rift_pet_e
{
  DR_PET_SHADOWY_TEAR,
  DR_PET_UNSTABLE_TEAR,
  DR_PET_CHAOS_TEAR,
  DR_PET_OVERFIEND
};

enum dominion_of_argus_pet_e
{
  DOA_PET_RANDOM = -1,
  DOA_PET_JAILER,
  DOA_PET_SACROLASH,
  DOA_PET_GRAND_WARLOCK,
  DOA_PET_INQUISITOR,
  DOA_PET_MAX
};

struct warlock_t;

// Finds an action with the given name. If no action exists, a new one will
// be created.
//
// Use this with secondary background actions to ensure the player only has
// one copy of the action.
template <typename Action, typename Actor, typename... Args>
action_t* get_action( util::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}

struct warlock_td_t : public actor_target_data_t
{
  struct debuffs_t
  {
    // Aff
    propagate_const<buff_t*> haunt;

    // Demo
    propagate_const<buff_t*> doom;

    // Destruction
    propagate_const<buff_t*> lake_of_fire;
    propagate_const<buff_t*> shadowburn;
    propagate_const<buff_t*> havoc;

    // Diabolist
    propagate_const<buff_t*> cloven_soul;

    // Hellcaller
    propagate_const<buff_t*> blackened_soul; // Dummy/Hidden debuff that triggers stack collapse
    propagate_const<buff_t*> wither; // Dummy debuff to show Wither dot info (stacks, uptime, ...) in the final report
  } debuffs;

  struct dots_t
  {
    // Cross-spec
    propagate_const<dot_t*> drain_life;
    propagate_const<dot_t*> corruption;

    // Aff
    propagate_const<dot_t*> agony;
    propagate_const<dot_t*> seed_of_corruption;
    propagate_const<dot_t*> drain_soul;
    propagate_const<dot_t*> unstable_affliction;
    propagate_const<dot_t*> malefic_grasp;

    // Destro
    propagate_const<dot_t*> immolate;

    // Hellcaller
    propagate_const<dot_t*> wither;

    // Soul Harvester
    propagate_const<dot_t*> soul_anathema;
    propagate_const<dot_t*> shared_fate;
  } dots;

  double soc_threshold; // Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  { soc_threshold = 0; }

  void target_demise();

  int count_affliction_dots() const;
};

// Shuffled Bag RNG (sampling without replacement)
// Each draw removes an item until the bag is exhausted, then it automatically resets
template <typename T>
struct shuffled_bag_rng_t
{
private:
  player_t* player;
  std::vector<T> bag;
  int remaining;

public:
  shuffled_bag_rng_t( std::vector<T> items, player_t* p )
    : player( p ), bag( std::move( items ) ), remaining( as<int>( bag.size() ) )
  { assert( !bag.empty() ); }

  shuffled_bag_rng_t( std::initializer_list<T> items, player_t* p )
    : shuffled_bag_rng_t( std::vector<T>( items ), p )
  {}

  void reset()
  { remaining = as<int>( bag.size() ); }

  T draw()
  {
    if ( remaining == 0 )
      reset();

    const int index = player->rng().range( 0, remaining );
    const T item = bag[ index ];

    std::swap( bag[ index ], bag[ remaining - 1 ] );
    remaining--;

    return item;
  }

  int num_remaining() const
  { return remaining; }

  int size() const
  { return as<int>( bag.size() ); }
};

// Fixed-cycle proc helper
// Advances a counter on each call and triggers every Nth event
struct fixed_cycle_proc_t : public proc_rng_t
{
private:
  using proc_reset_counter_fn = std::function<unsigned( unsigned )>;

  proc_reset_counter_fn proc_reset_fn;
  unsigned counter;
  unsigned trigger_count;
  bool random_initial_state;

public:
  static constexpr rng_type_e rng_type = RNG_CUSTOM;

  fixed_cycle_proc_t( std::string_view n, player_t* p, unsigned trigger_count_,
                      bool random_initial_state_ = false, proc_reset_counter_fn proc_reset_fn_ = nullptr )
    : proc_rng_t( rng_type, n, p ),
      proc_reset_fn( std::move( proc_reset_fn_ ) ),
      counter( 0u ),
      trigger_count( trigger_count_ ),
      random_initial_state( random_initial_state_ )
  { assert( trigger_count > 0u ); }

  void reset( reset_type_e reset_type ) override
  {
    switch ( reset_type )
    {
      case reset_type_e::COMBAT:
        counter = proc_reset_fn ? proc_reset_fn( trigger_count ) : 0u;
        assert( counter < trigger_count );
        break;
      case reset_type_e::ITERATION:
        counter = random_initial_state ? player->rng().range( 0u, trigger_count ) : 0u;
        break;
      default:
        assert( false && "Unhandled reset_type_e value" );
    }
  }

  int trigger( action_state_t* = nullptr ) override
  {
    counter++;

    if ( player->sim->debug )
      player->sim->print_debug( "Fixed Cycle Proc: {}, current_count={} trigger_count={}, proc={}", name(), counter,
                                trigger_count, counter >= trigger_count );

    if ( counter >= trigger_count )
    {
      reset( reset_type_e::COMBAT );
      return true;
    }

    return false;
  }

  unsigned get_counter() const
  { return counter; }

  unsigned get_trigger_count() const
  { return trigger_count; }

  void set_counter( unsigned v )
  {
    assert( v < trigger_count );
    counter = v;
  }
};

// utility to create target_effect_t compatible functions from warlock_td_t member references
template <typename T>
static std::function<int( actor_target_data_t* )> d_fn( T d, bool stack = true )
{
  if constexpr ( std::is_invocable_v<T, warlock_td_t::debuffs_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast< warlock_td_t*>( t )->debuffs )->check();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast< warlock_td_t*>( t )->debuffs )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<T, warlock_td_t::dots_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast< warlock_td_t*>( t )->dots )->current_stack();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast< warlock_td_t*>( t )->dots )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of warlock_td_t" );
    return nullptr;
  }
}

struct warlock_t : public parse_player_effects_t
{
public:
  player_t* havoc_target;
  std::vector<action_t*> havoc_spells; // Used for smarter target cache invalidation.
  player_t* haunt_target; // Used for tracking the current haunt target
  player_t* patient_zero_target; // Used to track which target benefits from the Patient Zero talent damage increase
  std::vector<event_t*> wild_imp_spawns; // Used for tracking incoming imps from HoG TODO: Is this still needed with faster spawns?
  int diabolic_ritual; // Used to cycle between the three different Diabolic Ritual buffs
  bool demonic_art_buff_replaced; // Used to not spawn the Demonic Art demon if the buff is replaced by another
  timespan_t wild_imp_ic_shared_offset; // Used as a shared offset when scheduling Wild Imp Infernal Command periodic events

  unsigned n_active_pets;

  // This should hold any spell data that is guaranteed in the base class or spec, without talents or other external systems required
  struct base_t
  {
    // Shared
    const spell_data_t* nethermancy; // Int bonus for all cloth slots
    const spell_data_t* drain_life;
    const spell_data_t* corruption;
    const spell_data_t* shadow_bolt;
    const spell_data_t* shadow_bolt_energize;

    // Affliction
    const spell_data_t* affliction_warlock; // Spec aura
    const spell_data_t* potent_afflictions; // Affliction Mastery - Increased DoT and Malefic Rapture damage

    // Demonology
    const spell_data_t* demonology_warlock; // Spec aura
    const spell_data_t* master_demonologist; // Demonology Mastery - Increased demon damage
    const spell_data_t* wild_imp; // Data for pet summoning (HoG)
    const spell_data_t* wild_imp_2; // Data for pet summoning (Inner Demons / Spiteful Reconstitution / To Hell and Back)
    const spell_data_t* fel_firebolt_2; // Still a separate spell (learned automatically). Reduces pet's energy cost
    const spell_data_t* infernal_command_buff; // This still applies but with 0 value

    // Destruction
    const spell_data_t* destruction_warlock; // Spec aura
    const spell_data_t* chaotic_energies; // Destruction Mastery - Increased spell damage with random range
    const spell_data_t* immolate; // Replaces Corruption
    const spell_data_t* immolate_old; // For some reason, the spellbook spell is now a new spell, but it points to this old one
    const spell_data_t* immolate_dot; // Primary spell data only contains information on direct damage
    const spell_data_t* incinerate; // Replaces Shadow Bolt
    const spell_data_t* incinerate_energize; // Soul Shard data is in a separate spell
  } warlock_base;

  // Main pet held in active, guardians should be handled by pet spawners.
  struct pets_t
  {
    warlock_pet_t* active;

    spawner::pet_spawner_t<pets::destruction::infernal_t, warlock_t> infernals;

    spawner::pet_spawner_t<pets::affliction::darkglare_t, warlock_t> darkglares;
    spawner::pet_spawner_t<pets::affliction::desperate_soul_t, warlock_t> desperate_souls;

    spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers;
    spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
    spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;
    spawner::pet_spawner_t<pets::demonology::grimoire_imp_lord_t, warlock_t> grimoire_imp_lords;
    spawner::pet_spawner_t<pets::demonology::grimoire_fel_ravager_t, warlock_t> grimoire_fel_ravagers;
    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;
    spawner::pet_spawner_t<pets::demonology::doomguard_t, warlock_t> doomguards;
    spawner::pet_spawner_t<pets::demonology::lady_sacrolash_t, warlock_t> lady_sacrolash;
    spawner::pet_spawner_t<pets::demonology::grand_warlock_alythess_t, warlock_t> grand_warlock_alythess;
    spawner::pet_spawner_t<pets::demonology::antoran_inquisitor_t, warlock_t> antoran_inquisitor;
    spawner::pet_spawner_t<pets::demonology::antoran_jailer_t, warlock_t> antoran_jailer;

    spawner::pet_spawner_t<pets::destruction::shadowy_tear_t, warlock_t> shadowy_rifts;
    spawner::pet_spawner_t<pets::destruction::unstable_tear_t, warlock_t> unstable_rifts;
    spawner::pet_spawner_t<pets::destruction::chaos_tear_t, warlock_t> chaos_rifts;
    spawner::pet_spawner_t<pets::destruction::infernal_roc_t, warlock_t> rocs;
    spawner::pet_spawner_t<pets::destruction::overfiend_t, warlock_t> overfiends;

    spawner::pet_spawner_t<pets::diabolist::overlord_t, warlock_t> overlords;
    spawner::pet_spawner_t<pets::diabolist::mother_of_chaos_t, warlock_t> mothers;
    spawner::pet_spawner_t<pets::diabolist::pit_lord_t, warlock_t> pit_lords;

    spawner::pet_spawner_t<pets::diabolist::infernal_fragment_t, warlock_t> fragments;

    spawner::pet_spawner_t<pets::diabolist::diabolic_imp_t, warlock_t> diabolic_imps;

    spawner::pet_spawner_t<pets::soul_harvester::rampaging_demonic_soul_t, warlock_t> demonic_souls;

    pets_t( warlock_t* w );
  } warlock_pet_list;

  std::vector<std::string> pet_name_list;

  // Talents
  struct talents_t
  {
    // Class Tree

    player_talent_t demonic_embrace;
    player_talent_t demonic_fortitude;
    player_talent_t pact_of_the_annihilan;
    player_talent_t pact_of_the_satyr;
    player_talent_t pact_of_the_eredar;
    player_talent_t soulburn;
    const spell_data_t* soulburn_buff; // This buff is applied after using Soulburn and prevents another usage unless cleared

    // Specializations

    // Shared
    player_talent_t summoners_embrace;
    player_talent_t grimoire_of_sacrifice; // Aff/Destro only
    const spell_data_t* grimoire_of_sacrifice_buff; // 1 hour duration, enables proc functionality, canceled if pet summoned
    const spell_data_t* grimoire_of_sacrifice_proc; // Damage data is here, but RPPM of proc trigger is in buff data

    // Affliction
    player_talent_t agony;
    const spell_data_t* agony_energize;
    player_talent_t unstable_affliction;
    const spell_data_t* unstable_affliction_2; // Soul Shard on demise (learned automatically)
    player_talent_t seed_of_corruption;
    const spell_data_t* seed_of_corruption_aoe; // Explosion damage when Seed ticks
    const spell_data_t* seed_of_corruption_is_out_dnt;

    player_talent_t nightfall;
    const spell_data_t* nightfall_buff;
    const spell_data_t* nightfall_buff_2;
    player_talent_t haunt;
    player_talent_t shared_agony;

    player_talent_t improved_shadow_bolt;
    player_talent_t drain_soul; // This represents the talent node but not much else
    const spell_data_t* drain_soul_dot; // Contains all channel data
    player_talent_t improved_haunt;
    player_talent_t absolute_corruption;
    player_talent_t siphon_life;

    player_talent_t cunning_cruelty; // Note: Damage formula in the tooltip indicates this is affected by Imp. Shadow Bolt and Sargerei Technique
    const spell_data_t* shadowbolt_volley; // Proc chance is not listed on spell data. Appears to be 50% regardless of talent. Last checked 2024-07-07
    player_talent_t withering_bolt; // Increased damage on Shadow Bolt/Drain Soul based on active DoT count on target
    player_talent_t creeping_death;
    player_talent_t dark_harvest; // Buffs from hitting targets with Soul Rot
    const spell_data_t* dark_harvest_dmg;

    player_talent_t practiced_pestilence;
    player_talent_t summon_darkglare;
    const spell_data_t* eye_beam; // Darkglare pet ability
    const spell_data_t* darkglare_presence_buff; // DoT +% dmg buff
    // Summoner's Embrace (shared with Destruction)
    // Grimoire of Sacrifice (shared with Destruction)
    player_talent_t cull_the_weak;

    player_talent_t malediction;
    player_talent_t sudden_onset;
    player_talent_t eye_contract;
    player_talent_t malefic_grasp;
    const spell_data_t* malefic_grasp_2;
    const spell_data_t* malefic_grasp_3;
    const spell_data_t* agony_mg;
    const spell_data_t* unstable_affliction_mg;
    const spell_data_t* corruption_mg;
    const spell_data_t* wither_mg;
    player_talent_t nether_plating;
    player_talent_t sacrolashs_dark_strike; // Increased Corruption ticking damage, and ticks extend Curses (not implemented)
    player_talent_t contagion;

    player_talent_t shard_instability;
    const spell_data_t* shard_instability_buff;
    player_talent_t niskaran_methods;
    player_talent_t potent_soul_shards;
    player_talent_t nocturnal_yield;

    player_talent_t xavius_gambit; // Unstable Affliction Damage Multiplier
    player_talent_t ravenous_afflictions;
    player_talent_t seeds_of_destruction;

    player_talent_t fatal_echoes;
    player_talent_t cascading_calamity;
    const spell_data_t* cascading_calamity_buff;
    player_talent_t deaths_embrace; // Volatile Agony and Perpetual Unstability are unaffected by this
    player_talent_t patient_zero;
    player_talent_t sow_the_seeds;

    // Affliction Apex
    player_talent_t shadow_of_nathreza_1;
    player_talent_t shadow_of_nathreza_2;
    player_talent_t shadow_of_nathreza_3;
    const spell_data_t* summon_desperate_soul;
    const spell_data_t* shadow_of_nathreza_dot;
    const spell_data_t* wrath_of_nathreza; // Trigger missile spell
    const spell_data_t* wrath_of_nathreza_impact;

    // Demonology
    player_talent_t hand_of_guldan;
    const spell_data_t* hand_of_guldan_cast;
    const spell_data_t* hog_impact; // Secondary spell responsible for impact damage

    player_talent_t demoniac;
    const spell_data_t* demonbolt_spell;
    const spell_data_t* demonbolt_energize;
    const spell_data_t* demonic_core_spell;
    const spell_data_t* demonic_core_buff;
    player_talent_t call_dreadstalkers;
    const spell_data_t* call_dreadstalkers_summon_1; // Contains summon data
    const spell_data_t* call_dreadstalkers_summon_2; // Contains summon data

    player_talent_t dominant_hand;
    player_talent_t fel_intellect;
    player_talent_t practiced_rituals;
    player_talent_t dreadlash;

    player_talent_t imperator; // Increased critical strike chance for Wild Imps' Fel Firebolt (additive)
    player_talent_t implosion;
    const spell_data_t* implosion_aoe; // Note: in combat logs this is attributed to the player, not the imploding pet
    player_talent_t power_siphon;
    const spell_data_t* power_siphon_buff; // Semi-hidden aura that controls the bonus Demonbolt damage
    player_talent_t summon_felguard;

    player_talent_t infernal_rapidity;
    player_talent_t rune_of_shadows;
    player_talent_t carnivorous_stalkers; // Chance for Dreadstalkers to perform additional Dreadbites
    player_talent_t fel_armaments;

    player_talent_t imp_gang_boss;
    const spell_data_t* imp_gang_boss_buff; // Buff on Wild Imps
    player_talent_t demonic_brutality;
    player_talent_t inner_demons;
    player_talent_t summon_demonic_tyrant;
    const spell_data_t* demonic_power_buff;
    player_talent_t blighted_maw;
    const spell_data_t* blighted_maw_dmg;
    player_talent_t improved_demonic_tactics;
    player_talent_t empowered_felstorm;

    player_talent_t spiteful_reconstitution; // Increased Implosion damage and consuming Demonic Core may spawn a Wild Imp
    player_talent_t tyrants_oblation;
    const spell_data_t* tyrants_oblation_buff;
    player_talent_t antoran_armaments;
    player_talent_t flametouched;
    const spell_data_t* flametouched_buff;

    player_talent_t demonic_knowledge;
    player_talent_t sacrificed_souls;
    player_talent_t reign_of_tyranny;
    player_talent_t master_summoner;
    player_talent_t demonic_calling;

    player_talent_t doom;
    const spell_data_t* doom_debuff;
    const spell_data_t* doom_dmg;
    player_talent_t hellbent_commander;
    const spell_data_t* hellbent_commander_buff;
    player_talent_t grimoire_imp_lord;
    player_talent_t grimoire_fel_ravager;
    const spell_data_t* grimoire_of_service_buff;
    player_talent_t summon_vilefiend;
    const spell_data_t* vilefiend;
    const spell_data_t* gloomhound;
    const spell_data_t* charhound;
    const spell_data_t* bile_spit;
    const spell_data_t* headbutt;

    player_talent_t summon_doomguard;
    const spell_data_t* doom_bolt_volley;
    player_talent_t to_hell_and_back;
    const spell_data_t* unstable_soul_buff;
    player_talent_t stabilized_portals;
    player_talent_t mark_of_shatug;
    const spell_data_t* gloom_slash;
    player_talent_t mark_of_fharg;
    const spell_data_t* infernal_presence;
    const spell_data_t* infernal_presence_dmg;

    // Demo Apex Talents
    player_talent_t dominion_of_argus_1;
    player_talent_t dominion_of_argus_2;
    player_talent_t dominion_of_argus_3;
    const spell_data_t* dominion_of_argus_1_buff;
    const spell_data_t* dominion_of_argus_3_gain;
    const spell_data_t* doa_lady_sacrolash_summon;
    const spell_data_t* doa_grand_warlock_alythess_summon;
    const spell_data_t* doa_antoran_inquisitor_summon;
    const spell_data_t* doa_antoran_jailer_summon;

    // Destruction
    player_talent_t chaos_bolt;
    player_talent_t conflagrate; // Base 2 charges
    const spell_data_t* conflagrate_2; // Energize data
    player_talent_t rain_of_fire;
    const spell_data_t* rain_of_fire_tick;

    player_talent_t improved_conflagrate; // +1 charge for Conflagrate
    player_talent_t backdraft;
    const spell_data_t* backdraft_buff;
    player_talent_t practiced_chaos;
    player_talent_t roaring_blaze;

    player_talent_t explosive_potential; // Reduces base Conflagrate cooldown by 2 seconds
    player_talent_t mayhem; // It appears that the only spells that can proc Mayhem are ones that can be Havoc'd
    player_talent_t havoc; // Talent data for Havoc is both the debuff and the action
    const spell_data_t* havoc_debuff; // This is a second copy of the talent data for use in places that are shared by Havoc and Mayhem
    player_talent_t scalding_flames; // Increased Immolate damage and duration

    player_talent_t shadowburn;
    const spell_data_t* shadowburn_2; // Contains Soul Shard energize data
    player_talent_t backlash; // Crit chance increase. NOT IMPLEMENTED: Instant Incinerate proc when physically attacked
    player_talent_t improved_havoc;
    player_talent_t ashen_remains; // Increased Chaos Bolt and Incinerate damage to targets afflicted by Immolate
    player_talent_t cataclysm;

    player_talent_t fiendish_cruelty;
    const spell_data_t* fiendish_cruelty_buff;
    player_talent_t chaotic_inferno;
    const spell_data_t* chaotic_inferno_buff;
    player_talent_t flashpoint; // Stacking haste buff from Immolate ticks on high-health targets
    const spell_data_t* flashpoint_buff;
    player_talent_t summon_infernal;
    const spell_data_t* summon_infernal_main; // Data for main infernal summoning
    const spell_data_t* infernal_awakening; // AoE on impact is attributed to the Warlock
    const spell_data_t* immolation_buff; // Buff on Infernal pet
    const spell_data_t* immolation_dmg; // Ticking AoE damage from buff
    const spell_data_t* embers; // Buff which generates Soul Shards
    const spell_data_t* burning_ember; // Energize data for Soul Shards
    player_talent_t emberstorm;
    player_talent_t fire_and_brimstone;
    player_talent_t lake_of_fire;
    const spell_data_t* lake_of_fire_aoe;
    const spell_data_t* lake_of_fire_tick;
    const spell_data_t* lake_of_fire_debuff;

    player_talent_t reverse_entropy;
    const spell_data_t* reverse_entropy_buff;
    player_talent_t internal_combustion;
    const spell_data_t* internal_combustion_dmg;
    player_talent_t crashing_chaos; // Summon Infernal increases the damage of next 8 Chaos Bolt or Rain of Fire casts
    const spell_data_t* crashing_chaos_buff;
    player_talent_t rain_of_chaos;
    const spell_data_t* rain_of_chaos_buff;
    const spell_data_t* summon_infernal_roc; // Contains Rain of Chaos infernal duration
    // Summoner's Embrace (shared with Affliction)
    // Grimoire of Sacrifice (shared with Affliction)

    player_talent_t ruin; // Damage increase to several spells
    player_talent_t improved_chaos_bolt;
    player_talent_t destructive_rapidity;
    player_talent_t devastation;

    player_talent_t dimensional_rift;
    const spell_data_t* shadowy_tear_summon; // This only creates the "pet"
    const spell_data_t* shadow_barrage; // Casts Rift version of Shadow Bolt on ticks
    const spell_data_t* rift_shadow_bolt; // Separate ID from Warlock's Shadow Bolt
    const spell_data_t* unstable_tear_summon; // This only creates the "pet"
    const spell_data_t* chaos_barrage; // Triggers ticks of Chaos Barrage bolts
    const spell_data_t* chaos_barrage_tick;
    const spell_data_t* chaos_tear_summon; // This only creates the "pet"
    const spell_data_t* rift_chaos_bolt; // Separate ID from Warlock's Chaos Bolt
    player_talent_t soul_fire;
    const spell_data_t* soul_fire_2; // Contains Soul Shard energize data
    player_talent_t chaos_incarnate; // Greater mastery value for some spells
    player_talent_t conflagration_of_chaos; // Conflagrate/Shadowburn has chance to make next cast of it a guaranteed crit
    const spell_data_t* conflagration_of_chaos_buff; // Player buff which affects next Conflagrate/Shadowburn
    player_talent_t diabolic_embers; // Incinerate generates more Soul Shards
    player_talent_t demonfire_infusion;
    player_talent_t channel_demonfire;
    const spell_data_t* channel_demonfire_tick;
    const spell_data_t* channel_demonfire_travel; // Only holds travel speed

    player_talent_t avatar_of_destruction;
    const spell_data_t* summon_overfiend;
    const spell_data_t* overfiend_buff; // Buff on Warlock while Overfiend is out, generates Soul Shards
    const spell_data_t* overfiend_cb; // Chaos Bolt cast by Overfiend
    player_talent_t inferno;
    player_talent_t alythesss_ire;
    const spell_data_t* alythesss_ire_buff;
    player_talent_t raging_demonfire;

    player_talent_t embers_of_nihilam_1;
    player_talent_t embers_of_nihilam_2;
    player_talent_t embers_of_nihilam_3;
    const spell_data_t* echo_of_sargeras; // Damage spell (1265884)
    const spell_data_t* vision_of_nihilam; // Crit/Haste buff (1265939)
  } talents;

  struct hero_talents_t
  {
    // Diabolist
    player_talent_t diabolic_ritual;
    const spell_data_t* ritual_overlord; // Diabolic Ritual: X buffs
    const spell_data_t* ritual_mother;
    const spell_data_t* ritual_pit_lord;
    const spell_data_t* art_overlord; // Demonic Art: X buffs
    const spell_data_t* art_mother;
    const spell_data_t* art_pit_lord;
    const spell_data_t* summon_overlord;
    const spell_data_t* summon_mother;
    const spell_data_t* summon_pit_lord;
    const spell_data_t* wicked_cleave; // Overlord
    const spell_data_t* chaos_salvo; // Mother of Chaos
    const spell_data_t* chaos_salvo_missile;
    const spell_data_t* chaos_salvo_dmg;
    const spell_data_t* felseeker; // Pit Lord
    const spell_data_t* felseeker_dmg;

    player_talent_t cloven_souls;
    const spell_data_t* cloven_soul_debuff;
    player_talent_t touch_of_rancora;
    player_talent_t secrets_of_the_coven;
    const spell_data_t* infernal_bolt;
    const spell_data_t* infernal_bolt_buff;

    player_talent_t cruelty_of_kerxan;
    player_talent_t infernal_machine;

    player_talent_t flames_of_xoroth;
    player_talent_t abyssal_dominion;
    const spell_data_t* abyssal_dominion_buff;
    const spell_data_t* infernal_fragmentation; // TODO: Re-check damage of Infernal Fragments
    player_talent_t gloom_of_nathreza;

    player_talent_t ruination; // TODO: Check damage and buff values closer to release, affected_by lists may be on cast spell not damage in data and could be changed later by Blizzard
    const spell_data_t* ruination_buff;
    const spell_data_t* ruination_cast;
    const spell_data_t* ruination_impact;
    const spell_data_t* diabolic_imp;
    const spell_data_t* diabolic_bolt;
    player_talent_t diabolic_oculi;
    const spell_data_t* demonic_oculi_buff;
    const spell_data_t* eye_explosion;
    player_talent_t looks_that_kill;
    const spell_data_t* diabolic_gaze_dmg_1;
    const spell_data_t* diabolic_gaze_dmg_2;
    const spell_data_t* diabolic_gaze_dmg_3;
    player_talent_t minds_eyes;
    const spell_data_t* minds_eyes_buff;

    // Hellcaller
    player_talent_t wither;
    const spell_data_t* wither_direct; // TODO: Damage values are picking up some other weird effects similar to Flames of Xoroth. Check damage again after main implementation work is done
    const spell_data_t* wither_dot;

    player_talent_t xalans_ferocity;
    player_talent_t blackened_soul;
    const spell_data_t* blackened_soul_trigger; // Contains interval for stack collapse
    const spell_data_t* blackened_soul_dmg;
    player_talent_t xalans_cruelty;

    player_talent_t hatefury_rituals;
    player_talent_t bleakheart_tactics;
    player_talent_t illhoofs_design;

    player_talent_t mark_of_xavius;
    player_talent_t seeds_of_their_demise;
    player_talent_t mark_of_perotharn;

    player_talent_t through_the_felvine;
    player_talent_t devil_fruit;
    player_talent_t alzzins_iniquity;

    player_talent_t malevolence;
    const spell_data_t* malevolence_buff;
    const spell_data_t* malevolence_dmg;

    // Soul Harvester
    player_talent_t demonic_soul;
    const spell_data_t* succulent_soul; // Buff triggered by Demonic Soul proc
    const spell_data_t* demonic_soul_dmg;

    player_talent_t necrolyte_teachings;
    player_talent_t soul_anathema;
    const spell_data_t* soul_anathema_dot;
    player_talent_t demoniacs_fervor;

    player_talent_t shared_fate;
    const spell_data_t* shared_fate_dot;
    player_talent_t feast_of_souls;
    const spell_data_t* marked_soul;

    player_talent_t wicked_reaping;
    const spell_data_t* wicked_reaping_dmg;
    player_talent_t quietus;
    player_talent_t sataiels_volition;

    player_talent_t shadow_of_death;
    const spell_data_t* shadow_of_death_energize;

    player_talent_t manifested_avarice;
    const spell_data_t* manifested_avarice_spell;
    player_talent_t shared_vessel;
    player_talent_t eternal_hunger;
  } hero;

  struct proc_actions_t
  {
    action_t* doom_proc;
    action_t* rain_of_fire_tick;
    action_t* lake_of_fire_tick;
    action_t* blackened_soul;
    action_t* malevolence;
    action_t* demonic_soul;
    action_t* shared_fate;
    action_t* wicked_reaping;
    action_t* dimensional_rift;
    action_t* demonfire_infusion;
    action_t* eye_explosion;
    action_t* diabolic_gaze_1;
    action_t* diabolic_gaze_2;
    action_t* diabolic_gaze_3;
    action_t* diabolic_oculi;
    action_t* blighted_maw;
    action_t* echo_of_sargeras;
    action_t* echo_of_sargeras_cb;
    action_t* echo_of_sargeras_sb;
    action_t* echo_of_sargeras_rof;
    action_t* embers_of_nihilam;
    action_t* shadow_of_nathreza;
  } proc_actions;

  struct pet_summons_t
  {
    action_t* desperate_soul;
    action_t* wild_imp;
    action_t* wild_imp_2;
    action_t* dreadstalker_1;
    action_t* dreadstalker_2;
    action_t* vilefiend;
    action_t* lady_sacrolash;
    action_t* grand_warlock_alythess;
    action_t* antoran_inquisitor;
    action_t* antoran_jailer;
    action_t* infernal;
    action_t* roc;
    action_t* fragment;
    action_t* overfiend;
    action_t* shadowy_rift;
    action_t* unstable_rift;
    action_t* chaos_rift;
    action_t* overlord;
    action_t* mother;
    action_t* pit_lord;
    action_t* diabolic_imp;
    action_t* manifested_demonic_soul;
  } summons;

  struct proc_data_entries_t
  {
    proc_data_t shadow_bolt_energize;
    proc_data_t agony_energize;
    proc_data_t demonbolt_energize;
    proc_data_t incinerate_energize;
    proc_data_t marked_soul;
  } proc_data_entries;

  struct tier_sets_t
  {
    const spell_data_t* wl_affliction_12_0_class_set_2pc;
    const spell_data_t* wl_affliction_12_0_class_set_4pc;
    const spell_data_t* wl_demonology_12_0_class_set_2pc;
    const spell_data_t* wl_demonology_12_0_class_set_4pc;
    const spell_data_t* wl_destruction_12_0_class_set_2pc;
    const spell_data_t* wl_destruction_12_0_class_set_4pc;
  } tier;

  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> dark_harvest;
    propagate_const<cooldown_t*> soul_fire;
    propagate_const<cooldown_t*> summon_doomguard;
    propagate_const<cooldown_t*> felstorm_icd;
    propagate_const<cooldown_t*> echo_of_sargeras; // ICD for Embers of Nihilam rank 4 procs
    propagate_const<cooldown_t*> blackened_soul; // Internal cooldown on triggering stack increase to Wither
  } cooldowns;

  // Buffs
  struct buffs_t
  {
    // Shared Buffs
    propagate_const<buff_t*> grimoire_of_sacrifice; // Buff which grants damage proc
    propagate_const<buff_t*> soulburn;
    propagate_const<buff_t*> pet_movement; // One unified buff for some form of pet movement stat tracking

    // Affliction Buffs
    propagate_const<buff_t*> nightfall;
    propagate_const<buff_t*> darkglare_presence;
    propagate_const<buff_t*> shard_instability;
    propagate_const<buff_t*> cascading_calamity;
    propagate_const<buff_t*> seed_of_corruption_is_out_dnt;

    // Demonology Buffs
    propagate_const<buff_t*> demonic_core;
    propagate_const<buff_t*> power_siphon; // Hidden buff from Power Siphon that increases damage of successive Demonbolts
    propagate_const<buff_t*> inner_demons;
    propagate_const<buff_t*> tyrants_oblation;
    propagate_const<buff_t*> hellbent_commander;
    propagate_const<buff_t*> wild_imps; // Buff for tracking how many Wild Imps are currently out (does NOT include imps waiting to be spawned)
    propagate_const<buff_t*> dreadstalkers; // Buff for tracking number of Dreadstalkers currently out
    propagate_const<buff_t*> vilefiend; // Buff for tracking number of Vilefiends currently out
    propagate_const<buff_t*> grimoire_imp_lord;
    propagate_const<buff_t*> grimoire_fel_ravager;
    propagate_const<buff_t*> doomguard;
    propagate_const<buff_t*> tyrant; // Buff for tracking if Demonic Tyrant is currently out
    propagate_const<buff_t*> dominion_of_argus;

    // Destruction Buffs
    propagate_const<buff_t*> backdraft;
    propagate_const<buff_t*> reverse_entropy;
    propagate_const<buff_t*> fiendish_cruelty;
    propagate_const<buff_t*> chaotic_inferno;
    propagate_const<buff_t*> rain_of_chaos;
    propagate_const<buff_t*> conflagration_of_chaos;
    propagate_const<buff_t*> flashpoint;
    propagate_const<buff_t*> crashing_chaos;
    propagate_const<buff_t*> alythesss_ire;
    propagate_const<buff_t*> summon_overfiend;
    propagate_const<buff_t*> vision_of_nihilam;

    // Diabolist Buffs
    propagate_const<buff_t*> ritual_overlord;
    propagate_const<buff_t*> ritual_mother;
    propagate_const<buff_t*> ritual_pit_lord;
    propagate_const<buff_t*> art_overlord;
    propagate_const<buff_t*> art_mother;
    propagate_const<buff_t*> art_pit_lord;
    propagate_const<buff_t*> infernal_bolt;
    propagate_const<buff_t*> abyssal_dominion;
    propagate_const<buff_t*> ruination;
    propagate_const<buff_t*> demonic_oculi;
    propagate_const<buff_t*> minds_eyes;

    // Hellcaller Buffs
    propagate_const<buff_t*> malevolence;

    // Soul Harvester Buffs
    propagate_const<buff_t*> succulent_soul;
    propagate_const<buff_t*> manifested_demonic_soul;
  } buffs;

  // Gains - Many are automatically handled
  struct gains_t
  {
    // Class Talents

    // Affliction
    gain_t* agony;
    gain_t* drain_soul;
    gain_t* unstable_affliction_refund;

    // Demonology
    gain_t* soul_strike; // Only with Fel Invocation talent

    // Destruction
    gain_t* incinerate_crits;
    gain_t* immolate;
    gain_t* immolate_crits;
    gain_t* infernal;
    gain_t* shadowburn_refund;
    gain_t* summon_overfiend;
    gain_t* dominion_of_argus;

    // Diabolist

    // Hellcaller
    gain_t* wither;
    gain_t* wither_crits;

    // Soul Harvester
    gain_t* feast_of_souls;
    gain_t* shadow_of_death;
  } gains;

  // Procs
  struct procs_t
  {
    // Class Talents

    // Affliction
    proc_t* nightfall;
    proc_t* shadowbolt_volley;
    proc_t* ravenous_afflictions;
    proc_t* shard_instability;
    proc_t* fatal_echoes;
    proc_t* wrath_of_nathreza;

    // Demonology
    proc_t* demonic_core_dogs;
    proc_t* demonic_core_imps_fade;
    proc_t* demonic_core_imps_implosion;
    proc_t* carnivorous_stalkers;
    proc_t* infernal_rapidity;
    proc_t* spiteful_reconstitution;
    proc_t* demonic_knowledge;

    // Destruction
    proc_t* reverse_entropy;
    proc_t* rain_of_chaos;
    proc_t* mayhem;
    proc_t* fiendish_cruelty;
    proc_t* chaotic_inferno;
    proc_t* dimensional_rift;
    proc_t* avatar_of_destruction;
    proc_t* conflagration_of_chaos;
    proc_t* demonfire_infusion_inc;
    proc_t* demonfire_infusion_dot;
    proc_t* alythesss_ire;
    proc_t* echo_of_sargeras;
    proc_t* echo_of_sargeras_cb;
    proc_t* echo_of_sargeras_sb;
    proc_t* echo_of_sargeras_rof;

    // Diabolist

    // Hellcaller
    proc_t* blackened_soul;
    proc_t* bleakheart_tactics;
    proc_t* seeds_of_their_demise;
    proc_t* mark_of_perotharn;
    proc_t* devil_fruit;

    // Soul Harvester
    proc_t* succulent_soul;
    proc_t* feast_of_souls;
    proc_t* manifested_avarice;
  } procs;

  struct cycle_proc_t
  {
    fixed_cycle_proc_t* alythesss_ire;
  } cycle_proc;

  struct deck_rng_t
  {
    shuffled_rng_t* rain_of_chaos;
    shuffled_rng_t* demonic_knowledge;
    std::unique_ptr<shuffled_bag_rng_t<dimensional_rift_pet_e>> dimensional_rift_summon;
  } deck_rng;

  struct rppm_rng_t
  {
    real_ppm_t* ravenous_afflictions;
    real_ppm_t* wrath_of_nathreza;
    real_ppm_t* devil_fruit;
  } rppm_rng;

  struct progress_rng_t
  {
    threshold_rng_t* agony_energize;
    threshold_rng_t* nightfall;
    threshold_rng_t* seeds_of_their_demise;
  } progress_rng;

  struct prd_rng_t
  {
    accumulated_rng_t* cunning_cruelty;
    accumulated_rng_t* shard_instability_ds;
    accumulated_rng_t* shard_instability_sb;
    accumulated_rng_t* fatal_echoes;
    accumulated_rng_t* fiendish_cruelty;
    accumulated_rng_t* chaotic_inferno;
    accumulated_rng_t* dimensional_rift;
    accumulated_rng_t* echo_of_sargeras;
    accumulated_rng_t* succulent_soul;
    accumulated_rng_t* manifested_avarice;
    accumulated_rng_t* feast_of_souls;
    accumulated_rng_t* demoniac_imp_fade;
    accumulated_rng_t* spiteful_reconstitution;
    accumulated_rng_t* bleakheart_tactics;
    accumulated_rng_t* mark_of_perotharn;
    double infernal_rapidity_prd_c_value;
  } prd_rng;

  struct flat_rng_t
  {
    simple_proc_t* immolate_crit_energize; // TODO: Need to check the type of rng
    simple_proc_t* demoniac_imp_implosion;
    simple_proc_t* carnivorous_stalkers;
    simple_proc_t* demonfire_infusion_dot; // TODO: Need to check the type of rng
    simple_proc_t* demonfire_infusion_inc; // TODO: Need to check the type of rng
    simple_proc_t* alythesss_ire_shift;
    simple_proc_t* wither_crit_energize;   // TODO: Need to check the type of rng
    simple_proc_t* blackened_soul;
  } flat_rng;

  struct rng_settings_t
  {
    struct rng_setting_t
    {
      double setting_value;
      double default_value;
      std::string option_name;
      double min = std::numeric_limits<double>::lowest();
      double max = std::numeric_limits<double>::max();
    };

    // Affliction
    rng_setting_t agony_energize = { 0.370, 0.370, "agony_energize", 0.0 };
    rng_setting_t nightfall = { 0.130, 0.130, "nightfall", 0.0 };
    rng_setting_t cunning_cruelty_sb = { 0.50, 0.50, "cunning_cruelty_sb", 0.0 };
    rng_setting_t cunning_cruelty_ds = { 0.25, 0.25, "cunning_cruelty_ds", 0.0 };

    // Demonology
    rng_setting_t demoniac_imp_fade_hard_cap = { 21.0, 21.0, "demoniac_imp_fade_hard_cap", 0.0 };
    rng_setting_t spiteful_reconstitution = { 0.10, 0.10, "spiteful_reconstitution", 0.0 };
    rng_setting_t spiteful_reconstitution_hard_cap = { 21.0, 21.0, "spiteful_reconstitution_hard_cap", 0.0 };
    rng_setting_t demonic_knowledge_rank1_cards = { 6.0, 6.0, "demonic_knowledge_rank1_cards", 0.0 };
    rng_setting_t demonic_knowledge_rank2_cards = { 12.0, 12.0, "demonic_knowledge_rank2_cards", 0.0 };
    rng_setting_t demonic_knowledge_deck_size = { 80.0, 80.0, "demonic_knowledge_deck_size", 0.0 };

    // Destruction
    rng_setting_t rain_of_chaos_cards = { 3.0, 3.0, "rain_of_chaos_cards", 0.0 };
    rng_setting_t rain_of_chaos_deck_size = { 20.0, 20.0, "rain_of_chaos_deck_size", 0.0 };
    rng_setting_t alythesss_ire_shift = { 0.01, 0.01, "alythesss_ire_shift", 0.0 };
    rng_setting_t echo_of_sargeras = { 0.10, 0.10, "echo_of_sargeras", 0.0 };

    // Diabolist

    // Hellcaller
    rng_setting_t blackened_soul = { 0.23, 0.23, "blackened_soul", 0.0 };
    rng_setting_t bleakheart_tactics = { 0.15, 0.15, "bleakheart_tactics", 0.0 };
    rng_setting_t seeds_of_their_demise = { 0.240, 0.240, "seeds_of_their_demise", 0.0 };
    rng_setting_t mark_of_perotharn = { 0.15, 0.15, "mark_of_perotharn", 0.0 };

    // Soul Harvester
    rng_setting_t succulent_soul_aff = { 0.225, 0.225, "succulent_soul_aff", 0.0 };
    rng_setting_t succulent_soul_demo = { 0.15, 0.15, "succulent_soul_demo", 0.0 };
    rng_setting_t feast_of_souls_aff = { 0.12, 0.12, "feast_of_souls_aff", 0.0 };
    rng_setting_t feast_of_souls_aff_quietus = { 0.04, 0.04, "feast_of_souls_aff_quietus", 0.0 };
    rng_setting_t feast_of_souls_demo = { 0.10, 0.10, "feast_of_souls_demo", 0.0 };
    rng_setting_t feast_of_souls_hard_cap_aff = { 26.0, 26.0, "feast_of_souls_hard_cap_aff", 0.0 };
    rng_setting_t feast_of_souls_hard_cap_demo = { 26.0, 26.0, "feast_of_souls_hard_cap_demo", 0.0 };
    rng_setting_t manifested_avarice = { 0.10, 0.10, "manifested_avarice", 0.0 };

    template <typename F>
    void for_each( F&& f )
    {
      f( agony_energize );
      f( nightfall );
      f( cunning_cruelty_sb );
      f( cunning_cruelty_ds );
      f( demoniac_imp_fade_hard_cap );
      f( spiteful_reconstitution );
      f( spiteful_reconstitution_hard_cap );
      f( demonic_knowledge_rank1_cards );
      f( demonic_knowledge_rank2_cards );
      f( demonic_knowledge_deck_size );
      f( rain_of_chaos_cards );
      f( rain_of_chaos_deck_size );
      f( alythesss_ire_shift );
      f( echo_of_sargeras );
      f( blackened_soul );
      f( bleakheart_tactics );
      f( seeds_of_their_demise );
      f( mark_of_perotharn );
      f( succulent_soul_aff );
      f( succulent_soul_demo );
      f( feast_of_souls_aff );
      f( feast_of_souls_aff_quietus );
      f( feast_of_souls_demo );
      f( feast_of_souls_hard_cap_aff );
      f( feast_of_souls_hard_cap_demo );
      f( manifested_avarice );
    }
  } rng_settings;

  int initial_soul_shards;
  std::string default_pet;
  bool disable_auto_felstorm; // For Demonology main pet
  bool normalize_destruction_mastery;
  bool eye_explosion_instanced_bug_cb;
  bool eye_explosion_instanced_bug_sb;
  bool eye_explosion_instanced_bug_rof;
  double tyrant_antoran_armaments_target_mul;

  warlock_t( sim_t* sim, util::string_view name, race_e r );

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_rng() override;
  void init_action_list() override;
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  std::string aura_expr_from_spell_id( unsigned int spell_id, bool on_self = true ) const override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                            const assisted_combat_step_data_t& step ) const override;
  void init_resources( bool force ) override;
  void init_special_effects() override;
  void reset() override;
  void create_options() override;
  void parse_player_effects();
  const spell_data_t* conditional_spell_lookup( bool fn, int id );
  void add_rng_option( warlock_t::rng_settings_t::rng_setting_t& );
  int get_spawning_imp_count(); // TODO: Decide if still needed
  timespan_t time_to_imps( int count ); // TODO: Decide if still needed
  int active_demon_count( bool include_diabolist = true ) const;
  std::pair<timespan_t, timespan_t> dreadstalkers_delay_duration_adjustment_helper( const player_t& target ); // TODO: Move to helpers? or implement in call_dreadstalkers_t?
  void create_actions() override;
  void create_affliction_proc_actions();
  void create_demonology_proc_actions();
  void create_destruction_proc_actions();
  void create_diabolist_proc_actions();
  void create_hellcaller_proc_actions();
  void create_soul_harvester_proc_actions();
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = {} ) override;
  void create_pets() override;
  std::string create_profile( save_e ) override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_SPELL; }
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void init_blizzard_action_list() override;
  void combat_begin() override;
  void init_assessors() override;
  void init_finished() override;
  void invalidate_cache( cache_e c ) override;
  double composite_mastery() const override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::string default_potion() const override { return warlock_apl::potion( this ); }
  std::string default_flask() const override { return warlock_apl::flask( this ); }
  std::string default_food() const override { return warlock_apl::food( this ); }
  std::string default_rune() const override { return warlock_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return warlock_apl::temporary_enchant( this ); }
  std::vector<player_t*> get_smart_targets( const std::vector<player_t*>& tl, propagate_const<dot_t*> warlock_td_t::dots_t::* dot, int n_targets, player_t* exclude = nullptr, double range = 0.0, bool really_smart = false );
  player_t* get_smart_target( const std::vector<player_t*>& tl, propagate_const<dot_t*> warlock_td_t::dots_t::* dot, player_t* exclude = nullptr, double range = 0.0, bool really_smart = false );
  double resource_gain( resource_e resource_type, double amount, gain_t* source = nullptr, action_t* action = nullptr ) override;
  void feast_of_souls_gain( bool from_quietus_seed = false );
  void summon_dominion_of_argus_pet( dominion_of_argus_pet_e pet );

  bool affliction() const;
  bool demonology() const;
  bool destruction() const;
  bool diabolist() const;
  bool hellcaller() const;
  bool soul_harvester() const;

  template <set_bonus_type_e Tier>
  bool active_2pc() const
  {
    return sets->has_set_bonus( specialization(), Tier, B2 );
  }

  template <set_bonus_type_e Tier>
  bool active_4pc() const
  {
    return sets->has_set_bonus( specialization(), Tier, B4 );
  }

  target_specific_t<warlock_td_t> target_data;

  const warlock_td_t* find_target_data( const player_t* target ) const override
  { return target_data[ target ]; }

  warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }

  template <typename T>
  bool dot_or_debuff_active( T d, warlock_td_t* t )
  {
    if constexpr ( std::is_invocable_v<T, warlock_td_t::debuffs_t> )
    {
      return std::invoke( d, t->debuffs )->check() > 0;
    }
    else if constexpr ( std::is_invocable_v<T, warlock_td_t::dots_t> )
    {
      return std::invoke( d, t->dots )->is_ticking();
    }
    else
    {
      sim->error( SEVERE, "%s dot_or_debuff_active: Unsupported type passed.\n", name() );
      return false;
    }
  }

  action_t* create_action_warlock( util::string_view, util::string_view );

  action_t* create_action_affliction( util::string_view, util::string_view );
  void create_buffs_affliction();
  void init_spells_affliction();
  void init_gains_affliction();
  void init_rng_affliction();
  void init_procs_affliction();

  action_t* create_action_demonology( util::string_view, util::string_view );
  void create_buffs_demonology();
  void init_spells_demonology();
  void init_gains_demonology();
  void init_rng_demonology();
  void init_procs_demonology();

  action_t* create_action_destruction( util::string_view, util::string_view );
  void create_buffs_destruction();
  void init_spells_destruction();
  void init_gains_destruction();
  void init_rng_destruction();
  void init_procs_destruction();

  action_t* create_action_diabolist( util::string_view, util::string_view );
  void create_buffs_diabolist();
  void init_spells_diabolist();
  void init_gains_diabolist();
  void init_rng_diabolist();
  void init_procs_diabolist();

  action_t* create_action_hellcaller( util::string_view, util::string_view );
  void create_buffs_hellcaller();
  void init_spells_hellcaller();
  void init_gains_hellcaller();
  void init_rng_hellcaller();
  void init_procs_hellcaller();

  action_t* create_action_soul_harvester( util::string_view, util::string_view );
  void create_buffs_soul_harvester();
  void init_spells_soul_harvester();
  void init_gains_soul_harvester();
  void init_rng_soul_harvester();
  void init_procs_soul_harvester();

  void init_proc_data_entries();
  pet_t* create_main_pet( util::string_view pet_name, util::string_view pet_type );
  std::unique_ptr<expr_t> create_pet_expression( util::string_view name_str );
};

namespace helpers
{
  struct imp_delay_event_t : public player_event_t
  {
    imp_delay_event_t( warlock_t*, double, double, int );
    timespan_t diff;
    int index;
    virtual const char* name() const override;
    virtual void execute() override;
    timespan_t expected_time();
  };

  struct ua_stack_drop_event_t : public player_event_t
  {
    ua_stack_drop_event_t( warlock_t*, dot_t*, timespan_t );
    dot_t* dot;
    virtual const char* name() const override;
    virtual void execute() override;
  };

  void trigger_blackened_soul( warlock_t* p, bool malevolence );

  void trigger_echo_of_sargeras( warlock_t* p, player_t* target, action_t* echo_action, proc_t* proc );

  void trigger_wrath_of_nathreza( warlock_t* p, player_t* target );
}
}  // namespace warlock
