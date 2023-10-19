#pragma once
#include "class_modules/apl/apl_demon_hunter.hpp"

#include "simulationcraft.hpp"
namespace demonhunter
{
struct demon_hunter_t;
struct soul_fragment_t;

constexpr unsigned MAX_SOUL_FRAGMENTS      = 5;
constexpr double VENGEFUL_RETREAT_DISTANCE = 20.0;

enum soul_fragment : unsigned
{
  GREATER         = 0x01,
  LESSER          = 0x02,
  GREATER_DEMON   = 0x04,
  EMPOWERED_DEMON = 0x08,

  ANY_GREATER = ( GREATER | GREATER_DEMON | EMPOWERED_DEMON ),
  ANY_DEMON   = ( GREATER_DEMON | EMPOWERED_DEMON ),
  ANY         = 0xFF
};

soul_fragment operator&( soul_fragment l, soul_fragment r )
{
  return static_cast<soul_fragment>( static_cast<unsigned>( l ) & static_cast<unsigned>( r ) );
}

const char* get_soul_fragment_str( soul_fragment type )
{
  switch ( type )
  {
    case soul_fragment::ANY:
      return "soul fragment";
    case soul_fragment::GREATER:
      return "greater soul fragment";
    case soul_fragment::LESSER:
      return "lesser soul fragment";
    case soul_fragment::GREATER_DEMON:
      return "greater demon fragment";
    case soul_fragment::EMPOWERED_DEMON:
      return "empowered demon fragment";
    default:
      return "";
  }
}

// Movement Buff definition =================================================
struct movement_buff_t : public buff_t
{
  double yards_from_melee;
  double distance_moved;
  demon_hunter_t* dh;

  movement_buff_t( demon_hunter_t* p, util::string_view name, const spell_data_t* spell_data = spell_data_t::nil(),
                   const item_t* item = nullptr );

  bool trigger( int s = 1, double v = DEFAULT_VALUE(), double c = -1.0, timespan_t d = timespan_t::min() ) override;
};

using data_t        = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

namespace actions
{
struct demon_hunter_attack_t;

namespace attacks
{
struct auto_attack_damage_t;
}
}  // namespace actions

namespace spells
{

}  // namespace spells

namespace heals
{

}  // namespace heals

namespace attacks
{

}  // namespace attacks

namespace items
{
}

namespace live_demon_hunter
{
#include "class_modules/sc_demon_hunter_live.inc"
};

// Target Data
// ==========================================================================
struct demon_hunter_td_t : public actor_target_data_t
{
public:
  struct dots_t
  {
    // Shared
    dot_t* the_hunt;

    // Havoc
    dot_t* burning_wound;
    dot_t* trail_of_ruin;

    // Vengeance
    dot_t* fiery_brand;
    dot_t* sigil_of_flame;

  } dots;

  struct debuffs_t
  {
    // Havoc
    buff_t* burning_wound;
    buff_t* essence_break;
    buff_t* initiative_tracker;
    buff_t* serrated_glaive;

    // Vengeance
    buff_t* frailty;

    // Set Bonuses
    buff_t* t29_vengeance_4pc;
  } debuffs;

  demon_hunter_td_t( player_t* target, demon_hunter_t& p );

  demon_hunter_t& dh()
  {
    return *debug_cast<demon_hunter_t*>( source );
  }
  const demon_hunter_t& dh() const
  {
    return *debug_cast<demon_hunter_t*>( source );
  }

  void target_demise();
};

struct demon_hunter_t : public player_t
{
public:
  using base_t = player_t;

  // Data collection for cooldown waste
  auto_dispose<std::vector<data_t*>> cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*>> cd_waste_iter;

  // Autoattacks
  actions::attacks::auto_attack_damage_t* melee_main_hand;
  actions::attacks::auto_attack_damage_t* melee_off_hand;

  double metamorphosis_health;  // Vengeance temp health from meta;
  double expected_max_health;

  // Soul Fragments
  unsigned next_fragment_spawn;  // determines whether the next fragment spawn
                                 // on the left or right
  auto_dispose<std::vector<soul_fragment_t*>> soul_fragments;
  event_t* soul_fragment_pick_up;

  double frailty_accumulator;  // Frailty healing accumulator
  event_t* frailty_driver;

  bool fodder_initiative;
  double shattered_destiny_accumulator;
  double darkglare_boon_cdr_roll;

  event_t* exit_melee_event;  // Event to disable melee abilities mid-VR.

  // Buffs
  struct buffs_t
  {
    // General
    damage_buff_t* demon_soul;
    damage_buff_t* empowered_demon_soul;
    buff_t* immolation_aura;
    buff_t* metamorphosis;
    buff_t* fodder_to_the_flame;

    // Havoc
    buff_t* blind_fury;
    buff_t* blur;
    damage_buff_t* chaos_theory;
    buff_t* death_sweep;
    buff_t* fel_barrage;
    buff_t* furious_gaze;
    damage_buff_t* inertia;
    buff_t* initiative;
    buff_t* inner_demon;
    damage_buff_t* momentum;
    buff_t* out_of_range;
    damage_buff_t* restless_hunter;
    buff_t* tactical_retreat;
    buff_t* unbound_chaos;

    movement_buff_t* fel_rush_move;
    movement_buff_t* vengeful_retreat_move;
    movement_buff_t* metamorphosis_move;

    // Vengeance
    buff_t* demon_spikes;
    buff_t* calcified_spikes;
    buff_t* painbringer;
    absorb_buff_t* soul_barrier;
    buff_t* soul_furnace_damage_amp;
    buff_t* soul_furnace_stack;
    buff_t* soul_fragments;

    // Set Bonuses
    damage_buff_t* t29_havoc_4pc;
    buff_t* t30_havoc_2pc;
    buff_t* t30_havoc_4pc;
    buff_t* t30_vengeance_2pc;
    buff_t* t30_vengeance_4pc;
    damage_buff_t* t31_vengeance_2pc;
  } buff;

  // Talents
  struct talents_t
  {
    struct class_talents_t
    {
      player_talent_t vengeful_retreat;
      player_talent_t blazing_path;
      player_talent_t sigil_of_misery;  // NYI

      player_talent_t unrestrained_fury;
      player_talent_t imprison;               // No Implementation
      player_talent_t shattered_restoration;  // NYI Vengeance

      player_talent_t vengeful_bonds;  // No Implementation
      player_talent_t improved_disrupt;
      player_talent_t bouncing_glaives;
      player_talent_t consume_magic;
      player_talent_t improved_sigil_of_misery;

      player_talent_t pursuit;
      player_talent_t disrupting_fury;
      player_talent_t felblade;
      player_talent_t swallowed_anger;
      player_talent_t charred_warblades;  // NYI Vengeance

      player_talent_t felfire_haste;  // NYI
      player_talent_t master_of_the_glaive;
      player_talent_t champion_of_the_glaive;
      player_talent_t aura_of_pain;
      player_talent_t precise_sigils;    // Partial NYI (debuff Sigils)
      player_talent_t lost_in_darkness;  // No Implementation

      player_talent_t chaos_nova;
      player_talent_t soul_rending;
      player_talent_t infernal_armor;
      player_talent_t aldrachi_design;

      player_talent_t chaos_fragments;
      player_talent_t illidari_knowledge;
      player_talent_t demonic;
      player_talent_t will_of_the_illidari;  // NYI Vengeance
      player_talent_t live_by_the_glaive;    // NYI

      player_talent_t internal_struggle;
      player_talent_t darkness;  // No Implementation
      player_talent_t soul_sigils;
      player_talent_t quickened_sigils;

      player_talent_t erratic_felheart;
      player_talent_t long_night;   // No Implementation
      player_talent_t pitch_black;  // No Implementation
      player_talent_t rush_of_chaos;
      player_talent_t demon_muzzle;  // NYI Vengeance
      player_talent_t flames_of_fury;

      player_talent_t collective_anguish;
      player_talent_t fodder_to_the_flame;
      player_talent_t the_hunt;
      player_talent_t elysian_decree;

    } demon_hunter;

    struct havoc_talents_t
    {
      player_talent_t eye_beam;

      player_talent_t critical_chaos;
      player_talent_t insatiable_hunger;
      player_talent_t demon_blades;
      player_talent_t burning_hatred;

      player_talent_t improved_fel_rush;
      player_talent_t dash_of_chaos;  // NYI
      player_talent_t improved_chaos_strike;
      player_talent_t first_blood;
      player_talent_t accelerated_blade;
      player_talent_t demon_hide;

      player_talent_t desperate_instincts;  // No Implementation
      player_talent_t netherwalk;           // No Implementation
      player_talent_t deflecting_dance;     // NYI
      player_talent_t mortal_dance;         // No Implementation

      player_talent_t initiative;
      player_talent_t scars_of_suffering;
      player_talent_t chaotic_transformation;
      player_talent_t furious_throws;
      player_talent_t trail_of_ruin;

      player_talent_t unbound_chaos;
      player_talent_t blind_fury;
      player_talent_t looks_can_kill;
      player_talent_t dancing_with_fate;
      player_talent_t growing_inferno;

      player_talent_t tactical_retreat;
      player_talent_t isolated_prey;
      player_talent_t furious_gaze;
      player_talent_t relentless_onslaught;
      player_talent_t burning_wound;

      player_talent_t momentum;
      player_talent_t inertia;  // NYI
      player_talent_t chaos_theory;
      player_talent_t restless_hunter;
      player_talent_t inner_demon;
      player_talent_t serrated_glaive;  // Partially implemented
      player_talent_t ragefire;

