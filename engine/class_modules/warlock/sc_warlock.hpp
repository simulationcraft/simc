#pragma once
#include "simulationcraft.hpp"

#include "player/pet_spawner.hpp"
#include "sc_warlock_pets.hpp"

namespace warlock
{
struct warlock_t;

//Used for version checking in code (e.g. PTR vs Live)
enum version_check_e
{
  VERSION_PTR,
  VERSION_9_1_0,
  VERSION_9_0_5,
  VERSION_9_0_0,
  VERSION_ANY
};

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
  // Cross-spec
  propagate_const<dot_t*> dots_drain_life;
  propagate_const<dot_t*> dots_drain_life_aoe; // SL - Soul Rot covenant effect
  propagate_const<dot_t*> dots_scouring_tithe;
  propagate_const<dot_t*> dots_impending_catastrophe;
  propagate_const<dot_t*> dots_soul_rot;

  // Aff
  propagate_const<dot_t*> dots_agony;
  propagate_const<dot_t*> dots_corruption; //TODO: Add offspec corruption
  propagate_const<dot_t*> dots_seed_of_corruption;
  propagate_const<dot_t*> dots_drain_soul;
  propagate_const<dot_t*> dots_siphon_life;
  propagate_const<dot_t*> dots_phantom_singularity;
  propagate_const<dot_t*> dots_unstable_affliction;
  propagate_const<dot_t*> dots_vile_taint;

  propagate_const<buff_t*> debuffs_haunt;
  propagate_const<buff_t*> debuffs_shadow_embrace; //9.1 PTR - Same behavior as 9.0 but enabled by talent

  // Destro
  propagate_const<dot_t*> dots_immolate;

  propagate_const<buff_t*> debuffs_shadowburn;
  propagate_const<buff_t*> debuffs_eradication;
  propagate_const<buff_t*> debuffs_roaring_blaze;
  propagate_const<buff_t*> debuffs_havoc;

  // SL - Legendary
  propagate_const<buff_t*> debuffs_odr;

  // SL - Conduit
  propagate_const<buff_t*> debuffs_combusting_engine;

  // Demo
  propagate_const<dot_t*> dots_doom;

  propagate_const<buff_t*> debuffs_from_the_shadows;

  double soc_threshold; //Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  {
    soc_threshold = 0;
  }

  void target_demise();

  int count_affliction_dots();
};

struct warlock_t : public player_t
{
public:
  player_t* havoc_target;
  player_t* ua_target; //Used for handling Unstable Affliction target swaps
  std::vector<action_t*> havoc_spells;  // Used for smarter target cache invalidation.
  double agony_accumulator;
  double corruption_accumulator;
  std::vector<event_t*> wild_imp_spawns;      // Used for tracking incoming imps from HoG

  unsigned active_pets;

  // Main pet held in active/last, guardians should be handled by pet spawners. TODO: Use spawner for Infernal/Darkglare?
  struct pets_t
  {
    warlock_pet_t* active;
    warlock_pet_t* last;
    static const int INFERNAL_LIMIT  = 1;
    static const int DARKGLARE_LIMIT = 1;

    //TODO: Refactor infernal code including new talent Rain of Chaos
    std::array<pets::destruction::infernal_t*, INFERNAL_LIMIT> infernals;
    spawner::pet_spawner_t<pets::destruction::infernal_t, warlock_t>
        roc_infernals;  // Infernal(s) summoned by Rain of Chaos

    std::array<pets::affliction::darkglare_t*, DARKGLARE_LIMIT> darkglare;

    spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers;
    spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
    spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;
    spawner::pet_spawner_t<pets::demonology::grimoire_felguard_pet_t, warlock_t> grimoire_felguards;

    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;

