// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "class_modules/apl/apl_demon_hunter.hpp"

namespace
{  // UNNAMED NAMESPACE
// ==========================================================================
// Demon Hunter
// ==========================================================================

/* ==========================================================================
// Shadowlands To-Do
// ==========================================================================

  Existing Issues
  ---------------

  * Add option for Greater Soul spawns on add demise (% chance?) simulating adds in M+/dungeon style situations

  New Issues
  ----------

  Vengeance:
  * Add Revel in Pain passive
  ** Ruinous Bulwark Absorb

  Maybe:
  ** Darkness (?)
  ** Sigil of Silence (?)
  ** Sigil of Misery (?)
*/

// Forward Declarations
class demon_hunter_t;
struct soul_fragment_t;

namespace buffs
{}
namespace actions
{
  struct demon_hunter_attack_t;
  namespace attacks
  {
    struct auto_attack_damage_t;
  }
}
namespace items
{
}

// Target Data
class demon_hunter_td_t : public actor_target_data_t
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

constexpr unsigned MAX_SOUL_FRAGMENTS = 5;
constexpr double VENGEFUL_RETREAT_DISTANCE = 20.0;

enum class soul_fragment : unsigned
{
  GREATER = 0x01,
  LESSER = 0x02,
  GREATER_DEMON = 0x04,
  EMPOWERED_DEMON = 0x08,

  ANY_GREATER = ( GREATER | GREATER_DEMON | EMPOWERED_DEMON ),
  ANY_DEMON = ( GREATER_DEMON | EMPOWERED_DEMON ),
  ANY = 0xFF
};

soul_fragment operator&( soul_fragment l, soul_fragment r )
{
  return static_cast<soul_fragment> ( static_cast<unsigned>( l ) & static_cast<unsigned>( r ) );
}

[[maybe_unused]] soul_fragment operator|( soul_fragment l, soul_fragment r )
{
  return static_cast<soul_fragment> ( static_cast<unsigned>( l ) | static_cast<unsigned>( r ) );
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

struct movement_buff_t : public buff_t
{
  double yards_from_melee;
  double distance_moved;
  demon_hunter_t* dh;

  movement_buff_t( demon_hunter_t* p, util::string_view name, const spell_data_t* spell_data = spell_data_t::nil(), const item_t* item = nullptr );

  bool trigger( int s = 1, double v = DEFAULT_VALUE(), double c = -1.0, timespan_t d = timespan_t::min() ) override;
};

using data_t = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

/* Demon Hunter class definition
 *
 * Derived from player_t. Contains everything that defines the Demon Hunter
 * class.
 */
class demon_hunter_t : public player_t
{
public:
  using base_t = player_t;

  // Data collection for cooldown waste
  auto_dispose<std::vector<data_t*>> cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*>> cd_waste_iter;

  // Autoattacks
  actions::attacks::auto_attack_damage_t* melee_main_hand;
  actions::attacks::auto_attack_damage_t* melee_off_hand;

  double metamorphosis_health;    // Vengeance temp health from meta;
  double expected_max_health;

  // Soul Fragments
  unsigned next_fragment_spawn;  // determines whether the next fragment spawn
                                 // on the left or right
  auto_dispose<std::vector<soul_fragment_t*>> soul_fragments;
  event_t* soul_fragment_pick_up;

  double frailty_accumulator;  // Frailty healing accumulator
  event_t* frailty_driver;

  double ragefire_accumulator;
  unsigned ragefire_crit_accumulator;
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

    // Havoc
    buff_t* blind_fury;
    buff_t* blur;
    damage_buff_t* chaos_theory;
    buff_t* death_sweep;
    buff_t* furious_gaze;
    buff_t* growing_inferno;
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
    buff_t* painbringer;
    absorb_buff_t* soul_barrier;
    buff_t* soul_furnace_damage_amp;
    buff_t* soul_furnace_stack;
    buff_t* soul_fragments;
    buff_t* charred_flesh;

    // Set Bonuses
    damage_buff_t* t29_havoc_4pc;
    buff_t* t30_havoc_2pc;
    buff_t* t30_havoc_4pc;
    buff_t* t30_vengeance_2pc;
    buff_t* t30_vengeance_4pc;
  } buff;

  // Talents
  struct talents_t
  {
    struct class_talents_t
    {
      player_talent_t vengeful_retreat;
      player_talent_t blazing_path;
      player_talent_t sigil_of_flame;

      player_talent_t unrestrained_fury;
      player_talent_t imprison;                   // No Implementation
      player_talent_t shattered_restoration;      // NYI Vengeance

      player_talent_t vengeful_bonds;             // No Implementation
      player_talent_t improved_disrupt;
      player_talent_t bouncing_glaives;
      player_talent_t consume_magic;
      player_talent_t flames_of_fury;

      player_talent_t pursuit;
      player_talent_t disrupting_fury;
      player_talent_t aura_of_pain;
      player_talent_t felblade;
      player_talent_t swallowed_anger;
      player_talent_t charred_warblades;          // NYI Vengeance

      player_talent_t felfire_haste;              // NYI
      player_talent_t master_of_the_glaive;
      player_talent_t rush_of_chaos;
      player_talent_t concentrated_sigils;
      player_talent_t precise_sigils;             // Partial NYI (debuff Sigils)
      player_talent_t lost_in_darkness;           // No Implementation

      player_talent_t chaos_nova;
      player_talent_t soul_rending;
      player_talent_t infernal_armor;
      player_talent_t sigil_of_misery;            // NYI

      player_talent_t chaos_fragments;
      player_talent_t unleashed_power;
      player_talent_t illidari_knowledge;
      player_talent_t demonic;
      player_talent_t first_of_the_illidari;
      player_talent_t will_of_the_illidari;       // NYI Vengeance
      player_talent_t improved_sigil_of_misery;
      player_talent_t misery_in_defeat;           // NYI

      player_talent_t internal_struggle;
      player_talent_t darkness;                   // No Implementation
      player_talent_t soul_sigils;
      player_talent_t aldrachi_design;

      player_talent_t erratic_felheart;
      player_talent_t long_night;                 // No Implementation
      player_talent_t pitch_black;
      player_talent_t the_hunt;
      player_talent_t demon_muzzle;               // NYI Vengeance
      player_talent_t extended_sigils;

      player_talent_t collective_anguish;
      player_talent_t unnatural_malice;
      player_talent_t relentless_pursuit;
      player_talent_t quickened_sigils;

    } demon_hunter;

    struct havoc_talents_t
    {
      player_talent_t eye_beam;

      player_talent_t improved_chaos_strike;
      player_talent_t insatiable_hunger;
      player_talent_t demon_blades;
      player_talent_t felfire_heart;

      player_talent_t demonic_appetite;
      player_talent_t improved_fel_rush;
      player_talent_t first_blood;
      player_talent_t furious_throws;
      player_talent_t burning_hatred;

      player_talent_t critical_chaos;
      player_talent_t mortal_dance;               // No Implementation
      player_talent_t dancing_with_fate;

      player_talent_t initiative;
      player_talent_t desperate_instincts;        // No Implementation
      player_talent_t netherwalk;                 // No Implementation
      player_talent_t chaotic_transformation;
      player_talent_t fel_eruption;
      player_talent_t trail_of_ruin;

      player_talent_t unbound_chaos;
      player_talent_t blind_fury;
      player_talent_t looks_can_kill;
      player_talent_t serrated_glaive;
      player_talent_t growing_inferno;

      player_talent_t tactical_retreat;
      player_talent_t isolated_prey;
      player_talent_t furious_gaze;
      player_talent_t relentless_onslaught;
      player_talent_t burning_wound;

      player_talent_t momentum;
      player_talent_t chaos_theory;
      player_talent_t restless_hunter;
      player_talent_t inner_demon;
      player_talent_t accelerated_blade;
      player_talent_t ragefire;

      player_talent_t know_your_enemy;
      player_talent_t glaive_tempest;
      player_talent_t fel_barrage;
      player_talent_t cycle_of_hatred;
      player_talent_t fodder_to_the_flame;
      player_talent_t elysian_decree;
      player_talent_t soulrend;

      player_talent_t essence_break;
      player_talent_t shattered_destiny;
      player_talent_t any_means_necessary;

    } havoc;

    struct vengeance_talents_t
    {
      player_talent_t fel_devastation;

      player_talent_t frailty;
      player_talent_t fiery_brand;

      player_talent_t perfectly_balanced_glaive;
      player_talent_t deflecting_spikes;          // NYI
      player_talent_t meteoric_strikes;

      player_talent_t shear_fury;
      player_talent_t fracture;
      player_talent_t calcified_spikes;           // NYI
      player_talent_t roaring_fire;               // NYI
      player_talent_t sigil_of_silence;           // NYI
      player_talent_t retaliation;                // NYI
      player_talent_t fel_flame_fortification;    // NYI

      player_talent_t spirit_bomb;
      player_talent_t feast_of_souls;             // NYI
      player_talent_t agonizing_flames;
      player_talent_t extended_spikes;
      player_talent_t burning_blood;
      player_talent_t soul_barrier;               // NYI
      player_talent_t bulk_extraction;            // NYI
      player_talent_t sigil_of_chains;            // NYI

      player_talent_t void_reaver;
      player_talent_t fallout;
      player_talent_t ruinous_bulwark;            // NYI
      player_talent_t volatile_flameblood;
      player_talent_t revel_in_pain;              // NYI

      player_talent_t soul_furnace;
      player_talent_t painbringer;
      player_talent_t darkglare_boon;
      player_talent_t fiery_demise;
      player_talent_t chains_of_anger;

      player_talent_t focused_cleave;
      player_talent_t soulmonger;                 // NYI
      player_talent_t stoke_the_flames;
      player_talent_t burning_alive;              // NYI
      player_talent_t cycle_of_binding;

      player_talent_t vulnerability;
      player_talent_t feed_the_demon;             // NYI
      player_talent_t charred_flesh;              // NYI

      player_talent_t soulcrush;
      player_talent_t soul_carver;
      player_talent_t last_resort;                // NYI
      player_talent_t fodder_to_the_flame;        // NYI
      player_talent_t elysian_decree;
      player_talent_t down_in_flames;

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

    // Havoc
    const spell_data_t* havoc_demon_hunter;
    const spell_data_t* annihilation;
    const spell_data_t* blade_dance;
    const spell_data_t* blur;
    const spell_data_t* chaos_strike;
    const spell_data_t* death_sweep;
    const spell_data_t* demons_bite;
    const spell_data_t* fel_rush;

    const spell_data_t* blade_dance_2;
    const spell_data_t* burning_wound_debuff;
    const spell_data_t* chaos_strike_fury;
    const spell_data_t* chaos_strike_refund;
    const spell_data_t* chaos_theory_buff;
    const spell_data_t* demon_blades_damage;
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
    const spell_data_t* isolated_prey_fury;
    const spell_data_t* momentum_buff;
    const spell_data_t* ragefire_damage;
    const spell_data_t* serrated_glaive_debuff;
    const spell_data_t* soulrend_debuff;
    const spell_data_t* restless_hunter_buff;
    const spell_data_t* tactical_retreat_buff;
    const spell_data_t* unbound_chaos_buff;

    // Vengeance
    const spell_data_t* vengeance_demon_hunter;
    const spell_data_t* demon_spikes;
    const spell_data_t* infernal_strike;
    const spell_data_t* soul_cleave;

    const spell_data_t* demonic_wards_2;
    const spell_data_t* demonic_wards_3;
    const spell_data_t* fiery_brand_debuff;
    const spell_data_t* frailty_debuff;
    const spell_data_t* charred_flesh_buff;
    const spell_data_t* riposte;
    const spell_data_t* soul_cleave_2;
    const spell_data_t* thick_skin;
    const spell_data_t* painbringer_buff;
    const spell_data_t* soul_furnace_damage_amp;
    const spell_data_t* soul_furnace_stack;
    const spell_data_t* immolation_aura_cdr;
    const spell_data_t* soul_fragments_buff;
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
    // Auxilliary
    const spell_data_t* t30_havoc_2pc_buff;
    const spell_data_t* t30_havoc_4pc_refund;
    const spell_data_t* t30_havoc_4pc_buff;
    double t30_havoc_2pc_fury_tracker = 0.0;
    const spell_data_t* t30_vengeance_2pc_buff;
    const spell_data_t* t30_vengeance_4pc_buff;
    double t30_vengeance_4pc_soul_fragments_tracker = 0;
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
    gain_t* isolated_prey;
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
    proc_t* relentless_pursuit;
    proc_t* soul_fragment_greater;
    proc_t* soul_fragment_greater_demon;
    proc_t* soul_fragment_empowered_demon;
    proc_t* soul_fragment_lesser;

    // Havoc
    proc_t* demonic_appetite;
    proc_t* demons_bite_in_meta;
    proc_t* chaos_strike_in_essence_break;
    proc_t* annihilation_in_essence_break;
    proc_t* blade_dance_in_essence_break;
    proc_t* death_sweep_in_essence_break;
    proc_t* chaos_strike_in_serrated_glaive;
    proc_t* annihilation_in_serrated_glaive;
    proc_t* eye_beam_tick_in_serrated_glaive;
    proc_t* felblade_reset;
    proc_t* shattered_destiny;
    proc_t* eye_beam_canceled;

    // Vengeance
    proc_t* soul_fragment_expire;
    proc_t* soul_fragment_overflow;
    proc_t* soul_fragment_from_shear;
    proc_t* soul_fragment_from_fracture;
    proc_t* soul_fragment_from_fallout;
    proc_t* soul_fragment_from_meta;
    proc_t* soul_fragment_from_hunger;

    // Set Bonuses
    proc_t* soul_fragment_from_t29_2pc;
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
    heal_t* consume_soul_greater = nullptr;
    heal_t* consume_soul_lesser = nullptr;
    spell_t* immolation_aura = nullptr;
    spell_t* immolation_aura_initial = nullptr;
    spell_t* collective_anguish = nullptr;

    // Havoc
    spell_t* burning_wound = nullptr;
    attack_t* demon_blades = nullptr;
    spell_t* inner_demon = nullptr;
    spell_t* ragefire = nullptr;
    attack_t* relentless_onslaught = nullptr;
    attack_t* relentless_onslaught_annihilation = nullptr;

    // Vengeance
    spell_t* infernal_armor  = nullptr;
    heal_t* frailty_heal     = nullptr;
    spell_t* fiery_brand_t30 = nullptr;
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
    // Chance to proc initiative off of the fodder demon (ie. not get damaged by it first)
    double fodder_to_the_flame_initiative_chance = 0.85;
    int fodder_to_the_flame_kill_seconds = 4;
    double darkglare_boon_cdr_high_roll_seconds = 18;
    // Chance of souls to be incidentally picked up on any movement ability due to being in pickup range
    double soul_fragment_movement_consume_chance = 0.85;
  } options;

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
  void set_out_of_range( timespan_t duration );
  void adjust_movement();
  double calculate_expected_max_health() const;
  unsigned consume_soul_fragments( soul_fragment = soul_fragment::ANY, bool heal = true, unsigned max = MAX_SOUL_FRAGMENTS );
  unsigned consume_nearby_soul_fragments( soul_fragment = soul_fragment::ANY );
  unsigned get_active_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
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

    auto action = new T( n, this, std::forward<Ts>( args )... );
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

// Movement Buff definition =================================================

struct exit_melee_event_t : public event_t
{
  demon_hunter_t& dh;
  movement_buff_t* trigger_buff;

  exit_melee_event_t( demon_hunter_t* p, timespan_t delay, movement_buff_t* trigger_buff )
    : event_t( *p, delay ), dh( *p ),
    trigger_buff(trigger_buff)
  {
    assert(delay > timespan_t::zero());
  }

  const char* name() const override
  {
    return "exit_melee_event";
  }

  void execute() override
  {
    // Trigger an out of range buff based on the distance to return plus remaining movement aura time
    if (trigger_buff && trigger_buff->yards_from_melee > 0.0)
    {
      const timespan_t base_duration = timespan_t::from_seconds(trigger_buff->yards_from_melee / dh.cache.run_speed());
      dh.set_out_of_range(base_duration + trigger_buff->remains());
    }

    dh.exit_melee_event = nullptr;
  }
};

bool movement_buff_t::trigger( int s, double v, double c, timespan_t d )
{
  assert( distance_moved > 0 );
  assert( buff_duration() > timespan_t::zero() );

  // Check if we're already moving away from the target, if so we will now be moving towards it
  if ( dh->current.distance_to_move || dh->buff.out_of_range->check() ||
       dh->buff.vengeful_retreat_move->check() || dh->buff.metamorphosis_move->check() )
  {
    dh->set_out_of_range(timespan_t::zero());
    yards_from_melee = 0.0;
  }
  // Since we're not yet moving, we should be moving away from the target
  else
  {
    // Calculate the number of yards away from melee this will send us.
    // This is equal to reach + melee range times the direction factor
    // With 2.0 being moving fully "across" the target and 1.0 moving fully "away"
    yards_from_melee = std::max( 0.0, distance_moved - ( ( dh->get_target_reach() + 5.0 ) * dh->options.movement_direction_factor ) );
  }

  if ( yards_from_melee > 0.0 )
  {
    assert(!dh->exit_melee_event);

    // Calculate the amount of time it will take for the movement to carry us out of range
    const timespan_t delay = buff_duration() * (1.0 - (yards_from_melee / distance_moved));

    assert(delay > timespan_t::zero() );

    // Schedule event to set us out of melee.
    dh->exit_melee_event = make_event<exit_melee_event_t>(*sim, dh, delay, this);
  }

  // TODO -- Make this actually inherit from the base movement_buff_t class
  if ( dh->buffs.static_empowerment )
  {
    dh->buffs.static_empowerment->expire();
  }

  return buff_t::trigger( s, v, c, d );
}

// Soul Fragment definition =================================================

struct soul_fragment_t
{
  struct fragment_expiration_t : public event_t
  {
    soul_fragment_t* frag;

    fragment_expiration_t( soul_fragment_t* s )
      : event_t( *s->dh, s->dh->spell.soul_fragment->duration() ), frag( s )
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
                                get_soul_fragment_str( frag->type ),
                                frag->dh->get_active_soul_fragments( frag->type ),
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
      if (sim().debug)
      {
        sim().out_debug.printf( "%s %s becomes active. active=%u total=%u", frag->dh->name(),
                                get_soul_fragment_str( frag->type ),
                                frag->dh->get_active_soul_fragments( frag->type ) + 1,
                                frag->dh->get_total_soul_fragments( frag->type ) );
      }

      frag->activate = nullptr;
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
    if ( activation && consume_on_activation || velocity == 0 )
      return timespan_t::zero();

    // 2023-06-26 -- Recent testing appears to show a roughly fixed 1s activation time
    if ( activation )
      return 1_s;

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
    x = dh->x_position + ( dh->next_fragment_spawn % 2 ? -distance : distance );
    y = dh->y_position + distance;

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
      action_t* consume_action = is_type( soul_fragment::ANY_GREATER ) ?
        dh->active.consume_soul_greater : dh->active.consume_soul_lesser;

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
      timespan_t duration = timespan_t::from_seconds( dh->talent.vengeance.feed_the_demon->effectN( 1 ).base_value() ) / 10;
      dh->cooldown.demon_spikes->adjust( -duration );
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
  demon_hunter_pet_t( sim_t* sim, demon_hunter_t& owner,
                      util::string_view pet_name, pet_e pt,
                      bool guardian = false )
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
      {85, {{0, 453, 883, 353, 159, 225}}},
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

}  // end namespace actions ( for pets )

}  // END pets NAMESPACE

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
    bool direct = false;
    bool periodic = false;
  };

  struct
  {
    // Havoc
    affect_flags any_means_necessary;
    affect_flags demonic_presence;
    bool chaos_theory = false;
    bool essence_break = false;
    bool burning_wound = false;
    bool serrated_glaive = false;

    // Vengeance
    bool frailty = false;
    bool fiery_demise = false;
    bool fires_of_fel = false;
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

  demon_hunter_action_t( util::string_view n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
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
    ab::apply_affecting_aura( p->talent.demon_hunter.rush_of_chaos );
    ab::apply_affecting_aura( p->talent.demon_hunter.precise_sigils );
    ab::apply_affecting_aura( p->talent.demon_hunter.unleashed_power );
    ab::apply_affecting_aura( p->talent.demon_hunter.first_of_the_illidari );
    ab::apply_affecting_aura( p->talent.demon_hunter.improved_sigil_of_misery );
    ab::apply_affecting_aura( p->talent.demon_hunter.erratic_felheart );
    ab::apply_affecting_aura( p->talent.demon_hunter.pitch_black );
    ab::apply_affecting_aura( p->talent.demon_hunter.extended_sigils );
    ab::apply_affecting_aura( p->talent.demon_hunter.quickened_sigils );
    ab::apply_affecting_aura( p->talent.demon_hunter.unnatural_malice );

    ab::apply_affecting_aura( p->talent.havoc.improved_chaos_strike );
    ab::apply_affecting_aura( p->talent.havoc.insatiable_hunger );
    ab::apply_affecting_aura( p->talent.havoc.felfire_heart );
    ab::apply_affecting_aura( p->talent.havoc.improved_fel_rush );
    ab::apply_affecting_aura( p->talent.havoc.blind_fury );
    ab::apply_affecting_aura( p->talent.havoc.looks_can_kill );
    ab::apply_affecting_aura( p->talent.havoc.tactical_retreat );
    ab::apply_affecting_aura( p->talent.havoc.accelerated_blade );
    ab::apply_affecting_aura( p->talent.havoc.any_means_necessary );
    ab::apply_affecting_aura( p->talent.havoc.dancing_with_fate );

    ab::apply_affecting_aura( p->talent.vengeance.perfectly_balanced_glaive );
    ab::apply_affecting_aura( p->talent.vengeance.meteoric_strikes );
    ab::apply_affecting_aura( p->talent.vengeance.burning_blood );
    ab::apply_affecting_aura( p->talent.vengeance.ruinous_bulwark );
    ab::apply_affecting_aura( p->talent.vengeance.chains_of_anger );
    ab::apply_affecting_aura( p->talent.vengeance.stoke_the_flames );
    ab::apply_affecting_aura( p->talent.vengeance.down_in_flames );

    // Rank Passives
    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      // Rank Passives
      ab::apply_affecting_aura( p->spec.immolation_aura_3 );

      // Set Bonus Passives
      ab::apply_affecting_aura( p->set_bonuses.t29_havoc_2pc );

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
    else // DEMON_HUNTER_VENGEANCE
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

    auto register_damage_buff = [this]( damage_buff_t* buff ) {
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
    register_damage_buff( p()->buff.restless_hunter );
    register_damage_buff( p()->buff.t29_havoc_4pc );

    if ( track_cd_waste )
    {
      cd_wasted_exec =
        p()-> template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p()->cd_waste_exec );
      cd_wasted_cumulative = p()-> template get_data_entry<simple_sample_data_with_min_max_t, data_t>(
        ab::name_str, p()->cd_waste_cumulative );
      cd_wasted_iter =
        p()-> template get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, p()->cd_waste_iter );
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

    if ( affected_by.frailty )
    {
      m *= 1.0 + p()->spec.frailty_debuff->effectN( 4 ).percent() * td( target )->debuffs.frailty->check();
    }

    if ( affected_by.fiery_demise && td(target)->dots.fiery_brand->is_ticking() )
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
            p()->buff.metamorphosis->extend_duration( p(), p()->talent.havoc.shattered_destiny->effectN( 1 ).time_value() );
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
          p()->sim->print_debug( "{} procced Seething Fury ({}/{})", *p(), p()->set_bonuses.t30_havoc_2pc_fury_tracker, threshold );
          if ( p()->set_bonuses.t30_havoc_4pc->ok() )
          {
            p()->buff.t30_havoc_4pc->trigger();
            p()->resource_gain( RESOURCE_FURY, p()->set_bonuses.t30_havoc_4pc_refund->effectN( 1 ).base_value(), p()->gain.seething_fury );
          }
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

  void accumulate_ragefire( action_state_t* s )
  {
    if ( !p()->talent.havoc.ragefire->ok() )
      return;

    if ( !( s->result_amount > 0 && s->result == RESULT_CRIT ) )
      return;

    if ( p()->ragefire_crit_accumulator >= p()->talent.havoc.ragefire->effectN( 2 ).base_value() )
      return;

    const double multiplier = p()->talent.havoc.ragefire->effectN( 1 ).percent();
    p()->ragefire_accumulator += s->result_amount * multiplier;
    p()->ragefire_crit_accumulator++;
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
  demon_hunter_heal_t( util::string_view n, demon_hunter_t* p,
                       const spell_data_t* s = spell_data_t::nil(),
                       util::string_view o = {} )
    : base_t( n, p, s, o )
  {
    harmful = false;
    set_target( p );
  }
};

struct demon_hunter_spell_t : public demon_hunter_action_t<spell_t>
{
  demon_hunter_spell_t( util::string_view n, demon_hunter_t* p,
                        const spell_data_t* s = spell_data_t::nil(),
                        util::string_view o = {} )
    : base_t( n, p, s, o )
  {}
};

struct demon_hunter_sigil_t : public demon_hunter_spell_t
{
  timespan_t sigil_delay;
  timespan_t sigil_activates;
  std::vector<cooldown_t*> sigil_cooldowns;
  timespan_t sigil_cooldown_adjust;

  demon_hunter_sigil_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
    : demon_hunter_spell_t( n, p, s ),
    sigil_delay( delay ),
    sigil_activates( timespan_t::zero() )
  {
    aoe = -1;
    background = dual = ground_aoe = true;
    assert( delay > timespan_t::zero() );

    if ( p->talent.vengeance.cycle_of_binding->ok() )
    {
      sigil_cooldowns = { p->cooldown.sigil_of_flame, p->cooldown.elysian_decree, p->cooldown.sigil_of_misery, p->cooldown.sigil_of_silence };
      sigil_cooldown_adjust =
          -timespan_t::from_seconds( p->talent.vengeance.cycle_of_binding->effectN( 1 ).base_value() );
    }
  }

  void place_sigil( player_t* target )
  {
    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( target )
      .x( p()->talent.demon_hunter.concentrated_sigils->ok() ? p()->x_position : target->x_position )
      .y( p()->talent.demon_hunter.concentrated_sigils->ok() ? p()->y_position : target->y_position )
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
      p()->spawn_soul_fragment( soul_fragment::LESSER, num_souls, false);
    }
    if ( hit_any_target && p()->talent.vengeance.cycle_of_binding->ok() )
    {
      std::vector<cooldown_t*> sigils_on_cooldown;
      range::copy_if( sigil_cooldowns, std::back_inserter( sigils_on_cooldown ),
                      []( cooldown_t* c ) { return c->down(); } );
      if ( !sigils_on_cooldown.empty() )
      {
        cooldown_t* sigil_cooldown = sigils_on_cooldown[ static_cast<int>(rng().range( sigils_on_cooldown.size() )) ];
        sigil_cooldown->adjust( sigil_cooldown_adjust );
      }
    }
  }

  std::unique_ptr<expr_t> create_sigil_expression( util::string_view name );
};

struct demon_hunter_attack_t : public demon_hunter_action_t<melee_attack_t>
{
  demon_hunter_attack_t( util::string_view n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
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
      energize_type = action_energize::ON_CAST;
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
    may_miss = false;
    background = true;

    if ( p->talent.havoc.demonic_appetite->ok() )
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
        return std::max( player->resources.max[ RESOURCE_HEALTH ] * vengeance_heal->effectN( 3 ).percent(),
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
  { return 0_s; }
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
    : demon_hunter_action_t( "soul_barrier", p, p->talent.vengeance.soul_barrier, options_str ),
    souls_consumed( 0 )
  {
    // Doesn't get populated from spell data correctly since this is not actually an AP coefficient
    base_dd_min = base_dd_max = 0;
    attack_power_mod.direct = data().effectN( 1 ).percent();
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

  soul_cleave_heal_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_heal_t( name, p, p->spec.soul_cleave )
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
  frailty_heal_t( demon_hunter_t* p )
    : demon_hunter_heal_t( "frailty_heal", p, p->find_spell( 227255 ) )
  {
    background = true;
    may_crit   = false;
  }
};

} // end namespace spells

// ==========================================================================
// Demon Hunter spells
// ==========================================================================

namespace spells
{

// Blur =====================================================================

struct blur_t : public demon_hunter_spell_t
{
  blur_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "blur", p, p->spec.blur, options_str )
  {
    may_miss = false;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p()->buff.blur->trigger();
  }
};

// Bulk Extraction ==========================================================

struct bulk_extraction_t : public demon_hunter_spell_t
{
  bulk_extraction_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "bulk_extraction", p, p->talent.vengeance.bulk_extraction, options_str )
  {
    aoe = -1;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    unsigned num_souls = std::min( execute_state->n_targets, as<unsigned>( data().effectN( 2 ).base_value() ) );
    p()->spawn_soul_fragment( soul_fragment::LESSER, num_souls, true );
  }
};

// Chaos Nova ===============================================================

struct chaos_nova_t : public demon_hunter_spell_t
{
  chaos_nova_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "chaos_nova", p, p->talent.demon_hunter.chaos_nova, options_str )
  {
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( !s->target->is_boss() && p()->talent.demon_hunter.chaos_fragments->ok() )
    {
      if ( p()->rng().roll( data().effectN( 3 ).percent() ) )
      {
        p()->spawn_soul_fragment( soul_fragment::LESSER );
      }
    }
  }
};

// Consume Magic ============================================================

struct consume_magic_t : public demon_hunter_spell_t
{
  consume_magic_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "consume_magic", p, p->talent.demon_hunter.consume_magic, options_str )
  {
    may_miss = false;

    // TOCHECK if this is really needed, probably auto-parses
    if ( p->talent.demon_hunter.swallowed_anger->ok() )
    {
      const spelleffect_data_t& effect = data().effectN( 2 );
      energize_type = action_energize::ON_CAST;
      energize_resource = effect.resource_gain_type();
      energize_amount = effect.resource( energize_resource );
    }
    else
    {
      energize_type = action_energize::NONE;
    }
  }

  bool ready() override
  {
    // Currently no support for magic debuffs on bosses, just return FALSE
    return false;
  }
};

// Demon Spikes =============================================================

struct demon_spikes_t : public demon_hunter_spell_t
{
  demon_spikes_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t("demon_spikes", p, p->spec.demon_spikes, options_str)
  {
    may_miss = harmful = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    p()->buff.demon_spikes->trigger();
  }
};

// Disrupt ==================================================================

struct disrupt_t : public demon_hunter_spell_t
{
  disrupt_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "disrupt", p, p->spell.disrupt, options_str )
  {
    may_miss = false;
    is_interrupt = true;

    if ( p->talent.demon_hunter.disrupting_fury->ok() )
    {
      const spelleffect_data_t& effect = p->talent.demon_hunter.disrupting_fury->effectN( 1 ).trigger()->effectN( 1 );
      energize_type = action_energize::ON_CAST;
      energize_resource = effect.resource_gain_type();
      energize_amount = effect.resource( energize_resource );
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return demon_hunter_spell_t::target_ready( candidate_target );
  }
};

// Eye Beam =================================================================

struct eye_beam_t : public demon_hunter_spell_t
{
  struct eye_beam_tick_t : public demon_hunter_spell_t
  {
    eye_beam_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.eye_beam_damage )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = as<int>( p->talent.havoc.eye_beam->effectN( 5 ).base_value() );
    }

    double action_multiplier() const override
    {
      double m = demon_hunter_spell_t::action_multiplier();

      if ( p()->talent.havoc.isolated_prey->ok() )
      {
        if ( targets_in_range_list( target_list() ).size() == 1 )
        {
          m *= 1.0 + p()->talent.havoc.isolated_prey->effectN( 2 ).percent();
        }
      }

      return m;
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = demon_hunter_spell_t::composite_target_multiplier( target );

      return m;
    }

    double composite_persistent_multiplier( const action_state_t* state ) const override
    {
      double m = demon_hunter_spell_t::composite_persistent_multiplier( state );

      m *= 1.0 + p()->buff.t30_havoc_4pc->stack_value();

      return m;
    }

    timespan_t execute_time() const override
    {
      // Eye Beam is applied via a player aura and experiences aura delay in applying damage tick events
      // Not a perfect implementation, but closer than the instant execution in current sims
      return rng().gauss( p()->sim->default_aura_delay, p()->sim->default_aura_delay_stddev );
    }
  };