      player_talent_t know_your_enemy;
      player_talent_t glaive_tempest;
      player_talent_t cycle_of_hatred;
      player_talent_t soulscar;
      player_talent_t chaotic_disposition;

      player_talent_t essence_break;
      player_talent_t fel_barrage;  // Old implementation
      player_talent_t shattered_destiny;
      player_talent_t any_means_necessary;
      player_talent_t a_fire_inside;  // NYI

    } havoc;

    struct vengeance_talents_t
    {
      player_talent_t fel_devastation;

      player_talent_t frailty;
      player_talent_t fiery_brand;

      player_talent_t perfectly_balanced_glaive;
      player_talent_t deflecting_spikes;
      player_talent_t meteoric_strikes;

      player_talent_t shear_fury;
      player_talent_t fracture;
      player_talent_t calcified_spikes;  // NYI
      player_talent_t roaring_fire;      // NYI
      player_talent_t sigil_of_silence;  // Partial Implementation
      player_talent_t retaliation;
      player_talent_t fel_flame_fortification;  // NYI

      player_talent_t spirit_bomb;
      player_talent_t feast_of_souls;  // NYI
      player_talent_t agonizing_flames;
      player_talent_t extended_spikes;
      player_talent_t burning_blood;
      player_talent_t soul_barrier;     // NYI
      player_talent_t bulk_extraction;  // NYI
      player_talent_t ascending_flame;

      player_talent_t void_reaver;
      player_talent_t fallout;
      player_talent_t ruinous_bulwark;  // NYI
      player_talent_t volatile_flameblood;
      player_talent_t revel_in_pain;  // NYI

      player_talent_t soul_furnace;
      player_talent_t painbringer;
      player_talent_t sigil_of_chains;  // Partial Implementation
      player_talent_t fiery_demise;
      player_talent_t chains_of_anger;

      player_talent_t focused_cleave;
      player_talent_t soulmonger;  // NYI
      player_talent_t stoke_the_flames;
      player_talent_t burning_alive;
      player_talent_t cycle_of_binding;

      player_talent_t vulnerability;
      player_talent_t feed_the_demon;
      player_talent_t charred_flesh;

      player_talent_t soulcrush;
      player_talent_t soul_carver;
      player_talent_t last_resort;  // NYI
      player_talent_t darkglare_boon;
      player_talent_t down_in_flames;
      player_talent_t illuminated_sigils;  // Partial Implementation

    } vengeance;

  } talent;

  // Spell Data
  struct spells_t
  {
    // Core Class Spells
    const spell_data_t* chaos_brand;
    const spell_data_t* disrupt;
    const spell_data_t* immolation_aura;
    const spell_data_t* throw_glaive;
    const spell_data_t* sigil_of_flame;
    const spell_data_t* spectral_sight;

    // Class Passives
    const spell_data_t* all_demon_hunter;
    const spell_data_t* critical_strikes;
    const spell_data_t* immolation_aura_2;
    const spell_data_t* leather_specialization;

    // Background Spells
    const spell_data_t* collective_anguish;
    const spell_data_t* collective_anguish_damage;
    const spell_data_t* demon_soul;
    const spell_data_t* demon_soul_empowered;
    const spell_data_t* felblade_damage;
    const spell_data_t* felblade_reset_havoc;
    const spell_data_t* felblade_reset_vengeance;
    const spell_data_t* immolation_aura_damage;
    const spell_data_t* infernal_armor_damage;
    const spell_data_t* sigil_of_flame_damage;
    const spell_data_t* sigil_of_flame_fury;
    const spell_data_t* soul_fragment;

    // Cross-Expansion Override Spells
    const spell_data_t* elysian_decree;
    const spell_data_t* elysian_decree_damage;
    const spell_data_t* fodder_to_the_flame;
    const spell_data_t* fodder_to_the_flame_damage;
    const spell_data_t* the_hunt;

  } spell;

  // Specialization Spells
  struct spec_t
  {
    // General
    const spell_data_t* consume_soul_greater;
    const spell_data_t* consume_soul_lesser;
    const spell_data_t* demonic_wards;
    const spell_data_t* metamorphosis;
    const spell_data_t* metamorphosis_buff;
    const spell_data_t* sigil_of_misery;
    const spell_data_t* sigil_of_misery_debuff;

    // Havoc
    const spell_data_t* havoc_demon_hunter;
    const spell_data_t* annihilation;
    const spell_data_t* blade_dance;
    const spell_data_t* blur;
    const spell_data_t* chaos_strike;
    const spell_data_t* death_sweep;
    const spell_data_t* demons_bite;
    const spell_data_t* fel_rush;
    const spell_data_t* fel_eruption;

    const spell_data_t* blade_dance_2;
    const spell_data_t* burning_wound_debuff;
    const spell_data_t* chaos_strike_fury;
    const spell_data_t* chaos_strike_refund;
    const spell_data_t* chaos_theory_buff;
    const spell_data_t* demon_blades_damage;
    const spell_data_t* demonic_appetite;
    const spell_data_t* demonic_appetite_fury;
    const spell_data_t* essence_break_debuff;
    const spell_data_t* eye_beam_damage;
    const spell_data_t* fel_rush_damage;
    const spell_data_t* first_blood_blade_dance_damage;
    const spell_data_t* first_blood_blade_dance_2_damage;
    const spell_data_t* first_blood_death_sweep_damage;
    const spell_data_t* first_blood_death_sweep_2_damage;
    const spell_data_t* furious_gaze_buff;
    const spell_data_t* glaive_tempest_damage;
    const spell_data_t* immolation_aura_3;
    const spell_data_t* initiative_buff;
    const spell_data_t* inner_demon_buff;
    const spell_data_t* inner_demon_damage;
    const spell_data_t* momentum_buff;
    const spell_data_t* inertia_buff;
    const spell_data_t* ragefire_damage;
    const spell_data_t* serrated_glaive_debuff;
    const spell_data_t* soulscar_debuff;
    const spell_data_t* restless_hunter_buff;
    const spell_data_t* tactical_retreat_buff;
    const spell_data_t* unbound_chaos_buff;
    const spell_data_t* chaotic_disposition_damage;

    // Vengeance
    const spell_data_t* vengeance_demon_hunter;
    const spell_data_t* demon_spikes;
    const spell_data_t* infernal_strike;
    const spell_data_t* soul_cleave;

    const spell_data_t* demonic_wards_2;
    const spell_data_t* demonic_wards_3;
    const spell_data_t* fiery_brand_debuff;
    const spell_data_t* frailty_debuff;
    const spell_data_t* riposte;
    const spell_data_t* soul_cleave_2;
    const spell_data_t* thick_skin;
    const spell_data_t* painbringer_buff;
    const spell_data_t* calcified_spikes_buff;
    const spell_data_t* soul_furnace_damage_amp;
    const spell_data_t* soul_furnace_stack;
    const spell_data_t* immolation_aura_cdr;
    const spell_data_t* soul_fragments_buff;
    const spell_data_t* retaliation_damage;
    const spell_data_t* sigil_of_silence;
    const spell_data_t* sigil_of_silence_debuff;
    const spell_data_t* sigil_of_chains;
    const spell_data_t* sigil_of_chains_debuff;
  } spec;

  // Set Bonus effects
  struct set_bonuses_t
  {
    const spell_data_t* t29_havoc_2pc;
    const spell_data_t* t29_havoc_4pc;
    const spell_data_t* t29_vengeance_2pc;
    const spell_data_t* t29_vengeance_4pc;
    const spell_data_t* t30_havoc_2pc;
    const spell_data_t* t30_havoc_4pc;
    const spell_data_t* t30_vengeance_2pc;
    const spell_data_t* t30_vengeance_4pc;
    const spell_data_t* t31_havoc_2pc;
    const spell_data_t* t31_havoc_4pc;
    const spell_data_t* t31_vengeance_2pc;
    const spell_data_t* t31_vengeance_4pc;
    // Auxilliary
    const spell_data_t* t30_havoc_2pc_buff;
    const spell_data_t* t30_havoc_4pc_refund;
    const spell_data_t* t30_havoc_4pc_buff;
    double t30_havoc_2pc_fury_tracker = 0.0;
    const spell_data_t* t30_vengeance_2pc_buff;
    const spell_data_t* t30_vengeance_4pc_buff;
    double t30_vengeance_4pc_soul_fragments_tracker = 0;
    double t31_vengeance_4pc_fury_tracker           = 0;
    const spell_data_t* t31_vengeance_2pc_buff;
    const spell_data_t* t31_vengeance_4pc_proc;
  } set_bonuses;

  // Mastery Spells
  struct mastery_t
  {
    // Havoc
    const spell_data_t* demonic_presence;
    const spell_data_t* any_means_necessary;
    const spell_data_t* any_means_necessary_tuning;
    // Vengeance
    const spell_data_t* fel_blood;
    const spell_data_t* fel_blood_rank_2;
  } mastery;

  // Cooldowns
  struct cooldowns_t
  {
    // General
    cooldown_t* consume_magic;
    cooldown_t* disrupt;
    cooldown_t* elysian_decree;
    cooldown_t* felblade;
    cooldown_t* fel_eruption;
    cooldown_t* immolation_aura;
    cooldown_t* the_hunt;
    cooldown_t* spectral_sight;