    //Nether Portal demons - Check regularly for accuracy!
    spawner::pet_spawner_t<pets::demonology::random_demons::shivarra_t, warlock_t> shivarra;
    spawner::pet_spawner_t<pets::demonology::random_demons::darkhound_t, warlock_t> darkhounds;
    spawner::pet_spawner_t<pets::demonology::random_demons::bilescourge_t, warlock_t> bilescourges;
    spawner::pet_spawner_t<pets::demonology::random_demons::urzul_t, warlock_t> urzuls;
    spawner::pet_spawner_t<pets::demonology::random_demons::void_terror_t, warlock_t> void_terrors;
    spawner::pet_spawner_t<pets::demonology::random_demons::wrathguard_t, warlock_t> wrathguards;
    spawner::pet_spawner_t<pets::demonology::random_demons::vicious_hellhound_t, warlock_t> vicious_hellhounds;
    spawner::pet_spawner_t<pets::demonology::random_demons::illidari_satyr_t, warlock_t> illidari_satyrs;
    spawner::pet_spawner_t<pets::demonology::random_demons::eyes_of_guldan_t, warlock_t> eyes_of_guldan;
    spawner::pet_spawner_t<pets::demonology::random_demons::prince_malchezaar_t, warlock_t> prince_malchezaar;

    pets_t( warlock_t* w );
  } warlock_pet_list;

  std::vector<std::string> pet_name_list;

  //TODO: Refactor or remove this whole damn section
  struct active_t
  {
    spell_t* rain_of_fire; //TODO: SL Beta - This is the definition for the ground aoe event, should we move this?
    spell_t* bilescourge_bombers; //TODO: SL Beta - This is the definition for the ground aoe event, should we move this?
    spell_t* summon_random_demon; //TODO: SL Beta - This is the definition for a helper action for Nether Portal, should we move this?
    melee_attack_t* soul_strike; //TODO: SL Beta - This is currently unused, Felguard pets are using manual spellID. Fix or remove.
  } active;

  // Talents
  struct talents_t
  {
    // shared
    // tier 30 - Not Implemented
    const spell_data_t* demon_skin;
    const spell_data_t* burning_rush;
    const spell_data_t* dark_pact;

    // tier 40 - Not Implemented
    const spell_data_t* darkfury;
    const spell_data_t* mortal_coil;
    const spell_data_t* howl_of_terror;

    // tier 45 (Aff/Destro)
    const spell_data_t* grimoire_of_sacrifice;

    // tier 50
    const spell_data_t* soul_conduit; //(t45 for demo)

    // AFF
    // tier 15
    const spell_data_t* nightfall; //TODO: RNG information is missing from spell data, and data also says buff can potentially stack to 2. Serious testing needed, especially with multiple corruptions out!
    const spell_data_t* inevitable_demise;
    const spell_data_t* drain_soul;

    // tier 25
    const spell_data_t* writhe_in_agony;
    const spell_data_t* absolute_corruption;
    const spell_data_t* siphon_life;

    // tier 35
    const spell_data_t* sow_the_seeds;
    const spell_data_t* phantom_singularity; //TODO: Dot duration uses hardcoded tick count and should be pulled from spell data instead
    const spell_data_t* vile_taint;

    // tier 45
    const spell_data_t* shadow_embrace; //9.1 PTR - Replaces Dark Caller
    const spell_data_t* dark_caller; //9.1 PTR - Removed as talent
    const spell_data_t* haunt;
    // grimoire of sacrifice

    // tier 50
    // soul conduit
    const spell_data_t* creeping_death;
    const spell_data_t* dark_soul_misery;

    // DEMO
    // tier 15
    const spell_data_t* dreadlash;
    const spell_data_t* bilescourge_bombers;
    const spell_data_t* demonic_strength; //TODO: Investigate pet buff usage

    // tier 25
    const spell_data_t* demonic_calling;
    const spell_data_t* power_siphon;
    const spell_data_t* doom; //TODO: Haste/refresh behavior needs checking still

    // tier 35
    const spell_data_t* from_the_shadows;
    const spell_data_t* soul_strike;
    const spell_data_t* summon_vilefiend;

    // tier 45
    // soul conduit
    const spell_data_t* inner_demons;
    const spell_data_t* grimoire_felguard; //TODO: Check summoning/buff durations and if permanent Felguard is affected by this ability

    // tier 50
    const spell_data_t* sacrificed_souls; //TODO: Check what pets/guardians count for this talent. Also relevant for demonic consumption
    const spell_data_t* demonic_consumption;
    const spell_data_t* nether_portal;

    // DESTRO
    // tier 15
    const spell_data_t* flashover;
    const spell_data_t* eradication;
    const spell_data_t* soul_fire; //TODO: Confirm refresh/pandemic behavior matchup for immolates