  timespan_t trigger_delay;

  eye_beam_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "eye_beam", p, p->talent.havoc.eye_beam, options_str )
  {
    may_miss = false;
    channeled = true;
    tick_on_application = false;

    // 6/6/2020 - Override the lag handling for Eye Beam so that it doesn't use channeled ready behavior
    //            In-game tests have shown it is possible to cast after faster than the 250ms channel_lag using a nochannel macro
    ability_lag         = p->world_lag;
    ability_lag_stddev  = p->world_lag_stddev;

    tick_action = p->get_background_action<eye_beam_tick_t>( "eye_beam_tick" );

    if ( p->active.collective_anguish )
    {
      add_child( p->active.collective_anguish );
    }
  }

  void tick( dot_t* d ) override
  {
    demon_hunter_spell_t::tick( d );

    // Serrated glaive stats tracking
    if ( td( target )->debuffs.serrated_glaive->up() )
      p()->proc.eye_beam_tick_in_serrated_glaive->occur();
  }

  void last_tick( dot_t* d ) override
  {
    demon_hunter_spell_t::last_tick( d );

    // If Eye Beam is canceled early, cancel Blind Fury and skip granting Furious Gaze
    // Collective Anguish is *not* canceled when early canceling Eye Beam, however
    if ( d->current_tick < d->num_ticks() )
    {
      p()->buff.blind_fury->cancel();
      p()->proc.eye_beam_canceled->occur();
    }
    else if ( p()->talent.havoc.furious_gaze->ok() )
    {
      p()->buff.furious_gaze->trigger();
    }
  }

  void execute() override
  {
    // Trigger Meta before the execute so that the channel duration is affected by Meta haste
    p()->trigger_demonic();

    demon_hunter_spell_t::execute();
    timespan_t duration = composite_dot_duration( execute_state );

    p()->buff.t30_havoc_4pc->expire();

    // Since Demonic triggers Meta with 6s + hasted duration, need to extend by the hasted duration after have an execute_state
    if ( p()->talent.demon_hunter.demonic->ok() )
    {
      p()->buff.metamorphosis->extend_duration( p(), duration );
    }

    if ( p()->talent.havoc.blind_fury->ok() )
    {
      p()->buff.blind_fury->trigger( duration );
    }

    // Collective Anguish
    if ( p()->active.collective_anguish )
    {
      p()->active.collective_anguish->set_target( target );
      p()->active.collective_anguish->execute();
    }
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  { return result_amount_type::DMG_DIRECT; }
};

// Fel Barrage ==============================================================

struct fel_barrage_t : public demon_hunter_spell_t
{
  struct fel_barrage_tick_t : public demon_hunter_spell_t
  {
    fel_barrage_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->talent.havoc.fel_barrage->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = as<int>( data().effectN( 2 ).base_value() );
    }
  };

  fel_barrage_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t("fel_barrage", p, p->talent.havoc.fel_barrage, options_str)
  {
    may_miss = false;
    channeled = true;
    tick_action = p->get_background_action<fel_barrage_tick_t>( "fel_barrage_tick" );
  }

  timespan_t composite_dot_duration( const action_state_t* /* s */ ) const override
  {
    // 6/20/2018 -- Channel duration is currently not affected by Haste, although tick rate is
    // DFALPHA TOCHECK -- Is this still true?
    return dot_duration;
  }

  bool usable_moving() const override
  {
    return true;
  }
};

// Fel Devastation ==========================================================

struct fel_devastation_t : public demon_hunter_spell_t
{
  struct fel_devastation_tick_t : public demon_hunter_spell_t
  {
    fel_devastation_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->talent.vengeance.fel_devastation->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = -1;
    }
  };

  heals::fel_devastation_heal_t* heal;

  fel_devastation_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "fel_devastation", p, p->talent.vengeance.fel_devastation, options_str ),
    heal( nullptr )
  {
    may_miss = false;
    channeled = true;

    tick_action = p->get_background_action<fel_devastation_tick_t>( "fel_devastation_tick" );

    /* NYI
    if ( p->spec.fel_devastation_rank_2->ok() )
    {
      heal = p->get_background_action<heals::fel_devastation_heal_t>( "fel_devastation_heal" );
    }
    */

    if ( p->active.collective_anguish )
    {
      add_child( p->active.collective_anguish );
    }
  }

  void execute() override
  {
    p()->trigger_demonic();
    demon_hunter_spell_t::execute();

    // Since Demonic triggers Meta with 6s + hasted duration, need to extend by the hasted duration after have an execute_state
    if ( p()->talent.demon_hunter.demonic->ok() )
    {
      p()->buff.metamorphosis->extend_duration( p(), composite_dot_duration( execute_state ) );
    }

    // Collective Anquish
    if ( p()->active.collective_anguish )
    {
      p()->active.collective_anguish->set_target( target );
      p()->active.collective_anguish->execute();
    }
  }

  void last_tick( dot_t* d ) override
  {
    demon_hunter_spell_t::last_tick( d );

    if ( p()->talent.vengeance.darkglare_boon->ok() )
    {
      // CDR reduction and Fury refund are separate rolls per Realz
      double base_cooldown = p()->talent.vengeance.fel_devastation->cooldown().total_seconds();
      timespan_t minimum_cdr_reduction =
          timespan_t::from_seconds( p()->talent.vengeance.darkglare_boon->effectN( 1 ).percent() * base_cooldown );
      timespan_t maximum_cdr_reduction =
          timespan_t::from_seconds( p()->talent.vengeance.darkglare_boon->effectN( 2 ).percent() * base_cooldown );
      timespan_t cdr_reduction   = rng().range( minimum_cdr_reduction, maximum_cdr_reduction );
      double minimum_fury_refund = p()->talent.vengeance.darkglare_boon->effectN( 3 ).base_value();
      double maximum_fury_refund = p()->talent.vengeance.darkglare_boon->effectN( 4 ).base_value();
      double fury_refund         = rng().range( minimum_fury_refund, maximum_fury_refund );

      p()->darkglare_boon_cdr_roll = cdr_reduction.total_seconds();
      p()->sim->print_debug( "{} rolled {}s of CDR on Fel Devastation from Darkglare Boon", *p(),
                             p()->darkglare_boon_cdr_roll );
      p()->cooldown.fel_devastation->adjust( -cdr_reduction );
      p()->resource_gain( RESOURCE_FURY, fury_refund, p()->gain.darkglare_boon );
    }
  }

  void tick( dot_t* d ) override
  {
    if ( heal )
    {
      heal->execute(); // Heal happens before damage
    }

    demon_hunter_spell_t::tick( d );
  }
};

// Fel Eruption =============================================================

struct fel_eruption_t : public demon_hunter_spell_t
{
  fel_eruption_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "fel_eruption", p, p->talent.havoc.fel_eruption, options_str )
  {
  }
};

// Fiery Brand ==============================================================

struct fiery_brand_t : public demon_hunter_spell_t
{
  struct fiery_brand_state_t : public action_state_t
  {
    bool primary;

    fiery_brand_state_t( action_t* a, player_t* target )
      : action_state_t( a, target ), primary( false )
    {
    }

    void initialize() override
    {
      action_state_t::initialize();
      primary = false;
    }

    void copy_state( const action_state_t* s ) override
    {
      action_state_t::copy_state( s );
      primary = debug_cast<const fiery_brand_state_t*>( s )->primary;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );
      s << " primary=" << primary;
      return s;
    }
  };

  struct fiery_brand_dot_t : public demon_hunter_spell_t
  {
    fiery_brand_dot_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.fiery_brand_debuff )
    {
      background = dual = true;
      dot_behavior = DOT_EXTEND;

      // Spread radius used for Burning Alive.
      if ( p->talent.vengeance.burning_alive->ok() )
      {
        radius = p->find_spell( 207760 )->effectN( 1 ).radius_max();
      }
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
    {
      // Fiery Brand can be extended beyond the 10s duration, but any hardcast Fiery Brand always overwrites existing
      // DoT to 10s.
      if ( triggered_duration == p()->spec.fiery_brand_debuff->duration() )
      {
        return triggered_duration;
      }
      return demon_hunter_spell_t::calculate_dot_refresh_duration( dot, triggered_duration );
    }

    action_state_t* new_state() override
    {
      return new fiery_brand_state_t( this, target );
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( !t ) t = target;
      if ( !t ) return nullptr;

      return td( t )->dots.fiery_brand;
    }

    void tick( dot_t* d ) override
    {
      demon_hunter_spell_t::tick( d );

      trigger_burning_alive( d );
    }

    void trigger_burning_alive( dot_t* d )
    {
      if ( !p()->talent.vengeance.burning_alive->ok() )
        return;

      if ( !debug_cast<fiery_brand_state_t*>( d->state )->primary )
        return;

      // Invalidate and retrieve the new target list
      target_cache.is_valid = false;
      std::vector<player_t*> targets = target_list();
      if ( targets.size() == 1 )
        return;

      // Remove all the targets with existing Fiery Brand DoTs
      auto it = std::remove_if( targets.begin(), targets.end(), [ this ]( player_t* target ) {
        return this->td( target )->dots.fiery_brand->is_ticking();
      } );
      targets.erase( it, targets.end() );

      if ( targets.empty() )
        return;

      // Execute a dot on a random target
      player_t* target = targets[ static_cast<int>( p()->rng().range( 0, static_cast<double>( targets.size() ) ) ) ];
      this->set_target( target );
      this->schedule_execute();
    }
  };

  fiery_brand_dot_t* dot_action;

  fiery_brand_t( util::string_view name, demon_hunter_t* p, util::string_view options_str = {} )
    : demon_hunter_spell_t( name, p, p->talent.vengeance.fiery_brand, options_str ), dot_action( nullptr )
  {
    use_off_gcd = true;

    dot_action        = p->get_background_action<fiery_brand_dot_t>( name_str + "_dot" );
    dot_action->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if ( result_is_miss( s->result ) )
      return;

    if ( p()->talent.vengeance.charred_flesh->ok() )
    {
      p()->buff.charred_flesh->trigger();
    }

    // Trigger the initial DoT action and set the primary flag for use with Burning Alive
    dot_action->set_target( s->target );
    fiery_brand_state_t* fb_state = debug_cast<fiery_brand_state_t*>( dot_action->get_state() );
    dot_action->snapshot_state( fb_state, result_amount_type::DMG_OVER_TIME );
    fb_state->primary = true;
    dot_action->schedule_execute( fb_state );
  }
};

// Glaive Tempest ===========================================================

struct glaive_tempest_t : public demon_hunter_spell_t
{
  struct glaive_tempest_damage_t : public demon_hunter_attack_t
  {
    glaive_tempest_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->spec.glaive_tempest_damage )
    {
      background = dual = ground_aoe = true;
      aoe = -1;
      reduced_aoe_targets = p->talent.havoc.glaive_tempest->effectN( 2 ).base_value();
    }
  };

  glaive_tempest_damage_t* glaive_tempest_mh;
  glaive_tempest_damage_t* glaive_tempest_oh;

  glaive_tempest_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "glaive_tempest", p, p->talent.havoc.glaive_tempest, options_str )
  {
    school = SCHOOL_CHAOS; // Reporting purposes only
    glaive_tempest_mh = p->get_background_action<glaive_tempest_damage_t>( "glaive_tempest_mh" );
    glaive_tempest_oh = p->get_background_action<glaive_tempest_damage_t>( "glaive_tempest_oh" );
    add_child( glaive_tempest_mh );
    add_child( glaive_tempest_oh );
  }

  void make_ground_aoe_event( glaive_tempest_damage_t* action )
  {
    // Has one initial period for 2*(6+1) ticks plus hasted duration and tick rate
    // However, hasted duration is snapshotted on cast, which is likely a bug
    // This allows it to sometimes tick more or less than the intended 14 times
    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( execute_state->target )
      .x( p()->x_position )
      .y( p()->y_position )
      .pulse_time( 500_ms )
      .hasted( ground_aoe_params_t::hasted_with::ATTACK_HASTE )
      .duration( data().duration() * p()->cache.attack_haste() )
      .action( action ), true );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    make_ground_aoe_event( glaive_tempest_mh );
    make_ground_aoe_event( glaive_tempest_oh );
    trigger_cycle_of_hatred();
  }
};

// Sigil of Flame ===========================================================

struct sigil_of_flame_damage_t : public demon_hunter_sigil_t
{
  sigil_of_flame_damage_t( util::string_view name, demon_hunter_t* p, timespan_t delay )
    : demon_hunter_sigil_t( name, p, p->spell.sigil_of_flame_damage, delay )
  {
    if ( p->talent.demon_hunter.flames_of_fury->ok() )
    {
      energize_type = action_energize::ON_HIT;
      energize_resource = RESOURCE_FURY;
      energize_amount = p->talent.demon_hunter.flames_of_fury->effectN( 1 ).resource();
    }
  }

  double action_ta_multiplier() const override
  {
    double am = demon_hunter_sigil_t::action_ta_multiplier();

    // 2023-05-01 -- Sigil of Flame's DoT currently deals twice the intended damage.
    //               There are currently two Apply Aura: Periodic Damage effects in the spell data
    //               (Effects #2 and #3) which could potentially be the reason for this.
    if ( p()->bugs )
      am *= 2.0;

    return am;
  }

  void impact( action_state_t* s ) override
  {
    // Sigil of Flame can apply Frailty if Frailty is talented
    if ( result_is_hit( s->result ) && p()->talent.vengeance.frailty->ok() )
    {
      td( s->target )->debuffs.frailty->trigger();
    }

    demon_hunter_sigil_t::impact( s );
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t ) t = target;
    if ( !t ) return nullptr;
    return td( t )->dots.sigil_of_flame;
  }
};

struct sigil_of_flame_t : public demon_hunter_spell_t
{
  sigil_of_flame_damage_t* sigil;

  sigil_of_flame_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "sigil_of_flame", p, p->talent.demon_hunter.sigil_of_flame, options_str ),
    sigil( nullptr )
  {
    may_miss = false;

    sigil = p->get_background_action<sigil_of_flame_damage_t>( "sigil_of_flame_damage", ground_aoe_duration );
    sigil->stats = stats;

    if ( p->spell.sigil_of_flame_fury->ok() )
    {
      energize_type = action_energize::ON_CAST;
      energize_resource = RESOURCE_FURY;
      energize_amount = p->spell.sigil_of_flame_fury->effectN( 1 ).resource();
    }

    if ( !p->active.frailty_heal )
    {
      p->active.frailty_heal = new heals::frailty_heal_t( p );
    }
    // Add damage modifiers in sigil_of_flame_damage_t, not here.
  }

  void init_finished() override
  {
    demon_hunter_spell_t::init_finished();
    harmful = !is_precombat; // Do not count towards the precombat hostile action limit
  }

  bool usable_precombat() const override
  {
    return true; // Has virtual travel time due to Sigil activation delay
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( sigil )
    {
      sigil->place_sigil( execute_state->target );
    }
  }

  std::unique_ptr<expr_t> create_expression(util::string_view name) override
  {
    if ( auto e = sigil->create_sigil_expression( name ) )
      return e;

    return demon_hunter_spell_t::create_expression(name);
  }

  bool verify_actor_spec() const override
  {
    // Spell data still has a Vengeance Demon Hunter class association
    return p()->talent.demon_hunter.sigil_of_flame->ok();
  }
};

// Collective Anguish =======================================================

struct collective_anguish_t : public demon_hunter_spell_t
{
  struct collective_anguish_tick_t : public demon_hunter_spell_t
  {
    collective_anguish_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spell.collective_anguish_damage )
    {
      // TOCHECK: Currently does not use split damage on beta but probably will at some point
      dual = true;
      aoe = -1;
    }

    double composite_crit_chance() const override
    {
      // Appears to have a hard-coded 100% spell crit rate on the Eye Beam cast not in data
      if ( p()->specialization() == DEMON_HUNTER_VENGEANCE )
        return 1.0;

      return demon_hunter_spell_t::composite_crit_chance();
    }
  };

  collective_anguish_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_spell_t( name, p, p->spell.collective_anguish )
  {
    may_miss = channeled = false;
    dual = true;

    tick_action = p->get_background_action<collective_anguish_tick_t>( "collective_anguish_tick" );
  }

  // Behaves as a channeled spell, although we can't set channeled = true since it is background
  void init() override
  {
    demon_hunter_spell_t::init();

    // Channeled dots get haste snapshotted in action_t::init() and we replicate that here
    update_flags &= ~STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    // Replicate channeled action_t::composite_dot_duration() here
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }
};

// Infernal Strike ==========================================================

struct infernal_strike_t : public demon_hunter_spell_t
{
  struct infernal_strike_impact_t : public demon_hunter_spell_t
  {
    sigil_of_flame_damage_t* sigil;

    infernal_strike_impact_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 189112 ) ),
      sigil( nullptr )
    {
      background = dual = true;
      aoe = -1;

      /* NYI
      if ( p->talent.abyssal_strike->ok() )
      {
        timespan_t sigil_delay = p->talent.demon_hunter.sigil_of_flame->duration() -
          p->talent.demon_hunter.quickened_sigils->effectN( 1 ).time_value();
        sigil = p->get_background_action<sigil_of_flame_damage_t>( "abyssal_strike", sigil_delay );
      }
      */
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      if ( sigil )
      {
        sigil->place_sigil( execute_state->target );
      }
    }
  };

  infernal_strike_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "infernal_strike", p, p->spec.infernal_strike, options_str )
  {
    may_miss = false;
    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;
    travel_speed = 1.0;  // allows use precombat

    impact_action = p->get_background_action<infernal_strike_impact_t>( "infernal_strike_impact" );
    impact_action->stats = stats;
  }

  // leap travel time, independent of distance
  timespan_t travel_time() const override
  { return 1_s; }
};

// Immolation Aura ==========================================================

struct immolation_aura_t : public demon_hunter_spell_t
{
  struct infernal_armor_damage_t : public demon_hunter_spell_t
  {
    infernal_armor_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spell.infernal_armor_damage )
    {
    }
  };

  struct immolation_aura_damage_t : public demon_hunter_spell_t
  {
    bool initial;

    immolation_aura_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, bool initial )
      : demon_hunter_spell_t( name, p, s ), initial( initial )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = p->spell.immolation_aura->effectN( 2 ).base_value();

      // Rename gain for periodic energizes. Initial hit action doesn't energize.
      // Gains are encoded in the 258922 spell data differently for Havoc vs. Vengeance
      gain = p->get_gain( "immolation_aura_tick" );
      if ( !initial )
      {
        if ( p->specialization() == DEMON_HUNTER_VENGEANCE )
        {
          energize_amount = data().effectN( 3 ).base_value();
        }
        else // DEMON_HUNTER_HAVOC
        {
          energize_amount = p->talent.havoc.burning_hatred->ok() ? data().effectN( 2 ).base_value() : 0;
        }
      }
    }

    double action_multiplier() const override
    {
      double am = demon_hunter_spell_t::action_multiplier();

      am *= 1.0 + p()->buff.growing_inferno->stack_value();

      return am;
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        if ( initial && p()->talent.vengeance.fallout->ok() && rng().roll( 0.60 ) )
        {
          p()->spawn_soul_fragment( soul_fragment::LESSER, 1 );
          p()->proc.soul_fragment_from_fallout->occur();
        }

        if ( p()->talent.vengeance.charred_flesh->ok() && p()->buff.charred_flesh->up() )
        {
          demon_hunter_td_t* target_data = td( s->target );
          if ( target_data->dots.fiery_brand->is_ticking() )
          {
            target_data->dots.fiery_brand->adjust_duration(
                p()->talent.vengeance.charred_flesh->effectN( 1 ).time_value() );
          }
        }

        if ( s->result == RESULT_CRIT && p()->talent.vengeance.volatile_flameblood->ok() &&
             p()->cooldown.volatile_flameblood_icd->up() )
        {
          p()->resource_gain( RESOURCE_FURY, p()->talent.vengeance.volatile_flameblood->effectN( 1 ).base_value(),
                              p()->talent.vengeance.volatile_flameblood->effectN( 1 ).m_delta(),
                              p()->gain.volatile_flameblood );
          p()->cooldown.volatile_flameblood_icd->start(
              p()->talent.vengeance.volatile_flameblood->internal_cooldown() );
        }

        accumulate_ragefire( s );
      }
    }

    result_amount_type amount_type( const action_state_t*, bool ) const override
    {
      return initial ? result_amount_type::DMG_DIRECT : result_amount_type::DMG_OVER_TIME;
    }
  };

  immolation_aura_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "immolation_aura", p, p->spell.immolation_aura, options_str )
  {
    may_miss = false;
    dot_duration = timespan_t::zero();

    apply_affecting_aura(p->spec.immolation_aura_cdr);

    if ( !p->active.immolation_aura )
    {
      p->active.immolation_aura = p->get_background_action<immolation_aura_damage_t>(
        "immolation_aura_tick", data().effectN( 1 ).trigger(), false );
      p->active.immolation_aura->stats = stats;
    }

    if ( !p->active.immolation_aura_initial && p->spell.immolation_aura_damage->ok() )
    {
      p->active.immolation_aura_initial = p->get_background_action<immolation_aura_damage_t>(
        "immolation_aura_initial", p->spell.immolation_aura_damage, true );
      p->active.immolation_aura_initial->stats = stats;
    }

    if ( p->talent.demon_hunter.infernal_armor->ok() && !p->active.infernal_armor )
    {
      p->active.infernal_armor = p->get_background_action<infernal_armor_damage_t>( "infernal_armor" );
      add_child( p->active.infernal_armor );
    }

    if ( p->talent.havoc.ragefire->ok() )
    {
      add_child( p->active.ragefire );
    }

    // Add damage modifiers in immolation_aura_damage_t, not here.
  }

  bool usable_precombat() const override
  {
    return true; // Disables initial hit if used at time 0
  }

  void execute() override
  {
    p()->buff.immolation_aura->trigger();
    demon_hunter_spell_t::execute();
  }
};