    // Havoc
    cooldown_t* blade_dance;
    cooldown_t* blur;
    cooldown_t* chaos_nova;
    cooldown_t* chaos_strike_refund_icd;
    cooldown_t* essence_break;
    cooldown_t* demonic_appetite;
    cooldown_t* eye_beam;
    cooldown_t* fel_barrage;
    cooldown_t* fel_rush;
    cooldown_t* metamorphosis;
    cooldown_t* netherwalk;
    cooldown_t* relentless_onslaught_icd;
    cooldown_t* throw_glaive;
    cooldown_t* vengeful_retreat;
    cooldown_t* movement_shared;

    // Vengeance
    cooldown_t* demon_spikes;
    cooldown_t* fiery_brand;
    cooldown_t* fel_devastation;
    cooldown_t* sigil_of_chains;
    cooldown_t* sigil_of_flame;
    cooldown_t* sigil_of_misery;
    cooldown_t* sigil_of_silence;
    cooldown_t* volatile_flameblood_icd;
  } cooldown;

  // Gains
  struct gains_t
  {
    // General
    gain_t* miss_refund;
    gain_t* immolation_aura;

    // Havoc
    gain_t* blind_fury;
    gain_t* demonic_appetite;
    gain_t* tactical_retreat;

    // Vengeance
    gain_t* metamorphosis;
    gain_t* volatile_flameblood;
    gain_t* darkglare_boon;

    // Set Bonuses
    gain_t* seething_fury;
  } gain;

  // Benefits
  struct benefits_t
  {
  } benefits;

  // Procs
  struct procs_t
  {
    // General
    proc_t* delayed_aa_range;
    proc_t* delayed_aa_channel;
    proc_t* soul_fragment_greater;
    proc_t* soul_fragment_greater_demon;
    proc_t* soul_fragment_empowered_demon;
    proc_t* soul_fragment_lesser;
    proc_t* felblade_reset;

    // Havoc
    proc_t* demonic_appetite;
    proc_t* demons_bite_in_meta;
    proc_t* chaos_strike_in_essence_break;
    proc_t* annihilation_in_essence_break;
    proc_t* blade_dance_in_essence_break;
    proc_t* death_sweep_in_essence_break;
    proc_t* chaos_strike_in_serrated_glaive;
    proc_t* annihilation_in_serrated_glaive;
    proc_t* throw_glaive_in_serrated_glaive;
    proc_t* shattered_destiny;
    proc_t* eye_beam_canceled;

    // Vengeance
    proc_t* soul_fragment_expire;
    proc_t* soul_fragment_overflow;
    proc_t* soul_fragment_from_shear;
    proc_t* soul_fragment_from_fracture;
    proc_t* soul_fragment_from_fallout;
    proc_t* soul_fragment_from_meta;

    // Set Bonuses
    proc_t* soul_fragment_from_t29_2pc;
    proc_t* soul_fragment_from_t31_4pc;
  } proc;

  // RPPM objects
  struct rppms_t
  {
    real_ppm_t* felblade;

    // Havoc
    real_ppm_t* demonic_appetite;
  } rppm;

  // Shuffled proc objects
  struct shuffled_rngs_t
  {
  } shuffled_rngs;

  // Special
  struct actives_t
  {
    // General
    heal_t* consume_soul_greater     = nullptr;
    heal_t* consume_soul_lesser      = nullptr;
    spell_t* immolation_aura         = nullptr;
    spell_t* immolation_aura_initial = nullptr;
    spell_t* collective_anguish      = nullptr;

    // Havoc
    spell_t* burning_wound                      = nullptr;
    attack_t* demon_blades                      = nullptr;
    spell_t* fel_barrage                        = nullptr;
    spell_t* fodder_to_the_flame_damage         = nullptr;
    spell_t* inner_demon                        = nullptr;
    spell_t* ragefire                           = nullptr;
    attack_t* relentless_onslaught              = nullptr;
    attack_t* relentless_onslaught_annihilation = nullptr;
    attack_t* throw_glaive_t31_proc             = nullptr;
    attack_t* throw_glaive_bd_throw             = nullptr;
    attack_t* throw_glaive_ds_throw             = nullptr;

    // Vengeance
    spell_t* infernal_armor     = nullptr;
    spell_t* retaliation        = nullptr;
    heal_t* frailty_heal        = nullptr;
    spell_t* fiery_brand_t30    = nullptr;
    spell_t* sigil_of_flame_t31 = nullptr;
  } active;

  // Pets
  struct pets_t
  {
  } pets;

  // Options
  struct demon_hunter_options_t
  {
    double initial_fury = 0;
    // Override for target's hitbox size, relevant for Fel Rush and Vengeful Retreat. -1.0 uses default SimC value.
    double target_reach = -1.0;
    // Relative directionality for movement events, 1.0 being directly away and 2.0 being perpendicular
    double movement_direction_factor = 1.8;
    // Chance every second to get hit by the fodder demon
    double fodder_to_the_flame_initiative_chance = 0.20;
    // How many seconds of CDR from Darkglare Boon is considered a high roll
    double darkglare_boon_cdr_high_roll_seconds = 18;
    // Chance of souls to be incidentally picked up on any movement ability due to being in pickup range
    double soul_fragment_movement_consume_chance = 0.85;
  } options;

  struct uptimes_t
  {
    uptime_t* charred_flesh_fiery_brand;
    uptime_t* charred_flesh_sigil_of_flame;
  } uptime;

  demon_hunter_t( sim_t* sim, util::string_view name, race_e r );

  // overridden player_t init functions
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void copy_from( player_t* source ) override;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  void create_buffs() override;
  std::unique_ptr<expr_t> create_expression( util::string_view ) override;
  void create_options() override;
  pet_t* create_pet( util::string_view name, util::string_view type ) override;
  std::string create_profile( save_e ) override;
  void init_absorb_priority() override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_procs() override;
  void init_uptimes() override;
  void init_resources( bool force ) override;
  void init_special_effects() override;
  void init_rng() override;
  void init_scaling() override;
  void init_spells() override;
  void invalidate_cache( cache_e ) override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;

  // custom demon_hunter_t init functions
private:
  void create_cooldowns();
  void create_gains();
  void create_benefits();

public:
  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  // overridden player_t stat functions
  double composite_armor() const override;
  double composite_base_armor_multiplier() const override;
  double composite_armor_multiplier() const override;
  double composite_attack_power_multiplier() const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_crit_avoidance() const override;
  double composite_dodge() const override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_leech() const override;
  double composite_melee_crit_chance() const override;
  double composite_melee_expertise( const weapon_t* ) const override;
  double composite_parry() const override;
  double composite_parry_rating() const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double composite_spell_crit_chance() const override;
  double composite_mastery() const override;
  double composite_damage_versatility() const override;
  double composite_heal_versatility() const override;
  double composite_mitigation_versatility() const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double passive_movement_modifier() const override;
  double temporary_movement_modifier() const override;

  // overridden player_t combat functions
  void assess_damage( school_e, result_amount_type, action_state_t* s ) override;
  void combat_begin() override;
  const demon_hunter_td_t* find_target_data( const player_t* target ) const override;
  demon_hunter_td_t* get_target_data( player_t* target ) const override;
  void interrupt() override;
  void regen( timespan_t periodicity ) override;
  double resource_gain( resource_e, double, gain_t* source = nullptr, action_t* action = nullptr ) override;
  double resource_gain( resource_e, double, double, gain_t* source = nullptr, action_t* action = nullptr );
  void recalculate_resource_max( resource_e, gain_t* source = nullptr ) override;
  void reset() override;
  void merge( player_t& other ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void apply_affecting_auras( action_t& action ) override;

  // custom demon_hunter_t functions
  const spelleffect_data_t* find_spelleffect( const spell_data_t* spell, effect_subtype_t subtype = A_MAX,
                                              int misc_value               = P_GENERIC,
                                              const spell_data_t* affected = spell_data_t::nil(),
                                              effect_type_t type           = E_APPLY_AURA );
  const spell_data_t* find_spell_override( const spell_data_t* base, const spell_data_t* passive );
  const spell_data_t* find_spell_override( const spell_data_t* base, std::vector<const spell_data_t*> passives );
  void set_out_of_range( timespan_t duration );
  void adjust_movement();
  double calculate_expected_max_health() const;
  unsigned consume_soul_fragments( soul_fragment = soul_fragment::ANY, bool heal = true,
                                   unsigned max = MAX_SOUL_FRAGMENTS );
  unsigned consume_nearby_soul_fragments( soul_fragment = soul_fragment::ANY );
  unsigned get_active_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
  unsigned get_inactive_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
  unsigned get_total_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
  void activate_soul_fragment( soul_fragment_t* );
  void spawn_soul_fragment( soul_fragment, unsigned = 1, bool = false );
  void spawn_soul_fragment( soul_fragment, unsigned, player_t* target, bool = false );
  void trigger_demonic();
  double get_target_reach() const
  {
    return options.target_reach >= 0 ? options.target_reach : sim->target->combat_reach;
  }

  // Secondary Action Tracking
private:
  std::vector<action_t*> background_actions;

public:
  template <typename T, typename... Ts>
  T* find_background_action( util::string_view n = {} )
  {
    T* found_action = nullptr;
    for ( auto action : background_actions )
    {
      found_action = dynamic_cast<T*>( action );
      if ( found_action )
      {
        if ( n.empty() || found_action->name_str == n )
          break;
        else
          found_action = nullptr;
      }
    }
    return found_action;
  }

  template <typename T, typename... Ts>
  T* get_background_action( util::string_view n, Ts&&... args )
  {
    auto it = range::find( background_actions, n, &action_t::name_str );
    if ( it != background_actions.cend() )
    {
      return dynamic_cast<T*>( *it );
    }

    auto action        = new T( n, this, std::forward<Ts>( args )... );
    action->background = true;
    background_actions.push_back( action );
    return action;
  }

  // Cooldown Tracking
  template <typename T_CONTAINER, typename T_DATA>
  T_CONTAINER* get_data_entry( util::string_view name, std::vector<T_DATA*>& entries )
  {
    for ( size_t i = 0; i < entries.size(); i++ )
    {
      if ( entries[ i ]->first == name )
      {
        return &( entries[ i ]->second );
      }
    }

    entries.push_back( new T_DATA( name, T_CONTAINER() ) );
    return &( entries.back()->second );
  }

private:
  target_specific_t<demon_hunter_td_t> _target_data;
};

namespace pets
{
// ==========================================================================
// Demon Hunter Pets
// ==========================================================================

/* Demon Hunter pet base
 *
 * defines characteristics common to ALL Demon Hunter pets
 */
struct demon_hunter_pet_t : public pet_t
{
  demon_hunter_pet_t( sim_t* sim, demon_hunter_t& owner, util::string_view pet_name, pet_e pt, bool guardian = false )
    : pet_t( sim, &owner, pet_name, pt, guardian )
  {
  }