    // tier 25
    const spell_data_t* reverse_entropy; //Note: talent spell (not the buff spell) contains RPPM data
    const spell_data_t* internal_combustion;
    const spell_data_t* shadowburn;

    // tier 35
    const spell_data_t* inferno; //TODO: Confirm interaction between Inferno and Rank 2 Rain of Fire, as well as if soul shard generation is per-target hit
    const spell_data_t* fire_and_brimstone;
    const spell_data_t* cataclysm;

    // tier 45
    const spell_data_t* roaring_blaze; //TODO: Confirm interaction between roaring blaze and eradication
    const spell_data_t* rain_of_chaos;
    // grimoire of sacrifice

    // tier 50
    // soul conduit
    const spell_data_t* channel_demonfire; //TODO: Confirm aoe behavior on each tick is correct in sims
    const spell_data_t* dark_soul_instability;
  } talents;

  struct legendary_t
  {
    // Legendaries
    // Cross-spec
    item_runeforge_t claw_of_endereth;
    item_runeforge_t relic_of_demonic_synergy; //TODO: Do pet and warlock procs share a single RPPM?
    item_runeforge_t wilfreds_sigil_of_superior_summoning;
    // Affliction
    item_runeforge_t malefic_wrath;
    item_runeforge_t perpetual_agony_of_azjaqir;
    item_runeforge_t sacrolashs_dark_strike; //TODO: Check if slow effect (unimplemented atm) can proc anything important
    item_runeforge_t wrath_of_consumption;
    // Demonology
    item_runeforge_t balespiders_burning_core;
    item_runeforge_t forces_of_the_horned_nightmare;
    item_runeforge_t grim_inquisitors_dread_calling;
    item_runeforge_t implosive_potential;
    // Destruction
    item_runeforge_t cinders_of_the_azjaqir;
    item_runeforge_t embers_of_the_diabolic_raiment;
    item_runeforge_t madness_of_the_azjaqir;
    item_runeforge_t odr_shawl_of_the_ymirjar;
    // Covenant
    item_runeforge_t languishing_soul_detritus;
    item_runeforge_t shard_of_annihilation;
    item_runeforge_t decaying_soul_satchel;
    item_runeforge_t contained_perpetual_explosion;
  } legendary;

  struct conduit_t
  {
    // Conduits
    // Covenant Abilities
    conduit_data_t catastrophic_origin;   // Venthyr
    conduit_data_t soul_eater;            // Night Fae
    conduit_data_t fatal_decimation;      // Necrolord
    conduit_data_t soul_tithe;            // Kyrian
    // Affliction
    conduit_data_t cold_embrace; //9.1 PTR - Removed
    conduit_data_t corrupting_leer;
    conduit_data_t focused_malignancy;
    conduit_data_t rolling_agony;
    conduit_data_t withering_bolt; //9.1 PTR - New, replaces Cold Embrace
    // Demonology
    conduit_data_t borne_of_blood;
    conduit_data_t carnivorous_stalkers;
    conduit_data_t fel_commando;
    conduit_data_t tyrants_soul;
    // Destruction
    conduit_data_t ashen_remains;
    conduit_data_t combusting_engine;
    conduit_data_t duplicitous_havoc;
    conduit_data_t infernal_brand;
  } conduit;

  struct covenant_t
  {
    // Covenant Abilities
    const spell_data_t* decimating_bolt;        // Necrolord
    const spell_data_t* impending_catastrophe;  // Venthyr
    const spell_data_t* scouring_tithe;         // Kyrian
    const spell_data_t* soul_rot;               // Night Fae
  } covenant;

  // Mastery Spells
  struct mastery_spells_t
  {
    const spell_data_t* potent_afflictions;
    const spell_data_t* master_demonologist;
    const spell_data_t* chaotic_energies;
  } mastery_spells;