// Metamorphosis ============================================================

struct metamorphosis_t : public demon_hunter_spell_t
{
  struct metamorphosis_impact_t : public demon_hunter_spell_t
  {
    metamorphosis_impact_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 200166 ) )
    {
      background = dual = true;
      aoe = -1;
    }
  };

  double landing_distance;
  timespan_t gcd_lag;

  metamorphosis_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "metamorphosis", p, p->spec.metamorphosis ),
    landing_distance( 0.0 )
  {
    add_option( opt_float( "landing_distance", landing_distance, 0.0, 40.0 ) );
    parse_options( options_str );

    may_miss = false;
    dot_duration = timespan_t::zero();

    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      base_teleport_distance  = data().max_range();
      movement_directionality = movement_direction_type::OMNI;
      min_gcd                 = 1_s;  // Cannot use skills during travel time, adjusted below
      travel_speed            = 1.0;  // Allows use in the precombat list

      // If we are landing outside of the impact radius, we don't need to assign the impact spell
      if ( landing_distance < 8.0 )
      {
        impact_action = p->get_background_action<metamorphosis_impact_t>( "metamorphosis_impact" );
      }

      // Don't assign the stats here because we don't want Meta to show up in the DPET chart
    }
    else // DEMON_HUNTER_VENGEANCE
    {
    }
  }

  // Meta leap travel time and self-pacify is a 1s hidden aura (201453) regardless of distance
  // This is affected by aura lag and will slightly delay execution of follow-up attacks
  // Not always relevant as GCD can be longer than the 1s + lag ability delay outside of lust
  void schedule_execute( action_state_t* s ) override
  {
    gcd_lag = rng().gauss( sim->gcd_lag, sim->gcd_lag_stddev );
    min_gcd = 1_s + gcd_lag;
    demon_hunter_spell_t::schedule_execute( s );
  }

  timespan_t travel_time() const override
  {
    if ( p()->specialization() == DEMON_HUNTER_HAVOC )
      return min_gcd;
    else // DEMON_HUNTER_VENGEANCE
      return timespan_t::zero();
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( p()->specialization() == DEMON_HUNTER_HAVOC )
    {
      // 2023-01-31 -- Metamorphosis's "extension" mechanic technically fades and reapplies the buff
      //               This means it (probably inadvertently) triggers Restless Hunter
      if ( p()->talent.havoc.restless_hunter->ok() && p()->buff.metamorphosis->check() )
      {
        p()->cooldown.fel_rush->reset( false, 1 );
        p()->buff.restless_hunter->trigger();
      }

      // Buff is gained at the start of the leap.
      p()->buff.metamorphosis->extend_duration_or_trigger();
      p()->buff.inner_demon->trigger();

      if ( p()->talent.havoc.chaotic_transformation->ok() )
      {
        p()->cooldown.eye_beam->reset( false );
        p()->cooldown.blade_dance->reset( false );
      }

      // Cancel all previous movement events, as Metamorphosis is ground-targeted
      // If we are landing outside of point-blank range, trigger the movement buff
      p()->set_out_of_range( timespan_t::zero() ); 
      if ( landing_distance > 0.0 )
      {
        p()->buff.metamorphosis_move->distance_moved = landing_distance;
        p()->buff.metamorphosis_move->trigger();
      }
    }
    else // DEMON_HUNTER_VENGEANCE
    {
      p()->buff.metamorphosis->trigger();
    }
  }

  bool ready() override
  {
    // Not usable during the root effect of Stormeater's Boon
    if ( p()->buffs.stormeaters_boon && p()->buffs.stormeaters_boon->check() )
      return false;

    return demon_hunter_spell_t::ready();
  }
};

// Pick up Soul Fragment ====================================================

struct pick_up_fragment_t : public demon_hunter_spell_t
{
  enum class soul_fragment_select_mode
  {
    NEAREST,
    NEWEST,
    OLDEST,
  };

  struct pick_up_event_t : public event_t
  {
    demon_hunter_t* dh;
    soul_fragment_t* frag;
    expr_t* expr;

    pick_up_event_t( soul_fragment_t* f, timespan_t time, expr_t* e )
      : event_t( *f->dh, time ),
      dh( f->dh ),
      frag( f ),
      expr( e )
    {
    }

    const char* name() const override
    {
      return "Soul Fragment pick up";
    }

    void execute() override
    {
      // Evaluate if_expr to make sure the actor still wants to consume.
      if ( frag && frag->active() && ( !expr || expr->eval() ) && dh->active.consume_soul_greater )
      {
        frag->consume( true );
      }

      dh->soul_fragment_pick_up = nullptr;
    }
  };

  std::vector<soul_fragment_t*>::iterator it;
  soul_fragment_select_mode select_mode;
  soul_fragment type;

  pick_up_fragment_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "pick_up_fragment", p, spell_data_t::nil() ),
      select_mode( soul_fragment_select_mode::OLDEST ),
      type( soul_fragment::ANY )
  {
    std::string mode_str, type_str;
    add_option( opt_string( "mode", mode_str ) );
    add_option( opt_string( "type", type_str ) );

    parse_options( options_str );
    parse_mode( mode_str );
    parse_type( type_str );

    trigger_gcd = timespan_t::zero();
    // use_off_gcd = true;
    may_miss = callbacks = harmful = false;
    range = 5.0;  // Disallow use outside of melee.
  }

  void parse_mode( util::string_view value )
  {
    if ( value == "close" || value == "near" || value == "closest" || value == "nearest" )
    {
      select_mode = soul_fragment_select_mode::NEAREST;
    }
    else if ( value == "new" || value == "newest" )
    {
      select_mode = soul_fragment_select_mode::NEWEST;
    }
    else if ( value == "old" || value == "oldest" )
    {
      select_mode = soul_fragment_select_mode::OLDEST;
    }
    else if ( !value.empty() )
    {
      sim->errorf(
        "%s uses bad parameter for pick_up_soul_fragment option "
        "\"mode\". Valid options: closest, newest, oldest",
        sim->active_player->name() );
    }
  }

  void parse_type( util::string_view value )
  {
    if ( value == "greater" )
    {
      type = soul_fragment::ANY_GREATER;
    }
    else if ( value == "lesser" )
    {
      type = soul_fragment::LESSER;
    }
    else if ( value == "all" || value == "any" )
    {
      type = soul_fragment::ANY;
    }
    else if ( value == "demon" )
    {
      type = soul_fragment::ANY_DEMON;
    }
    else if ( !value.empty() )
    {
      sim->errorf(
        "%s uses bad parameter for pick_up_soul_fragment option "
        "\"type\". Valid options: greater, lesser, any",
        sim->active_player->name() );
    }
  }

  timespan_t calculate_movement_time( soul_fragment_t* frag )
  {
    // Fragments have a 6 yard trigger radius
    double dtm = std::max( 0.0, frag->get_distance( p() ) - 6.0 );
    timespan_t mt = timespan_t::from_seconds( dtm / p()->cache.run_speed() );
    return mt;
  }

  soul_fragment_t* select_fragment()
  {
    soul_fragment_t* candidate = nullptr;
    timespan_t candidate_value;

    switch ( select_mode )
    {
      case soul_fragment_select_mode::NEAREST:
        candidate_value = timespan_t::max();
        for ( auto frag : p()->soul_fragments )
        {
          timespan_t movement_time = calculate_movement_time( frag );
          if ( frag->is_type( type ) && frag->active() && frag->remains() > movement_time )
          {
            if ( movement_time < candidate_value )
            {
              candidate_value = movement_time;
              candidate = frag;
            }
          }
        }

        break;
      case soul_fragment_select_mode::NEWEST:
        candidate_value = timespan_t::min();
        for ( auto frag : p()->soul_fragments )
        {
          timespan_t remains = frag->remains();
          if ( frag->is_type( type ) && frag->active() && remains > calculate_movement_time( frag ) )
          {
            if ( remains > candidate_value )
            {
              candidate_value = remains;
              candidate = frag;
            }
          }
        }

        break;
      case soul_fragment_select_mode::OLDEST:
      default:
        candidate_value = timespan_t::max();
        for ( auto frag : p()->soul_fragments )
        {
          timespan_t remains = frag->remains();
          if ( frag->is_type( type ) && frag->active() && remains > calculate_movement_time( frag ) )
          {
            if ( remains < candidate_value )
            {
              candidate_value = remains;
              candidate = frag;
            }
          }
        }

        break;
    }

    return candidate;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    soul_fragment_t* frag = select_fragment();
    assert( frag );
    timespan_t time = calculate_movement_time( frag );

    assert( p()->soul_fragment_pick_up == nullptr );
    p()->soul_fragment_pick_up = make_event<pick_up_event_t>( *sim, frag, time, if_expr.get() );
  }

  bool ready() override
  {
    if ( p()->soul_fragment_pick_up )
    {
      return false;
    }

    if ( !p()->get_active_soul_fragments( type ) )
    {
      return false;
    }

    if ( !demon_hunter_spell_t::ready() )
    {
      return false;
    }

    // Not usable during the root effect of Stormeater's Boon
    if ( p()->buffs.stormeaters_boon && p()->buffs.stormeaters_boon->check() )
      return false;

    // Catch edge case where a fragment exists but we can't pick it up in time.
    return select_fragment() != nullptr;
  }
};

// Spirit Bomb ==============================================================

struct spirit_bomb_t : public demon_hunter_spell_t
{
  struct spirit_bomb_state_t : public action_state_t
  {
    bool t29_vengeance_4pc_proc;

    spirit_bomb_state_t(action_t* a, player_t* target): action_state_t(a, target), t29_vengeance_4pc_proc(false)
    {
    }

    void initialize() override
    {
      action_state_t::initialize();
      t29_vengeance_4pc_proc = false;
    }

    void copy_state(const action_state_t* s) override
    {
      action_state_t::copy_state(s);
      t29_vengeance_4pc_proc = debug_cast<const spirit_bomb_state_t*>(s)->t29_vengeance_4pc_proc;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );
      s << " t29_vengeance_4pc_proc=" << t29_vengeance_4pc_proc;
      return s;
    }
  };

  struct spirit_bomb_damage_t : public demon_hunter_spell_t
  {
    spirit_bomb_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 247455 ) )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = p->talent.vengeance.spirit_bomb->effectN( 2 ).base_value();
    }

    action_state_t* new_state() override
    {
      return new spirit_bomb_state_t(this, target);
    }

    void snapshot_state( action_state_t* state, result_amount_type rt ) override
    {
      if ( p()->set_bonuses.t29_vengeance_4pc->ok() &&
           rng().roll( p()->set_bonuses.t29_vengeance_4pc->effectN( 2 ).percent() ) )
      {
        debug_cast<spirit_bomb_state_t*>( state )->t29_vengeance_4pc_proc = true;
      }
      // snapshot_state after so that it includes the DA multiplier from the proc
      demon_hunter_spell_t::snapshot_state( state, rt );
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      p()->buff.soul_furnace_damage_amp->expire();
    }

    void impact(action_state_t* s) override
    {
      demon_hunter_spell_t::impact(s);

      if ( result_is_hit( s->result ) )
      {
        td( s->target )->debuffs.frailty->trigger();
        if ( debug_cast<spirit_bomb_state_t*>( s )->t29_vengeance_4pc_proc )
        {
          td( s->target )->debuffs.t29_vengeance_4pc->trigger();
        }
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = demon_hunter_spell_t::composite_da_multiplier( s );

      if ( p()->buff.soul_furnace_damage_amp->up() )
      {
        m *= 1.0 + p()->buff.soul_furnace_damage_amp->check_value();
      }

      if ( debug_cast<const spirit_bomb_state_t*>( s )->t29_vengeance_4pc_proc )
      {
        m *= 1.0 + p()->set_bonuses.t29_vengeance_4pc->effectN( 1 ).percent();
      }

      return m;
    }
  };

  spirit_bomb_damage_t* damage;
  unsigned max_fragments_consumed;

  spirit_bomb_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "spirit_bomb", p, p->talent.vengeance.spirit_bomb, options_str ),
    max_fragments_consumed( static_cast<unsigned>( data().effectN( 2 ).base_value() ) )
  {
    may_miss = proc = callbacks = false;

    damage = p->get_background_action<spirit_bomb_damage_t>( "spirit_bomb_damage" );
    damage->stats = stats;

    if ( !p->active.frailty_heal )
    {
      p->active.frailty_heal = new heals::frailty_heal_t( p );
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    // Soul fragments consumed are capped for Spirit Bomb
    const int fragments_consumed = p()->consume_soul_fragments( soul_fragment::ANY, true, max_fragments_consumed );
    if ( fragments_consumed > 0 )
    {
      damage->set_target( target );
      action_state_t* damage_state = damage->get_state();
      damage->snapshot_state( damage_state, result_amount_type::DMG_DIRECT );
      damage_state->da_multiplier *= fragments_consumed;
      damage->schedule_execute( damage_state );
      damage->execute_event->reschedule( timespan_t::from_seconds( 1.0 ) );
    }
  }

  bool ready() override
  {
    if ( p()->get_active_soul_fragments() < 1 )
      return false;

    return demon_hunter_spell_t::ready();
  }
};

// Elysian Decree ===========================================================

struct elysian_decree_t : public demon_hunter_spell_t
{
  struct elysian_decree_sigil_t : public demon_hunter_sigil_t
  {
    elysian_decree_sigil_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
      : demon_hunter_sigil_t( name, p, s, delay )
    {
      reduced_aoe_targets = p->spell.elysian_decree->effectN( 1 ).base_value();
    }

    void execute() override
    {
      demon_hunter_sigil_t::execute();
      p()->spawn_soul_fragment( soul_fragment::LESSER, 3 );
    }
  };

  elysian_decree_sigil_t* sigil;
  elysian_decree_sigil_t* repeat_decree_sigil;

  elysian_decree_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "elysian_decree", p, p->spell.elysian_decree, options_str ),
    sigil( nullptr ), repeat_decree_sigil( nullptr )
  {
    if ( p->spell.elysian_decree->ok() )
    {
      sigil = p->get_background_action<elysian_decree_sigil_t>( "elysian_decree_sigil", p->spell.elysian_decree_damage, ground_aoe_duration );
      sigil->stats = stats;
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    sigil->place_sigil( target );
    if ( repeat_decree_sigil )
      repeat_decree_sigil->place_sigil( target );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( sigil )
    {
      if ( auto e = sigil->create_sigil_expression( name ) )
        return e;
    }

    return demon_hunter_spell_t::create_expression( name );
  }
};

// Fodder to the Flame ======================================================

struct fodder_to_the_flame_cb_t : public dbc_proc_callback_t
{
  struct fodder_to_the_flame_damage_t : public demon_hunter_spell_t
  {
    fodder_to_the_flame_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spell.fodder_to_the_flame_damage )
    {
      background = true;
      aoe = -1;
      reduced_aoe_targets = p->spell.fodder_to_the_flame->effectN( 1 ).base_value();
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      // Simulate triggering initiative from hitting the demon
      if ( p()->talent.havoc.initiative->ok() && p()->rng().roll( p()->options.fodder_to_the_flame_initiative_chance ) )
      {
        p()->buff.initiative->trigger();
      }

      p()->spawn_soul_fragment( soul_fragment::EMPOWERED_DEMON );
    }
  };

  // Dummy trigger spell in order to trigger cast callbacks
  struct fodder_to_the_flame_spawn_trigger_t : public demon_hunter_spell_t
  {
    struct fodder_to_the_flame_state_t : public action_state_t
    {
      fodder_to_the_flame_state_t( action_t* a, player_t* target ) : action_state_t( a, target ) {}

      proc_types2 cast_proc_type2() const override
      { return PROC2_CAST; }
    };

    fodder_to_the_flame_spawn_trigger_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spell.fodder_to_the_flame )
    {
      quiet = true;
    }

    proc_types proc_type() const override
    { return PROC1_NONE_SPELL; }

    action_state_t* new_state() override
    { return new fodder_to_the_flame_state_t( this, target ); }
  };

  fodder_to_the_flame_damage_t* damage;
  fodder_to_the_flame_spawn_trigger_t* spawn_trigger;
  timespan_t trigger_delay;
  demon_hunter_t* dh;

  fodder_to_the_flame_cb_t( demon_hunter_t* p, const special_effect_t& e )
    : dbc_proc_callback_t( p, e ), dh( p )
  {
    damage = dh->get_background_action<fodder_to_the_flame_damage_t>( "fodder_to_the_flame" );
    spawn_trigger = dh->get_background_action<fodder_to_the_flame_spawn_trigger_t>( "fodder_to_the_flame_spawn" );
    trigger_delay = timespan_t::from_seconds( p->options.fodder_to_the_flame_kill_seconds );
  }

  void execute( action_t* a, action_state_t* s ) override
  {
    dbc_proc_callback_t::execute( a, s );
    make_event<delayed_execute_event_t>( *dh->sim, dh, damage, s->target, trigger_delay );
    spawn_trigger->execute();
  }
};

// The Hunt =================================================================

struct the_hunt_t : public demon_hunter_spell_t
{
  struct the_hunt_damage_t : public demon_hunter_spell_t
  {
    struct the_hunt_dot_t : public demon_hunter_spell_t
    {
      the_hunt_dot_t( util::string_view name, demon_hunter_t* p )
        : demon_hunter_spell_t( name, p, p->spell.the_hunt->effectN( 1 ).trigger()
                                          ->effectN( 4 ).trigger() )
      {
        dual = true;
        aoe = as<int>( p->spell.the_hunt->effectN( 2 ).trigger()->effectN( 1 ).base_value() );
      }
    };

    the_hunt_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spell.the_hunt->effectN( 1 ).trigger() )
    {
      dual = true;
      impact_action = p->get_background_action<the_hunt_dot_t>( "the_hunt_dot" );
    }
  };

  the_hunt_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "the_hunt", p, p->spell.the_hunt, options_str )
  {
    movement_directionality = movement_direction_type::TOWARDS;
    impact_action = p->get_background_action<the_hunt_damage_t>( "the_hunt_damage" );
    impact_action->stats = stats;
    impact_action->impact_action->stats = stats;
  }

  void execute() override
  {
    p()->buff.momentum->trigger();

    demon_hunter_spell_t::execute();

    p()->set_out_of_range( timespan_t::zero() ); // Cancel all other movement

    p()->consume_nearby_soul_fragments( soul_fragment::LESSER );
  }

  timespan_t travel_time() const override
  { return 100_ms; }

  // Bypass the normal demon_hunter_spell_t out of range and movement ready checks
  bool ready() override
  {
    // Not usable during the root effect of Stormeater's Boon
    if ( p()->buffs.stormeaters_boon && p()->buffs.stormeaters_boon->check() )
      return false;

    return spell_t::ready();
  }
};

}  // end namespace spells

// ==========================================================================
// Demon Hunter attacks
// ==========================================================================

namespace attacks
{

// Auto Attack ==============================================================

  struct auto_attack_damage_t : public demon_hunter_attack_t
  {
    enum class aa_contact
    {
      MELEE,
      LOST_CHANNEL,
      LOST_RANGE,
    };

    struct status_t
    {
      aa_contact main_hand, off_hand;
    } status;

    auto_attack_damage_t( util::string_view name, demon_hunter_t* p, weapon_t* w, const spell_data_t* s = spell_data_t::nil() )
      : demon_hunter_attack_t( name, p, s )
    {
      school = SCHOOL_PHYSICAL;
      special = false;
      background = repeating = may_glance = may_crit = true;
      allow_class_ability_procs = not_a_proc = true;
      trigger_gcd = timespan_t::zero();
      weapon = w;
      weapon_multiplier = 1.0;
      base_execute_time = weapon->swing_time;

      status.main_hand = status.off_hand = aa_contact::LOST_RANGE;

      if ( p->dual_wield() )
      {
        base_hit -= 0.19;
      }
    }

    double action_multiplier() const override
    {
      double m = demon_hunter_attack_t::action_multiplier();

      // Registered Damage Buffs
      for ( auto damage_buff : auto_attack_damage_buffs )
        m *= damage_buff->stack_value_auto_attack();

      // Class Passive
      m *= 1.0 + p()->spec.havoc_demon_hunter->effectN( 8 ).percent();

      return m;
    }

    void reset() override
    {
      demon_hunter_attack_t::reset();

      status.main_hand = status.off_hand = aa_contact::LOST_RANGE;
    }

    timespan_t execute_time() const override
    {
      aa_contact c = weapon->slot == SLOT_MAIN_HAND ? status.main_hand : status.off_hand;

      switch ( c )
      {
        // Start 500ms polling for being "back in range".
        case aa_contact::LOST_CHANNEL:
        case aa_contact::LOST_RANGE:
          return timespan_t::from_millis( 500 );
        default:
          return demon_hunter_attack_t::execute_time();
      }
    }

    void trigger_demon_blades( action_state_t* s )
    {
      if ( !p()->talent.havoc.demon_blades->ok() )
        return;

      if ( !result_is_hit( s->result ) )
        return;

      if ( !rng().roll( p()->talent.havoc.demon_blades->effectN( 1 ).percent() ) )
        return;

      p()->active.demon_blades->set_target( s->target );
      p()->active.demon_blades->execute();
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      trigger_demon_blades( s );
    }

    void schedule_execute( action_state_t* s ) override
    {
      demon_hunter_attack_t::schedule_execute( s );

      if ( weapon->slot == SLOT_MAIN_HAND )
        status.main_hand = aa_contact::MELEE;
      else if ( weapon->slot == SLOT_OFF_HAND )
        status.off_hand = aa_contact::MELEE;
    }

    void execute() override
    {
      if ( p()->current.distance_to_move > 5 || p()->buff.out_of_range->check() )
      {
        aa_contact c = aa_contact::LOST_RANGE;
        p()->proc.delayed_aa_range->occur();

        if ( weapon->slot == SLOT_MAIN_HAND )
        {
          status.main_hand = c;
          player->main_hand_attack->cancel();
        }
        else
        {
          status.off_hand = c;
          player->off_hand_attack->cancel();
        }
        return;
      }

      demon_hunter_attack_t::execute();
    }
  };

struct auto_attack_t : public demon_hunter_attack_t
{
  auto_attack_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "auto_attack", p, spell_data_t::nil(), options_str )
  {
    range                   = 5;
    trigger_gcd             = timespan_t::zero();

    p->melee_main_hand      = new auto_attack_damage_t( "auto_attack_mh", p, &( p->main_hand_weapon ) );
    p->main_hand_attack     = p->melee_main_hand;

    p->melee_off_hand       = new auto_attack_damage_t( "auto_attack_oh", p, &( p->off_hand_weapon ) );
    p->off_hand_attack      = p->melee_off_hand;
    p->off_hand_attack->id  = 1;
  }

  void execute() override
  {
    p()->main_hand_attack->set_target( target );
    if ( p()->main_hand_attack->execute_event == nullptr )
    {
      p()->main_hand_attack->schedule_execute();
    }

    p()->off_hand_attack->set_target( target );
    if ( p()->off_hand_attack->execute_event == nullptr )
    {
      p()->off_hand_attack->schedule_execute();
    }
  }

  bool ready() override
  {
    // Range check
    if ( !demon_hunter_attack_t::ready() )
      return false;

    if ( p()->main_hand_attack->execute_event == nullptr || p()->off_hand_attack->execute_event == nullptr )
      return true;

    return false;
  }
};

// Blade Dance =============================================================