  struct _stat_list_t
  {
    int level;
    std::array<double, ATTRIBUTE_MAX> stats;
  };

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    base.position = POSITION_BACK;
    base.distance = 3;

    owner_coeff.ap_from_sp = 1.0;
    owner_coeff.sp_from_sp = 1.0;

    // Base Stats, same for all pets. Depends on level
    static const _stat_list_t pet_base_stats[] = {
        //   none, str, agi, sta, int, spi
        { 85, { { 0, 453, 883, 353, 159, 225 } } },
    };

    // Loop from end to beginning to get the data for the highest available
    // level equal or lower than the player level
    int i = as<int>( std::size( pet_base_stats ) );
    while ( --i > 0 )
    {
      if ( pet_base_stats[ i ].level <= level() )
        break;
    }
    if ( i >= 0 )
      base.stats.attribute = pet_base_stats[ i ].stats;
  }

  void schedule_ready( timespan_t delta_time, bool waiting ) override
  {
    if ( main_hand_attack && !main_hand_attack->execute_event )
    {
      main_hand_attack->schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    // Orc racial
    m *= 1.0 + o().racials.command->effectN( 1 ).percent();

    return m;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_ENERGY;
  }

  demon_hunter_t& o() const
  {
    return static_cast<demon_hunter_t&>( *owner );
  }
};

namespace actions
{  // namespace for pet actions

}  // namespace actions

}  // namespace pets

namespace actions
{
/* This is a template for common code for all Demon Hunter actions.
 * The template is instantiated with either spell_t, heal_t or absorb_t as the
 * 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived
 * class,
 * don't skip it and call spell_t/heal_t or absorb_t directly.
 */
template <typename Base>
class demon_hunter_action_t : public Base
{
public:
  double energize_delta;

  // Cooldown tracking
  bool track_cd_waste;
  simple_sample_data_with_min_max_t *cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;

  // Affect flags for various dynamic effects
  struct affect_flags
  {
    bool direct   = false;
    bool periodic = false;
  };

  struct
  {
    // Havoc
    affect_flags any_means_necessary;
    affect_flags demonic_presence;
    bool chaos_theory    = false;
    bool essence_break   = false;
    bool burning_wound   = false;
    bool serrated_glaive = false;

    // Vengeance
    bool frailty           = false;
    bool fiery_demise      = false;
    bool fires_of_fel      = false;
    bool t31_vengeance_2pc = true;
  } affected_by;

  std::vector<damage_buff_t*> direct_damage_buffs;
  std::vector<damage_buff_t*> periodic_damage_buffs;
  std::vector<damage_buff_t*> auto_attack_damage_buffs;
  std::vector<damage_buff_t*> crit_chance_buffs;

  void parse_affect_flags( const spell_data_t* spell, affect_flags& flags )
  {
    for ( const spelleffect_data_t& effect : spell->effects() )
    {
      if ( !effect.ok() || effect.type() != E_APPLY_AURA || effect.subtype() != A_ADD_PCT_MODIFIER )
        continue;

      if ( ab::data().affected_by( effect ) )
      {
        switch ( effect.misc_value1() )
        {
          case P_GENERIC:
            flags.direct = true;
            break;
          case P_TICK_DAMAGE:
            flags.periodic = true;
            break;
        }
      }
    }
  }

  demon_hunter_action_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                         util::string_view o = {} )
    : ab( n, p, s ),
      energize_delta( 0.0 ),
      track_cd_waste( s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero() ),
      cd_wasted_exec( nullptr ),
      cd_wasted_cumulative( nullptr ),
      cd_wasted_iter( nullptr )
  {
    ab::parse_options( o );

    // Talent Passives
    ab::apply_affecting_aura( p->talent.demon_hunter.blazing_path );
    ab::apply_affecting_aura( p->talent.demon_hunter.improved_disrupt );
    ab::apply_affecting_aura( p->talent.demon_hunter.bouncing_glaives );
    ab::apply_affecting_aura( p->talent.demon_hunter.aura_of_pain );
    ab::apply_affecting_aura( p->talent.demon_hunter.master_of_the_glaive );
    ab::apply_affecting_aura( p->talent.demon_hunter.champion_of_the_glaive );
    ab::apply_affecting_aura( p->talent.demon_hunter.rush_of_chaos );
    ab::apply_affecting_aura( p->talent.demon_hunter.precise_sigils );
    ab::apply_affecting_aura( p->talent.demon_hunter.improved_sigil_of_misery );
    ab::apply_affecting_aura( p->talent.demon_hunter.erratic_felheart );
    ab::apply_affecting_aura( p->talent.demon_hunter.pitch_black );
    ab::apply_affecting_aura( p->talent.demon_hunter.flames_of_fury );
    ab::apply_affecting_aura( p->talent.demon_hunter.quickened_sigils );
    ab::apply_affecting_aura( p->talent.demon_hunter.fodder_to_the_flame );

    ab::apply_affecting_aura( p->talent.havoc.insatiable_hunger );
    ab::apply_affecting_aura( p->talent.havoc.improved_fel_rush );
    ab::apply_affecting_aura( p->talent.havoc.improved_chaos_strike );
    ab::apply_affecting_aura( p->talent.havoc.demon_hide );
    ab::apply_affecting_aura( p->talent.havoc.blind_fury );
    ab::apply_affecting_aura( p->talent.havoc.looks_can_kill );
    ab::apply_affecting_aura( p->talent.havoc.tactical_retreat );
    ab::apply_affecting_aura( p->talent.havoc.accelerated_blade );
    ab::apply_affecting_aura( p->talent.havoc.any_means_necessary );
    ab::apply_affecting_aura( p->talent.havoc.dancing_with_fate );

    ab::apply_affecting_aura( p->talent.vengeance.perfectly_balanced_glaive );
    ab::apply_affecting_aura( p->talent.vengeance.meteoric_strikes );
    ab::apply_affecting_aura( p->talent.vengeance.burning_blood );
    ab::apply_affecting_aura( p->talent.vengeance.ascending_flame );
    ab::apply_affecting_aura( p->talent.vengeance.ruinous_bulwark );
    ab::apply_affecting_aura( p->talent.vengeance.chains_of_anger );
    ab::apply_affecting_aura( p->talent.vengeance.stoke_the_flames );
    ab::apply_affecting_aura( p->talent.vengeance.down_in_flames );
    ab::apply_affecting_aura( p->talent.vengeance.illuminated_sigils );

    // Rank Passives
    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      // Rank Passives
      ab::apply_affecting_aura( p->spec.immolation_aura_3 );

      // Set Bonus Passives
      ab::apply_affecting_aura( p->set_bonuses.t29_havoc_2pc );
      ab::apply_affecting_aura( p->set_bonuses.t31_havoc_4pc );

      // Affect Flags
      parse_affect_flags( p->mastery.demonic_presence, affected_by.demonic_presence );
      parse_affect_flags( p->mastery.any_means_necessary, affected_by.any_means_necessary );

      if ( p->talent.havoc.essence_break->ok() )
      {
        affected_by.essence_break = ab::data().affected_by( p->spec.essence_break_debuff );
      }

      if ( p->spec.burning_wound_debuff->ok() )
      {
        affected_by.burning_wound = ab::data().affected_by( p->spec.burning_wound_debuff->effectN( 2 ) );
      }

      if ( p->talent.havoc.chaos_theory->ok() )
      {
        affected_by.chaos_theory = ab::data().affected_by( p->spec.chaos_theory_buff->effectN( 1 ) );
      }

      if ( p->talent.havoc.serrated_glaive->ok() )
      {
        affected_by.serrated_glaive = ab::data().affected_by( p->spec.serrated_glaive_debuff );
      }
    }
    else  // DEMON_HUNTER_VENGEANCE
    {
      // Rank Passives

      // Set Bonus Passives
      ab::apply_affecting_aura( p->set_bonuses.t30_vengeance_4pc );

      // Talents
      if ( p->talent.vengeance.vulnerability->ok() )
      {
        affected_by.frailty = ab::data().affected_by( p->spec.frailty_debuff->effectN( 4 ) );
      }
      if ( p->talent.vengeance.fiery_demise->ok() )
      {
        affected_by.fiery_demise = ab::data().affected_by( p->spec.fiery_brand_debuff->effectN( 2 ) ) ||
                                   ( p->set_bonuses.t30_vengeance_4pc->ok() &&
                                     ab::data().affected_by_label( p->spec.fiery_brand_debuff->effectN( 4 ) ) );
      }
      if ( p->set_bonuses.t30_vengeance_2pc->ok() )
      {
        affected_by.fires_of_fel = ab::data().affected_by( p->set_bonuses.t30_vengeance_2pc_buff->effectN( 1 ) ) ||
                                   ab::data().affected_by( p->set_bonuses.t30_vengeance_2pc_buff->effectN( 2 ) );
      }
    }
  }