  //TODO: Are there any other cooldown reducing/resetting mechanisms not currently in this struct?
  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> phantom_singularity;
    propagate_const<cooldown_t*> darkglare;
    propagate_const<cooldown_t*> demonic_tyrant;
    propagate_const<cooldown_t*> scouring_tithe;
    propagate_const<cooldown_t*> infernal;
    propagate_const<cooldown_t*> shadowburn;
  } cooldowns;

  //TODO: this struct is supposedly for passives per the comment here, but that is potentially outdated. Consider refactoring and reorganizing ALL of this.
  //TODO: Does find spec spell have rank support? If so, USE IT in assigning data to these
  // Passives
  struct specs_t
  {
    // All Specs
    const spell_data_t* nethermancy; //The probably actual spell controlling armor type bonus. NOTE: Level req is missing, this matches in game behavior.
    const spell_data_t* demonic_embrace; //Warlock stamina passive
    //TODO: Corruption is now class-wide
    //TODO: Ritual of Doom?

    // Affliction only
    const spell_data_t* affliction; //Spec aura
    const spell_data_t* agony; //This is the primary active ability
    const spell_data_t* agony_2; //Rank 2 passive (increased stacks)
    const spell_data_t* corruption_2; //Rank 2 passive (instant cast)
    const spell_data_t* corruption_3; //Rank 3 passive (damage on cast component)
    const spell_data_t* summon_darkglare; //This is the active summon ability
    const spell_data_t* unstable_affliction;  //This is the primary active ability
    const spell_data_t* unstable_affliction_2; //Rank 2 passive (soul shard on death)
    const spell_data_t* unstable_affliction_3; //Rank 3 passive (increased duration)
    const spell_data_t* summon_darkglare_2; //9.1 PTR - Now a passive learned at level 58

    // Demonology only
    const spell_data_t* demonology; //Spec aura
    const spell_data_t* call_dreadstalkers_2; //Rank 2 passive (reduced cast time, increased pet move speed)
    const spell_data_t* demonic_core; //Spec passive for the ability. See also: buffs.demonic_core
    const spell_data_t* fel_firebolt_2; //Rank 2 passive (reduced energy)
    const spell_data_t* summon_demonic_tyrant_2; //Rank 2 passive (instant soul shard generation)
    //TODO: Should Implosion be in this list? Currently in spells.implosion_aoe

    // Destruction only
    const spell_data_t* destruction; //Spec aura
    const spell_data_t* conflagrate; //TODO: This is the primary active ability, but is not currently being used. Fix this.
    const spell_data_t* conflagrate_2; //Rank 2 passive (increased charges)
    const spell_data_t* havoc; //This is the primary active ability
    //TODO: debuffs_havoc is currently referencing the spellID directly for rank 2, fix or remove.
    const spell_data_t* havoc_2; //Rank 2 passive (increased duration)
    const spell_data_t* immolate; //TODO: this is supposed to be the primary active ability but is not being used at the moment - fix this
    const spell_data_t* rain_of_fire_2; //Rank 2 passive (increased damage)
    const spell_data_t* summon_infernal_2; //Rank 2 passive (impact damage)
  } spec;

  // Buffs
  struct buffs_t
  {
    propagate_const<buff_t*> demonic_power; //Buff from Summon Demonic Tyrant (increased demon damage + duration)
    propagate_const<buff_t*> grimoire_of_sacrifice; //Buff which grants damage proc

    // Affliction Buffs
    propagate_const<buff_t*> drain_life; //Dummy buff used internally for handling Inevitable Demise cases
    propagate_const<buff_t*> nightfall;
    propagate_const<buff_t*> inevitable_demise;
    propagate_const<buff_t*> dark_soul_misery;

    // Demonology Buffs
    propagate_const<buff_t*> demonic_core;
    propagate_const<buff_t*> power_siphon; //Hidden buff from Power Siphon that increases damage of successive Demonbolts
    propagate_const<buff_t*> demonic_calling;
    propagate_const<buff_t*> inner_demons;
    propagate_const<buff_t*> nether_portal;
    propagate_const<buff_t*> wild_imps; //Buff for tracking how many Wild Imps are currently out (does NOT include imps waiting to be spawned)
    propagate_const<buff_t*> dreadstalkers; //Buff for tracking number of Dreadstalkers currently out
    propagate_const<buff_t*> vilefiend; //Buff for tracking if Vilefiend is currently out
    propagate_const<buff_t*> tyrant; //Buff for tracking if Demonic Tyrant is currently out
    propagate_const<buff_t*> portal_summons; //TODO: Fix tracking with this or remove
    propagate_const<buff_t*> grimoire_felguard; //Buff for tracking if GFG pet is currently out
    propagate_const<buff_t*> prince_malchezaar; //Buff for tracking Malchezaar (who is currently disabled in sims)
    propagate_const<buff_t*> eyes_of_guldan; //Buff for tracking if rare random summon is currently out

    // Destruction Buffs
    propagate_const<buff_t*> backdraft; //Buff associated with Conflagrate
    propagate_const<buff_t*> reverse_entropy;
    propagate_const<buff_t*> rain_of_chaos;
    propagate_const<buff_t*> dark_soul_instability;

    // Covenants
    propagate_const<buff_t*> decimating_bolt;
    propagate_const<buff_t*> tyrants_soul;
    propagate_const<buff_t*> soul_tithe;
    propagate_const<buff_t*> soul_rot; // Buff for determining if Drain Life is zero cost and aoe.

    // Legendaries
    propagate_const<buff_t*> madness_of_the_azjaqir;
    propagate_const<buff_t*> balespiders_burning_core;
    propagate_const<buff_t*> malefic_wrath;
    propagate_const<buff_t*> wrath_of_consumption;
    propagate_const<buff_t*> implosive_potential;
    propagate_const<buff_t*> implosive_potential_small;
    propagate_const<buff_t*> dread_calling;
    propagate_const<buff_t*> demonic_synergy;
    propagate_const<buff_t*> languishing_soul_detritus;
    propagate_const<buff_t*> shard_of_annihilation;
    propagate_const<buff_t*> decaying_soul_satchel_haste; //These are one unified buff in-game but splitting them in simc to make it easier to apply stat pcts
    propagate_const<buff_t*> decaying_soul_satchel_crit;
  } buffs;

  //TODO: Determine if any gains are not currently being tracked
  // Gains - Many of these are automatically handled for resource gains if get_gain( name ) is given the same name as the action source
  struct gains_t
  {
    gain_t* soul_conduit;

    gain_t* agony;
    gain_t* drain_soul;
    gain_t* unstable_affliction_refund;

    gain_t* conflagrate;
    gain_t* incinerate;
    gain_t* incinerate_crits;
    gain_t* incinerate_fnb;
    gain_t* incinerate_fnb_crits;
    gain_t* immolate;
    gain_t* immolate_crits;
    gain_t* soul_fire;
    gain_t* infernal;
    gain_t* shadowburn_refund;
    gain_t* inferno;

    gain_t* miss_refund;

    gain_t* shadow_bolt;
    gain_t* doom;
    gain_t* summon_demonic_tyrant;

    // SL
    gain_t* scouring_tithe;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* soul_conduit;

    // aff
    proc_t* nightfall;
    proc_t* corrupting_leer;
    proc_t* malefic_wrath;
    std::vector<proc_t*> malefic_rapture;

    // demo
    proc_t* demonic_calling;
    proc_t* one_shard_hog;
    proc_t* two_shard_hog;
    proc_t* three_shard_hog;
    proc_t* summon_random_demon;
    proc_t* portal_summon;
    proc_t* carnivorous_stalkers; // SL - Conduit
    proc_t* horned_nightmare; // SL - Legendary

    // destro
    proc_t* reverse_entropy;
    proc_t* rain_of_chaos;
  } procs;

  int initial_soul_shards;
  std::string default_pet;
  shuffled_rng_t* rain_of_chaos_rng;
  const spell_data_t* version_9_1_0_data;

  warlock_t( sim_t* sim, util::string_view name, race_e r );

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_rng() override;
  void init_action_list() override;
  void init_resources( bool force ) override;
  void init_special_effects() override;
  void reset() override;
  void create_options() override;
  int get_spawning_imp_count();
  timespan_t time_to_imps( int count );
  int imps_spawned_during( timespan_t period );
  void darkglare_extension_helper( warlock_t* p, timespan_t darkglare_extension );
  void malignancy_reduction_helper();
  bool min_version_check( version_check_e version ) const;
  action_t* create_action( util::string_view name, const std::string& options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = "" ) override;
  void create_pets() override;
  std::string create_profile( save_e ) override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override
  {
    return ROLE_SPELL;
  }
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  double composite_rating_multiplier( rating_e rating ) const override;
  void invalidate_cache( cache_e ) override;
  double composite_spell_crit_chance() const override;
  double composite_spell_haste() const override;
  double composite_melee_haste() const override;
  double composite_melee_crit_chance() const override;
  double composite_mastery() const override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  void combat_begin() override;
  void init_assessors() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;
  void apply_affecting_auras( action_t& action ) override;

  target_specific_t<warlock_td_t> target_data;

  const warlock_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }

  // sc_warlock_affliction
  action_t* create_action_affliction( util::string_view action_name, const std::string& options_str );
  void create_buffs_affliction();
  void init_spells_affliction();
  void init_gains_affliction();
  void init_rng_affliction();
  void init_procs_affliction();
  void create_options_affliction();
  void create_apl_affliction();

  // sc_warlock_demonology
  action_t* create_action_demonology( util::string_view action_name, const std::string& options_str );
  void create_buffs_demonology();
  void init_spells_demonology();
  void init_gains_demonology();
  void init_rng_demonology();
  void init_procs_demonology();
  void create_options_demonology();
  void create_apl_demonology();

  // sc_warlock_destruction
  action_t* create_action_destruction( util::string_view action_name, const std::string& options_str );
  void create_buffs_destruction();
  void init_spells_destruction();
  void init_gains_destruction();
  void init_rng_destruction();
  void init_procs_destruction();
  void create_options_destruction();
  void create_apl_destruction();

  // sc_warlock_pets
  pet_t* create_main_pet( util::string_view pet_name, util::string_view pet_type );
  pet_t* create_demo_pet( util::string_view pet_name, util::string_view pet_type );
  void create_all_pets();
  std::unique_ptr<expr_t> create_pet_expression( util::string_view name_str );