struct blade_dance_base_t : public demon_hunter_attack_t
{
  struct trail_of_ruin_dot_t : public demon_hunter_spell_t
  {
    trail_of_ruin_dot_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->talent.havoc.trail_of_ruin->effectN( 1 ).trigger() )
    {
      background = dual = true;
    }
  };

  struct blade_dance_damage_t : public demon_hunter_attack_t
  {
    timespan_t delay;
    action_t* trail_of_ruin_dot;
    bool last_attack;
    bool from_first_blood;

    blade_dance_damage_t( util::string_view name, demon_hunter_t* p, const spelleffect_data_t& eff,
                          const spell_data_t* first_blood_override = nullptr  )
      : demon_hunter_attack_t( name, p, first_blood_override ? first_blood_override : eff.trigger() ),
      delay( timespan_t::from_millis( eff.misc_value1() ) ),
      trail_of_ruin_dot( nullptr ),
      last_attack( false ),
      from_first_blood( first_blood_override != nullptr )
    {
      background = dual = true;
      aoe = ( from_first_blood ) ? 0 : -1;
      reduced_aoe_targets = p->find_spell( 199552 )->effectN( 1 ).base_value(); // Use first impact spell
    }

    size_t available_targets( std::vector< player_t* >& tl ) const override
    {
      demon_hunter_attack_t::available_targets( tl );

      if ( p()->talent.havoc.first_blood->ok() && !from_first_blood )
      {
        // Ensure the non-First Blood AoE spell doesn't hit the primary target
        tl.erase( std::remove_if( tl.begin(), tl.end(), [this]( player_t* t ) {
          return t == this->target;
        } ), tl.end() );
      }

      return tl.size();
    }

    double action_multiplier() const override
    {
      double am = demon_hunter_attack_t::action_multiplier();

      if ( from_first_blood )
      {
        am *= 1.0 + p()->talent.havoc.first_blood->effectN( 1 ).percent();
      }

      return am;
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      if ( last_attack )
      {
        if ( trail_of_ruin_dot )
        {
          trail_of_ruin_dot->set_target( s->target );
          trail_of_ruin_dot->execute();
        }
      }

      if ( p()->set_bonuses.t29_havoc_4pc->ok() )
      {
        double chance = p()->set_bonuses.t29_havoc_4pc->effectN( 1 ).percent();
        if ( s->result == RESULT_CRIT )
          chance *= 2;

        if ( p()->rng().roll( chance ) )
          p()->buff.t29_havoc_4pc->trigger();
      }
    }

    void execute() override
    {
      demon_hunter_attack_t::execute();

      if ( last_attack && p()->buff.restless_hunter->check() )
      {
        // 2023-01-31 -- If Restless Hunter is triggered when the delayed final impact is queued, it does not fade
        //               Seems similar to some other 500ms buff protection in the game
        if( !p()->bugs || sim->current_time() - p()->buff.restless_hunter->last_trigger_time() > 0.5_s )
        {
          p()->buff.restless_hunter->expire();
        }
      }
    }
  };

  std::vector<blade_dance_damage_t*> attacks;
  std::vector<blade_dance_damage_t*> first_blood_attacks;
  buff_t* dodge_buff;
  trail_of_ruin_dot_t* trail_of_ruin_dot;
  timespan_t ability_cooldown;

  blade_dance_base_t( util::string_view n, demon_hunter_t* p,
                      const spell_data_t* s, util::string_view options_str, buff_t* dodge_buff )
    : demon_hunter_attack_t( n, p, s, options_str ), dodge_buff( dodge_buff ), trail_of_ruin_dot ( nullptr )
  {
    may_miss = false;
    cooldown = p->cooldown.blade_dance;  // Blade Dance/Death Sweep Category Cooldown
    range = 5.0; // Disallow use outside of melee range.

    ability_cooldown = data().cooldown();
    if ( data().affected_by( p->spec.blade_dance_2->effectN( 1 ) ) )
    {
      ability_cooldown += p->spec.blade_dance_2->effectN( 1 ).time_value();
    }

    if ( p->talent.havoc.trail_of_ruin->ok() )
    {
      trail_of_ruin_dot = p->get_background_action<trail_of_ruin_dot_t>( "trail_of_ruin" );
    }
  }

  void init_finished() override
  {
    demon_hunter_attack_t::init_finished();

    for ( auto& attack : attacks )
    {
      attack->stats = stats;
    }

    if( attacks.back() )
    {
      attacks.back()->last_attack = true;

      // Trail of Ruin is added to the final hit in the attack list
      if ( p()->talent.havoc.trail_of_ruin->ok() && trail_of_ruin_dot )
      {
        attacks.back()->trail_of_ruin_dot = trail_of_ruin_dot;
      }
    }

    if ( p()->talent.havoc.first_blood->ok() )
    {
      for ( auto& attack : first_blood_attacks )
      {
        attack->stats = first_blood_attacks.front()->stats;
      }

      add_child( first_blood_attacks.front() );

      if ( first_blood_attacks.back() )
      {
        first_blood_attacks.back()->last_attack = true;

        // Trail of Ruin is added to the final hit in the attack list
        if ( p()->talent.havoc.trail_of_ruin->ok() && trail_of_ruin_dot )
        {
          first_blood_attacks.back()->trail_of_ruin_dot = trail_of_ruin_dot;
        }
      }
    }
  }

  void execute() override
  {
    // Blade Dance/Death Sweep Shared Category Cooldown
    cooldown->duration = ability_cooldown;

    demon_hunter_attack_t::execute();

    p()->buff.chaos_theory->trigger();
    trigger_cycle_of_hatred();

    // Metamorphosis benefit and Essence Break stats tracking
    if ( p()->buff.metamorphosis->up() )
    {
      if ( td( target )->debuffs.essence_break->up() )
        p()->proc.death_sweep_in_essence_break->occur();
    }
    else
    {
      if ( td( target )->debuffs.essence_break->up() )
        p()->proc.blade_dance_in_essence_break->occur();
    }

    // Create Strike Events
    if ( !p()->talent.havoc.first_blood->ok() || p()->sim->target_non_sleeping_list.size() > 1 )
    {
      for ( auto& attack : attacks )
      {
        make_event<delayed_execute_event_t>( *sim, p(), attack, target, attack->delay );
      }
    }

    if ( p()->talent.havoc.first_blood->ok() )
    {
      for ( auto& attack : first_blood_attacks )
      {
        make_event<delayed_execute_event_t>( *sim, p(), attack, target, attack->delay );
      }
    }

    if ( dodge_buff )
    {
      dodge_buff->trigger();
    }
  }

  bool has_amount_result() const override
  { return true; }
};

struct blade_dance_t : public blade_dance_base_t
{
  blade_dance_t( demon_hunter_t* p, util::string_view options_str )
    : blade_dance_base_t( "blade_dance", p, p->spec.blade_dance, options_str, nullptr )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "blade_dance_1", data().effectN( 2 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "blade_dance_2", data().effectN( 3 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "blade_dance_3", data().effectN( 4 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "blade_dance_4", data().effectN( 5 ) ) );
    }

    if ( p->talent.havoc.first_blood->ok() && first_blood_attacks.empty() )
    {
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_t>(
        "blade_dance_first_blood", data().effectN( 2 ), p->spec.first_blood_blade_dance_damage ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_t>(
        "blade_dance_first_blood_2", data().effectN( 3 ), p->spec.first_blood_blade_dance_damage ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_t>(
        "blade_dance_first_blood_3", data().effectN( 4 ), p->spec.first_blood_blade_dance_damage ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_t>(
        "blade_dance_first_blood_4", data().effectN( 5 ), p->spec.first_blood_blade_dance_2_damage ) );
    }
  }

  bool ready() override
  {
    if ( !blade_dance_base_t::ready() )
      return false;

    return !p()->buff.metamorphosis->check();
  }
};

// Death Sweep ==============================================================

struct death_sweep_t : public blade_dance_base_t
{
  death_sweep_t( demon_hunter_t* p, util::string_view options_str )
    : blade_dance_base_t( "death_sweep", p, p->spec.death_sweep, options_str, nullptr )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "death_sweep_1", data().effectN( 2 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "death_sweep_2", data().effectN( 3 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "death_sweep_3", data().effectN( 4 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "death_sweep_4", data().effectN( 5 ) ) );
    }

    if ( p->talent.havoc.first_blood->ok() && first_blood_attacks.empty() )
    {
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_t>(
        "death_sweep_first_blood", data().effectN( 2 ), p->spec.first_blood_death_sweep_damage ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_t>(
        "death_sweep_first_blood_2", data().effectN( 3 ), p->spec.first_blood_death_sweep_damage ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_t>(
        "death_sweep_first_blood_3", data().effectN( 4 ), p->spec.first_blood_death_sweep_damage ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_t>(
        "death_sweep_first_blood_4", data().effectN( 5 ), p->spec.first_blood_death_sweep_2_damage ) );
    }
  }

  void execute() override
  {
    assert( p()->buff.metamorphosis->check() );

    // If Metamorphosis has less than 1s remaining, it gets extended so the whole Death Sweep happens during Meta.
    if ( p()->buff.metamorphosis->remains_lt( 1_s ) )
    {
      p()->buff.metamorphosis->extend_duration( p(), 1_s - p()->buff.metamorphosis->remains() );
    }

    blade_dance_base_t::execute();
  }

  bool ready() override
  {
    if ( !blade_dance_base_t::ready() )
      return false;

    // Death Sweep can be queued in the last 250ms, so need to ensure meta is still up after that.
    return ( p()->buff.metamorphosis->remains() > cooldown->queue_delay() );
  }
};

// Chaos Strike =============================================================

struct chaos_strike_base_t : public demon_hunter_attack_t
{
  struct chaos_strike_damage_t: public demon_hunter_attack_t {
    timespan_t delay;
    chaos_strike_base_t* parent;
    bool may_refund;
    double refund_proc_chance;

    chaos_strike_damage_t( util::string_view name, demon_hunter_t* p, const spelleffect_data_t& eff, chaos_strike_base_t* a )
      : demon_hunter_attack_t( name, p, eff.trigger() ),
      delay( timespan_t::from_millis( eff.misc_value1() ) ),
      parent( a ),
      refund_proc_chance( 0.0 )
    {
      assert( eff.type() == E_TRIGGER_SPELL );
      background = dual = true;

      // 2021-06-22 -- It once again appears that Onslaught procs can proc refunds
      may_refund = ( weapon == &( p->off_hand_weapon ) );
      if ( may_refund )
      {
        refund_proc_chance = p->spec.chaos_strike_refund->proc_chance();
      }
    }

    double get_refund_proc_chance()
    {
      double chance = refund_proc_chance;

      if ( p()->buff.chaos_theory->check() )
      {
        chance += p()->buff.chaos_theory->data().effectN( 2 ).percent();
      }

      if ( p()->talent.havoc.critical_chaos->ok() )
      {
        // DFALPHA TOCHECK -- Double check this uses the correct crit calculations
        chance += p()->talent.havoc.critical_chaos->effectN( 2 ).percent() * p()->cache.attack_crit_chance();
      }

      return chance;
    }

    void execute() override
    {
      demon_hunter_attack_t::execute();
      
      if ( may_refund )
      {
        // Technically this appears to have a 0.5s ICD, but this is handled elsewhere
        // Onslaught can currently proc refunds due to being delayed by 600ms
        if ( p()->cooldown.chaos_strike_refund_icd->up() && p()->rng().roll( this->get_refund_proc_chance() ) )
        {
          p()->resource_gain( RESOURCE_FURY, p()->spec.chaos_strike_fury->effectN( 1 ).resource( RESOURCE_FURY ), parent->gain );
          p()->cooldown.chaos_strike_refund_icd->start( p()->spec.chaos_strike_refund->internal_cooldown() );
        }
                
        if ( p()->talent.havoc.chaos_theory->ok() )
        {
          p()->buff.chaos_theory->expire();
        }
      }
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      // Relentless Onslaught cannot self-proc and is delayed by ~300ms after the normal OH impact
      if ( p()->talent.havoc.relentless_onslaught->ok() )
      {
        if ( result_is_hit( s->result ) && may_refund && !parent->from_onslaught )
        {
          double chance = p()->talent.havoc.relentless_onslaught->effectN( 1 ).percent();
          if ( p()->cooldown.relentless_onslaught_icd->up() && p()->rng().roll( chance ) )
          {
            attack_t* const relentless_onslaught = p()->buff.metamorphosis->check() ?
              p()->active.relentless_onslaught_annihilation : p()->active.relentless_onslaught;
            make_event<delayed_execute_event_t>( *sim, p(), relentless_onslaught, target, this->delay );
            p()->cooldown.relentless_onslaught_icd->start( p()->talent.havoc.relentless_onslaught->internal_cooldown() );
          }
        }
      }

      // DFALPHA TOCHECK -- Does this proc from Relentless Onslaught?
      if ( p()->set_bonuses.t29_havoc_4pc->ok() )
      {
        double chance = p()->set_bonuses.t29_havoc_4pc->effectN( 1 ).percent();
        if ( s->result == RESULT_CRIT )
          chance *= 2;

        if ( p()->rng().roll( chance ) )
          p()->buff.t29_havoc_4pc->trigger();
      }
    }
  };

  std::vector<chaos_strike_damage_t*> attacks;
  bool from_onslaught;

  chaos_strike_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view options_str = {}, bool from_onslaught = false )
    : demon_hunter_attack_t( n, p, s, options_str ),
    from_onslaught( from_onslaught )
  {
  }

  double cost() const override
  {
    if ( from_onslaught )
      return 0.0;

    return demon_hunter_attack_t::cost();
  }

  void init_finished() override
  {
    demon_hunter_attack_t::init_finished();

    // Use one stats object for all parts of the attack.
    for ( auto& attack : attacks )
    {
      attack->stats = stats;
    }
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    // Create Strike Events
    for ( auto& attack : attacks )
    {
      make_event<delayed_execute_event_t>( *sim, p(), attack, target, attack->delay );
    }

    // Metamorphosis benefit and Essence Break + Serrated Glaive stats tracking
    if ( p()->buff.metamorphosis->up() )
    {
      if ( td( target )->debuffs.essence_break->up() )
        p()->proc.annihilation_in_essence_break->occur();

      if ( td( target )->debuffs.serrated_glaive->up() )
        p()->proc.annihilation_in_serrated_glaive->occur();
    }
    else
    {
      if ( td( target )->debuffs.essence_break->up() )
        p()->proc.chaos_strike_in_essence_break->occur();

      if ( td( target )->debuffs.serrated_glaive->up() )
        p()->proc.chaos_strike_in_serrated_glaive->occur();
    }

    // Demonic Appetite
    if ( !from_onslaught && p()->talent.havoc.demonic_appetite->ok() && p()->rppm.demonic_appetite->trigger() )
    {
      p()->proc.demonic_appetite->occur();
      p()->spawn_soul_fragment( soul_fragment::LESSER );
    }

    if ( p()->buff.inner_demon->check() )
    {
      make_event<delayed_execute_event_t>( *sim, p(), p()->active.inner_demon, target, 1.25_s );
      p()->buff.inner_demon->expire();
    }

    trigger_cycle_of_hatred();
  }

  bool has_amount_result() const override
  { return true; }
};

struct chaos_strike_t : public chaos_strike_base_t
{
  chaos_strike_t( util::string_view name, demon_hunter_t* p, util::string_view options_str = {}, bool from_onslaught = false )
    : chaos_strike_base_t( name, p, p->spec.chaos_strike, options_str, from_onslaught )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_1", name ), data().effectN( 2 ), this ) );
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_2", name ), data().effectN( 3 ), this ) );
    }

    if ( !from_onslaught && p->active.relentless_onslaught )
    {
      add_child( p->active.relentless_onslaught );
    }
  }

  bool ready() override
  {
    if ( p()->buff.metamorphosis->check() )
    {
      return false;
    }

    return chaos_strike_base_t::ready();
  }
};

// Annihilation =============================================================

struct annihilation_t : public chaos_strike_base_t
{
  annihilation_t( util::string_view name, demon_hunter_t* p, util::string_view options_str = {}, bool from_onslaught = false )
    : chaos_strike_base_t( name, p, p->spec.annihilation, options_str, from_onslaught )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_1", name ), data().effectN( 2 ), this ) );
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_2", name ), data().effectN( 3 ), this ) );
    }

    if ( !from_onslaught && p->active.relentless_onslaught_annihilation )
    {
      add_child( p->active.relentless_onslaught_annihilation );
    }
  }

  bool ready() override
  {
    if ( !p()->buff.metamorphosis->check() )
    {
      return false;
    }

    return chaos_strike_base_t::ready();
  }
};

// Burning Wound ============================================================

struct burning_wound_t : public demon_hunter_spell_t
{
  burning_wound_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_spell_t( name, p, p->spec.burning_wound_debuff )
  {
    dual = true;
    aoe = 0;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // This DoT has a limit 3 effect, oldest applications are removed first
      if ( player->get_active_dots( internal_id ) > data().max_targets() )
      {
        player_t* lowest_duration =
          *std::min_element( sim->target_non_sleeping_list.begin(), sim->target_non_sleeping_list.end(),
                             [this, s]( player_t* lht, player_t* rht ) {
          if ( s->target == lht )
            return false;

          dot_t* lhd = td( lht )->dots.burning_wound;
          if ( !lhd->is_ticking() )
            return false;

          dot_t* rhd = td( rht )->dots.burning_wound;
          if ( !rhd->is_ticking() )
            return true;

          return lhd->remains() < rhd->remains();
        } );

        auto tdata = td( lowest_duration );
        p()->sim->print_debug( "{} removes burning_wound on {} with duration {}",
                               *p(), *lowest_duration, tdata->dots.burning_wound->remains().total_seconds() );
        tdata->debuffs.burning_wound->cancel();
        tdata->dots.burning_wound->cancel();

        assert( player->get_active_dots( internal_id ) <= data().max_targets() );
      }

      td( s->target )->debuffs.burning_wound->trigger();
    }
  }

  void last_tick( dot_t* d ) override
  {
    demon_hunter_spell_t::last_tick( d );
    td( d->target )->debuffs.burning_wound->expire();
  }
};

// Demon's Bite =============================================================

struct demons_bite_t : public demon_hunter_attack_t
{
  demons_bite_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "demons_bite", p, p->spec.demons_bite, options_str )
  {
    energize_delta = energize_amount * data().effectN( 3 ).m_delta();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );

    if ( p()->talent.havoc.insatiable_hunger->ok() )
    {
      ea += static_cast<int>( p()->rng().range( p()->talent.havoc.insatiable_hunger->effectN( 3 ).base_value(),
                                                1 + p()->talent.havoc.insatiable_hunger->effectN( 4 ).base_value() ) );
    }

    return ea;
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( p()->buff.metamorphosis->check() )
    {
      p()->proc.demons_bite_in_meta->occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade( s );

    if ( p()->spec.burning_wound_debuff->ok() )
    {
      p()->active.burning_wound->execute_on_target( s->target );
    }
  }

  bool verify_actor_spec() const override
  {
    if ( p()->talent.havoc.demon_blades->ok() )
      return false;

    return demon_hunter_attack_t::verify_actor_spec();
  }
};

// Demon Blades =============================================================

struct demon_blades_t : public demon_hunter_attack_t
{
  demon_blades_t( demon_hunter_t* p )
    : demon_hunter_attack_t( "demon_blades", p, p->spec.demon_blades_damage )
  {
    background = true;
    energize_delta = energize_amount * data().effectN( 2 ).m_delta();
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade( s );

    if ( p()->spec.burning_wound_debuff->ok() )
    {
      p()->active.burning_wound->execute_on_target( s->target );
    }
  }
};

// Essence Break ============================================================

struct essence_break_t : public demon_hunter_attack_t
{
  essence_break_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "essence_break", p, p->talent.havoc.essence_break, options_str )
  {
    aoe = -1;
    reduced_aoe_targets = p->talent.havoc.essence_break->effectN( 2 ).base_value();
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // Debuff application appears to be delayed by 250-300ms according to logs
      buff_t* debuff = td( s->target )->debuffs.essence_break;
      make_event( *p()->sim, 250_ms, [ debuff ] { debuff->trigger(); } );
    }
  }
};

// Felblade =================================================================
// TODO: Real movement stuff.

struct felblade_t : public demon_hunter_attack_t
{
  struct felblade_damage_t : public demon_hunter_attack_t
  {
    felblade_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->spell.felblade_damage )
    {
      background = dual = true;
      gain = p->get_gain( "felblade" );
    }
  };

  felblade_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "felblade", p, p->talent.demon_hunter.felblade, options_str )
  {
    may_block = false;
    movement_directionality = movement_direction_type::TOWARDS;

    execute_action = p->get_background_action<felblade_damage_t>( "felblade_damage" );
    execute_action->stats = stats;

    // Add damage modifiers in felblade_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();
    p()->set_out_of_range( timespan_t::zero() ); // Cancel all other movement
  }
};

// Fel Rush =================================================================

struct fel_rush_t : public demon_hunter_attack_t
{
  struct fel_rush_damage_t : public demon_hunter_spell_t
  {
    fel_rush_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.fel_rush_damage )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = p->spec.fel_rush_damage->effectN( 2 ).base_value();
    }

    double action_multiplier() const override
    {
      double am = demon_hunter_spell_t::action_multiplier();

      am *= 1.0 + p()->buff.unbound_chaos->value();

      return am;
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      if ( p()->talent.havoc.isolated_prey->ok() && num_targets_hit == 1 )
      {
        p()->resource_gain( RESOURCE_FURY, p()->spec.isolated_prey_fury->effectN( 1 ).resource(),
                            p()->spec.isolated_prey_fury->effectN( 1 ).m_delta(), p()->gain.isolated_prey );
      }
    }
  };

  bool a_cancel;
  timespan_t gcd_lag;

  fel_rush_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "fel_rush", p, p->spec.fel_rush ),
      a_cancel( false )
  {
    add_option( opt_bool( "animation_cancel", a_cancel ) );
    parse_options( options_str );

    may_miss = may_dodge = may_parry = may_block = false;
    min_gcd = trigger_gcd;

    execute_action = p->get_background_action<fel_rush_damage_t>( "fel_rush_damage" );
    execute_action->stats = stats;

    if ( !a_cancel )
    {
      // Fel Rush does damage in a further line than it moves you
      base_teleport_distance = execute_action->radius - 5;
      movement_directionality = movement_direction_type::OMNI;
      p->buff.fel_rush_move->distance_moved = base_teleport_distance;
    }

    // Add damage modifiers in fel_rush_damage_t, not here.
  }

  void execute() override
  {
    p()->buff.momentum->trigger();

    demon_hunter_attack_t::execute();

    p()->buff.unbound_chaos->expire();

    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    p()->cooldown.movement_shared->start( timespan_t::from_seconds( 1.0 ) );

    p()->consume_nearby_soul_fragments( soul_fragment::LESSER );

    if ( !a_cancel )
    {
      p()->buff.fel_rush_move->trigger();
    }
  }

  void schedule_execute( action_state_t* s ) override
  {
    // Fel Rush's loss of control causes a GCD lag after the loss ends.
    // Calculate this once on schedule_execute since gcd() is called multiple times
    gcd_lag = rng().gauss( sim->gcd_lag, sim->gcd_lag_stddev );
    demon_hunter_attack_t::schedule_execute( s );
  }

  timespan_t gcd() const override
  {
    return demon_hunter_attack_t::gcd() + gcd_lag;
  }

  bool ready() override
  {
    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    if ( p()->cooldown.movement_shared->down() )
      return false;

    // Not usable during the root effect of Stormeater's Boon
    if ( p()->buffs.stormeaters_boon && p()->buffs.stormeaters_boon->check() )
      return false;

    return demon_hunter_attack_t::ready();
  }
};

// Fracture =================================================================

struct fracture_t : public demon_hunter_attack_t
{
  struct fracture_damage_t : public demon_hunter_attack_t
  {
    int soul_fragments_to_spawn;

    fracture_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_attack_t( name, p, s )
    {
      dual     = true;
      may_miss = may_dodge = may_parry = false;

      // Fracture currently spawns an even number of Soul Fragments baseline, half on main hand hit and half on
      // offhand hit.
      // In the event that Blizzard ever changes the baseline amount to an odd amount, we will assume that the extra
      // Soul Fragment will be spawned by the main hand hit.
      int number_of_soul_fragments_to_spawn = as<int>( p->talent.vengeance.fracture->effectN( 1 ).base_value() );
      int number_of_soul_fragments_to_spawn_per_hit  = number_of_soul_fragments_to_spawn / 2;
      int number_of_soul_fragments_to_spawn_leftover = number_of_soul_fragments_to_spawn % 2;
      if ( weapon == &( p->main_hand_weapon ) )
      {
        soul_fragments_to_spawn =
            number_of_soul_fragments_to_spawn_per_hit + number_of_soul_fragments_to_spawn_leftover;
      }
      else
      {
        soul_fragments_to_spawn = number_of_soul_fragments_to_spawn_per_hit;
      }
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      p()->spawn_soul_fragment( soul_fragment::LESSER, soul_fragments_to_spawn );
      for ( unsigned i = 0; i < soul_fragments_to_spawn; i++ )
      {
        p()->proc.soul_fragment_from_fracture->occur();
      }
    }
  };

  fracture_damage_t* mh, *oh;

  fracture_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "fracture", p, p->talent.vengeance.fracture, options_str )
  {
    mh = p->get_background_action<fracture_damage_t>( "fracture_mh", data().effectN( 2 ).trigger() );
    oh = p->get_background_action<fracture_damage_t>( "fracture_oh", data().effectN( 3 ).trigger() );
    mh->stats = oh->stats = stats;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );

    if ( p()->buff.metamorphosis->check() )
    {
      ea += p()->spec.metamorphosis_buff->effectN( 10 ).resource( RESOURCE_FURY );
    }
    if ( p()->set_bonuses.t29_vengeance_2pc->ok() )
    {
      ea *= 1.0 + p()->set_bonuses.t29_vengeance_2pc->effectN( 2 ).percent();
    }

    return ea;
  }

  double composite_da_multiplier(const action_state_t* s) const override
  {
    double m = demon_hunter_attack_t::composite_da_multiplier(s);

    if ( p()->set_bonuses.t29_vengeance_2pc->ok() )
    {
      m *= 1.0 + p()->set_bonuses.t29_vengeance_2pc->effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade( s );

    /*
     * logged event ordering for Fracture:
     * - cast Fracture (225919 - main hand spell ID)
     * - cast Fracture (263642 - container spell ID)
     * - apply Fires of Fel buff to player if T30 2pc is active
     * - apply Fiery Brand to target if player has Recrimination buff from T30 4pc
     *   - apply Fiery Brand debuff (207771)
     *   - Fiery Brand initial damage event (204021)
     *   - remove Recrimination buff
     * - cast Fracture (225921 - offhand spell ID)
     * - apply Fires of Fel buff to player if T30 2pc is active
     * - apply Fires of Fel buff to player if T30 2pc is active and Metamorphosis is active
     * - apply Fires of Fel buff to player if T30 2pc is active and T29 2pc procs
     * - generate Soul Fragment from main hand hit
     * - generate Soul Fragment from offhand hit
     * - generate Soul Fragment if Metamorphosis is active
     * - generate Soul Fragment if T29 2pc procs
     * because Fires of Fel is currently applied by calling "spawn_soul_fragment", the ordering listed above is
     * slightly collapsed.
     */
    if ( result_is_hit( s->result ) )
    {
      int number_of_soul_fragments_to_spawn = as<int>( data().effectN( 1 ).base_value() );
      // divide the number in 2 as half come from main hand, half come from offhand.
      int number_of_soul_fragments_to_spawn_per_hit = number_of_soul_fragments_to_spawn / 2;
      // handle leftover souls in the event that blizz ever changes Fracture to an odd number of souls generated
      int number_of_soul_fragments_to_spawn_leftover = number_of_soul_fragments_to_spawn % 2;

      mh->set_target( s->target );
      mh->execute();

      // t30 4pc proc happens after main hand execute but before offhand execute
      // this matters because the offhand hit benefits from Fiery Demise that may be
      // applied because of the t30 4pc proc
      if ( p()->buff.t30_vengeance_4pc->up() )
      {
        p()->active.fiery_brand_t30->execute_on_target( s->target );
        p()->buff.t30_vengeance_4pc->expire();
      }

      // offhand hit is ~150ms after cast
      make_event<delayed_execute_event_t>( *p()->sim, p(), oh, s->target,
                                           timespan_t::from_millis( data().effectN( 3 ).misc_value1() ) );

      if ( p()->buff.metamorphosis->check() )
      {
        // In all reviewed logs, it's always 500ms (based on Fires of Fel application)
        make_event(sim, 500_ms, [ this ]() {
          p()->spawn_soul_fragment( soul_fragment::LESSER );
          p()->proc.soul_fragment_from_meta->occur();
        });
      }

      // 15% chance to generate an extra soul fragment not included in spell data
      if ( p()->set_bonuses.t29_vengeance_2pc->ok() && rng().roll( 0.15 ) )
      {
        p()->spawn_soul_fragment( soul_fragment::LESSER );
        p()->proc.soul_fragment_from_t29_2pc->occur();
      }
    }
  }
};

// Ragefire Talent ==========================================================

struct ragefire_t : public demon_hunter_spell_t
{
  ragefire_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_spell_t( name, p, p->spec.ragefire_damage )
  {
    aoe = -1;
  }
};

// Inner Demon Talent =======================================================

struct inner_demon_t : public demon_hunter_spell_t
{
  inner_demon_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_spell_t( name, p, p->spec.inner_demon_damage )
  {
    aoe = -1;
  }
};

// Shear ====================================================================