  demon_hunter_t* p()
  {
    return static_cast<demon_hunter_t*>( ab::player );
  }

  const demon_hunter_t* p() const
  {
    return static_cast<const demon_hunter_t*>( ab::player );
  }

  demon_hunter_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  void init() override
  {
    ab::init();

    auto register_damage_buff = [ this ]( damage_buff_t* buff ) {
      if ( buff->is_affecting_direct( ab::s_data ) )
        direct_damage_buffs.push_back( buff );

      if ( buff->is_affecting_periodic( ab::s_data ) )
        periodic_damage_buffs.push_back( buff );

      if ( ab::repeating && !ab::special && !ab::s_data->ok() && buff->auto_attack_mod.multiplier != 1.0 )
        auto_attack_damage_buffs.push_back( buff );

      if ( buff->is_affecting_crit_chance( ab::s_data ) )
        crit_chance_buffs.push_back( buff );
    };

    direct_damage_buffs.clear();
    periodic_damage_buffs.clear();
    auto_attack_damage_buffs.clear();
    crit_chance_buffs.clear();

    register_damage_buff( p()->buff.demon_soul );
    register_damage_buff( p()->buff.empowered_demon_soul );
    register_damage_buff( p()->buff.momentum );
    register_damage_buff( p()->buff.inertia );
    register_damage_buff( p()->buff.restless_hunter );
    register_damage_buff( p()->buff.t29_havoc_4pc );
    register_damage_buff( p()->buff.t31_vengeance_2pc );

    if ( track_cd_waste )
    {
      cd_wasted_exec =
          p()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p()->cd_waste_exec );
      cd_wasted_cumulative = p()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>(
          ab::name_str, p()->cd_waste_cumulative );
      cd_wasted_iter =
          p()->template get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, p()->cd_waste_iter );
    }
  }

  void init_finished() override
  {
    // Update the stats reporting for spells that use background sub-spells
    if ( ab::execute_action && ab::execute_action->school != SCHOOL_NONE )
      ab::stats->school = ab::execute_action->school;
    else if ( ab::impact_action && ab::impact_action->school != SCHOOL_NONE )
      ab::stats->school = ab::impact_action->school;
    else if ( ab::tick_action && ab::tick_action->school != SCHOOL_NONE )
      ab::stats->school = ab::tick_action->school;

    // For reporting purposes only, as the game displays this as SCHOOL_CHAOS
    if ( ab::stats->school == SCHOOL_CHROMATIC )
      ab::stats->school = SCHOOL_CHAOS;
    if ( ab::stats->school == SCHOOL_CHROMASTRIKE )
      ab::stats->school = SCHOOL_CHAOS;

    ab::init_finished();
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    if ( affected_by.essence_break )
    {
      m *= 1.0 + td( target )->debuffs.essence_break->check_value();
    }

    if ( affected_by.serrated_glaive )
    {
      m *= 1.0 + td( target )->debuffs.serrated_glaive->check_value();
    }

    if ( affected_by.burning_wound )
    {
      m *= 1.0 + td( target )->debuffs.burning_wound->check_value();
    }

    if ( p()->specialization() == DEMON_HUNTER_VENGEANCE && td( target )->debuffs.frailty->check() )
    {
      if ( affected_by.frailty )
      {
        m *= 1.0 + p()->talent.vengeance.vulnerability->effectN( 1 ).percent() * td( target )->debuffs.frailty->check();
      }
      else if ( !ab::special )  // AA modifier
      {
        m *= 1.0 + p()->talent.vengeance.vulnerability->effectN( 1 ).percent() * td( target )->debuffs.frailty->check();
      }
    }

    if ( affected_by.fiery_demise && td( target )->dots.fiery_brand->is_ticking() )
    {
      m *= 1.0 + p()->talent.vengeance.fiery_demise->effectN( 1 ).percent();
    }

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_da_multiplier( s );

    // Registered Damage Buffs
    for ( auto damage_buff : direct_damage_buffs )
      m *= damage_buff->stack_value_direct();

    if ( affected_by.demonic_presence.direct )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.any_means_necessary.direct )
    {
      m *= 1.0 + p()->cache.mastery_value() * ( 1.0 + p()->mastery.any_means_necessary_tuning->effectN( 1 ).percent() );
    }

    if ( affected_by.fires_of_fel )
    {
      m *= 1.0 + p()->buff.t30_vengeance_2pc->check_stack_value();
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_ta_multiplier( s );

    // Registered Damage Buffs
    for ( auto damage_buff : periodic_damage_buffs )
      m *= damage_buff->stack_value_periodic();

    if ( affected_by.demonic_presence.periodic )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.any_means_necessary.periodic )
    {
      m *= 1.0 + p()->cache.mastery_value() * ( 1.0 + p()->mastery.any_means_necessary_tuning->effectN( 2 ).percent() );
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = ab::composite_crit_chance();

    // Registered Damage Buffs
    for ( auto crit_chance_buff : crit_chance_buffs )
      c += crit_chance_buff->stack_value_crit_chance();

    if ( affected_by.chaos_theory && p()->buff.chaos_theory->up() )
    {
      const double bonus = p()->rng().range( p()->talent.havoc.chaos_theory->effectN( 1 ).percent(),
                                             p()->talent.havoc.chaos_theory->effectN( 2 ).percent() );
      c += bonus;
    }

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cm = ab::composite_crit_damage_bonus_multiplier();
    return cm;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = ab::composite_energize_amount( s );

    if ( energize_delta > 0 )
    {
      // Round the entire post-delta value as some effects (Demon Blades) with deltas have fractional values
      ea = static_cast<int>( ea + p()->rng().range( 0, 1 + energize_delta ) - ( energize_delta / 2.0 ) );
    }

    return ea;
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    accumulate_spirit_bomb( d->state );
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::result_is_hit( s->result ) )
    {
      accumulate_spirit_bomb( s );
      trigger_chaos_brand( s );
      trigger_initiative( s );
      trigger_t31_vengeance_2pc( s );
    }
  }

  bool ready() override
  {
    if ( ( ab::execute_time() > timespan_t::zero() || ab::channeled ) && p()->buff.out_of_range->check() )
    {
      return false;
    }

    if ( p()->buff.out_of_range->check() && ab::range <= 5.0 )
    {
      return false;
    }

    if ( p()->buff.fel_rush_move->check() )
    {
      return false;
    }

    return ab::ready();
  }

  void update_ready( timespan_t cd ) override
  {
    if ( cd_wasted_exec &&
         ( cd > timespan_t::zero() || ( cd <= timespan_t::zero() && ab::cooldown->duration > timespan_t::zero() ) ) &&
         ab::cooldown->current_charge == ab::cooldown->charges && ab::cooldown->last_charged > timespan_t::zero() &&
         ab::cooldown->last_charged < ab::sim->current_time() )
    {
      double time_ = ( ab::sim->current_time() - ab::cooldown->last_charged ).total_seconds();
      if ( p()->sim->debug )
      {
        p()->sim->out_debug.printf( "%s %s cooldown waste tracking waste=%.3f exec_time=%.3f", p()->name(), ab::name(),
                                    time_, ab::time_to_execute.total_seconds() );
      }
      time_ -= ab::time_to_execute.total_seconds();

      if ( time_ > 0 )
      {
        cd_wasted_exec->add( time_ );
        cd_wasted_iter->add( time_ );
      }
    }

    ab::update_ready( cd );
  }

  void trigger_fury_refund()
  {
    if ( ab::resource_current == RESOURCE_FURY )
    {
      p()->resource_gain( ab::resource_current, ab::last_resource_cost * 0.80, p()->gain.miss_refund );
    }
  }

  void consume_resource() override
  {
    ab::consume_resource();

    if ( ab::current_resource() == RESOURCE_FURY && ab::last_resource_cost > 0 )
    {
      if ( !ab::hit_any_target )
      {
        trigger_fury_refund();
      }

      if ( p()->talent.havoc.shattered_destiny->ok() )
      {
        // DFALPHA TOCHECK -- Does this carry over across from pre-Meta or reset?
        p()->shattered_destiny_accumulator += ab::last_resource_cost;
        const double threshold = p()->talent.havoc.shattered_destiny->effectN( 2 ).base_value();
        while ( p()->shattered_destiny_accumulator >= threshold )
        {
          p()->shattered_destiny_accumulator -= threshold;
          if ( p()->buff.metamorphosis->check() )
          {
            p()->buff.metamorphosis->extend_duration( p(),
                                                      p()->talent.havoc.shattered_destiny->effectN( 1 ).time_value() );
            p()->proc.shattered_destiny->occur();
          }
        }
      }

      if ( p()->set_bonuses.t30_havoc_2pc->ok() )
      {
        p()->set_bonuses.t30_havoc_2pc_fury_tracker += ab::last_resource_cost;
        const double threshold = p()->set_bonuses.t30_havoc_2pc->effectN( 1 ).base_value();
        p()->sim->print_debug( "{} spent {} toward Seething Fury ({}/{})", *p(), ab::last_resource_cost,
                               p()->set_bonuses.t30_havoc_2pc_fury_tracker, threshold );
        if ( p()->set_bonuses.t30_havoc_2pc_fury_tracker >= threshold )
        {
          p()->set_bonuses.t30_havoc_2pc_fury_tracker -= threshold;
          p()->buff.t30_havoc_2pc->trigger();
          p()->sim->print_debug( "{} procced Seething Fury ({}/{})", *p(), p()->set_bonuses.t30_havoc_2pc_fury_tracker,
                                 threshold );
          if ( p()->set_bonuses.t30_havoc_4pc->ok() )
          {
            p()->buff.t30_havoc_4pc->trigger();
            p()->resource_gain( RESOURCE_FURY, p()->set_bonuses.t30_havoc_4pc_refund->effectN( 1 ).base_value(),
                                p()->gain.seething_fury );
          }
        }
      }

      if ( p()->set_bonuses.t31_vengeance_4pc->ok() )
      {
        p()->set_bonuses.t31_vengeance_4pc_fury_tracker += ab::last_resource_cost;
        const double threshold = p()->set_bonuses.t31_vengeance_4pc->effectN( 1 ).base_value();
        p()->sim->print_debug( "{} spent {} toward T31 Vengeance 4pc ({}/{})", *p(), ab::last_resource_cost,
                               p()->set_bonuses.t31_vengeance_4pc_fury_tracker, threshold );
        if ( p()->set_bonuses.t31_vengeance_4pc_fury_tracker >= threshold )
        {
          p()->set_bonuses.t31_vengeance_4pc_fury_tracker -= threshold;
          timespan_t time_reduction = p()->set_bonuses.t31_vengeance_4pc->effectN( 2 ).time_value();
          p()->cooldown.sigil_of_flame->adjust( -time_reduction );
          p()->sim->print_debug( "{} procced T31 Vengeance 4pc ({}/{})", *p(),
                                 p()->set_bonuses.t31_vengeance_4pc_fury_tracker, threshold );
        }
      }
    }
  }

  void accumulate_spirit_bomb( action_state_t* s )
  {
    if ( !p()->talent.vengeance.spirit_bomb->ok() )
      return;

    if ( !( ab::harmful && s->result_amount > 0 ) )
      return;

    const demon_hunter_td_t* target_data = td( s->target );
    if ( !target_data->debuffs.frailty->up() )
      return;

    const double multiplier = target_data->debuffs.frailty->stack_value();
    p()->frailty_accumulator += s->result_amount * multiplier;
  }

  void trigger_felblade( action_state_t* s )
  {
    if ( ab::result_is_miss( s->result ) )
      return;

    if ( p()->talent.demon_hunter.felblade->ok() && p()->rppm.felblade->trigger() )
    {
      p()->proc.felblade_reset->occur();
      p()->cooldown.felblade->reset( true );
    }
  }

  void trigger_chaos_brand( action_state_t* s )
  {
    if ( ab::sim->overrides.chaos_brand )
      return;

    if ( ab::result_is_miss( s->result ) || s->result_amount == 0.0 )
      return;

    if ( s->target->debuffs.chaos_brand && p()->spell.chaos_brand->ok() )
    {
      s->target->debuffs.chaos_brand->trigger();
    }
  }

  void hit_fodder( bool kill = false )
  {
    if ( !p()->buff.fodder_to_the_flame->check() )
      return;

    if ( p()->fodder_initiative )
    {
      p()->buff.initiative->trigger();
      p()->fodder_initiative = false;
    }

    if ( kill )
      p()->buff.fodder_to_the_flame->expire();
  }

  void trigger_initiative( action_state_t* s )
  {
    if ( ab::result_is_miss( s->result ) || s->result_amount == 0.0 )
      return;

    if ( !p()->talent.havoc.initiative->ok() )
      return;

    if ( !s->target->is_enemy() || td( s->target )->debuffs.initiative_tracker->check() )
      return;

    p()->buff.initiative->trigger();
    td( s->target )->debuffs.initiative_tracker->trigger();
  }

  void trigger_t31_vengeance_2pc( action_state_t* s )
  {
    if ( !p()->set_bonuses.t31_vengeance_2pc->ok() )
      return;

    if ( !affected_by.t31_vengeance_2pc )
      return;

    const demon_hunter_td_t* target_data = td( s->target );
    if ( !target_data->dots.sigil_of_flame->is_ticking() )
      return;

    p()->buff.t31_vengeance_2pc->trigger();
  }

  void trigger_cycle_of_hatred()
  {
    if ( !p()->talent.havoc.cycle_of_hatred->ok() )
      return;

    if ( !p()->cooldown.eye_beam->down() )
      return;

    timespan_t adjust_seconds = p()->talent.havoc.cycle_of_hatred->effectN( 1 ).time_value();
    p()->cooldown.eye_beam->adjust( -adjust_seconds );
  }