private:
  void apl_precombat();
  void apl_default();
  void apl_global_filler();
};

namespace actions
{
//Event for triggering delayed refunds from Soul Conduit
//Delay prevents instant reaction time issues for rng refunds
struct sc_event_t : public player_event_t
{
  gain_t* shard_gain;
  warlock_t* pl;
  int shards_used;

  sc_event_t( warlock_t* p, int c )
    : player_event_t( *p, 100_ms ),
    shard_gain( p->gains.soul_conduit ),
    pl( p ),
    shards_used( c )
  {
  }

  virtual const char* name() const override
  {
    return "soul_conduit_event";
  }

  virtual void execute() override
  {
    double soul_conduit_rng = pl->talents.soul_conduit->effectN( 1 ).percent();

    for ( int i = 0; i < shards_used; i++ )
    {
      if ( rng().roll( soul_conduit_rng ) )
      {
        pl->sim->print_log( "Soul Conduit proc occurred for Warlock {}, refunding 1.0 soul shards.", pl->name() );
        pl->resource_gain( RESOURCE_SOUL_SHARD, 1.0, shard_gain );
        pl->procs.soul_conduit->occur();
      }
    }
  }
};

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ) : heal_t( n, p, p->find_spell( id ) )
  {
    target = p;
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }
};

struct warlock_spell_t : public spell_t
{
public:
  gain_t* gain;
  bool can_havoc; //Needed in main module for cross-spec spells such as Covenants
  bool affected_by_woc; // SL - Legendary (Wrath of Consumption) checker
  bool affected_by_soul_tithe; // SL - Covenant (Kyrian) checker
  //TODO: Refactor affected_by stuff to be more streamlined