struct shear_t : public demon_hunter_attack_t
{
  shear_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t("shear", p, p->find_specialization_spell( "Shear" ), options_str )
  {
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade( s );

    if ( result_is_hit( s->result ) )
    {
      p()->spawn_soul_fragment( soul_fragment::LESSER );
      p()->proc.soul_fragment_from_shear->occur();

      if ( p()->buff.metamorphosis->check() )
      {
        p()->spawn_soul_fragment( soul_fragment::LESSER );
        p()->proc.soul_fragment_from_meta->occur();
      }
    }

    if ( p()->buff.t30_vengeance_4pc->up() )
    {
      p()->active.fiery_brand_t30->execute_on_target( s->target );
      p()->buff.t30_vengeance_4pc->expire();
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );

    if ( p()->buff.metamorphosis->check() )
    {
      ea += p()->spec.metamorphosis_buff->effectN( 4 ).resource( RESOURCE_FURY );
    }
    if ( p()->talent.vengeance.shear_fury->ok() )
    {
      ea += p()->talent.vengeance.shear_fury->effectN( 1 ).resource( RESOURCE_FURY );
    }
    if ( p()->set_bonuses.t29_vengeance_2pc->ok() )
    {
      ea *= 1.0 + p()->set_bonuses.t29_vengeance_2pc->effectN( 2 ).percent();
    }

    return ea;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = demon_hunter_attack_t::composite_da_multiplier( s );

    if ( p()->set_bonuses.t29_vengeance_2pc->ok() )
    {
      m *= 1.0 + p()->set_bonuses.t29_vengeance_2pc->effectN( 1 ).percent();
    }

    return m;
  }

  bool verify_actor_spec() const override
  {
    if ( p()->talent.vengeance.fracture->ok() )
      return false;

    return demon_hunter_attack_t::verify_actor_spec();
  }
};

// Soul Cleave ==============================================================

struct soul_cleave_t : public demon_hunter_attack_t
{
  struct soul_cleave_state_t : public action_state_t
  {
    bool t29_vengeance_4pc_proc;

    soul_cleave_state_t(action_t* a, player_t* target): action_state_t(a, target), t29_vengeance_4pc_proc(false)
    {
    }

    void initialize() override
    {
      action_state_t::initialize();
      t29_vengeance_4pc_proc = false;
    }

    void copy_state(const action_state_t* s) override
    {
      action_state_t::copy_state(s);
      t29_vengeance_4pc_proc = debug_cast<const soul_cleave_state_t*>(s)->t29_vengeance_4pc_proc;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );
      s << " t29_vengeance_4pc_proc=" << t29_vengeance_4pc_proc;
      return s;
    }
  };

  struct soul_cleave_damage_t : public demon_hunter_attack_t
  {
    soul_cleave_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_attack_t( name, p, s )
    {
      dual                = true;
      aoe                 = -1;
      reduced_aoe_targets = data().effectN( 2 ).base_value();
    }

    action_state_t* new_state() override
    {
      return new soul_cleave_state_t(this, target);
    }

    void snapshot_state( action_state_t* state, result_amount_type rt ) override
    {
      if ( p()->set_bonuses.t29_vengeance_4pc->ok() &&
           rng().roll( p()->set_bonuses.t29_vengeance_4pc->effectN( 2 ).percent() ) )
      {
        debug_cast<soul_cleave_state_t*>( state )->t29_vengeance_4pc_proc = true;
      }
      // snapshot_state after so that it includes the DA multiplier from the proc
      demon_hunter_attack_t::snapshot_state( state, rt );
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      // Soul Cleave can apply Frailty if Void Reaver is talented
      if ( result_is_hit( s->result ) && p()->talent.vengeance.void_reaver->ok() )
      {
        td( s->target )->debuffs.frailty->trigger();
      }
      // Soul Cleave applies a stack of Frailty to the primary target if Soulcrush is talented,
      // doesn't need to hit.
      if ( s->chain_target == 0 && p()->talent.vengeance.soulcrush->ok() )
      {
        td( s->target )
            ->debuffs.frailty->trigger(
                timespan_t::from_seconds( p()->talent.vengeance.soulcrush->effectN( 2 ).base_value() ) );
      }

      if ( debug_cast<soul_cleave_state_t*>( s )->t29_vengeance_4pc_proc )
      {
        td( s->target )->debuffs.t29_vengeance_4pc->trigger();
      }
    }

    void execute() override
    {
      demon_hunter_attack_t::execute();

      p()->buff.soul_furnace_damage_amp->expire();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = demon_hunter_attack_t::composite_da_multiplier( s );

      if ( s->chain_target == 0 && p()->talent.vengeance.focused_cleave->ok() )
      {
        m *= 1.0 + p()->talent.vengeance.focused_cleave->effectN( 1 ).percent();
      }

      if ( p()->buff.soul_furnace_damage_amp->up() )
      {
        m *= 1.0 + p()->buff.soul_furnace_damage_amp->check_value();
      }

      if ( debug_cast<const soul_cleave_state_t*>( s )->t29_vengeance_4pc_proc )
      {
        m *= 1.0 + p()->set_bonuses.t29_vengeance_4pc->effectN( 1 ).percent();
      }

      return m;
    }
  };

  heals::soul_cleave_heal_t* heal;

  soul_cleave_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "soul_cleave", p, p->spec.soul_cleave, options_str ),
    heal( nullptr )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    attack_power_mod.direct = 0;  // This parent action deals no damage, parsed data is for the heal

    execute_action = p->get_background_action<soul_cleave_damage_t>( "soul_cleave_damage", data().effectN( 2 ).trigger() );
    execute_action->stats = stats;

    if ( p->spec.soul_cleave_2->ok() )
    {
      heal = p->get_background_action<heals::soul_cleave_heal_t>( "soul_cleave_heal" );
    }

    if ( p->talent.vengeance.void_reaver->ok() && !p->active.frailty_heal )
    {
      p->active.frailty_heal = new heals::frailty_heal_t( p );
    }
    // Add damage modifiers in soul_cleave_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( heal )
    {
      heal->set_target( player );
      heal->execute();
    }

    // Soul fragments consumed are capped for Soul Cleave
    p()->consume_soul_fragments( soul_fragment::ANY, true, (unsigned)data().effectN( 3 ).base_value() );
  }
};

// Throw Glaive =============================================================

struct throw_glaive_t : public demon_hunter_attack_t
{
  struct throw_glaive_damage_t : public demon_hunter_attack_t
  {
    struct soulrend_t : public residual_action::residual_periodic_action_t<demon_hunter_attack_t>
    {
      soulrend_t( util::string_view name, demon_hunter_t* p ) :
        base_t( name, p, p->spec.soulrend_debuff )
      {
        dual = true;
      }

      void init() override
      {
        base_t::init();
        update_flags = 0; // Snapshots on refresh, does not update dynamically
      }
    };

    soulrend_t* soulrend;

    throw_glaive_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->spell.throw_glaive->effectN( 1 ).trigger() ),
      soulrend( nullptr )
    {
      background = dual = true;
      radius = 10.0;

      if ( p->talent.havoc.soulrend->ok() )
      {
        soulrend = p->get_background_action<soulrend_t>( "soulrend" );
      }
    }

    void impact( action_state_t* state ) override
    {
      demon_hunter_attack_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( soulrend )
        {
          const double dot_damage = state->result_amount * p()->talent.havoc.soulrend->effectN( 1 ).percent();
          residual_action::trigger( soulrend, state->target, dot_damage );
        }

        if ( p()->spec.burning_wound_debuff->ok() )
        {
          p()->active.burning_wound->execute_on_target( state->target );
        }

        if ( p()->spec.serrated_glaive_debuff->ok() )
        {
          td( state->target )->debuffs.serrated_glaive->trigger();
        }
      }
    }
  };

  throw_glaive_damage_t* furious_throws;

  throw_glaive_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "throw_glaive", p, p->spell.throw_glaive, options_str ),
    furious_throws( nullptr )
  {
    throw_glaive_damage_t* damage = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_damage" );
    execute_action = damage;
    execute_action->stats = stats;

    if ( damage->soulrend )
    {
      add_child( damage->soulrend );
    }

    if ( p->talent.havoc.furious_throws->ok() )
    {
      resource_current = RESOURCE_FURY;
      base_costs[ RESOURCE_FURY ] = p->talent.havoc.furious_throws->effectN( 1 ).base_value();
      furious_throws = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_furious_throws" );
      add_child( furious_throws );
    }
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( hit_any_target && furious_throws )
    {
      furious_throws->execute_on_target( target );
    }
  }
};

// Vengeful Retreat =========================================================

struct vengeful_retreat_t : public demon_hunter_spell_t
{
  struct vengeful_retreat_damage_t : public demon_hunter_spell_t
  {
    vengeful_retreat_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->talent.demon_hunter.vengeful_retreat->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = -1;
    }

    void execute() override
    {
      // Initiative reset mechanic happens prior to the ability dealing damage
      if ( p()->talent.havoc.initiative->ok() )
      {
        for ( auto p : sim->target_non_sleeping_list )
        {
          td( p )->debuffs.initiative_tracker->expire();
        }
      }

      demon_hunter_spell_t::execute();

      p()->buff.tactical_retreat->trigger();
    }
  };

  vengeful_retreat_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "vengeful_retreat", p, p->talent.demon_hunter.vengeful_retreat, options_str )
  {
    execute_action = p->get_background_action<vengeful_retreat_damage_t>( "vengeful_retreat_damage" );
    execute_action->stats = stats;

    base_teleport_distance = VENGEFUL_RETREAT_DISTANCE;
    movement_directionality = movement_direction_type::OMNI;
    p->buff.vengeful_retreat_move->distance_moved = base_teleport_distance;

    // Add damage modifiers in vengeful_retreat_damage_t, not here.
  }

  void execute() override
  {
    p()->buff.momentum->trigger();

    demon_hunter_spell_t::execute();

    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    p()->cooldown.movement_shared->start( timespan_t::from_seconds( 1.0 ) );
    p()->buff.vengeful_retreat_move->trigger();

    p()->consume_nearby_soul_fragments( soul_fragment::LESSER );
  }

  bool ready() override
  {
    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    if ( p()->cooldown.movement_shared->down() )
      return false;

    // Not usable during the root effect of Stormeater's Boon
    if ( p()->buffs.stormeaters_boon && p()->buffs.stormeaters_boon->check() )
      return false;

    return demon_hunter_spell_t::ready();
  }
};

// Soul Carver =========================================================
struct soul_carver_t : public demon_hunter_attack_t
{
  struct soul_carver_oh_t : public demon_hunter_attack_t
  {
    soul_carver_oh_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->talent.vengeance.soul_carver->effectN( 3 ).trigger() )
    {
      background = dual = true;
      may_miss = may_parry = may_dodge = false;  // TOCHECK
    }
  };

  soul_carver_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "soul_carver", p, p->talent.vengeance.soul_carver, options_str )
  {
    impact_action = p->get_background_action<soul_carver_oh_t>( "soul_carver_oh" );
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    p()->spawn_soul_fragment( soul_fragment::LESSER, as<unsigned int>( data().effectN( 3 ).base_value() ) );
  }

  void tick( dot_t* d ) override
  {
    demon_hunter_attack_t::tick( d );

    p()->spawn_soul_fragment( soul_fragment::LESSER, as<unsigned int>( data().effectN( 4 ).base_value() ) );
  }
};

}  // end namespace attacks

}  // end namespace actions

namespace buffs
{
template <typename BuffBase>
struct demon_hunter_buff_t : public BuffBase
{
  using base_t = demon_hunter_buff_t;

  demon_hunter_buff_t( demon_hunter_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(), const item_t* item = nullptr )
    : BuffBase( &p, name, s, item )
  { }
  demon_hunter_buff_t( demon_hunter_td_t& td, util::string_view name, const spell_data_t* s = spell_data_t::nil(), const item_t* item = nullptr )
    : BuffBase( td, name, s, item )
  { }

protected:

  demon_hunter_t* p()
  {
    return static_cast<demon_hunter_t*>( BuffBase::source );
  }

  const demon_hunter_t* p() const
  {
    return static_cast<const demon_hunter_t*>( BuffBase::source );
  }

private:
  using bb = BuffBase;
};

// Immolation Aura ==========================================================

struct immolation_aura_buff_t : public demon_hunter_buff_t<buff_t>
{
  immolation_aura_buff_t( demon_hunter_t* p )
    : base_t( *p, "immolation_aura", p->spell.immolation_aura )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS );
    apply_affecting_aura( p->spec.immolation_aura_3 );
    apply_affecting_aura( p->talent.havoc.felfire_heart );
    apply_affecting_aura( p->talent.vengeance.agonizing_flames );
    set_partial_tick( true );

    set_tick_callback( [ p ]( buff_t*, int, timespan_t ) {
      if ( p->talent.havoc.ragefire->ok() )
      {
        p->ragefire_crit_accumulator = 0;
      }
      p->active.immolation_aura->execute_on_target( p->target );
      p->buff.growing_inferno->trigger();
    } );

    if ( p->talent.havoc.growing_inferno->ok() )
    {
      set_stack_change_callback( [ p ]( buff_t*, int, int ) {
        p->buff.growing_inferno->expire();
      } );
    }

    if ( p->talent.vengeance.agonizing_flames->ok() )
    {
      add_invalidate( CACHE_RUN_SPEED );
    }

    if ( p->talent.demon_hunter.infernal_armor->ok() )
    {
      add_invalidate( CACHE_ARMOR );
    }
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    demon_hunter_buff_t<buff_t>::start( stacks, value, duration );

    if ( p()->talent.havoc.ragefire->ok() )
    {
      p()->ragefire_accumulator = 0.0;
      p()->ragefire_crit_accumulator = 0;
    }

    if ( p()->active.immolation_aura_initial && p()->sim->current_time() > 0_s )
    {
      p()->active.immolation_aura_initial->set_target( p()->target );
      p()->active.immolation_aura_initial->execute();
    }

    if ( p()->talent.havoc.unbound_chaos->ok() )
    {
      p()->buff.unbound_chaos->trigger();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    if ( p()->talent.havoc.ragefire->ok() )
    {
      p()->active.ragefire->execute_on_target( p()->target, p()->ragefire_accumulator );
      p()->ragefire_accumulator = 0.0;
      p()->ragefire_crit_accumulator = 0;
    }
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    // IA triggering multiple times fully resets the buff and triggers the instant damage again
    this->expire();
    return demon_hunter_buff_t<buff_t>::trigger( stacks, value, chance, duration );
  }
};

// Metamorphosis Buff =======================================================

struct metamorphosis_buff_t : public demon_hunter_buff_t<buff_t>
{
  metamorphosis_buff_t( demon_hunter_t* p )
    : base_t( *p, "metamorphosis", p->spec.metamorphosis_buff )
  {
    set_cooldown( timespan_t::zero() );
    buff_period = timespan_t::zero();
    tick_behavior = buff_tick_behavior::NONE;

    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      add_invalidate( CACHE_HASTE );
      add_invalidate( CACHE_LEECH );
    }
    else // DEMON_HUNTER_VENGEANCE
    {
      set_default_value_from_effect_type( A_MOD_INCREASE_HEALTH_PERCENT );
      add_invalidate( CACHE_ARMOR );
    }

    if ( p->talent.demon_hunter.soul_rending->ok() )
    {
      add_invalidate( CACHE_LEECH );
    }

    if ( p->talent.demon_hunter.first_of_the_illidari->ok() )
    {
      add_invalidate( CACHE_VERSATILITY );
    }
  }

  void trigger_demonic()
  {
    const timespan_t extend_duration = p()->talent.demon_hunter.demonic->effectN( 1 ).time_value();
    p()->buff.metamorphosis->extend_duration_or_trigger( extend_duration );
    p()->buff.inner_demon->trigger();
  }

  void start(int stacks, double value, timespan_t duration) override
  {
    demon_hunter_buff_t<buff_t>::start(stacks, value, duration);

    if ( p()->specialization() == DEMON_HUNTER_VENGEANCE )
    {
      p()->metamorphosis_health = p()->max_health() * value;
      p()->stat_gain( STAT_MAX_HEALTH, p()->metamorphosis_health, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    }
  }

  void expire_override(int expiration_stacks, timespan_t remaining_duration) override
  {
    demon_hunter_buff_t<buff_t>::expire_override(expiration_stacks, remaining_duration);

    if (p()->specialization() == DEMON_HUNTER_VENGEANCE)
    {
      p()->stat_loss(STAT_MAX_HEALTH, p()->metamorphosis_health, (gain_t*)nullptr, (action_t*)nullptr, true);
      p()->metamorphosis_health = 0;
    }

    if ( p()->talent.havoc.restless_hunter->ok() )
    {
      p()->cooldown.fel_rush->reset( false, 1 );
      p()->buff.restless_hunter->trigger();
    }
  }
};

// Demon Spikes buff ========================================================

struct demon_spikes_t : public demon_hunter_buff_t<buff_t>
{
  const timespan_t max_duration;

  demon_spikes_t(demon_hunter_t* p)
    : base_t( *p, "demon_spikes", p->find_spell( 203819 ) ),
      max_duration( base_buff_duration * 3 ) // Demon Spikes can only be extended to 3x its base duration
  {
    set_default_value_from_effect_type( A_MOD_PARRY_PERCENT );
    set_refresh_behavior( buff_refresh_behavior::EXTEND );
    apply_affecting_aura( p->talent.vengeance.extended_spikes );
    add_invalidate( CACHE_PARRY );
    add_invalidate( CACHE_ARMOR );
  }

  bool trigger(int stacks, double value, double chance, timespan_t duration) override
  {
    if (duration == timespan_t::min())
    {
      duration = buff_duration();
    }

    if (remains() + buff_duration() > max_duration)
    {
      duration = max_duration - remains();
    }

    if (duration <= timespan_t::zero())
    {
      return false;
    }

    return demon_hunter_buff_t::trigger(stacks, value, chance, duration);
  }
};

}  // end namespace buffs

// ==========================================================================
// Misc. Events and Structs
// ==========================================================================

// Frailty event ========================================================

struct frailty_event_t : public event_t
{
  demon_hunter_t* dh;

  frailty_event_t( demon_hunter_t* p, bool initial = false )
    : event_t( *p), dh( p )
  {
    timespan_t delta_time = timespan_t::from_seconds( 1.0 );
    if ( initial )
    {
      delta_time *= rng().real();
    }
    schedule( delta_time );
  }

  const char* name() const override
  {
    return "frailty_driver";
  }

  void execute() override
  {
    assert( dh ->frailty_accumulator >= 0.0 );

    if ( dh->frailty_accumulator > 0 )
    {
      action_t* a = dh->active.frailty_heal;
      a->base_dd_min = a->base_dd_max = dh->frailty_accumulator;
      a->execute();

      dh->frailty_accumulator = 0.0;
    }

    dh->frailty_driver = make_event<frailty_event_t>( sim(), dh );
  }
};

movement_buff_t::movement_buff_t( demon_hunter_t* p, util::string_view name, const spell_data_t* spell_data, const item_t* item )
  : buff_t( p, name, spell_data, item ), yards_from_melee( 0.0 ), distance_moved( 0.0 ), dh( p )
{
}

// ==========================================================================
// Targetdata Definitions
// ==========================================================================

demon_hunter_td_t::demon_hunter_td_t( player_t* target, demon_hunter_t& p )
  : actor_target_data_t( target, &p ), dots( dots_t() ), debuffs( debuffs_t() )
{
  if ( p.specialization() == DEMON_HUNTER_HAVOC )
  {
    debuffs.essence_break = make_buff( *this, "essence_break", p.find_spell( 320338 ) )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_cooldown( timespan_t::zero() );

    debuffs.initiative_tracker = make_buff( *this, "initiative_tracker", p.talent.havoc.initiative )
      ->set_duration( timespan_t::min() );

    debuffs.burning_wound = make_buff( *this, "burning_wound", p.spec.burning_wound_debuff )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS )
      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    dots.burning_wound = target->get_dot( "burning_wound", &p );
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    dots.fiery_brand = target->get_dot("fiery_brand", &p);
    debuffs.frailty  = make_buff( *this, "frailty", p.spec.frailty_debuff )
                          ->set_default_value_from_effect( 1 )
                          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                          ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                          ->set_period( 0_ms )
                          ->apply_affecting_aura( p.talent.vengeance.soulcrush );
    debuffs.t29_vengeance_4pc =
        make_buff( *this, "decrepit_souls",
                   p.set_bonuses.t29_vengeance_4pc->ok() ? p.find_spell( 394958 ) : spell_data_t::not_found() )
            ->set_default_value_from_effect( 1 )
            ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  dots.sigil_of_flame = target->get_dot( "sigil_of_flame", &p );
  dots.the_hunt = target->get_dot( "the_hunt_dot", &p );

  debuffs.serrated_glaive = make_buff( *this, "serrated_glaive", p.spec.serrated_glaive_debuff )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
    ->set_default_value( p.talent.havoc.serrated_glaive->effectN( 1 ).percent() );

  target->register_on_demise_callback( &p, [this]( player_t* ) { target_demise(); } );
}

void demon_hunter_td_t::target_demise()
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source->sim->event_mgr.canceled )
    return;

  demon_hunter_t* p = static_cast<demon_hunter_t*>( source );

  if ( p->talent.demon_hunter.relentless_pursuit->ok() && dots.the_hunt->is_ticking() )
  {
    timespan_t adjust_seconds = timespan_t::from_seconds( p->talent.demon_hunter.relentless_pursuit->effectN( 1 ).base_value() );
    p->cooldown.the_hunt->adjust( -adjust_seconds );
    p->proc.relentless_pursuit->occur();
  }

  // TODO: Make an option to register this for testing M+/dungeon scenarios
  //demon_hunter_t* p = static_cast<demon_hunter_t*>( source );
  //p->spawn_soul_fragment( soul_fragment::GREATER );
}

// ==========================================================================
// Demon Hunter Definitions
// ==========================================================================

demon_hunter_t::demon_hunter_t(sim_t* sim, util::string_view name, race_e r)
  : player_t(sim, DEMON_HUNTER, name, r),
  melee_main_hand( nullptr ),
  melee_off_hand( nullptr ),
  next_fragment_spawn( 0 ),
  soul_fragments(),
    frailty_accumulator( 0.0 ),
    frailty_driver( nullptr ),
  ragefire_accumulator( 0.0 ),
  ragefire_crit_accumulator( 0 ),
  shattered_destiny_accumulator( 0.0 ),
  darkglare_boon_cdr_roll(0.0),
  exit_melee_event( nullptr ),
  buff(),
  talent(),
  spec(),
  mastery(),
  cooldown(),
  gain(),
  benefits(),
  proc(),
  active(),
  pets(),
  options()
{
  create_cooldowns();
  create_gains();
  create_benefits();

  resource_regeneration = regen_type::DISABLED;
}

// ==========================================================================
// overridden player_t init functions
// ==========================================================================

// demon_hunter_t::convert_hybrid_stat ======================================

stat_e demon_hunter_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
    case STAT_STR_AGI:
      return STAT_AGILITY;
    case STAT_STR_INT:
      return STAT_NONE;
    case STAT_SPIRIT:
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
      return specialization() == DEMON_HUNTER_VENGEANCE ? s : STAT_NONE;
    default:
      return s;
  }
}

// demon_hunter_t::copy_from ================================================

void demon_hunter_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  auto source_p = debug_cast<demon_hunter_t*>( source );

  options = source_p->options;
}

// demon_hunter_t::create_action ============================================

action_t* demon_hunter_t::create_action( util::string_view name, util::string_view options_str )
{
  using namespace actions::heals;

  if ( name == "soul_barrier" )       return new soul_barrier_t( this, options_str );

  using namespace actions::spells;

  if ( name == "blur" )               return new blur_t( this, options_str );
  if ( name == "bulk_extraction" )    return new bulk_extraction_t( this, options_str );
  if ( name == "chaos_nova" )         return new chaos_nova_t( this, options_str );
  if ( name == "consume_magic" )      return new consume_magic_t( this, options_str );
  if ( name == "demon_spikes" )       return new demon_spikes_t( this, options_str );
  if ( name == "disrupt" )            return new disrupt_t( this, options_str );
  if ( name == "eye_beam" )           return new eye_beam_t( this, options_str );
  if ( name == "fel_barrage" )        return new fel_barrage_t( this, options_str );
  if ( name == "fel_eruption" )       return new fel_eruption_t( this, options_str );
  if ( name == "fel_devastation" )    return new fel_devastation_t( this, options_str );
  if ( name == "fiery_brand" )        return new fiery_brand_t( "fiery_brand", this, options_str );
  if ( name == "glaive_tempest" )     return new glaive_tempest_t( this, options_str );
  if ( name == "infernal_strike" )    return new infernal_strike_t( this, options_str );
  if ( name == "immolation_aura" )    return new immolation_aura_t( this, options_str );
  if ( name == "metamorphosis" )      return new metamorphosis_t( this, options_str );
  if ( name == "pick_up_fragment" )   return new pick_up_fragment_t( this, options_str );
  if ( name == "sigil_of_flame" )     return new sigil_of_flame_t( this, options_str );
  if ( name == "spirit_bomb" )        return new spirit_bomb_t( this, options_str );

  if ( name == "elysian_decree" )     return new elysian_decree_t( this, options_str );
  if ( name == "the_hunt" )           return new the_hunt_t( this, options_str );

  using namespace actions::attacks;

  if ( name == "auto_attack" )        return new auto_attack_t( this, options_str );
  if ( name == "annihilation" )       return new annihilation_t( "annihilation", this, options_str );
  if ( name == "blade_dance" )        return new blade_dance_t( this, options_str );
  if ( name == "chaos_strike" )       return new chaos_strike_t( "chaos_strike", this, options_str );
  if ( name == "essence_break" )      return new essence_break_t( this, options_str );
  if ( name == "death_sweep" )        return new death_sweep_t( this, options_str );
  if ( name == "demons_bite" )        return new demons_bite_t( this, options_str );
  if ( name == "felblade" )           return new felblade_t( this, options_str );
  if ( name == "fel_rush" )           return new fel_rush_t( this, options_str );
  if ( name == "fracture" )           return new fracture_t( this, options_str );
  if ( name == "shear" )              return new shear_t( this, options_str );
  if ( name == "soul_cleave" )        return new soul_cleave_t( this, options_str );
  if ( name == "throw_glaive" )       return new throw_glaive_t( this, options_str );
  if ( name == "vengeful_retreat" )   return new vengeful_retreat_t( this, options_str );
  if ( name == "soul_carver" )        return new soul_carver_t( this, options_str );

  return base_t::create_action( name, options_str );
}

// demon_hunter_t::create_buffs =============================================