protected:
  /// typedef for demon_hunter_action_t<action_base_t>
  using base_t = demon_hunter_action_t<Base>;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  using ab = Base;
};

// ==========================================================================
// Demon Hunter Ability Classes
// ==========================================================================

struct demon_hunter_heal_t : public demon_hunter_action_t<heal_t>
{
  demon_hunter_heal_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                       util::string_view o = {} )
    : base_t( n, p, s, o )
  {
    harmful = false;
    set_target( p );
  }
};

struct demon_hunter_spell_t : public demon_hunter_action_t<spell_t>
{
  demon_hunter_spell_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                        util::string_view o = {} )
    : base_t( n, p, s, o )
  {
  }
};

struct demon_hunter_sigil_t : public demon_hunter_spell_t
{
  timespan_t sigil_delay;
  timespan_t sigil_activates;
  std::vector<cooldown_t*> sigil_cooldowns;
  timespan_t sigil_cooldown_adjust;

  demon_hunter_sigil_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
    : demon_hunter_spell_t( n, p, s ), sigil_delay( delay ), sigil_activates( timespan_t::zero() )
  {
    aoe        = -1;
    background = dual = ground_aoe = true;
    assert( delay > timespan_t::zero() );

    if ( p->talent.vengeance.cycle_of_binding->ok() )
    {
      sigil_cooldowns = { p->cooldown.sigil_of_flame, p->cooldown.elysian_decree, p->cooldown.sigil_of_misery,
                          p->cooldown.sigil_of_silence };
      sigil_cooldown_adjust =
          -timespan_t::from_seconds( p->talent.vengeance.cycle_of_binding->effectN( 1 ).base_value() );
    }
  }

  void place_sigil( player_t* target )
  {
    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( target )
                                        .x( target->x_position )
                                        .y( target->y_position )
                                        .pulse_time( sigil_delay )
                                        .duration( sigil_delay )
                                        .action( this ) );

    sigil_activates = sim->current_time() + sigil_delay;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( hit_any_target && p()->talent.demon_hunter.soul_sigils->ok() )
    {
      unsigned num_souls = as<unsigned>( p()->talent.demon_hunter.soul_sigils->effectN( 1 ).base_value() );
      p()->spawn_soul_fragment( soul_fragment::LESSER, num_souls, false );
    }
    if ( hit_any_target && p()->talent.vengeance.cycle_of_binding->ok() )
    {
      std::vector<cooldown_t*> sigils_on_cooldown;
      range::copy_if( sigil_cooldowns, std::back_inserter( sigils_on_cooldown ),
                      []( cooldown_t* c ) { return c->down(); } );
      for ( auto sigil_cooldown : sigils_on_cooldown )
      {
        sigil_cooldown->adjust( sigil_cooldown_adjust );
      }
    }
  }

  std::unique_ptr<expr_t> create_sigil_expression( util::string_view name );
};

struct demon_hunter_attack_t : public demon_hunter_action_t<melee_attack_t>
{
  demon_hunter_attack_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                         util::string_view o = {} )
    : base_t( n, p, s, o )
  {
    special = true;
  }
};

// ==========================================================================
// Demon Hunter heals
// ==========================================================================

namespace heals
{

// Consume Soul =============================================================

struct consume_soul_t : public demon_hunter_heal_t
{
  struct demonic_appetite_energize_t : public demon_hunter_spell_t
  {
    demonic_appetite_energize_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.demonic_appetite_fury )
    {
      may_miss = may_block = may_dodge = may_parry = callbacks = false;
      background = quiet = dual = true;
      energize_type             = action_energize::ON_CAST;
    }
  };

  const soul_fragment type;
  const spell_data_t* vengeance_heal;
  const timespan_t vengeance_heal_interval;

  consume_soul_t( demon_hunter_t* p, util::string_view n, const spell_data_t* s, soul_fragment t )
    : demon_hunter_heal_t( n, p, s ),
      type( t ),
      vengeance_heal( p->find_specialization_spell( 203783 ) ),
      vengeance_heal_interval( timespan_t::from_seconds( vengeance_heal->effectN( 4 ).base_value() ) )
  {
    may_miss   = false;
    background = true;

    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      execute_action = p->get_background_action<demonic_appetite_energize_t>( "demonic_appetite_fury" );
    }
  }

  double calculate_heal( const action_state_t* ) const
  {
    if ( p()->specialization() == DEMON_HUNTER_HAVOC )
    {
      // Havoc always heals for the same percentage of HP, regardless of the soul type consumed
      return player->resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();
    }
    else
    {
      if ( type == soul_fragment::LESSER )
      {
        // Vengeance-Specific Healing Logic
        // This is not in the heal data and they use SpellId 203783 to control the healing parameters
        return std::max(
            player->resources.max[ RESOURCE_HEALTH ] * vengeance_heal->effectN( 3 ).percent(),
            player->compute_incoming_damage( vengeance_heal_interval ) * vengeance_heal->effectN( 2 ).percent() );
      }
      // SOUL_FRAGMENT_GREATER for Vengeance uses AP mod calculations
    }

    return 0.0;
  }

  double base_da_min( const action_state_t* s ) const override
  {
    return calculate_heal( s );
  }

  double base_da_max( const action_state_t* s ) const override
  {
    return calculate_heal( s );
  }

  // Handled in the delayed consume event, not the heal action
  timespan_t travel_time() const override
  {
    return 0_s;
  }
};