  warlock_spell_t( warlock_t* p, util::string_view n ) : warlock_spell_t( n, p, p->find_class_spell( n ) )
  {
  }

  warlock_spell_t( warlock_t* p, util::string_view n, specialization_e s )
    : warlock_spell_t( n, p, p->find_class_spell( n, s ) )
  {
  }

  warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : spell_t( token, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;
    weapon_multiplier = 0.0;
    gain              = player->get_gain( name_str );
    can_havoc         = false;

    //TOCHECK: Is there a way to link this to the buffs.x spell data so we don't have to remember this is hardcoded?
    affected_by_woc   = data().affected_by( p->find_spell( 337130 )->effectN( 1 ) );

    affected_by_soul_tithe = data().affected_by( p->find_spell( 340238 )->effectN( 1 ) );
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }

  warlock_td_t* td( player_t* t )
  {
    return p()->get_target_data( t );
  }

  const warlock_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  void reset() override
  {
    spell_t::reset();
  }

  double cost() const override
  {
    double c = spell_t::cost();
    return c;
  }

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
    {
      // lets try making all lock specs not react instantly to shard gen
      if ( p()->talents.soul_conduit->ok() )
      {
        make_event<sc_event_t>( *p()->sim, p(), as<int>( last_resource_cost ) );
      }

      if ( p()->legendary.wilfreds_sigil_of_superior_summoning->ok() )
      {
        switch ( p()->specialization() )
        {
          case WARLOCK_AFFLICTION:
            p()->cooldowns.darkglare->adjust( -last_resource_cost * p()->legendary.wilfreds_sigil_of_superior_summoning->effectN( 1 ).time_value(), false );
            break;
          case WARLOCK_DEMONOLOGY:
            p()->cooldowns.demonic_tyrant->adjust( -last_resource_cost * p()->legendary.wilfreds_sigil_of_superior_summoning->effectN( 2 ).time_value(), false );
            break;
          case WARLOCK_DESTRUCTION:
            p()->cooldowns.infernal->adjust( -last_resource_cost * p()->legendary.wilfreds_sigil_of_superior_summoning->effectN( 3 ).time_value(), false );
            break;
          default:
            break;
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = spell_t::composite_target_multiplier( t );
    return m;
  }

  double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();

    if ( p()->buffs.soul_tithe->check() && affected_by_soul_tithe )
      pm *= 1.0 + p()->buffs.soul_tithe->check_stack_value();

    pm *= 1.0 + p()->buffs.demonic_synergy->check_stack_value();

    return pm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = spell_t::composite_ta_multiplier( s );

    if ( p()->legendary.wrath_of_consumption.ok() && p()->buffs.wrath_of_consumption->check() && affected_by_woc )
      m *= 1.0 + p()->buffs.wrath_of_consumption->check_stack_value();

    return m;
  }

  void extend_dot( dot_t* dot, timespan_t extend_duration )
  {
    if ( dot->is_ticking() )
    {
      dot->adjust_duration( extend_duration, dot->current_action->dot_duration * 1.5 );
    }
  }

  //Destruction specific things for Havoc that unfortunately need to be in main module

  bool use_havoc() const
  {
    // Ensure we do not try to hit the same target twice.
    return can_havoc && p()->havoc_target && p()->havoc_target != target;
  }

  int n_targets() const override
  {
    if ( p()->specialization() == WARLOCK_DESTRUCTION && use_havoc() )
    {
      assert(spell_t::n_targets() == 0);
      return 2;
    }
    else
      return spell_t::n_targets();
  }

  size_t available_targets(std::vector<player_t*>& tl) const override
  {
    spell_t::available_targets(tl);

    // Check target list size to prevent some silly scenarios where Havoc target
    // is the only target in the list.
    if ( p()->specialization() == WARLOCK_DESTRUCTION && tl.size() > 1 && use_havoc())
    {
      // We need to make sure that the Havoc target ends up second in the target list,
      // so that Havoc spells can pick it up correctly.
      auto it = range::find(tl, p()->havoc_target);
      if (it != tl.end())
      {
        tl.erase(it);
        tl.insert(tl.begin() + 1, p()->havoc_target);
      }
    }

    return tl.size();
  }

  void init() override
  {
    spell_t::init();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && can_havoc )
    {
        // SL - Conduit
        base_aoe_multiplier *= p()->spec.havoc->effectN(1).percent() + p()->conduit.duplicitous_havoc.percent();
        p()->havoc_spells.push_back(this);
    }
  }