void demon_hunter_t::create_buffs()
{
  base_t::create_buffs();

  using namespace buffs;

  // General ================================================================

  buff.demon_soul = make_buff<damage_buff_t>( this, "demon_soul", spell.demon_soul );
  buff.empowered_demon_soul = make_buff<damage_buff_t>( this, "empowered_demon_soul", spell.demon_soul_empowered );
  buff.immolation_aura = new buffs::immolation_aura_buff_t( this );
  buff.metamorphosis = new buffs::metamorphosis_buff_t( this );

  // Havoc ==================================================================

  buff.out_of_range = make_buff( this, "out_of_range", spell_data_t::nil() )
    ->set_chance( 1.0 );

  buff.fel_rush_move = new movement_buff_t( this, "fel_rush_movement", spell_data_t::nil() );
  buff.fel_rush_move->set_chance( 1.0 )
    ->set_duration( spec.fel_rush->gcd() );

  buff.vengeful_retreat_move = new movement_buff_t( this, "vengeful_retreat_movement", spell_data_t::nil() );
  buff.vengeful_retreat_move
    ->set_chance( 1.0 )
    ->set_duration( talent.demon_hunter.vengeful_retreat->duration() );

  buff.metamorphosis_move = new movement_buff_t( this, "metamorphosis_movement", spell_data_t::nil() );
  buff.metamorphosis_move
    ->set_chance( 1.0 )
    ->set_duration( 1_s );

  buff.blind_fury = make_buff( this, "blind_fury", talent.havoc.eye_beam )
    ->set_default_value( talent.havoc.blind_fury->effectN( 3 ).resource( RESOURCE_FURY ) / 50 )
    ->set_cooldown( timespan_t::zero() )
    ->set_period( timespan_t::from_millis( 100 ) ) // Fake natural regeneration rate
    ->set_tick_on_application( false )
    ->set_tick_callback( [this]( buff_t* b, int, timespan_t ) {
      resource_gain( RESOURCE_FURY, b->check_value(), gain.blind_fury );
    } );

  buff.blur = make_buff( this, "blur", spec.blur->effectN( 1 ).trigger() )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
    ->set_cooldown( timespan_t::zero() )
    ->add_invalidate( CACHE_LEECH )
    ->add_invalidate( CACHE_DODGE );

  buff.furious_gaze = make_buff( this, "furious_gaze", spec.furious_gaze_buff )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.initiative = make_buff( this, "initiative", spec.initiative_buff )
    ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
    ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );

  buff.momentum = make_buff<damage_buff_t>( this, "momentum", spec.momentum_buff );
  buff.momentum->set_refresh_duration_callback( []( const buff_t* b, timespan_t d ) {
    return std::min( b->remains() + d, 10_s ); // Capped to 10 seconds
  } );

  buff.inner_demon = make_buff( this, "inner_demon", spec.inner_demon_buff );

  buff.restless_hunter = make_buff<damage_buff_t>( this, "restless_hunter", spec.restless_hunter_buff );

  buff.tactical_retreat = make_buff( this, "tactical_retreat", spec.tactical_retreat_buff )
    ->set_default_value_from_effect_type( A_PERIODIC_ENERGIZE )
    ->set_tick_callback( [this]( buff_t* b, int, timespan_t ) {
      resource_gain( RESOURCE_FURY, b->check_value(), gain.tactical_retreat );
    } );

  buff.unbound_chaos = make_buff( this, "unbound_chaos", spec.unbound_chaos_buff )
    ->set_default_value( talent.havoc.unbound_chaos->effectN( 2 ).percent() );

  buff.chaos_theory = make_buff<damage_buff_t>( this, "chaos_theory", spec.chaos_theory_buff );

  buff.growing_inferno = make_buff<buff_t>( this, "growing_inferno", talent.havoc.growing_inferno )
    ->set_default_value( talent.havoc.growing_inferno->effectN( 1 ).percent() )
    ->set_duration( 20_s );
  if ( talent.havoc.growing_inferno->ok() )
    buff.growing_inferno->set_max_stack( (int)( 10 / talent.havoc.growing_inferno->effectN( 1 ).percent() ) );

  // Vengeance ==============================================================

  buff.demon_spikes = new buffs::demon_spikes_t(this);

  buff.painbringer = make_buff( this, "painbringer", spec.painbringer_buff )
                         ->set_default_value( talent.vengeance.painbringer->effectN( 1 ).percent() )
                         ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                         ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                         ->set_period( 0_ms );

  buff.soul_furnace_damage_amp = make_buff( this, "soul_furnace_damage_amp", spec.soul_furnace_damage_amp )->set_default_value_from_effect( 1 );
  buff.soul_furnace_stack = make_buff( this, "soul_furnace_stack", spec.soul_furnace_stack );

  buff.soul_fragments = make_buff( this, "soul_fragments", spec.soul_fragments_buff )
    ->set_max_stack( 10 );

  buff.charred_flesh = make_buff( this, "charred_flesh", spec.charred_flesh_buff )->set_quiet( true );

  buff.soul_barrier = make_buff<absorb_buff_t>( this, "soul_barrier", talent.vengeance.soul_barrier );
  buff.soul_barrier->set_absorb_source( get_stats( "soul_barrier" ) )
    ->set_absorb_gain( get_gain( "soul_barrier" ) )
    ->set_absorb_high_priority( true )  // TOCHECK
    ->set_cooldown( timespan_t::zero() );

  // Set Bonus Items ========================================================

  buff.t29_havoc_4pc = make_buff<damage_buff_t>( this, "seething_chaos", set_bonuses.t29_havoc_4pc->ok() ?
                                                 find_spell( 394934 ) : spell_data_t::not_found() );
  buff.t29_havoc_4pc->set_refresh_behavior( buff_refresh_behavior::DURATION );

  buff.t30_havoc_2pc =
      make_buff<buff_t>( this, "seething_fury",
                         set_bonuses.t30_havoc_2pc->ok() ? set_bonuses.t30_havoc_2pc_buff : spell_data_t::not_found() )
          ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
          ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY );

  buff.t30_havoc_4pc =
      make_buff<buff_t>( this, "seething_potential",
                         set_bonuses.t30_havoc_4pc->ok() ? set_bonuses.t30_havoc_4pc_buff : spell_data_t::not_found() )
          ->set_default_value_from_effect( 1 );

  buff.t30_vengeance_2pc = make_buff<buff_t>( this, "fires_of_fel",
                                              set_bonuses.t30_vengeance_2pc->ok() ? set_bonuses.t30_vengeance_2pc_buff
                                                                                  : spell_data_t::not_found() )
                               ->set_default_value_from_effect( 1 )
                               ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  buff.t30_vengeance_4pc = make_buff<buff_t>(
      this, "recrimination", set_bonuses.t30_vengeance_4pc->ok() ? set_bonuses.t30_vengeance_4pc_buff : spell_data_t::not_found() );
}

struct metamorphosis_adjusted_cooldown_expr_t : public expr_t
{
  demon_hunter_t* dh;
  double cooldown_multiplier;

  metamorphosis_adjusted_cooldown_expr_t( demon_hunter_t* p, util::string_view name_str )
    : expr_t( name_str ), dh( p ), cooldown_multiplier( 1.0 )
  {
  }

  void calculate_multiplier()
  {
    double reduction_per_second = 0.0;
    cooldown_multiplier = 1.0 / ( 1.0 + reduction_per_second );
  }

  double evaluate() override
  {
    return dh->cooldown.metamorphosis->remains().total_seconds() * cooldown_multiplier;
  }
};

struct eye_beam_adjusted_cooldown_expr_t : public expr_t
{
  demon_hunter_t* dh;
  double cooldown_multiplier;

  eye_beam_adjusted_cooldown_expr_t( demon_hunter_t* p, util::string_view name_str )
    : expr_t( name_str ), dh( p ), cooldown_multiplier( 1.0 )
  {}

  void calculate_multiplier()
  {
    double reduction_per_second = 0.0;

    if ( dh->talent.havoc.cycle_of_hatred->ok() )
    {
      /* NYI
      action_t* chaos_strike = dh->find_action( "chaos_strike" );
      assert( chaos_strike );

      // Fury estimates are on the conservative end, intended to be rough approximation only
      double approx_fury_per_second = 15.5;

      // Use base haste only for approximation, don't want to calculate with temp buffs
      const double base_haste = 1.0 / dh->collected_data.buffed_stats_snapshot.attack_haste;
      approx_fury_per_second *= base_haste;

      // Assume 90% of Fury used on Chaos Strike/Annihilation
      const double approx_seconds_per_refund = ( chaos_strike->cost() / ( approx_fury_per_second * 0.9 ) )
        / dh->spec.chaos_strike_refund->proc_chance();

      if ( dh->talent.cycle_of_hatred->ok() )
        reduction_per_second += dh->talent.cycle_of_hatred->effectN( 1 ).base_value() / approx_seconds_per_refund;
        */
    }

    cooldown_multiplier = 1.0 / ( 1.0 + reduction_per_second );
  }

  double evaluate() override
  {
    // Need to calculate shoulders on first evaluation because we don't have crit/haste values on init
    if ( cooldown_multiplier == 1 && dh->talent.havoc.cycle_of_hatred->ok() )
    {
      calculate_multiplier();
    }

    return dh->cooldown.eye_beam->remains().total_seconds() * cooldown_multiplier;
  }
};

// demon_hunter_t::create_expression ========================================

std::unique_ptr<expr_t> demon_hunter_t::create_expression( util::string_view name_str )
{
  if ( name_str == "greater_soul_fragments" || name_str == "lesser_soul_fragments" ||
       name_str == "soul_fragments" || name_str == "demon_soul_fragments" )
  {
    struct soul_fragments_expr_t : public expr_t
    {
      demon_hunter_t* dh;
      soul_fragment type;

      soul_fragments_expr_t( demon_hunter_t* p, util::string_view n, soul_fragment t )
        : expr_t( n ), dh( p ), type( t )
      {
      }

      double evaluate() override
      {
        return dh->get_active_soul_fragments( type );
      }
    };

    soul_fragment type = soul_fragment::LESSER;

    if ( name_str == "soul_fragments" )
    {
      type = soul_fragment::ANY;
    }
    else if ( name_str == "greater_soul_fragments" )
    {
      type = soul_fragment::ANY_GREATER;
    }
    else if ( name_str == "demon_soul_fragments" )
    {
      type = soul_fragment::ANY_DEMON;
    }

    return std::make_unique<soul_fragments_expr_t>( this, name_str, type );
  }
  else if ( name_str == "cooldown.metamorphosis.adjusted_remains" )
  {
    return this->cooldown.metamorphosis->create_expression( "remains" );
  }
  else if ( name_str == "cooldown.eye_beam.adjusted_remains" )
  {
    if ( this->talent.havoc.cycle_of_hatred->ok() )
    {
      return std::make_unique<eye_beam_adjusted_cooldown_expr_t>( this, name_str );
    }
    else
    {
      return this->cooldown.eye_beam->create_expression( "remains" );
    }
  }
  else if ( util::str_compare_ci( name_str, "darkglare_boon_cdr_roll" ) )
  {
    return make_mem_fn_expr(name_str, *this, &demon_hunter_t::darkglare_boon_cdr_roll);
  }
  else if ( util::str_compare_ci( name_str, "darkglare_boon_cdr_high_roll" ) )
  {
    return make_fn_expr( name_str, [this] () {
      return this->darkglare_boon_cdr_roll >= this->options.darkglare_boon_cdr_high_roll_seconds;
    });
  }
  else if ( util::str_compare_ci( name_str, "fiery_brand_dot_primary_remains") )
  {
    return make_fn_expr( name_str, [this] () {
      auto active_enemies = this->sim->target_non_sleeping_list;
      auto primary_idx = std::find_if(active_enemies.begin(), active_enemies.end(), [this](player_t* target) {
        auto td = this->get_target_data(target);
        auto is_ticking = td->dots.fiery_brand->is_ticking();
        if (!is_ticking) {
          return false;
        }
        auto action_state = td->dots.fiery_brand->state;
        return debug_cast<actions::spells::fiery_brand_t::fiery_brand_state_t*>(action_state)->primary;
      });
      if (primary_idx == active_enemies.end()) {
        return 0_ms;
      }

      return this->get_target_data(*primary_idx)->dots.fiery_brand->remains();
    });
  }
  else if ( util::str_compare_ci( name_str, "fiery_brand_dot_primary_ticking") )
  {
    return make_fn_expr( name_str, [this] () {
      auto active_enemies = this->sim->target_non_sleeping_list;
      auto primary_idx = std::find_if(active_enemies.begin(), active_enemies.end(), [this](player_t* target) {
        auto td = this->get_target_data(target);
        auto is_ticking = td->dots.fiery_brand->is_ticking();
        if (!is_ticking) {
          return false;
        }
        auto action_state = td->dots.fiery_brand->state;
        return debug_cast<actions::spells::fiery_brand_t::fiery_brand_state_t*>(action_state)->primary;
      });
      if (primary_idx == active_enemies.end()) {
        return false;
      }

      return this->get_target_data(*primary_idx)->dots.fiery_brand->is_ticking();
    });
  }
  else if ( util::str_compare_ci( name_str, "seething_fury_threshold" ) )
  {
    return expr_t::create_constant( "seething_fury_threshold",
                                    this->set_bonuses.t30_havoc_2pc->effectN( 1 ).base_value() );
  } 
  else if ( util::str_compare_ci( name_str, "seething_fury_spent" ) )
  {
    return make_mem_fn_expr( "seething_fury_spent", this->set_bonuses,
                             &demon_hunter_t::set_bonuses_t::t30_havoc_2pc_fury_tracker );
  }
  else if ( util::str_compare_ci( name_str, "seething_fury_deficit" ) )
  {
    if ( this->set_bonuses.t30_havoc_2pc->ok() )
    {
      return make_fn_expr( "seething_fury_deficit", [ this ] {
        return this->set_bonuses.t30_havoc_2pc->effectN( 1 ).base_value() - this->set_bonuses.t30_havoc_2pc_fury_tracker;
      } );
    } 
    else {
      return expr_t::create_constant( "seething_fury_deficit", 0.0 );
    }
  }

  return player_t::create_expression( name_str );
}

// demon_hunter_t::create_options
// ==================================================

void demon_hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "target_reach", options.target_reach ) );
  add_option( opt_float( "movement_direction_factor", options.movement_direction_factor, 1.0, 2.0 ) );
  add_option( opt_float( "initial_fury", options.initial_fury, 0.0, 120 ) );
  add_option( opt_int( "fodder_to_the_flame_kill_seconds", options.fodder_to_the_flame_kill_seconds, 0, 10 ) );
  add_option( opt_float( "fodder_to_the_flame_initiative_chance", options.fodder_to_the_flame_initiative_chance, 0, 1 ) );
  add_option( opt_float( "darkglare_boon_cdr_high_roll_seconds", options.darkglare_boon_cdr_high_roll_seconds, 6, 24 ) );
  add_option( opt_float( "soul_fragment_movement_consume_chance", options.soul_fragment_movement_consume_chance, 0, 1 ) );
}

// demon_hunter_t::create_pet ===============================================

pet_t* demon_hunter_t::create_pet( util::string_view pet_name,
                                   util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  // Add pets here

  return nullptr;
}

// demon_hunter_t::create_profile ===========================================

std::string demon_hunter_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  // Log all options here

  return profile_str;
}

// demon_hunter_t::init_absorb_priority =====================================

void demon_hunter_t::init_absorb_priority()
{
  player_t::init_absorb_priority();

  absorb_priority.push_back( 227225 );  // Soul Barrier
}

// demon_hunter_t::init_action_list =========================================

void demon_hunter_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE ||
       off_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
    {
      sim->errorf(
        "Player %s does not have a valid main-hand and off-hand weapon.",
        name() );
    }
    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    demon_hunter_apl::havoc( this );
  }
  else if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    demon_hunter_apl::vengeance( this );
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

// demon_hunter_t::init_base_stats ==========================================

void demon_hunter_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5.0;

  base_t::init_base_stats();

  resources.base[ RESOURCE_FURY ] = 100;
  resources.base[ RESOURCE_FURY ] += talent.demon_hunter.unrestrained_fury->effectN( 1 ).base_value();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Avoidance diminishing Returns constants/conversions now handled in
  // player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// demon_hunter_t::init_procs ===============================================

void demon_hunter_t::init_procs()
{
  base_t::init_procs();

  // General
  proc.delayed_aa_range                 = get_proc( "delayed_aa_out_of_range" );
  proc.relentless_pursuit               = get_proc( "relentless_pursuit" );
  proc.soul_fragment_greater            = get_proc( "soul_fragment_greater" );
  proc.soul_fragment_greater_demon      = get_proc( "soul_fragment_greater_demon" );
  proc.soul_fragment_empowered_demon    = get_proc( "soul_fragment_empowered_demon" );
  proc.soul_fragment_lesser             = get_proc( "soul_fragment_lesser" );
  proc.felblade_reset                   = get_proc( "felblade_reset" );

  // Havoc
  proc.demonic_appetite                 = get_proc( "demonic_appetite" );
  proc.demons_bite_in_meta              = get_proc( "demons_bite_in_meta" );
  proc.chaos_strike_in_essence_break    = get_proc( "chaos_strike_in_essence_break" );
  proc.annihilation_in_essence_break    = get_proc( "annihilation_in_essence_break" );
  proc.blade_dance_in_essence_break     = get_proc( "blade_dance_in_essence_break" );
  proc.death_sweep_in_essence_break     = get_proc( "death_sweep_in_essence_break" );
  proc.chaos_strike_in_serrated_glaive  = get_proc( "chaos_strike_in_serrated_glaive" );
  proc.annihilation_in_serrated_glaive  = get_proc( "annihilation_in_serrated_glaive" );
  proc.eye_beam_tick_in_serrated_glaive = get_proc( "eye_beam_tick_in_serrated_glaive" );
  proc.shattered_destiny                = get_proc( "shattered_destiny" );
  proc.eye_beam_canceled                = get_proc( "eye_beam_canceled" );

  // Vengeance
  proc.soul_fragment_expire             = get_proc( "soul_fragment_expire" );
  proc.soul_fragment_overflow           = get_proc( "soul_fragment_overflow" );
  proc.soul_fragment_from_shear         = get_proc( "soul_fragment_from_shear" );
  proc.soul_fragment_from_fracture      = get_proc( "soul_fragment_from_fracture" );
  proc.soul_fragment_from_fallout       = get_proc( "soul_fragment_from_fallout" );
  proc.soul_fragment_from_meta          = get_proc( "soul_fragment_from_meta" );
  proc.soul_fragment_from_hunger        = get_proc( "soul_fragment_from_hunger" );

  // Set Bonuses
  proc.soul_fragment_from_t29_2pc       = get_proc( "soul_fragment_from_t29_2pc" );
}

// demon_hunter_t::init_resources ===========================================

void demon_hunter_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.current[ RESOURCE_FURY ] = options.initial_fury;
  expected_max_health = calculate_expected_max_health();
}

// demon_hunter_t::init_special_effects =====================================

void demon_hunter_t::init_special_effects()
{
  base_t::init_special_effects();

  if ( talent.havoc.fodder_to_the_flame->ok() )
  {
    auto const fodder_to_the_flame_driver = new special_effect_t( this );
    fodder_to_the_flame_driver->name_str = "fodder_to_the_flame_driver";
    fodder_to_the_flame_driver->spell_id = spell.fodder_to_the_flame->id();
    special_effects.push_back( fodder_to_the_flame_driver );

    auto cb = new actions::spells::fodder_to_the_flame_cb_t( this, *fodder_to_the_flame_driver );
    cb->initialize();
  }
}

// demon_hunter_t::init_rng =================================================

void demon_hunter_t::init_rng()
{
  // RPPM objects

  // General
  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    rppm.felblade = get_rppm( "felblade", spell.felblade_reset_havoc );
    rppm.demonic_appetite = get_rppm( "demonic_appetite", talent.havoc.demonic_appetite );
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    rppm.felblade = get_rppm( "felblade", spell.felblade_reset_vengeance );
  }

  player_t::init_rng();
}

// demon_hunter_t::init_scaling =============================================

void demon_hunter_t::init_scaling()
{
  base_t::init_scaling();

  scaling->enable( STAT_WEAPON_OFFHAND_DPS );

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
    scaling->enable( STAT_BONUS_ARMOR );

  scaling->disable( STAT_STRENGTH );
}

// demon_hunter_t::init_spells ==============================================