// Fel Devastation ==========================================================

struct fel_devastation_heal_t : public demon_hunter_heal_t
{
  fel_devastation_heal_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_heal_t( name, p, p->find_spell( 212106 ) )
  {
    background = true;
  }
};

// Soul Barrier =============================================================

struct soul_barrier_t : public demon_hunter_action_t<absorb_t>
{
  unsigned souls_consumed;

  soul_barrier_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_action_t( "soul_barrier", p, p->talent.vengeance.soul_barrier, options_str ), souls_consumed( 0 )
  {
    // Doesn't get populated from spell data correctly since this is not actually an AP coefficient
    base_dd_min = base_dd_max = 0;
    attack_power_mod.direct   = data().effectN( 1 ).percent();
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    double c = demon_hunter_action_t<absorb_t>::attack_direct_power_coefficient( s );

    c += souls_consumed * data().effectN( 2 ).percent();

    return c;
  }

  void execute() override
  {
    souls_consumed = p()->consume_soul_fragments();
    demon_hunter_action_t<absorb_t>::execute();
  }

  void impact( action_state_t* s ) override
  {
    p()->buff.soul_barrier->trigger( 1, s->result_amount );
  }
};

// Soul Cleave ==============================================================

struct soul_cleave_heal_t : public demon_hunter_heal_t
{
  struct feast_of_souls_heal_t : public demon_hunter_heal_t
  {
    feast_of_souls_heal_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_heal_t( name, p, p->find_spell( 207693 ) )
    {
      dual = true;
    }
  };

  soul_cleave_heal_t( util::string_view name, demon_hunter_t* p ) : demon_hunter_heal_t( name, p, p->spec.soul_cleave )
  {
    background = dual = true;

    // Clear out the costs since this is just a copy of the damage spell
    base_costs.fill( 0 );
    secondary_costs.fill( 0 );

    if ( p->talent.vengeance.feast_of_souls->ok() )
    {
      execute_action = p->get_background_action<feast_of_souls_heal_t>( "feast_of_souls" );
    }
  }
};

// Frailty ==============================================================

struct frailty_heal_t : public demon_hunter_heal_t
{
  frailty_heal_t( demon_hunter_t* p ) : demon_hunter_heal_t( "frailty_heal", p, p->find_spell( 227255 ) )
  {
    background = true;
    may_crit   = false;
  }
};

}  // namespace heals

// ==========================================================================
// Demon Hunter spells
// ==========================================================================

namespace spells
{

}  // end namespace spells

}  // end namespace actions

struct exit_melee_event_t : public event_t
{
  demon_hunter_t& dh;
  movement_buff_t* trigger_buff;

  exit_melee_event_t( demon_hunter_t* p, timespan_t delay, movement_buff_t* trigger_buff )
    : event_t( *p, delay ), dh( *p ), trigger_buff( trigger_buff )
  {
    assert( delay > timespan_t::zero() );
  }

  const char* name() const override
  {
    return "exit_melee_event";
  }

  void execute() override
  {
    // Trigger an out of range buff based on the distance to return plus remaining movement aura time
    if ( trigger_buff && trigger_buff->yards_from_melee > 0.0 )
    {
      const timespan_t base_duration =
          timespan_t::from_seconds( trigger_buff->yards_from_melee / dh.cache.run_speed() );
      dh.set_out_of_range( base_duration + trigger_buff->remains() );
    }

    dh.exit_melee_event = nullptr;
  }
};

bool movement_buff_t::trigger( int s, double v, double c, timespan_t d )
{
  assert( distance_moved > 0 );
  assert( buff_duration() > timespan_t::zero() );

  // Check if we're already moving away from the target, if so we will now be moving towards it
  if ( dh->current.distance_to_move || dh->buff.out_of_range->check() || dh->buff.vengeful_retreat_move->check() ||
       dh->buff.metamorphosis_move->check() )
  {
    dh->set_out_of_range( timespan_t::zero() );
    yards_from_melee = 0.0;
  }
  // Since we're not yet moving, we should be moving away from the target
  else
  {
    // Calculate the number of yards away from melee this will send us.
    // This is equal to reach + melee range times the direction factor
    // With 2.0 being moving fully "across" the target and 1.0 moving fully "away"
    yards_from_melee =
        std::max( 0.0, distance_moved - ( ( dh->get_target_reach() + 5.0 ) * dh->options.movement_direction_factor ) );
  }

  if ( yards_from_melee > 0.0 )
  {
    assert( !dh->exit_melee_event );

    // Calculate the amount of time it will take for the movement to carry us out of range
    const timespan_t delay = buff_duration() * ( 1.0 - ( yards_from_melee / distance_moved ) );

    assert( delay > timespan_t::zero() );

    // Schedule event to set us out of melee.
    dh->exit_melee_event = make_event<exit_melee_event_t>( *sim, dh, delay, this );
  }

  // TODO -- Make this actually inherit from the base movement_buff_t class
  if ( dh->buffs.static_empowerment )
  {
    dh->buffs.static_empowerment->expire();
  }

  return buff_t::trigger( s, v, c, d );
}

movement_buff_t::movement_buff_t( demon_hunter_t* p, util::string_view name, const spell_data_t* spell_data,
                                  const item_t* item )
  : buff_t( p, name, spell_data, item ), yards_from_melee( 0.0 ), distance_moved( 0.0 ), dh( p )
{
}

// Delayed Execute Event ====================================================
struct delayed_execute_event_t : public event_t
{
  action_t* action;
  player_t* target;

  delayed_execute_event_t( demon_hunter_t* p, action_t* a, player_t* t, timespan_t delay )
    : event_t( *p->sim, delay ), action( a ), target( t )
  {
    assert( action->background );
  }

  const char* name() const override
  {
    return action->name();
  }

  void execute() override
  {
    if ( !target->is_sleeping() )
    {
      action->set_target( target );
      action->execute();
    }
  }
};

// Soul Fragment definition =================================================

struct soul_fragment_t
{
  struct fragment_expiration_t : public event_t
  {
    soul_fragment_t* frag;

    fragment_expiration_t( soul_fragment_t* s ) : event_t( *s->dh, s->dh->spell.soul_fragment->duration() ), frag( s )
    {
    }

    const char* name() const override
    {
      return "Soul Fragment expiration";
    }

    void execute() override
    {
      if ( sim().debug )
      {
        sim().out_debug.printf( "%s %s expires. active=%u total=%u", frag->dh->name(),
                                get_soul_fragment_str( frag->type ), frag->dh->get_active_soul_fragments( frag->type ),
                                frag->dh->get_total_soul_fragments( frag->type ) );
      }

      frag->expiration = nullptr;
      frag->remove();
    }
  };

  struct fragment_activate_t : public event_t
  {
    soul_fragment_t* frag;

    fragment_activate_t( soul_fragment_t* s ) : event_t( *s->dh ), frag( s )
    {
      schedule( travel_time() );
    }

    const char* name() const override
    {
      return "Soul Fragment activate";
    }

    timespan_t travel_time() const
    {
      return frag->get_travel_time( true );
    }

    void execute() override
    {
      if ( sim().debug )
      {
        sim().out_debug.printf(
            "%s %s becomes active. active=%u total=%u", frag->dh->name(), get_soul_fragment_str( frag->type ),
            frag->dh->get_active_soul_fragments( frag->type ) + 1, frag->dh->get_total_soul_fragments( frag->type ) );
      }

      frag->activate   = nullptr;
      frag->expiration = make_event<fragment_expiration_t>( sim(), frag );
      frag->dh->activate_soul_fragment( frag );
    }
  };

  demon_hunter_t* dh;
  double x, y;
  event_t* activate;
  event_t* expiration;
  const soul_fragment type;
  bool consume_on_activation;

  soul_fragment_t( demon_hunter_t* p, soul_fragment t, bool consume_on_activation )
    : dh( p ), type( t ), consume_on_activation( consume_on_activation )
  {
    activate = expiration = nullptr;

    schedule_activate();
  }

  ~soul_fragment_t()
  {
    event_t::cancel( activate );
    event_t::cancel( expiration );
  }

  double get_distance( demon_hunter_t* p ) const
  {
    return p->get_position_distance( x, y );
  }

  timespan_t get_travel_time( bool activation = false ) const
  {
    double velocity = dh->spec.consume_soul_greater->missile_speed();
    if ( ( activation && consume_on_activation ) || velocity == 0 )
      return timespan_t::zero();

    // 2023-06-26 -- Recent testing appears to show a roughly fixed 1s activation time for Havoc
    if ( activation )
    {
      if ( dh->specialization() == DEMON_HUNTER_HAVOC )
      {
        return 1_s;
      }
      // 2023-07-27 -- Recent testing appears to show a roughly 0.85s activation time for Vengeance
      return 850_ms;
    }

    double distance = get_distance( dh );
    return timespan_t::from_seconds( distance / velocity );
  }