  //End Destruction specific things

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    return spell_t::create_expression( name_str );
  }
};

struct grimoire_of_sacrifice_damage_t : public warlock_spell_t
{
  grimoire_of_sacrifice_damage_t(warlock_t* p)
    : warlock_spell_t("grimoire_of_sacrifice_damage_proc", p, p->find_spell(196100))
  {
    background = true;
    proc = true;
  }
};

struct demonic_synergy_callback_t : public dbc_proc_callback_t
{
  warlock_t* owner;

  demonic_synergy_callback_t( warlock_t* p, special_effect_t& e )
    : dbc_proc_callback_t( p, e ), owner( p )
  {
  }

  void execute( action_t* /* a */, action_state_t* ) override
  {
    if ( owner->warlock_pet_list.active )
    {
      auto pet = owner->warlock_pet_list.active;
      //Always set the pet's buff value using the owner's to ensure specialization value is correct
      pet->buffs.demonic_synergy->trigger( 1, owner->buffs.demonic_synergy->default_value );
    }
  }
};

using residual_action_t = residual_action::residual_periodic_action_t<warlock_spell_t>;

struct summon_pet_t : public warlock_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  warlock_pet_t* pet;

private:
  void _init_summon_pet_t()
  {
    util::tokenize( pet_name );
    harmful = false;

    if ( data().ok() &&
         std::find( p()->pet_name_list.begin(), p()->pet_name_list.end(), pet_name ) == p()->pet_name_list.end() )
    {
      p()->pet_name_list.push_back( pet_name );
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname = "" )
    : warlock_spell_t( p, sname.empty() ? "Summon " + n : sname ),
      summoning_duration( timespan_t::zero() ),
      pet_name( sname.empty() ? n : sname ),
      pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id )
    : warlock_spell_t( n, p, p->find_spell( id ) ),
      summoning_duration( timespan_t::zero() ),
      pet_name( n ),
      pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd )
    : warlock_spell_t( n, p, sd ), summoning_duration( timespan_t::zero() ), pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  void init_finished() override
  {
    pet = debug_cast<warlock_pet_t*>( player->find_pet( pet_name ) );

    warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    pet->summon( summoning_duration );

    warlock_spell_t::execute();
  }

  bool ready() override
  {
    if ( !pet )
    {
      return false;
    }
    return warlock_spell_t::ready();
  }
};