void demon_hunter_t::init_spells()
{
  base_t::init_spells();

  // Specialization =========================================================

  // General Passives
  spell.all_demon_hunter        = dbc::get_class_passive( *this, SPEC_NONE );
  spell.chaos_brand             = find_spell( 1490 );
  spell.critical_strikes        = find_spell( 221351 );
  spell.leather_specialization  = find_specialization_spell( "Leather Specialization" );

  spell.demon_soul              = find_spell( 163073 );
  spell.demon_soul_empowered    = find_spell( 347765 );
  spell.soul_fragment           = find_spell( 204255 );

  // Shared Abilities
  spell.disrupt                 = find_class_spell( "Disrupt" );
  spell.immolation_aura         = find_class_spell( "Immolation Aura" );
  spell.immolation_aura_2       = find_rank_spell( "Immolation Aura", "Rank 2" );

  // Spec-Overriden Passives
  spec.demonic_wards       = find_specialization_spell( "Demonic Wards" );
  spec.demonic_wards_2     = find_rank_spell( "Demonic Wards", "Rank 2" );
  spec.demonic_wards_3     = find_rank_spell( "Demonic Wards", "Rank 3" );
  spec.immolation_aura_cdr = find_spell( 320378, DEMON_HUNTER_VENGEANCE );
  spec.thick_skin          = find_specialization_spell( "Thick Skin" );

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    spell.throw_glaive          = find_class_spell( "Throw Glaive" );
    spec.consume_soul_greater   = find_spell( 178963 );
    spec.consume_soul_lesser    = spec.consume_soul_greater;
    spec.metamorphosis          = find_class_spell( "Metamorphosis" );
    spec.metamorphosis_buff     = spec.metamorphosis->effectN( 2 ).trigger();
  }
  else
  {
    spell.throw_glaive          = find_specialization_spell( "Throw Glaive" );
    spec.consume_soul_greater   = find_spell( 210042 );
    spec.consume_soul_lesser    = find_spell( 203794 );
    spec.metamorphosis          = find_specialization_spell( "Metamorphosis" );
    spec.metamorphosis_buff     = spec.metamorphosis;
  }

  // Havoc Spells
  spec.havoc_demon_hunter     = find_specialization_spell( "Havoc Demon Hunter" );

  spec.annihilation           = find_spell( 201427, DEMON_HUNTER_HAVOC );
  spec.blade_dance            = find_specialization_spell( "Blade Dance" );
  spec.blade_dance_2          = find_rank_spell( "Blade Dance", "Rank 2" );
  spec.blur                   = find_specialization_spell( "Blur" );
  spec.chaos_strike           = find_specialization_spell( "Chaos Strike" );
  spec.chaos_strike_fury      = find_spell( 193840, DEMON_HUNTER_HAVOC );
  spec.chaos_strike_refund    = find_spell( 197125, DEMON_HUNTER_HAVOC );
  spec.death_sweep            = find_spell( 210152, DEMON_HUNTER_HAVOC );
  spec.demons_bite            = find_spell( 162243, DEMON_HUNTER_HAVOC );
  spec.fel_rush               = find_specialization_spell( "Fel Rush" );
  spec.fel_rush_damage        = find_spell( 192611, DEMON_HUNTER_HAVOC );
  spec.immolation_aura_3      = find_rank_spell( "Immolation Aura", "Rank 3" );

  // Vengeance Spells
  spec.vengeance_demon_hunter = find_specialization_spell( "Vengeance Demon Hunter" );

  spec.demon_spikes           = find_specialization_spell( "Demon Spikes" );
  spec.infernal_strike        = find_specialization_spell( "Infernal Strike" );
  spec.soul_cleave            = find_specialization_spell( "Soul Cleave" );
  spec.soul_cleave_2          = find_rank_spell( "Soul Cleave", "Rank 2" );
  spec.riposte                = find_specialization_spell( "Riposte" );
  spec.soul_fragments_buff    = find_spell( 203981, DEMON_HUNTER_VENGEANCE );

  // Masteries ==============================================================

  mastery.demonic_presence        = find_mastery_spell( DEMON_HUNTER_HAVOC );
  mastery.fel_blood               = find_mastery_spell( DEMON_HUNTER_VENGEANCE );
  mastery.fel_blood_rank_2        = find_rank_spell( "Mastery: Fel Blood", "Rank 2" );

  // Talents ================================================================

  talent.demon_hunter.vengeful_retreat = find_talent_spell( talent_tree::CLASS, "Vengeful Retreat" );
  talent.demon_hunter.blazing_path = find_talent_spell( talent_tree::CLASS, "Blazing Path" );
  talent.demon_hunter.sigil_of_flame = find_talent_spell( talent_tree::CLASS, "Sigil of Flame" );

  talent.demon_hunter.unrestrained_fury = find_talent_spell( talent_tree::CLASS, "Unrestrained Fury" );
  talent.demon_hunter.imprison = find_talent_spell( talent_tree::CLASS, "Imprison" );
  talent.demon_hunter.shattered_restoration = find_talent_spell( talent_tree::CLASS, "Shattered Restoration" );

  talent.demon_hunter.vengeful_bonds = find_talent_spell( talent_tree::CLASS, "Vengeful Bonds" );
  talent.demon_hunter.improved_disrupt = find_talent_spell( talent_tree::CLASS, "Improved Disrupt" );
  talent.demon_hunter.bouncing_glaives = find_talent_spell( talent_tree::CLASS, "Bouncing Glaives" );
  talent.demon_hunter.consume_magic = find_talent_spell( talent_tree::CLASS, "Consume Magic" );
  talent.demon_hunter.flames_of_fury = find_talent_spell( talent_tree::CLASS, "Flames of Fury" );

  talent.demon_hunter.pursuit = find_talent_spell( talent_tree::CLASS, "Pursuit" );
  talent.demon_hunter.disrupting_fury = find_talent_spell( talent_tree::CLASS, "Disrupting Fury" );
  talent.demon_hunter.aura_of_pain = find_talent_spell( talent_tree::CLASS, "Aura of Pain" );
  talent.demon_hunter.felblade = find_talent_spell( talent_tree::CLASS, "Felblade" );
  talent.demon_hunter.swallowed_anger = find_talent_spell( talent_tree::CLASS, "Swallowed Anger" );
  talent.demon_hunter.charred_warblades = find_talent_spell( talent_tree::CLASS, "Charred Warblades" );

  talent.demon_hunter.felfire_haste = find_talent_spell( talent_tree::CLASS, "Felfire Haste" );
  talent.demon_hunter.master_of_the_glaive = find_talent_spell( talent_tree::CLASS, "Master of the Glaive" );
  talent.demon_hunter.rush_of_chaos = find_talent_spell( talent_tree::CLASS, "Rush of Chaos" );
  talent.demon_hunter.concentrated_sigils = find_talent_spell( talent_tree::CLASS, "Concentrated Sigils" );
  talent.demon_hunter.precise_sigils = find_talent_spell( talent_tree::CLASS, "Precise Sigils" );
  talent.demon_hunter.lost_in_darkness = find_talent_spell( talent_tree::CLASS, "Lost in Darkness" );

  talent.demon_hunter.chaos_nova = find_talent_spell( talent_tree::CLASS, "Chaos Nova" );
  talent.demon_hunter.soul_rending = find_talent_spell( talent_tree::CLASS, "Soul Rending" );
  talent.demon_hunter.infernal_armor = find_talent_spell( talent_tree::CLASS, "Infernal Armor" );
  talent.demon_hunter.sigil_of_misery = find_talent_spell( talent_tree::CLASS, "Sigil of Misery" );

  talent.demon_hunter.chaos_fragments = find_talent_spell( talent_tree::CLASS, "Chaos Fragments" );
  talent.demon_hunter.unleashed_power = find_talent_spell( talent_tree::CLASS, "Unleashed Power" );
  talent.demon_hunter.illidari_knowledge = find_talent_spell( talent_tree::CLASS, "Illidari Knowledge" );
  talent.demon_hunter.demonic = find_talent_spell( talent_tree::CLASS, "Demonic" );
  talent.demon_hunter.first_of_the_illidari = find_talent_spell( talent_tree::CLASS, "First of the Illidari" );
  talent.demon_hunter.will_of_the_illidari = find_talent_spell( talent_tree::CLASS, "Will of the Illidari" );
  talent.demon_hunter.improved_sigil_of_misery = find_talent_spell( talent_tree::CLASS, "Improved Sigil of Misery" );
  talent.demon_hunter.misery_in_defeat = find_talent_spell( talent_tree::CLASS, "Misery in Defeat" );

  talent.demon_hunter.internal_struggle = find_talent_spell( talent_tree::CLASS, "Internal Struggle" );
  talent.demon_hunter.darkness = find_talent_spell( talent_tree::CLASS, "Darkness" );
  talent.demon_hunter.soul_sigils = find_talent_spell( talent_tree::CLASS, "Soul Sigils" );
  talent.demon_hunter.aldrachi_design = find_talent_spell( talent_tree::CLASS, "Aldrachi Design" );

  talent.demon_hunter.erratic_felheart = find_talent_spell( talent_tree::CLASS, "Erratic Felheart" );
  talent.demon_hunter.long_night = find_talent_spell( talent_tree::CLASS, "Long Night" );
  talent.demon_hunter.pitch_black = find_talent_spell( talent_tree::CLASS, "Pitch Black" );
  talent.demon_hunter.the_hunt = find_talent_spell( talent_tree::CLASS, "The Hunt" );
  talent.demon_hunter.demon_muzzle = find_talent_spell( talent_tree::CLASS, "Demon Muzzle" );
  talent.demon_hunter.extended_sigils = find_talent_spell( talent_tree::CLASS, "Extended Sigils" );

  talent.demon_hunter.collective_anguish = find_talent_spell( talent_tree::CLASS, "Collective Anguish" );
  talent.demon_hunter.unnatural_malice = find_talent_spell( talent_tree::CLASS, "Unnatural Malice" );
  talent.demon_hunter.relentless_pursuit = find_talent_spell( talent_tree::CLASS, "Relentless Pursuit" );
  talent.demon_hunter.quickened_sigils = find_talent_spell( talent_tree::CLASS, "Quickened Sigils" );

  // Havoc Talents

  talent.havoc.eye_beam = find_talent_spell( talent_tree::SPECIALIZATION, "Eye Beam" );

  talent.havoc.improved_chaos_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Chaos Strike" );
  talent.havoc.insatiable_hunger = find_talent_spell( talent_tree::SPECIALIZATION, "Insatiable Hunger" );
  talent.havoc.demon_blades = find_talent_spell( talent_tree::SPECIALIZATION, "Demon Blades" );
  talent.havoc.felfire_heart = find_talent_spell( talent_tree::SPECIALIZATION, "Felfire Heart" );

  talent.havoc.demonic_appetite = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Appetite" );
  talent.havoc.improved_fel_rush = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Fel Rush" );
  talent.havoc.first_blood = find_talent_spell( talent_tree::SPECIALIZATION, "First Blood" );
  talent.havoc.furious_throws = find_talent_spell( talent_tree::SPECIALIZATION, "Furious Throws" );
  talent.havoc.burning_hatred = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Hatred" );

  talent.havoc.critical_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Chaos" );
  talent.havoc.mortal_dance = find_talent_spell( talent_tree::SPECIALIZATION, "Mortal Dance" );
  talent.havoc.dancing_with_fate = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing with Fate" );

  talent.havoc.initiative = find_talent_spell( talent_tree::SPECIALIZATION, "Initiative" );
  talent.havoc.desperate_instincts = find_talent_spell( talent_tree::SPECIALIZATION, "Desperate Instincts" );
  talent.havoc.netherwalk = find_talent_spell( talent_tree::SPECIALIZATION, "Netherwalk" );
  talent.havoc.chaotic_transformation = find_talent_spell( talent_tree::SPECIALIZATION, "Chaotic Transformation" );
  talent.havoc.fel_eruption = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Eruption" );
  talent.havoc.trail_of_ruin = find_talent_spell( talent_tree::SPECIALIZATION, "Trail of Ruin" );

  talent.havoc.unbound_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Unbound Chaos" );
  talent.havoc.blind_fury = find_talent_spell( talent_tree::SPECIALIZATION, "Blind Fury" );
  talent.havoc.looks_can_kill = find_talent_spell( talent_tree::SPECIALIZATION, "Looks Can Kill" );
  talent.havoc.serrated_glaive = find_talent_spell( talent_tree::SPECIALIZATION, "Serrated Glaive" );
  talent.havoc.growing_inferno = find_talent_spell( talent_tree::SPECIALIZATION, "Growing Inferno" );

  talent.havoc.tactical_retreat = find_talent_spell( talent_tree::SPECIALIZATION, "Tactical Retreat" );
  talent.havoc.isolated_prey = find_talent_spell( talent_tree::SPECIALIZATION, "Isolated Prey" );
  talent.havoc.furious_gaze = find_talent_spell( talent_tree::SPECIALIZATION, "Furious Gaze" );
  talent.havoc.relentless_onslaught = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Onslaught" );
  talent.havoc.burning_wound = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Wound" );

  talent.havoc.momentum = find_talent_spell( talent_tree::SPECIALIZATION, "Momentum" );
  talent.havoc.chaos_theory = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Theory" );
  talent.havoc.restless_hunter = find_talent_spell( talent_tree::SPECIALIZATION, "Restless Hunter" );
  talent.havoc.inner_demon = find_talent_spell( talent_tree::SPECIALIZATION, "Inner Demon" );
  talent.havoc.accelerated_blade = find_talent_spell( talent_tree::SPECIALIZATION, "Accelerated Blade" );
  talent.havoc.ragefire = find_talent_spell( talent_tree::SPECIALIZATION, "Ragefire" );

  talent.havoc.know_your_enemy = find_talent_spell( talent_tree::SPECIALIZATION, "Know Your Enemy" );
  talent.havoc.glaive_tempest = find_talent_spell( talent_tree::SPECIALIZATION, "Glaive Tempest" );
  talent.havoc.fel_barrage = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Barrage" );
  talent.havoc.cycle_of_hatred = find_talent_spell( talent_tree::SPECIALIZATION, "Cycle of Hatred" );
  talent.havoc.fodder_to_the_flame = find_talent_spell( talent_tree::SPECIALIZATION, "Fodder to the Flame" );
  talent.havoc.elysian_decree = find_talent_spell( talent_tree::SPECIALIZATION, "Elysian Decree" );
  talent.havoc.soulrend = find_talent_spell( talent_tree::SPECIALIZATION, "Soulrend" );

  talent.havoc.essence_break = find_talent_spell( talent_tree::SPECIALIZATION, "Essence Break" );
  talent.havoc.shattered_destiny = find_talent_spell( talent_tree::SPECIALIZATION, "Shattered Destiny" );
  talent.havoc.any_means_necessary = find_talent_spell( talent_tree::SPECIALIZATION, "Any Means Necessary" );

  // Vengeance Talents

  talent.vengeance.fel_devastation = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Devastation" );

  talent.vengeance.frailty     = find_talent_spell( talent_tree::SPECIALIZATION, "Frailty" );
  talent.vengeance.fiery_brand = find_talent_spell( talent_tree::SPECIALIZATION, "Fiery Brand" );

  talent.vengeance.perfectly_balanced_glaive =
      find_talent_spell( talent_tree::SPECIALIZATION, "Perfectly Balanced Glaive" );
  talent.vengeance.deflecting_spikes = find_talent_spell( talent_tree::SPECIALIZATION, "Deflecting Spikes" );
  talent.vengeance.meteoric_strikes  = find_talent_spell( talent_tree::SPECIALIZATION, "Meteoric Strikes" );

  talent.vengeance.shear_fury       = find_talent_spell( talent_tree::SPECIALIZATION, "Shear Fury" );
  talent.vengeance.fracture         = find_talent_spell( talent_tree::SPECIALIZATION, "Fracture" );
  talent.vengeance.calcified_spikes = find_talent_spell( talent_tree::SPECIALIZATION, "Calcified Spikes" );
  talent.vengeance.roaring_fire     = find_talent_spell( talent_tree::SPECIALIZATION, "Roaring Fire" );
  talent.vengeance.sigil_of_silence = find_talent_spell( talent_tree::SPECIALIZATION, "Sigil of Silence" );
  talent.vengeance.retaliation      = find_talent_spell( talent_tree::SPECIALIZATION, "Retaliation" );
  talent.vengeance.fel_flame_fortification =
      find_talent_spell( talent_tree::SPECIALIZATION, "Fel Flame Fortification" );

  talent.vengeance.spirit_bomb      = find_talent_spell( talent_tree::SPECIALIZATION, "Spirit Bomb" );
  talent.vengeance.feast_of_souls   = find_talent_spell( talent_tree::SPECIALIZATION, "Feast of Souls" );
  talent.vengeance.agonizing_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Agonizing Flames" );
  talent.vengeance.extended_spikes  = find_talent_spell( talent_tree::SPECIALIZATION, "Extended Spikes" );
  talent.vengeance.burning_blood    = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Blood" );
  talent.vengeance.soul_barrier     = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Barrier" );
  talent.vengeance.bulk_extraction  = find_talent_spell( talent_tree::SPECIALIZATION, "Bulk Extraction" );
  talent.vengeance.sigil_of_chains  = find_talent_spell( talent_tree::SPECIALIZATION, "Sigil of Chains" );

  talent.vengeance.void_reaver         = find_talent_spell( talent_tree::SPECIALIZATION, "Void Reaver" );
  talent.vengeance.fallout             = find_talent_spell( talent_tree::SPECIALIZATION, "Fallout" );
  talent.vengeance.ruinous_bulwark     = find_talent_spell( talent_tree::SPECIALIZATION, "Ruinous Bulwark" );
  talent.vengeance.volatile_flameblood = find_talent_spell( talent_tree::SPECIALIZATION, "Volatile Flameblood" );
  talent.vengeance.revel_in_pain       = find_talent_spell( talent_tree::SPECIALIZATION, "Revel in Pain" );

  talent.vengeance.soul_furnace    = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Furnace" );
  talent.vengeance.painbringer     = find_talent_spell( talent_tree::SPECIALIZATION, "Painbringer" );
  talent.vengeance.darkglare_boon  = find_talent_spell( talent_tree::SPECIALIZATION, "Darkglare Boon" );
  talent.vengeance.fiery_demise    = find_talent_spell( talent_tree::SPECIALIZATION, "Fiery Demise" );
  talent.vengeance.chains_of_anger = find_talent_spell( talent_tree::SPECIALIZATION, "Chains of Anger" );

  talent.vengeance.focused_cleave   = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Cleave" );
  talent.vengeance.soulmonger       = find_talent_spell( talent_tree::SPECIALIZATION, "Soulmonger" );
  talent.vengeance.stoke_the_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Stoke the Flames" );
  talent.vengeance.burning_alive    = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Alive" );
  talent.vengeance.cycle_of_binding = find_talent_spell( talent_tree::SPECIALIZATION, "Cycle of Binding" );

  talent.vengeance.vulnerability  = find_talent_spell( talent_tree::SPECIALIZATION, "Vulnerability" );
  talent.vengeance.feed_the_demon = find_talent_spell( talent_tree::SPECIALIZATION, "Feed the Demon" );
  talent.vengeance.charred_flesh  = find_talent_spell( talent_tree::SPECIALIZATION, "Charred Flesh" );

  talent.vengeance.soulcrush           = find_talent_spell( talent_tree::SPECIALIZATION, "Soulcrush" );
  talent.vengeance.soul_carver         = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Carver" );
  talent.vengeance.last_resort         = find_talent_spell( talent_tree::SPECIALIZATION, "Last Resort" );
  talent.vengeance.fodder_to_the_flame = find_talent_spell( talent_tree::SPECIALIZATION, "Fodder to the Flame" );
  talent.vengeance.elysian_decree      = find_talent_spell( talent_tree::SPECIALIZATION, "Elysian Decree" );
  talent.vengeance.down_in_flames      = find_talent_spell( talent_tree::SPECIALIZATION, "Down in Flames" );

  // Class Background Spells
  spell.felblade_damage = talent.demon_hunter.felblade->ok() ? find_spell( 213243 ) : spell_data_t::not_found();
  spell.felblade_reset_havoc = talent.demon_hunter.felblade->ok() ? find_spell( 236167 ) : spell_data_t::not_found();
  spell.felblade_reset_vengeance = talent.demon_hunter.felblade->ok() ? find_spell( 203557 ) : spell_data_t::not_found();
  spell.infernal_armor_damage = talent.demon_hunter.infernal_armor->ok() ? find_spell( 320334 ) : spell_data_t::not_found();
  spell.immolation_aura_damage = spell.immolation_aura_2->ok() ? find_spell( 258921 ) : spell_data_t::not_found();
  spell.sigil_of_flame_damage = talent.demon_hunter.sigil_of_flame->ok() ? find_spell( 204598 ) : spell_data_t::not_found();
  spell.sigil_of_flame_fury = talent.demon_hunter.sigil_of_flame->ok() ? find_spell( 389787 ) : spell_data_t::not_found();
  spell.the_hunt = talent.demon_hunter.the_hunt;

  // Spec Background Spells
  mastery.any_means_necessary = talent.havoc.any_means_necessary;
  mastery.any_means_necessary_tuning = talent.havoc.any_means_necessary->ok() ? find_spell( 394486 ) : spell_data_t::not_found();

  spec.burning_wound_debuff = talent.havoc.burning_wound->effectN( 1 ).trigger();
  spec.chaos_theory_buff = talent.havoc.chaos_theory->ok() ? find_spell( 390195 ) : spell_data_t::not_found();
  spec.demon_blades_damage = talent.havoc.demon_blades->effectN( 1 ).trigger();
  spec.essence_break_debuff = talent.havoc.essence_break->ok() ? find_spell( 320338 ) : spell_data_t::not_found();
  spec.eye_beam_damage = talent.havoc.eye_beam->ok() ? find_spell( 198030 ) : spell_data_t::not_found();
  spec.demonic_appetite_fury = talent.havoc.demonic_appetite->ok() ? find_spell( 210041 ) : spell_data_t::not_found();
  spec.furious_gaze_buff = talent.havoc.furious_gaze->ok() ? find_spell( 343312 ) : spell_data_t::not_found();
  spec.first_blood_blade_dance_damage = talent.havoc.first_blood->ok() ? find_spell( 391374 ) : spell_data_t::not_found();
  spec.first_blood_blade_dance_2_damage = talent.havoc.first_blood->ok() ? find_spell( 391378 ) : spell_data_t::not_found();
  spec.first_blood_death_sweep_damage = talent.havoc.first_blood->ok() ? find_spell( 393055 ) : spell_data_t::not_found();
  spec.first_blood_death_sweep_2_damage = talent.havoc.first_blood->ok() ? find_spell( 393054 ) : spell_data_t::not_found();
  spec.glaive_tempest_damage = talent.havoc.glaive_tempest->ok() ? find_spell( 342857 ) : spell_data_t::not_found();
  spec.initiative_buff = talent.havoc.initiative->ok() ? find_spell( 391215 ) : spell_data_t::not_found();
  spec.inner_demon_buff = talent.havoc.inner_demon->ok() ? find_spell( 390145 ) : spell_data_t::not_found();
  spec.inner_demon_damage = talent.havoc.inner_demon->ok() ? find_spell( 390137 ) : spell_data_t::not_found();
  spec.isolated_prey_fury = talent.havoc.isolated_prey->ok() ? find_spell( 357323 ) : spell_data_t::not_found();
  spec.momentum_buff = talent.havoc.momentum->ok() ? find_spell( 208628 ) : spell_data_t::not_found();
  spec.ragefire_damage = talent.havoc.ragefire->ok() ? find_spell( 390197 ) : spell_data_t::not_found();
  spec.restless_hunter_buff = talent.havoc.restless_hunter->ok() ? find_spell( 390212 ) : spell_data_t::not_found();
  spec.serrated_glaive_debuff = talent.havoc.serrated_glaive->effectN( 1 ).trigger();
  spec.soulrend_debuff = talent.havoc.soulrend->ok() ? find_spell( 390181 ) : spell_data_t::not_found();
  spec.tactical_retreat_buff = talent.havoc.tactical_retreat->ok() ? find_spell( 389890 ) : spell_data_t::not_found();
  spec.unbound_chaos_buff = talent.havoc.unbound_chaos->ok() ? find_spell( 347462 ) : spell_data_t::not_found();

  spec.fiery_brand_debuff   = talent.vengeance.fiery_brand->ok() ? find_spell( 207771 ) : spell_data_t::not_found();
  spec.frailty_debuff       = talent.vengeance.frailty->ok() ? find_spell( 247456 ) : spell_data_t::not_found();
  spec.charred_flesh_buff   = talent.vengeance.charred_flesh->ok() ? find_spell( 336640 ) : spell_data_t::not_found();
  spec.painbringer_buff     = talent.vengeance.painbringer->ok() ? find_spell( 212988 ) : spell_data_t::not_found();
  spec.soul_furnace_damage_amp = talent.vengeance.soul_furnace->ok() ? find_spell( 391172 ) : spell_data_t::not_found();
  spec.soul_furnace_stack      = talent.vengeance.soul_furnace->ok() ? find_spell( 391166 ) : spell_data_t::not_found();

  if ( talent.havoc.elysian_decree->ok() || talent.vengeance.elysian_decree->ok() )
  {
    spell.elysian_decree = talent.havoc.elysian_decree;
    spell.elysian_decree_damage = find_spell( 389860 );
  }
  else
  {
    spell.elysian_decree = spell_data_t::not_found();
    spell.elysian_decree_damage = spell_data_t::not_found();
  }

  if ( talent.havoc.fodder_to_the_flame->ok() || talent.vengeance.fodder_to_the_flame->ok() )
  {
    spell.fodder_to_the_flame = talent.havoc.fodder_to_the_flame;
    spell.fodder_to_the_flame_damage = find_spell( 350631 ); // Reused
  }
  else
  {
    spell.fodder_to_the_flame = spell_data_t::not_found();
    spell.fodder_to_the_flame_damage = spell_data_t::not_found();
  }

  if ( talent.demon_hunter.collective_anguish->ok() )
  {
    spell.collective_anguish = specialization() == DEMON_HUNTER_HAVOC ? find_spell( 393831 ) : find_spell( 391057 );
    spell.collective_anguish_damage = ( specialization() == DEMON_HUNTER_HAVOC ?
                                        spell.collective_anguish->effectN( 1 ).trigger() :
                                        find_spell( 391058 ) );
  }
  else
  {
    spell.collective_anguish = spell_data_t::not_found();
    spell.collective_anguish_damage = spell_data_t::not_found();
  }

  // Set Bonus Items ========================================================

  set_bonuses.t29_havoc_2pc     = sets->set( DEMON_HUNTER_HAVOC, T29, B2 );
  set_bonuses.t29_havoc_4pc     = sets->set( DEMON_HUNTER_HAVOC, T29, B4 );
  set_bonuses.t29_vengeance_2pc = sets->set( DEMON_HUNTER_VENGEANCE, T29, B2 );
  set_bonuses.t29_vengeance_4pc = sets->set( DEMON_HUNTER_VENGEANCE, T29, B4 );
  set_bonuses.t30_havoc_2pc     = sets->set( DEMON_HUNTER_HAVOC, T30, B2 );
  set_bonuses.t30_havoc_4pc     = sets->set( DEMON_HUNTER_HAVOC, T30, B4 );
  set_bonuses.t30_vengeance_2pc = sets->set( DEMON_HUNTER_VENGEANCE, T30, B2 );
  set_bonuses.t30_vengeance_4pc = sets->set( DEMON_HUNTER_VENGEANCE, T30, B4 );

  // Set Bonus Auxilliary
  set_bonuses.t30_havoc_2pc_buff = set_bonuses.t30_havoc_2pc->ok() ? find_spell( 408737 ) : spell_data_t::not_found();
  set_bonuses.t30_havoc_4pc_buff = set_bonuses.t30_havoc_4pc->ok() ? find_spell( 408754 ) : spell_data_t::not_found();
  set_bonuses.t30_havoc_4pc_refund = set_bonuses.t30_havoc_4pc->ok() ? find_spell( 408757 ) : spell_data_t::not_found();
  set_bonuses.t30_vengeance_2pc_buff = set_bonuses.t30_vengeance_2pc->ok() ? find_spell( 409645 ) : spell_data_t::not_found();
  set_bonuses.t30_vengeance_4pc_buff = set_bonuses.t30_vengeance_4pc->ok() ? find_spell( 409877 ) : spell_data_t::not_found();

  // Spell Initialization ===================================================

  using namespace actions::attacks;
  using namespace actions::spells;
  using namespace actions::heals;

  active.consume_soul_greater = new consume_soul_t( this, "consume_soul_greater", spec.consume_soul_greater, soul_fragment::GREATER );
  active.consume_soul_lesser = new consume_soul_t( this, "consume_soul_lesser", spec.consume_soul_lesser, soul_fragment::LESSER );

  active.burning_wound = get_background_action<burning_wound_t>( "burning_wound" );

  if ( talent.havoc.demon_blades->ok() )
  {
    active.demon_blades = new demon_blades_t( this );
  }

  if ( talent.demon_hunter.collective_anguish->ok() )
  {
    active.collective_anguish = get_background_action<collective_anguish_t>( "collective_anguish" );
  }

  if ( talent.havoc.relentless_onslaught->ok() )
  {
    active.relentless_onslaught = get_background_action<chaos_strike_t>( "chaos_strike_onslaught", "", true );
    active.relentless_onslaught_annihilation = get_background_action<annihilation_t>( "annihilation_onslaught", "", true );
  }

  if ( talent.havoc.inner_demon->ok() )
  {
    active.inner_demon = get_background_action<inner_demon_t>( "inner_demon" );
  }

  if ( talent.havoc.ragefire->ok() )
  {
    active.ragefire = get_background_action<ragefire_t>( "ragefire" );
  }

  if ( set_bonuses.t30_vengeance_4pc->ok() )
  {
    fiery_brand_t* fiery_brand_t30 = get_background_action<fiery_brand_t>( "fiery_brand_t30" );
    fiery_brand_t30->internal_cooldown->base_duration = 0_s;
    fiery_brand_t30->cooldown->base_duration = 0_s;
    fiery_brand_t30->cooldown->charges = 0;
    fiery_brand_t30->dot_action->dot_duration = timespan_t::from_seconds(set_bonuses.t30_vengeance_4pc->effectN(2).base_value());
    active.fiery_brand_t30 = fiery_brand_t30;
  }
}

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
      if ( buff.demon_spikes->check() )
        invalidate_cache( CACHE_ARMOR );
      break;
    default:
      break;
  }
}

// demon_hunter_t::primary_resource =========================================

resource_e demon_hunter_t::primary_resource() const
{
  switch (specialization())
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
  switch (specialization())
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
      return demon_hunter_apl::flask_vengeance( this );
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
  switch (specialization())
  {
    case DEMON_HUNTER_VENGEANCE:
      return demon_hunter_apl::temporary_enchant_vengeance( this );
    default:
      return demon_hunter_apl::temporary_enchant_havoc( this );
  }
}

// ==========================================================================
// custom demon_hunter_t init functions
// ==========================================================================

// demon_hunter_t::create_cooldowns =========================================

void demon_hunter_t::create_cooldowns()
{
  // General
  cooldown.consume_magic        = get_cooldown( "consume_magic" );
  cooldown.disrupt              = get_cooldown( "disrupt" );
  cooldown.elysian_decree       = get_cooldown( "elysian_decree" );
  cooldown.felblade             = get_cooldown( "felblade" );
  cooldown.fel_eruption         = get_cooldown( "fel_eruption" );
  cooldown.immolation_aura      = get_cooldown( "immolation_aura" );
  cooldown.the_hunt             = get_cooldown( "the_hunt" );

  // Havoc
  cooldown.blade_dance              = get_cooldown( "blade_dance" );
  cooldown.blur                     = get_cooldown( "blur" );
  cooldown.chaos_nova               = get_cooldown( "chaos_nova" );
  cooldown.chaos_strike_refund_icd  = get_cooldown( "chaos_strike_refund_icd" );
  cooldown.essence_break            = get_cooldown( "essence_break" );
  cooldown.eye_beam                 = get_cooldown( "eye_beam" );
  cooldown.fel_barrage              = get_cooldown( "fel_barrage" );
  cooldown.fel_rush                 = get_cooldown( "fel_rush" );
  cooldown.metamorphosis            = get_cooldown( "metamorphosis" );
  cooldown.netherwalk               = get_cooldown( "netherwalk" );
  cooldown.relentless_onslaught_icd = get_cooldown( "relentless_onslaught_icd" );
  cooldown.throw_glaive             = get_cooldown( "throw_glaive" );
  cooldown.vengeful_retreat         = get_cooldown( "vengeful_retreat" );
  cooldown.movement_shared          = get_cooldown( "movement_shared" );

  // Vengeance
  cooldown.demon_spikes                      = get_cooldown( "demon_spikes" );
  cooldown.fiery_brand                       = get_cooldown( "fiery_brand" );
  cooldown.sigil_of_chains                   = get_cooldown( "sigil_of_chains" );
  cooldown.sigil_of_flame                    = get_cooldown( "sigil_of_flame" );
  cooldown.sigil_of_misery                   = get_cooldown( "sigil_of_misery" );
  cooldown.sigil_of_silence                  = get_cooldown( "sigil_of_silence" );
  cooldown.fel_devastation                   = get_cooldown( "fel_devastation" );
  cooldown.volatile_flameblood_icd           = get_cooldown( "volatile_flameblood_icd" );
}