  bool active() const
  {
    return expiration != nullptr;
  }

  void remove() const
  {
    std::vector<soul_fragment_t*>::iterator it =
        std::find( dh->soul_fragments.begin(), dh->soul_fragments.end(), this );

    assert( it != dh->soul_fragments.end() );

    dh->soul_fragments.erase( it );
    delete this;
  }

  timespan_t remains() const
  {
    if ( expiration )
    {
      return expiration->remains();
    }
    else
    {
      return dh->spell.soul_fragment->duration();
    }
  }

  bool is_type( soul_fragment type_mask ) const
  {
    return ( type_mask & type ) == type;
  }

  void set_position()
  {
    // Base position is up to 15 yards to the front right or front left for Vengeance, 9.5 yards for Havoc
    const double distance = ( dh->specialization() == DEMON_HUNTER_HAVOC ) ? 4.6066 : 10.6066;
    x                     = dh->x_position + ( dh->next_fragment_spawn % 2 ? -distance : distance );
    y                     = dh->y_position + distance;

    // Calculate random offset, 2-5 yards from the base position.
    double r_min = 2.0;
    double r_max = 5.0;
    // Nornmalizing factor
    double a = 2.0 / ( r_max * r_max - r_min * r_min );
    // Calculate distance from origin using power-law distribution for
    // uniformity.
    double dist = sqrt( 2.0 * dh->rng().real() / a + r_min * r_min );
    // Pick a random angle.
    double theta = dh->rng().range( 0.0, 2.0 * m_pi );
    // And finally, apply the offsets to x and y;
    x += dist * cos( theta );
    y += dist * sin( theta );

    dh->next_fragment_spawn++;
  }

  void schedule_activate()
  {
    set_position();

    activate = make_event<fragment_activate_t>( *dh->sim, this );
  }

  void consume( bool heal = true, bool instant = false )
  {
    assert( active() );

    if ( heal )
    {
      action_t* consume_action =
          is_type( soul_fragment::ANY_GREATER ) ? dh->active.consume_soul_greater : dh->active.consume_soul_lesser;

      timespan_t delay = get_travel_time();
      if ( instant || delay == 0_s )
      {
        consume_action->execute();
      }
      else
      {
        make_event<delayed_execute_event_t>( *dh->sim, dh, consume_action, dh, delay );
      }
    }

    if ( dh->talent.vengeance.feed_the_demon->ok() )
    {
      timespan_t duration =
          timespan_t::from_seconds( dh->talent.vengeance.feed_the_demon->effectN( 1 ).base_value() ) / 10;
      dh->cooldown.demon_spikes->adjust( -duration );
    }

    if ( dh->talent.vengeance.soul_furnace->ok() )
    {
      dh->buff.soul_furnace_stack->trigger();
      if ( dh->buff.soul_furnace_stack->at_max_stacks() )
      {
        dh->buff.soul_furnace_stack->expire();
        dh->buff.soul_furnace_damage_amp->trigger();
      }
    }
    if ( dh->set_bonuses.t30_vengeance_4pc->ok() )
    {
      dh->set_bonuses.t30_vengeance_4pc_soul_fragments_tracker += 1;
      if ( dh->set_bonuses.t30_vengeance_4pc_soul_fragments_tracker >=
           dh->set_bonuses.t30_vengeance_4pc->effectN( 1 ).base_value() )
      {
        dh->buff.t30_vengeance_4pc->trigger();
        dh->set_bonuses.t30_vengeance_4pc_soul_fragments_tracker -=
            dh->set_bonuses.t30_vengeance_4pc->effectN( 1 ).base_value();
      }
    }

    if ( is_type( soul_fragment::EMPOWERED_DEMON ) )
    {
      dh->buff.empowered_demon_soul->trigger();
    }
    else if ( is_type( soul_fragment::GREATER_DEMON ) )
    {
      dh->buff.demon_soul->trigger();
    }

    dh->buff.soul_fragments->decrement();
    remove();
  }
};

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
struct demon_hunter_report_t : public player_report_extension_t
{
public:
  demon_hunter_report_t( demon_hunter_t& player ) : p( player )
  {
  }

  void cdwaste_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th colspan=\"3\">Seconds per Execute</th>\n"
       << "<th colspan=\"3\">Seconds per Iteration</th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th>Ability</th>\n"
       << "<th>Average</th>\n"
       << "<th>Minimum</th>\n"
       << "<th>Maximum</th>\n"
       << "<th>Average</th>\n"
       << "<th>Minimum</th>\n"
       << "<th>Maximum</th>\n"
       << "</tr>\n";
  }

  void cdwaste_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void cdwaste_table_contents( report::sc_html_stream& os )
  {
    size_t n = 0;
    for ( size_t i = 0; i < p.cd_waste_exec.size(); i++ )
    {
      const data_t* entry = p.cd_waste_exec[ i ];
      if ( entry->second.count() == 0 )
      {
        continue;
      }

      const data_t* iter_entry = p.cd_waste_cumulative[ i ];

      action_t* a          = p.find_action( entry->first );
      std::string name_str = entry->first;
      if ( a )
      {
        name_str = report_decorators::decorated_action( *a );
      }
      else
      {
        name_str = util::encode_html( name_str );
      }

      std::string row_class_str;
      if ( ++n & 1 )
        row_class_str = " class=\"odd\"";

      os.printf( "<tr%s>", row_class_str.c_str() );
      os << "<td class=\"left\">" << name_str << "</td>";
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.min() );
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.max() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.min() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.max() );
      os << "</tr>\n";
    }
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    (void)p;
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
    if ( !p.cd_waste_exec.empty() )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown Waste Details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      cdwaste_table_header( os );
      cdwaste_table_contents( os );
      cdwaste_table_footer( os );

      os << "\t\t\t\t\t</div>\n";
      os << "<div class=\"clear\"></div>\n";
    }
    os << "\t\t\t\t\t</div>\n";
  }

private:
  demon_hunter_t& p;
};

// MODULE INTERFACE ==================================================

struct demon_hunter_module_t : public module_t
{
public:
  demon_hunter_module_t() : module_t( DEMON_HUNTER )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    if ( !sim->dbc->ptr )
    {
      auto p = new live_demon_hunter::demon_hunter_t( sim, name, r );
      p->report_extension =
          std::unique_ptr<player_report_extension_t>( new live_demon_hunter::demon_hunter_report_t( *p ) );
      return p;
    }
    auto p              = new demon_hunter_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new demon_hunter_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void init( player_t* ) const override
  {
  }

  void static_init() const override
  {
    using namespace items;
  }

  void register_hotfixes() const override
  {
    hotfix::register_spell( "Demon Hunter", "2023-05-28", "Manually set Consume Soul Fragment (Greater) travel speed.",
                            178963 )
        .field( "prj_speed" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 25.0 )
        .verification_value( 0.0 );
  }

  void combat_begin( sim_t* ) const override
  {
  }

  void combat_end( sim_t* ) const override
  {
  }
};

// demon_hunter_t::invalidate_cache =========================================

void demon_hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( mastery.demonic_presence->ok() )
      {
        invalidate_cache( CACHE_RUN_SPEED );
      }
      if ( mastery.fel_blood->ok() )
      {
        invalidate_cache( CACHE_ARMOR );
        invalidate_cache( CACHE_ATTACK_POWER );
      }
      break;
    case CACHE_CRIT_CHANCE:
      if ( spec.riposte->ok() )
        invalidate_cache( CACHE_PARRY );
      break;
    case CACHE_RUN_SPEED:
      adjust_movement();
      break;
    case CACHE_AGILITY:
      invalidate_cache( CACHE_ARMOR );
      break;
    default:
      break;
  }
}

// demon_hunter_t::primary_resource =========================================

resource_e demon_hunter_t::primary_resource() const
{
  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
    case DEMON_HUNTER_VENGEANCE:
      return RESOURCE_FURY;
    default:
      return RESOURCE_NONE;
  }
}

// demon_hunter_t::primary_role =============================================

role_e demon_hunter_t::primary_role() const
{
  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
      return ROLE_ATTACK;
    case DEMON_HUNTER_VENGEANCE:
      return ROLE_TANK;
    default:
      return ROLE_NONE;
  }
}

// demon_hunter_t::default_flask ===================================================

std::string demon_hunter_t::default_flask() const
{
  switch ( specialization() )
  {
    case DEMON_HUNTER_VENGEANCE:
      return is_ptr() ? demon_hunter_apl::flask_vengeance_ptr( this ) : demon_hunter_apl::flask_vengeance( this );
    default:
      return demon_hunter_apl::flask_havoc( this );
  }
}

// demon_hunter_t::default_potion ==================================================

std::string demon_hunter_t::default_potion() const
{
  return demon_hunter_apl::potion( this );
}

// demon_hunter_t::default_food ====================================================

std::string demon_hunter_t::default_food() const
{
  return demon_hunter_apl::food( this );
}

// demon_hunter_t::default_rune ====================================================

std::string demon_hunter_t::default_rune() const
{
  return demon_hunter_apl::rune( this );
}

// demon_hunter_t::default_temporary_enchant =======================================

std::string demon_hunter_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case DEMON_HUNTER_VENGEANCE:
      return demon_hunter_apl::temporary_enchant_vengeance_ptr( this );
    default:
      return demon_hunter_apl::temporary_enchant_havoc( this );
  }
}

}  // namespace demonhunter