struct summon_main_pet_t : public summon_pet_t
{
  cooldown_t* instant_cooldown;

  summon_main_pet_t( const std::string& n, warlock_t* p )
    : summon_pet_t( n, p ), instant_cooldown( p->get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown->duration = timespan_t::from_seconds( 60 );
    ignore_false_positive      = true;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p()->warlock_pet_list.active )
    {
      p()->warlock_pet_list.active->dismiss();
      p()->warlock_pet_list.active = nullptr;
    }
  }

  virtual bool ready() override
  {
    if ( p()->warlock_pet_list.active == pet )
      return false;

    return summon_pet_t::ready();
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p()->warlock_pet_list.active = p()->warlock_pet_list.last = pet;

    if ( p()->buffs.grimoire_of_sacrifice->check() )
      p()->buffs.grimoire_of_sacrifice->expire();
  }
};

// Event for spawning wild imps for Demonology
// Placed in warlock.cpp for expression purposes
struct imp_delay_event_t : public player_event_t
{
  timespan_t diff;

  imp_delay_event_t( warlock_t* p, double delay, double exp ) : player_event_t( *p, timespan_t::from_millis( delay ) )
  {
    diff = timespan_t::from_millis( exp - delay );
  }

  virtual const char* name() const override
  {
    return "imp_delay";
  }

  virtual void execute() override
  {
    warlock_t* p = static_cast<warlock_t*>( player() );

    p->warlock_pet_list.wild_imps.spawn();

    // Remove this event from the vector
    auto it = std::find( p->wild_imp_spawns.begin(), p->wild_imp_spawns.end(), this );
    if ( it != p->wild_imp_spawns.end() )
      p->wild_imp_spawns.erase( it );
  }

  // Used for APL expressions to estimate when imp is "supposed" to spawn
  timespan_t expected_time()
  {
    return std::max( 0_ms, this->remains() + diff );
  }
};
}  // namespace actions

namespace buffs
{
template <typename Base>
struct warlock_buff_t : public Base
{
public:
  typedef warlock_buff_t base_t;
  warlock_buff_t( warlock_td_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(),
                  const item_t* item = nullptr )
    : Base( p, name, s, item )
  {
  }

  warlock_buff_t( warlock_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(),
                  const item_t* item = nullptr )
    : Base( &p, name, s, item )
  {
  }

protected:
  warlock_t* p()
  {
    return static_cast<warlock_t*>( Base::source );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( Base::source );
  }
};
}  // namespace buffs


}  // namespace warlock