// demon_hunter_t::create_gains =============================================

void demon_hunter_t::create_gains()
{
  // General
  gain.miss_refund            = get_gain( "miss_refund" );

  // Havoc
  gain.blind_fury             = get_gain( "blind_fury" );
  gain.isolated_prey          = get_gain( "isolated_prey" );
  gain.tactical_retreat       = get_gain( "tactical_retreat" );

  // Vengeance
  gain.metamorphosis          = get_gain( "metamorphosis" );
  gain.darkglare_boon         = get_gain( "darkglare_boon" );
  gain.volatile_flameblood    = get_gain( "volatile_flameblood" );

  // Set Bonuses
  gain.seething_fury          = get_gain( "seething_fury" );
}

// demon_hunter_t::create_benefits ==========================================

void demon_hunter_t::create_benefits()
{
}

// ==========================================================================
// overridden player_t stat functions
// ==========================================================================

// demon_hunter_t::composite_armor ==========================================

double demon_hunter_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buff.demon_spikes->up() )
  {
    const double mastery_value = cache.mastery() * mastery.fel_blood ->effectN( 1 ).mastery_value();
    a += ( buff.demon_spikes->data().effectN( 2 ).percent() + mastery_value ) * cache.agility();
  }

  return a;
}

// demon_hunter_t::composite_base_armor_multiplier ==========================

double demon_hunter_t::composite_base_armor_multiplier() const
{
  double am = player_t::composite_base_armor_multiplier();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    am *= 1.0 + spec.thick_skin->effectN( 2 ).percent();

    if ( buff.metamorphosis->check() )
    {
      am *= 1.0 + spec.metamorphosis_buff->effectN( 7 ).percent();
    }
  }

  return am;
}

// demon_hunter_t::composite_armor_multiplier ===============================

double demon_hunter_t::composite_armor_multiplier() const
{
  double am = player_t::composite_armor_multiplier();

  if ( buff.immolation_aura->check() )
  {
    am *= 1.0 + talent.demon_hunter.infernal_armor->effectN( 1 ).percent();
  }

  return am;
}

// demon_hunter_t::composite_attack_power_multiplier ========================

double demon_hunter_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  ap *= 1.0 + cache.mastery() * mastery.fel_blood_rank_2->effectN( 1 ).mastery_value();

  return ap;
}

// demon_hunter_t::composite_attribute_multiplier ===========================

double demon_hunter_t::composite_attribute_multiplier( attribute_e a ) const
{
  double am = player_t::composite_attribute_multiplier( a );

  switch ( a )
  {
    case ATTR_STAMINA:
      am *= 1.0 + spec.thick_skin->effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return am;
}

// demon_hunter_t::composite_crit_avoidance =================================

double demon_hunter_t::composite_crit_avoidance() const
{
  double ca = player_t::composite_crit_avoidance();

  ca += spec.thick_skin->effectN( 4 ).percent();

  return ca;
}

// demon_hunter_t::composite_dodge ==========================================

double demon_hunter_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  d += buff.blur->check() * buff.blur->data().effectN( 2 ).percent();

  return d;
}

// demon_hunter_t::composite_melee_haste  ===================================

double demon_hunter_t::composite_melee_haste() const
{
  double mh = player_t::composite_melee_haste();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    mh /= 1.0 + buff.metamorphosis->check_value();
  }

  return mh;
}

// demon_hunter_t::composite_spell_haste ====================================

double demon_hunter_t::composite_spell_haste() const
{
  double sh = player_t::composite_spell_haste();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    sh /= 1.0 + buff.metamorphosis->check_value();
  }

  return sh;
}

// demon_hunter_t::composite_leech ==========================================

double demon_hunter_t::composite_leech() const
{
  double l = player_t::composite_leech();

  if ( buff.metamorphosis->check() )
  {
    if ( specialization() == DEMON_HUNTER_HAVOC )
    {
      l += spec.metamorphosis_buff->effectN( 3 ).percent();
    }

    l += talent.demon_hunter.soul_rending->effectN( 2 ).percent();
  }

  if ( talent.demon_hunter.soul_rending->ok() )
  {
    l += talent.demon_hunter.soul_rending->effectN( 1 ).percent();
  }

  return l;
}

// demon_hunter_t::composite_melee_crit_chance ==============================

double demon_hunter_t::composite_melee_crit_chance() const
{
  double mc = player_t::composite_melee_crit_chance();

  mc += spell.critical_strikes->effectN( 1 ).percent();

  return mc;
}

// demon_hunter_t::composite_melee_expertise ================================

double demon_hunter_t::composite_melee_expertise( const weapon_t* w ) const
{
  double me = player_t::composite_melee_expertise( w );

  me += spec.thick_skin->effectN( 3 ).percent();

  return me;
}

// demon_hunter_t::composite_parry ==========================================

double demon_hunter_t::composite_parry() const
{
  double cp = player_t::composite_parry();

  cp += talent.demon_hunter.aldrachi_design->effectN( 1 ).percent();

  return cp;
}

// demon_hunter_t::composite_parry_rating() =================================

double demon_hunter_t::composite_parry_rating() const
{
  double pr = player_t::composite_parry_rating();

  if ( spec.riposte->ok() )
  {
    pr += composite_melee_crit_rating();
  }

  return pr;
}

// demon_hunter_t::composite_player_multiplier ==============================

double demon_hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

// demon_hunter_t::composite_player_critical_damage_multiplier ==============

double demon_hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  if ( talent.havoc.know_your_enemy->ok() )
  {
    // 2022-11-28 -- Halving this value as it appears this still uses Modify Crit Damage Done% (163)
    //               However, it has been scripted to match the value of Spell Critical Damage (15)
    //               This does affect gear, however, so it is a player rather than spell modifier
    m *= 1.0 + talent.havoc.know_your_enemy->effectN( 2 ).percent() * cache.attack_crit_chance() * 0.5;
  }

  return m;
}

// demon_hunter_t::composite_spell_crit_chance ==============================

double demon_hunter_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  sc += spell.critical_strikes->effectN( 1 ).percent();

  return sc;
}

// demon_hunter_t::composite_mastery ========================================

double demon_hunter_t::composite_mastery() const
{
  double cm = player_t::composite_mastery();

  cm += talent.demon_hunter.internal_struggle->effectN( 1 ).base_value();

  return cm;
}

// demon_hunter_t::composite_damage_versatility ============================

double demon_hunter_t::composite_damage_versatility() const
{
  double cdv = player_t::composite_damage_versatility();

  if ( talent.demon_hunter.first_of_the_illidari->ok() && buff.metamorphosis->check() )
  {
    cdv += talent.demon_hunter.first_of_the_illidari->effectN( 2 ).percent();
  }

  return cdv;
}

// demon_hunter_t::composite_heal_versatility ==============================

double demon_hunter_t::composite_heal_versatility() const
{
  double chv = player_t::composite_heal_versatility();

  if ( talent.demon_hunter.first_of_the_illidari->ok() && buff.metamorphosis->check() )
  {
    chv += talent.demon_hunter.first_of_the_illidari->effectN( 2 ).percent();
  }

  return chv;
}

// demon_hunter_t::composite_mitigation_versatility ========================

double demon_hunter_t::composite_mitigation_versatility() const
{
  double cmv = player_t::composite_mitigation_versatility();

  if ( talent.demon_hunter.first_of_the_illidari->ok() && buff.metamorphosis->check() )
  {
    cmv += talent.demon_hunter.first_of_the_illidari->effectN( 2 ).percent() / 2;
  }

  return cmv;
}

// demon_hunter_t::matching_gear_multiplier =================================

double demon_hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( ( specialization() == DEMON_HUNTER_HAVOC && attr == ATTR_AGILITY ) ||
       ( specialization() == DEMON_HUNTER_VENGEANCE && attr == ATTR_STAMINA ) )
  {
    return spell.leather_specialization->effectN( 1 ).percent();
  }

  return 0.0;
}

// demon_hunter_t::passive_movement_modifier ================================

double demon_hunter_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( talent.demon_hunter.pursuit->ok() )
  {
    ms += cache.mastery() * talent.demon_hunter.pursuit->effectN( 1 ).mastery_value();
  }

  return ms;
}

// demon_hunter_t::temporary_movement_modifier ==============================

double demon_hunter_t::temporary_movement_modifier() const
{
  double ms = player_t::temporary_movement_modifier();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    ms = std::max( ms, buff.immolation_aura->value() );
  }

  return ms;
}

// ==========================================================================
// overridden player_t combat functions
// ==========================================================================

// demon_hunter_t::expected_max_health() ====================================

double demon_hunter_t::calculate_expected_max_health() const
{
  double slot_weights = 0;
  double prop_values = 0;
  for ( auto& item : items )
  {
    if ( item.slot == SLOT_SHIRT || item.slot == SLOT_RANGED ||
         item.slot == SLOT_TABARD || item.item_level() <= 0 )
    {
      continue;
    }

    const random_prop_data_t item_data = dbc->random_property(item.item_level());
    int index = item_database::random_suffix_type(item.parsed.data);
    if ( item_data.p_epic[0] == 0 )
    {
      continue;
    }
    slot_weights += item_data.p_epic[index] / item_data.p_epic[0];

    if (!item.active())
    {
      continue;
    }

    prop_values += item_data.p_epic[index];
  }

  double expected_health = (prop_values / slot_weights) * 8.318556;
  expected_health += base.stats.attribute[STAT_STAMINA];
  expected_health *= 1 + matching_gear_multiplier(ATTR_STAMINA);
  expected_health *= 1 + spec.thick_skin->effectN( 1 ).percent();
  expected_health *= current.health_per_stamina;
  return expected_health;
}

// demon_hunter_t::assess_damage ============================================

void demon_hunter_t::assess_damage( school_e school, result_amount_type dt, action_state_t* s )
{
  player_t::assess_damage( school, dt, s );

  // Benefit tracking
  if ( s->action->may_parry )
  {
    buff.demon_spikes->up();
  }

  if ( active.infernal_armor && buff.immolation_aura->check() && s->action->player->is_enemy() &&
       dbc::is_school( school, SCHOOL_PHYSICAL ) && dt == result_amount_type::DMG_DIRECT && s->action->result_is_hit( s->result ) )
  {
    active.infernal_armor->set_target( s->action->player );
    active.infernal_armor->execute();
  }
}

// demon_hunter_t::combat_begin =============================================

void demon_hunter_t::combat_begin()
{
  player_t::combat_begin();

  // Start event drivers
  if ( talent.vengeance.spirit_bomb->ok() )
  {
    frailty_driver = make_event<frailty_event_t>( *sim, this, true );
  }
}

// demon_hunter_t::interrupt ================================================

void demon_hunter_t::interrupt()
{
  event_t::cancel( soul_fragment_pick_up );
  base_t::interrupt();
}

// demon_hunter_t::regen ====================================================

void demon_hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );
}

// demon_hunter_t::resource_gain ============================================

double demon_hunter_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  return player_t::resource_gain( resource_type, amount, source, action );
}

double demon_hunter_t::resource_gain( resource_e resource_type, double amount, double delta_coeff, gain_t* source, action_t* action )
{
  double modified_amount = static_cast<int>( amount +
    rng().range( 0, 1 + ( amount * delta_coeff ) ) - ( ( amount * delta_coeff ) / 2.0 ) );
  return resource_gain( resource_type, modified_amount, source, action );
}

// demon_hunter_t::recalculate_resource_max =================================

void demon_hunter_t::recalculate_resource_max( resource_e r, gain_t* source )
{
  player_t::recalculate_resource_max( r, source );

  if ( r == RESOURCE_HEALTH )
  {
    // Update Metamorphosis' value for the new health amount.
    if ( specialization() == DEMON_HUNTER_VENGEANCE && buff.metamorphosis->check() )
    {
      assert( metamorphosis_health > 0 );

      double base_health = max_health() - metamorphosis_health;
      double new_health  = base_health * buff.metamorphosis->check_value();
      double diff        = new_health - metamorphosis_health;

      if ( diff != 0.0 )
      {
        if ( sim->debug )
        {
          sim->out_debug.printf(
            "%s adjusts %s temporary health. old=%.0f new=%.0f diff=%.0f",
            name(), buff.metamorphosis->name(), metamorphosis_health,
            new_health, diff );
        }

        resources.max[ RESOURCE_HEALTH ] += diff;
        resources.temporary[ RESOURCE_HEALTH ] += diff;
        if ( diff > 0 )
        {
          resource_gain( RESOURCE_HEALTH, diff );
        }
        else if ( diff < 0 )
        {
          resource_loss( RESOURCE_HEALTH, -diff );
        }

        metamorphosis_health += diff;
      }
    }
  }
}

// demon_hunter_t::target_mitigation ========================================

void demon_hunter_t::target_mitigation( school_e school, result_amount_type dt, action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( s->result_amount <= 0 )
  {
    return;
  }

  if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
  {
    s->result_amount *= 1.0 + talent.demon_hunter.illidari_knowledge->effectN( 1 ).percent();
  }



  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    s->result_amount *= 1.0 + buff.blur->value();

    if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
    {
      s->result_amount *= 1.0 + spec.demonic_wards->effectN( 1 ).percent()
        + spec.demonic_wards_2->effectN( 1 ).percent();
    }
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    s->result_amount *= 1.0 + spec.demonic_wards->effectN( 1 ).percent()
      + spec.demonic_wards_2->effectN( 1 ).percent()
      + spec.demonic_wards_3->effectN( 1 ).percent();

    s->result_amount *= 1.0 + buff.painbringer->check_stack_value();

    const demon_hunter_td_t* td = get_target_data( s->action->player );
    if ( td->dots.fiery_brand && td->dots.fiery_brand->is_ticking() )
    {
      s->result_amount *= 1.0 + spec.fiery_brand_debuff->effectN( 1 ).percent();
    }

    if ( td->debuffs.frailty->check() && talent.vengeance.void_reaver->ok() )
    {
      s->result_amount *= 1.0 + spec.frailty_debuff->effectN( 3 ).percent() * td->debuffs.frailty->check();
    }

    if ( td->debuffs.t29_vengeance_4pc->check() ) {
      s->result_amount *= 1.0 + td->debuffs.t29_vengeance_4pc->check_value();
    }
  }
}

// demon_hunter_t::reset ====================================================

void demon_hunter_t::reset()
{
  base_t::reset();

  soul_fragment_pick_up                                = nullptr;
  frailty_driver                                       = nullptr;
  exit_melee_event                                     = nullptr;
  next_fragment_spawn                                  = 0;
  metamorphosis_health                                 = 0;
  frailty_accumulator                                  = 0.0;
  ragefire_accumulator                                 = 0.0;
  ragefire_crit_accumulator                            = 0;
  shattered_destiny_accumulator                        = 0.0;
  darkglare_boon_cdr_roll                              = 0.0;
  set_bonuses.t30_havoc_2pc_fury_tracker               = 0.0;
  set_bonuses.t30_vengeance_4pc_soul_fragments_tracker = 0.0;

  for ( size_t i = 0; i < soul_fragments.size(); i++ )
  {
    delete soul_fragments[ i ];
  }

  soul_fragments.clear();
}

// demon_hunter_t::merge ==========================================================

void demon_hunter_t::merge( player_t& other )
{
  player_t::merge( other );

  const demon_hunter_t& s = static_cast<demon_hunter_t&>( other );

  for ( size_t i = 0, end = cd_waste_exec.size(); i < end; i++ )
  {
    cd_waste_exec[ i ]->second.merge( s.cd_waste_exec[ i ]->second );
    cd_waste_cumulative[ i ]->second.merge( s.cd_waste_cumulative[ i ]->second );
  }
}

// demon_hunter_t::datacollection_begin ===========================================

void demon_hunter_t::datacollection_begin()
{
  if ( active_during_iteration )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_iter[ i ]->second.reset();
    }
  }

  player_t::datacollection_begin();
}

// shaman_t::datacollection_end =============================================

void demon_hunter_t::datacollection_end()
{
  if ( requires_data_collection() )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_cumulative[ i ]->second.add( cd_waste_iter[ i ]->second.sum() );
    }
  }

  player_t::datacollection_end();
}

// ==========================================================================
// custom demon_hunter_t functions
// ==========================================================================

// demon_hunter_t::set_out_of_range =========================================

void demon_hunter_t::set_out_of_range( timespan_t duration )
{
  if ( duration <= timespan_t::zero() )
  {
    // Cancel all movement buffs and events
    buff.out_of_range->expire();
    buff.fel_rush_move->expire();
    buff.vengeful_retreat_move->expire();
    buff.metamorphosis_move->expire();
    event_t::cancel( exit_melee_event );
  }
  else
  {
    if ( buff.out_of_range->check() )
    {
      buff.out_of_range->extend_duration( this, duration - buff.out_of_range->remains() );
      buff.out_of_range->default_value = cache.run_speed();
    }
    else
    {
      buff.out_of_range->trigger( 1, cache.run_speed(), -1.0, duration );
    }
  }
}

// demon_hunter_t::adjust_movement ==========================================

void demon_hunter_t::adjust_movement()
{
  if ( buff.out_of_range->check() &&
       buff.out_of_range->remains() > timespan_t::zero() )
  {
    // Recalculate movement duration.
    assert( buff.out_of_range->value() > 0 );
    assert( !buff.out_of_range->expiration.empty() );

    timespan_t remains = buff.out_of_range->remains();
    remains *= buff.out_of_range->check_value() / cache.run_speed();

    set_out_of_range( remains );
  }
}

// demon_hunter_t::consume_soul_fragments ===================================

unsigned demon_hunter_t::consume_soul_fragments( soul_fragment type, bool heal, unsigned max )
{
  unsigned souls_consumed = 0;
  std::vector<soul_fragment_t*> candidates;

  // Look through the list of active soul fragments to populate the vector of fragments to remove.
  // We need to use a new list as to not change the underlying vector as we are iterating over it
  // since the consume() method calls erase() on the fragment being consumed
  for ( auto& it : soul_fragments )
  {
    if ( it->is_type( type ) && it->active() )
    {
      candidates.push_back( it );
    }
  }

  for ( auto& it : candidates )
  {
    it->consume( heal );
    souls_consumed++;
    if ( talent.vengeance.soul_furnace->ok() )
    {
      buff.soul_furnace_stack->trigger();
      if ( buff.soul_furnace_stack->at_max_stacks() )
      {
        buff.soul_furnace_stack->expire();
        buff.soul_furnace_damage_amp->trigger();
      }
    }
    if ( set_bonuses.t30_vengeance_4pc->ok())
    {
      set_bonuses.t30_vengeance_4pc_soul_fragments_tracker += 1;
      if ( set_bonuses.t30_vengeance_4pc_soul_fragments_tracker >=
           set_bonuses.t30_vengeance_4pc->effectN( 1 ).base_value() )
      {
        buff.t30_vengeance_4pc->trigger();
        set_bonuses.t30_vengeance_4pc_soul_fragments_tracker -=
            set_bonuses.t30_vengeance_4pc->effectN( 1 ).base_value();
      }
    }
    if ( souls_consumed >= max )
      break;
  }

  if ( sim->debug )
  {
    sim->out_debug.printf( "%s consumes %u %ss. remaining=%u", name(), souls_consumed,
                           get_soul_fragment_str( type ), 0,
                           get_total_soul_fragments( type ) );
  }

  if ( souls_consumed > 0 )
  {
    buff.painbringer->trigger( souls_consumed );
  }

  return souls_consumed;
}

// demon_hunter_t::consume_nearby_soul_fragments ======================================

unsigned demon_hunter_t::consume_nearby_soul_fragments( soul_fragment type )
{
  int soul_fragments_to_consume = 0;

  for ( auto& it : soul_fragments )
  {
    if ( it->is_type( type ) && it->active() && rng().roll( options.soul_fragment_movement_consume_chance ) )
    {
      soul_fragments_to_consume++;
    }
  }

  if ( soul_fragments_to_consume == 0 )
  {
    return 0;
  }

  event_t::cancel( soul_fragment_pick_up );
  return demon_hunter_t::consume_soul_fragments( type, true, soul_fragments_to_consume );
}

// demon_hunter_t::get_active_soul_fragments ================================

unsigned demon_hunter_t::get_active_soul_fragments( soul_fragment type_mask ) const
{
    return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
                            [ &type_mask ]( unsigned acc, soul_fragment_t* frag ) {
                              return acc + ( frag->is_type( type_mask ) && frag->active() );
                            } );
}

// demon_hunter_t::get_total_soul_fragments =================================

unsigned demon_hunter_t::get_total_soul_fragments( soul_fragment type_mask ) const
{
  if ( type_mask == soul_fragment::ANY )
  {
    return (unsigned)soul_fragments.size();
  }

  return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
                          [ &type_mask ]( unsigned acc, soul_fragment_t* frag ) {
                            return acc + frag->is_type( type_mask );
                          } );
}

// demon_hunter_t::activate_soul_fragment ===================================

void demon_hunter_t::activate_soul_fragment( soul_fragment_t* frag )
{
  buff.soul_fragments->trigger();

  // If we spawn a fragment with this flag, instantly consume it
  if ( frag->consume_on_activation )
  {
    frag->consume( true, true );
    return;
  }

  if ( frag->type == soul_fragment::LESSER )
  {
    unsigned active_fragments = get_active_soul_fragments( frag->type );
    if ( active_fragments > MAX_SOUL_FRAGMENTS )
    {
      // Find and delete the oldest active fragment of this type.
      for ( auto& it : soul_fragments )
      {
        if ( it->is_type( soul_fragment::LESSER ) && it->active() )
        {
          if ( specialization() == DEMON_HUNTER_HAVOC )
          {
            it->remove();
          }
          else // DEMON_HUNTER_VENGEANCE
          {
            // 7.2.5 -- When Soul Fragments are created that exceed the cap of 5 active fragments,
            // the oldest fragment is now automatically consumed if it is within 60 yards of the Demon Hunter.
            // If it is more than 60 yds from the Demon Hunter, it despawns.
            it->consume( true );

            if ( sim->debug )
            {
              sim->out_debug.printf( "%s consumes overflow fragment %ss. remaining=%u", name(),
                                     get_soul_fragment_str( soul_fragment::LESSER ), get_total_soul_fragments( soul_fragment::LESSER ) );
            }
          }

          proc.soul_fragment_overflow->occur();
          event_t::cancel( soul_fragment_pick_up );
          break;
        }
      }
    }

    assert( get_active_soul_fragments( soul_fragment::LESSER ) <= MAX_SOUL_FRAGMENTS );
  }
}

// demon_hunter_t::spawn_soul_fragment ======================================

void demon_hunter_t::spawn_soul_fragment( soul_fragment type, unsigned n, bool consume_on_activation )
{
  if ( type == soul_fragment::GREATER && sim->target->race == RACE_DEMON )
  {
    type = soul_fragment::GREATER_DEMON;
  }

  proc_t* soul_proc;
  switch ( type )
  {
    case soul_fragment::GREATER:          soul_proc = proc.soul_fragment_greater; break;
    case soul_fragment::GREATER_DEMON:    soul_proc = proc.soul_fragment_greater_demon; break;
    case soul_fragment::EMPOWERED_DEMON:  soul_proc = proc.soul_fragment_empowered_demon; break;
    default:                              soul_proc = proc.soul_fragment_lesser;
  }

  for ( unsigned i = 0; i < n; i++ )
  {
    soul_fragments.push_back( new soul_fragment_t( this, type, consume_on_activation ) );
    soul_proc->occur();
    // Soul Fragments from Fodder to the Flame do not stack the T30 2pc.
    if ( set_bonuses.t30_vengeance_2pc->ok() && type != soul_fragment::EMPOWERED_DEMON )
    {
      buff.t30_vengeance_2pc->trigger();
    }
  }

  sim->print_log( "{} creates {} {}. active={} total={}", *this, n, get_soul_fragment_str( type ),
                  get_active_soul_fragments( type ), get_total_soul_fragments( type ) );
}

void demon_hunter_t::spawn_soul_fragment( soul_fragment type, unsigned n, player_t* target, bool consume_on_activation )
{
  if ( type == soul_fragment::GREATER && target->race == RACE_DEMON )
  {
    type = soul_fragment::GREATER_DEMON;
  }

  demon_hunter_t::spawn_soul_fragment( type, n, consume_on_activation );
}

// demon_hunter_t::trigger_demonic ==========================================

void demon_hunter_t::trigger_demonic()
{
  if ( !talent.demon_hunter.demonic->ok() )
    return;

  debug_cast<buffs::metamorphosis_buff_t*>( buff.metamorphosis )->trigger_demonic();
}

// demon_hunter_sigil_t::create_sigil_expression ==================================

std::unique_ptr<expr_t> actions::demon_hunter_sigil_t::create_sigil_expression( util::string_view name )
{
  if ( util::str_compare_ci( name, "activation_time" ) || util::str_compare_ci( name, "delay" ) )
  {
    return make_ref_expr( name, sigil_delay );
  }
  else if ( util::str_compare_ci( name, "sigil_placed" ) || util::str_compare_ci( name, "placed" ) )
  {
    return make_fn_expr( name, [ this ] {
      if ( p()->sim->current_time() < sigil_activates )
        return 1;
      else
        return 0;
    });
  }

  return {};
}

const demon_hunter_td_t* demon_hunter_t::find_target_data( const player_t* target ) const
{
  return _target_data[ target ];
}

/* Always returns non-null targetdata pointer
 */
demon_hunter_td_t* demon_hunter_t::get_target_data( player_t* target ) const
{
  auto& td = _target_data[ target ];
  if ( !td )
  {
    td = new demon_hunter_td_t( target, const_cast<demon_hunter_t&>( *this ) );
  }
  return td;
}

void demon_hunter_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  action.apply_affecting_aura( spell.all_demon_hunter );
  action.apply_affecting_aura( spec.havoc_demon_hunter );
  action.apply_affecting_aura( spec.vengeance_demon_hunter );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class demon_hunter_report_t : public player_report_extension_t
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

      action_t* a = p.find_action( entry->first );
      std::string name_str = entry->first;
      if ( a )
      {
        name_str = report_decorators::decorated_action(*a);
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

using namespace unique_gear;
using namespace actions::spells;
using namespace actions::attacks;

namespace items
{
} // end namespace items

// MODULE INTERFACE ==================================================

class demon_hunter_module_t : public module_t
{
public:
  demon_hunter_module_t() : module_t( DEMON_HUNTER )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new demon_hunter_t( sim, name, r );
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
    hotfix::register_spell( "Demon Hunter", "2023-05-28", "Manually set Consume Soul Fragment (Greater) travel speed.", 178963 )
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

}  // UNNAMED NAMESPACE

const module_t* module_t::demon_hunter()
{
  static demon_hunter_module_t m;
  return &m;
}
