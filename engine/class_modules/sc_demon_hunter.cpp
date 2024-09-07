// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action/parse_effects.hpp"
#include "class_modules/apl/apl_demon_hunter.hpp"

#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE

// Forward Declarations
class demon_hunter_t;
struct soul_fragment_t;

namespace buffs
{
}
namespace actions
{
struct demon_hunter_attack_t;
namespace attacks
{
struct auto_attack_damage_t;
}
}  // namespace actions
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
    dot_t* sigil_of_flame;
    dot_t* sigil_of_doom;

    // Havoc
    dot_t* burning_wound;
    dot_t* trail_of_ruin;

    // Vengeance
    dot_t* fiery_brand;
  } dots;

  struct debuffs_t
  {
    // Shared
    buff_t* sigil_of_flame;
    buff_t* sigil_of_doom;

    // Havoc
    buff_t* burning_wound;
    buff_t* essence_break;
    buff_t* initiative_tracker;
    buff_t* serrated_glaive;

    // Vengeance
    buff_t* frailty;

    // Aldrachi Reaver
    buff_t* reavers_mark;

    // Set Bonuses
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
};

constexpr unsigned MAX_SOUL_FRAGMENTS      = 5;
constexpr double VENGEFUL_RETREAT_DISTANCE = 20.0;

enum class soul_fragment : unsigned
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

[[maybe_unused]] soul_fragment operator|( soul_fragment l, soul_fragment r )
{
  return static_cast<soul_fragment>( static_cast<unsigned>( l ) | static_cast<unsigned>( r ) );
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

  movement_buff_t( demon_hunter_t* p, util::string_view name, const spell_data_t* spell_data = spell_data_t::nil(),
                   const item_t* item = nullptr );

  bool trigger( int s = 1, double v = DEFAULT_VALUE(), double c = -1.0, timespan_t d = timespan_t::min() ) override;
};

using data_t        = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

// utility to create target_effect_t compatible functions from demon_hunter_td_t member references
template <typename T>
static std::function<int( actor_target_data_t* )> d_fn( T d, bool stack = true )
{
  if constexpr ( std::is_invocable_v<T, demon_hunter_td_t::debuffs_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<demon_hunter_td_t*>( t )->debuffs )->check();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<demon_hunter_td_t*>( t )->debuffs )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<T, demon_hunter_td_t::dots_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<demon_hunter_td_t*>( t )->dots )->current_stack();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<demon_hunter_td_t*>( t )->dots )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of demon_hunter_td_t" );
    return nullptr;
  }
}

enum demonsurge_ability
{
  SOUL_SUNDER,
  SPIRIT_BURST,
  SIGIL_OF_DOOM,
  CONSUMING_FIRE,
  FEL_DESOLATION,
  ABYSSAL_GAZE,
  ANNIHILATION,
  DEATH_SWEEP
};

const std::vector<demonsurge_ability> demonsurge_havoc_abilities{
    demonsurge_ability::SIGIL_OF_DOOM, demonsurge_ability::CONSUMING_FIRE, demonsurge_ability::ABYSSAL_GAZE,
    demonsurge_ability::ANNIHILATION, demonsurge_ability::DEATH_SWEEP };

const std::vector<demonsurge_ability> demonsurge_vengeance_abilities{
    demonsurge_ability::SOUL_SUNDER, demonsurge_ability::SPIRIT_BURST, demonsurge_ability::SIGIL_OF_DOOM,
    demonsurge_ability::CONSUMING_FIRE, demonsurge_ability::FEL_DESOLATION };

std::string demonsurge_ability_name( demonsurge_ability ability )
{
  switch ( ability )
  {
    case demonsurge_ability::SOUL_SUNDER:
      return "demonsurge_soul_sunder";
    case demonsurge_ability::SPIRIT_BURST:
      return "demonsurge_spirit_burst";
    case demonsurge_ability::SIGIL_OF_DOOM:
      return "demonsurge_sigil_of_doom";
    case demonsurge_ability::CONSUMING_FIRE:
      return "demonsurge_consuming_fire";
    case demonsurge_ability::FEL_DESOLATION:
      return "demonsurge_fel_desolation";
    case demonsurge_ability::ABYSSAL_GAZE:
      return "demonsurge_abyssal_gaze";
    case demonsurge_ability::ANNIHILATION:
      return "demonsurge_annihilation";
    case demonsurge_ability::DEATH_SWEEP:
      return "demonsurge_death_sweep";
    default:
      return "demonsurge_unknown";
  }
}

enum art_of_the_glaive_ability
{
  GLAIVE_FLURRY,
  RENDING_STRIKE
};

/* Demon Hunter class definition
 *
 * Derived from player_t. Contains everything that defines the Demon Hunter
 * class.
 */
class demon_hunter_t : public parse_player_effects_t
{
public:
  using base_t = parse_player_effects_t;

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

  double shattered_destiny_accumulator;

  event_t* exit_melee_event;  // Event to disable melee abilities mid-VR.

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* demon_soul;
    buff_t* empowered_demon_soul;
    buff_t* immolation_aura;
    buff_t* metamorphosis;

    // Havoc
    buff_t* blind_fury;
    buff_t* blur;
    buff_t* chaos_theory;
    buff_t* death_sweep;
    buff_t* fel_barrage;
    buff_t* furious_gaze;
    buff_t* inertia;
    buff_t* inertia_trigger;  // hidden buff that determines if we can trigger inertia
    buff_t* initiative;
    buff_t* inner_demon;
    buff_t* momentum;
    buff_t* out_of_range;
    buff_t* restless_hunter;
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

    // Aldrachi Reaver
    buff_t* reavers_glaive;
    buff_t* art_of_the_glaive;
    buff_t* glaive_flurry;
    buff_t* rending_strike;
    buff_t* warblades_hunger;
    buff_t* thrill_of_the_fight_attack_speed;
    buff_t* thrill_of_the_fight_damage;
    buff_t* art_of_the_glaive_first;
    buff_t* art_of_the_glaive_second_rending_strike;
    buff_t* art_of_the_glaive_second_glaive_flurry;

    // Fel-scarred
    buff_t* monster_rising;
    buff_t* student_of_suffering;
    buff_t* enduring_torment;
    buff_t* pursuit_of_angryness;  // passive periodic updater buff
    std::unordered_map<demonsurge_ability, buff_t*> demonsurge_abilities;
    buff_t* demonsurge_demonic;
    buff_t* demonsurge_hardcast;
    buff_t* demonsurge;

    // Set Bonuses
    buff_t* tww1_havoc_4pc;
    buff_t* tww1_vengeance_4pc;
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
      player_talent_t the_hunt;
      player_talent_t sigil_of_spite;

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
      player_talent_t a_fire_inside;

    } havoc;

    struct vengeance_talents_t
    {
      player_talent_t fel_devastation;

      player_talent_t frailty;
      player_talent_t fiery_brand;

      player_talent_t perfectly_balanced_glaive;
      player_talent_t deflecting_spikes;
      player_talent_t ascending_flame;

      player_talent_t shear_fury;
      player_talent_t fracture;
      player_talent_t calcified_spikes;
      player_talent_t roaring_fire;      // No Implementation
      player_talent_t sigil_of_silence;  // Partial Implementation
      player_talent_t retaliation;
      player_talent_t meteoric_strikes;

      player_talent_t spirit_bomb;
      player_talent_t feast_of_souls;
      player_talent_t agonizing_flames;
      player_talent_t extended_spikes;
      player_talent_t burning_blood;
      player_talent_t soul_barrier;  // NYI
      player_talent_t bulk_extraction;
      player_talent_t revel_in_pain;  // No Implementation

      player_talent_t void_reaver;
      player_talent_t fallout;
      player_talent_t ruinous_bulwark;  // No Implementation
      player_talent_t volatile_flameblood;
      player_talent_t fel_flame_fortification;  // No Implementation

      player_talent_t soul_furnace;
      player_talent_t painbringer;
      player_talent_t sigil_of_chains;  // Partial Implementation
      player_talent_t fiery_demise;
      player_talent_t chains_of_anger;

      player_talent_t focused_cleave;
      player_talent_t soulmonger;  // No Implementation
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

    struct aldrachi_reaver_talents_t
    {
      player_talent_t art_of_the_glaive;

      player_talent_t fury_of_the_aldrachi;
      player_talent_t evasive_action;  // No Implementation
      player_talent_t unhindered_assault;
      player_talent_t reavers_mark;

      player_talent_t aldrachi_tactics;
      player_talent_t army_unto_oneself;     // No Implementation
      player_talent_t incorruptible_spirit;  // NYI
      player_talent_t wounded_quarry;

      player_talent_t incisive_blade;
      player_talent_t keen_engagement;
      player_talent_t preemptive_strike;
      player_talent_t warblades_hunger;

      player_talent_t thrill_of_the_fight;
    } aldrachi_reaver;

    struct felscarred_talents_t
    {
      player_talent_t demonsurge;  // partially implemented

      player_talent_t wave_of_debilitation;  // No Implementation
      player_talent_t pursuit_of_angriness;
      player_talent_t focused_hatred;
      player_talent_t set_fire_to_the_pain;  // NYI
      player_talent_t improved_soul_rending;

      player_talent_t burning_blades;
      player_talent_t violent_transformation;
      player_talent_t enduring_torment;  // NYI for Vengeance

      player_talent_t untethered_fury;
      player_talent_t student_of_suffering;
      player_talent_t flamebound;
      player_talent_t monster_rising;

      player_talent_t demonic_intensity;
    } felscarred;
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
    const spell_data_t* sigil_of_spite;
    const spell_data_t* sigil_of_spite_damage;
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
    const spell_data_t* soulscar_debuff;
    const spell_data_t* restless_hunter_buff;
    const spell_data_t* tactical_retreat_buff;
    const spell_data_t* unbound_chaos_buff;
    const spell_data_t* chaotic_disposition_damage;
    const spell_data_t* essence_break;

    // Vengeance
    const spell_data_t* vengeance_demon_hunter;
    const spell_data_t* demon_spikes;
    const spell_data_t* infernal_strike;
    const spell_data_t* soul_cleave;
    const spell_data_t* shear;

    const spell_data_t* demonic_wards_2;
    const spell_data_t* demonic_wards_3;
    const spell_data_t* fiery_brand_debuff;
    const spell_data_t* frailty_debuff;
    const spell_data_t* riposte;
    const spell_data_t* soul_cleave_2;
    const spell_data_t* thick_skin;
    const spell_data_t* demon_spikes_buff;
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
    const spell_data_t* burning_alive_controller;
    const spell_data_t* infernal_strike_impact;
    const spell_data_t* spirit_bomb_damage;
    const spell_data_t* frailty_heal;
    const spell_data_t* feast_of_souls_heal;
    const spell_data_t* fel_devastation_2;
    const spell_data_t* fel_devastation_heal;
  } spec;

  struct hero_spec_t
  {
    // Aldrachi Reaver
    const spell_data_t* reavers_glaive;
    const spell_data_t* reavers_glaive_buff;
    const spell_data_t* reavers_mark;
    const spell_data_t* glaive_flurry;
    const spell_data_t* rending_strike;
    const spell_data_t* art_of_the_glaive_buff;
    const spell_data_t* art_of_the_glaive_damage;
    const spell_data_t* warblades_hunger_buff;
    const spell_data_t* warblades_hunger_damage;
    const spell_data_t* wounded_quarry_damage;
    const spell_data_t* thrill_of_the_fight_attack_speed_buff;
    const spell_data_t* thrill_of_the_fight_damage_buff;
    double wounded_quarry_proc_rate;

    // Fel-scarred
    const spell_data_t* burning_blades_debuff;
    const spell_data_t* enduring_torment_buff;
    const spell_data_t* monster_rising_buff;
    const spell_data_t* student_of_suffering_buff;
    const spell_data_t* demonsurge_demonic_buff;
    const spell_data_t* demonsurge_hardcast_buff;
    const spell_data_t* demonsurge_damage;
    const spell_data_t* demonsurge_stacking_buff;
    const spell_data_t* demonsurge_trigger;
    const spell_data_t* soul_sunder;
    const spell_data_t* spirit_burst;
    const spell_data_t* sigil_of_doom;
    const spell_data_t* sigil_of_doom_damage;
    const spell_data_t* abyssal_gaze;
    const spell_data_t* fel_desolation;
  } hero_spec;

  // Set Bonus effects
  struct set_bonuses_t
  {
    const spell_data_t* tww1_havoc_2pc;
    const spell_data_t* tww1_havoc_4pc;
    const spell_data_t* tww1_vengeance_2pc;
    const spell_data_t* tww1_vengeance_4pc;

    // Auxilliary
    const spell_data_t* tww1_havoc_4pc_buff;
    const spell_data_t* tww1_vengeance_4pc_buff;
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
    cooldown_t* sigil_of_spite;
    cooldown_t* felblade;
    cooldown_t* fel_eruption;
    cooldown_t* immolation_aura;
    cooldown_t* the_hunt;
    cooldown_t* spectral_sight;
    cooldown_t* sigil_of_flame;
    cooldown_t* sigil_of_misery;
    cooldown_t* metamorphosis;
    cooldown_t* throw_glaive;
    cooldown_t* vengeful_retreat;
    cooldown_t* chaos_nova;

    // Havoc
    cooldown_t* blade_dance;
    cooldown_t* blur;
    cooldown_t* chaos_strike_refund_icd;
    cooldown_t* essence_break;
    cooldown_t* demonic_appetite;
    cooldown_t* eye_beam;
    cooldown_t* fel_barrage;
    cooldown_t* fel_rush;
    cooldown_t* netherwalk;
    cooldown_t* relentless_onslaught_icd;
    cooldown_t* movement_shared;

    // Vengeance
    cooldown_t* demon_spikes;
    cooldown_t* fiery_brand;
    cooldown_t* fel_devastation;
    cooldown_t* sigil_of_chains;
    cooldown_t* sigil_of_silence;
    cooldown_t* volatile_flameblood_icd;
    cooldown_t* soul_cleave;

    // Aldrachi Reaver
    cooldown_t* art_of_the_glaive_consumption_icd;

    // Fel-scarred
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

    // Fel-scarred
    gain_t* student_of_suffering;
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
    proc_t* soul_fragment_from_soul_sigils;

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
    proc_t* soul_fragment_from_sigil_of_spite;
    proc_t* soul_fragment_from_fallout;
    proc_t* soul_fragment_from_meta;
    proc_t* soul_fragment_from_bulk_extraction;

    // Aldrachi Reaver
    proc_t* soul_fragment_from_aldrachi_tactics;
    proc_t* soul_fragment_from_wounded_quarry;

    // Fel-scarred

    // Set Bonuses
    proc_t* soul_fragment_from_vengeance_twws1_2pc;
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
  } shuffled_rng;

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
    spell_t* inner_demon                        = nullptr;
    spell_t* ragefire                           = nullptr;
    attack_t* relentless_onslaught              = nullptr;
    attack_t* relentless_onslaught_annihilation = nullptr;
    action_t* soulscar                          = nullptr;

    // Vengeance
    spell_t* infernal_armor = nullptr;
    spell_t* retaliation    = nullptr;
    heal_t* frailty_heal    = nullptr;

    // Aldrachi Reaver
    attack_t* art_of_the_glaive = nullptr;
    attack_t* preemptive_strike = nullptr;
    attack_t* warblades_hunger  = nullptr;
    attack_t* wounded_quarry    = nullptr;

    // Fel-scarred
    action_t* burning_blades = nullptr;
    action_t* demonsurge     = nullptr;
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
    // Chance of souls to be incidentally picked up on any movement ability due to being in pickup range
    double soul_fragment_movement_consume_chance = 0.85;
    // Proc rate for Wounded Quarry for Vengeance
    double wounded_quarry_chance_vengeance = 0.30;
    // Proc rate for Wounded Quarry for Havoc
    double wounded_quarry_chance_havoc = 0.10;
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
  void init_uptimes() override;
  void init_resources( bool force ) override;
  void init_special_effects() override;
  void init_rng() override;
  void init_scaling() override;
  void init_spells() override;
  void init_finished() override;
  bool validate_fight_style( fight_style_e style ) const override;
  void invalidate_cache( cache_e ) override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;
  //  double resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action ) override;

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
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double stacking_movement_modifier() const override;

  // overridden player_t combat functions
  void assess_damage( school_e, result_amount_type, action_state_t* s ) override;
  void combat_begin() override;
  const demon_hunter_td_t* find_target_data( const player_t* target ) const override;
  demon_hunter_td_t* get_target_data( player_t* target ) const override;
  void interrupt() override;
  void regen( timespan_t periodicity ) override;
  double resource_gain( resource_e, double, gain_t* source = nullptr, action_t* action = nullptr ) override;
  double resource_gain( resource_e, double, double, gain_t* source = nullptr, action_t* action = nullptr );
  double resource_loss( resource_e, double, gain_t* source = nullptr, action_t* action = nullptr ) override;
  void recalculate_resource_max( resource_e, gain_t* source = nullptr ) override;
  void reset() override;
  void merge( player_t& other ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void apply_affecting_auras( action_t& action ) override;
  void analyze( sim_t& sim ) override;

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
  void trigger_demonsurge( demonsurge_ability );
  double get_target_reach() const
  {
    return options.target_reach >= 0 ? options.target_reach : sim->target->combat_reach;
  }
  void parse_player_effects();

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
  for ( const auto& cb : player->callbacks_on_movement )
  {
    if ( !check() )
      cb( true );
  }

  return buff_t::trigger( s, v, c, d );
}

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

    if ( activation )
    {
      // 2023-06-26 -- Recent testing appears to show a roughly fixed 1s activation time for Havoc
      if ( dh->specialization() == DEMON_HUNTER_HAVOC )
      {
        return 1_s;
      }
      // 2024-02-12 -- Recent testing appears to show a roughly 0.76s activation time for Vengeance
      //               with some slight variance
      return dh->rng().gauss<760, 120>();
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
          timespan_t::from_seconds( dh->talent.vengeance.feed_the_demon->effectN( 1 ).base_value() / 100 );
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

    dh->buff.painbringer->trigger();
    dh->buff.art_of_the_glaive->trigger();
    dh->buff.tww1_vengeance_4pc->trigger();
    dh->buff.warblades_hunger->trigger();

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
class demon_hunter_action_t : public parse_action_effects_t<Base>
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
    affect_flags any_means_necessary_full;
    affect_flags demonic_presence;
    bool chaos_theory = false;
  } affected_by;

  void parse_affect_flags( const spell_data_t* spell, affect_flags& flags )
  {
    for ( const spelleffect_data_t& effect : spell->effects() )
    {
      if ( !effect.ok() || effect.type() != E_APPLY_AURA ||
           ( effect.subtype() != A_ADD_PCT_MODIFIER && effect.subtype() != A_ADD_PCT_LABEL_MODIFIER ) )
        continue;

      if ( ab::data().affected_by( effect ) || ab::data().affected_by_label( effect ) )
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

    ab::apply_affecting_aura( p->talent.aldrachi_reaver.incisive_blade );

    ab::apply_affecting_aura( p->talent.felscarred.flamebound );

    // Rank Passives
    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      // Rank Passives
      ab::apply_affecting_aura( p->spec.immolation_aura_3 );

      // Set Bonus Passives
      ab::apply_affecting_aura( p->set_bonuses.tww1_havoc_2pc );
      ab::apply_affecting_aura( p->set_bonuses.tww1_havoc_4pc );

      // Affect Flags
      parse_affect_flags( p->mastery.demonic_presence, affected_by.demonic_presence );
      parse_affect_flags( p->mastery.any_means_necessary, affected_by.any_means_necessary );

      if ( p->talent.havoc.chaos_theory->ok() )
      {
        affected_by.chaos_theory = ab::data().affected_by( p->spec.chaos_theory_buff->effectN( 1 ) );
      }
    }
    else  // DEMON_HUNTER_VENGEANCE
    {
      // Rank Passives

      // Set Bonus Passives
      ab::apply_affecting_aura( p->set_bonuses.tww1_vengeance_2pc );
      ab::apply_affecting_aura( p->set_bonuses.tww1_vengeance_4pc );

      // Affect Flags

      // Talents
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

    apply_buff_effects();
    apply_debuff_effects();

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

  // Intended for parsing effects from buffs that have an allowlist of abilities
  // that they affect.
  void apply_buff_effects()
  {
    // Shared
    ab::parse_effects( p()->buff.demon_soul );
    ab::parse_effects( p()->buff.empowered_demon_soul );

    // Havoc
    ab::parse_effects( p()->buff.momentum );
    ab::parse_effects( p()->buff.inertia );
    ab::parse_effects( p()->buff.restless_hunter );
    ab::parse_effects( p()->buff.tww1_havoc_4pc );

    // Vengeance
    ab::parse_effects( p()->buff.soul_furnace_damage_amp );

    // Aldrachi Reaver
    ab::parse_effects( p()->buff.warblades_hunger );
    ab::parse_effects( p()->buff.thrill_of_the_fight_damage );
    // Art of the Glaive empowered ability buffs GO
    std::vector<int> art_of_the_glaive_affected_list = {};
    auto art_of_the_glaive = [ &art_of_the_glaive_affected_list, this ]( int idx, buff_t* buff ) {
      auto in_whitelist = [ &art_of_the_glaive_affected_list, this ]() {
        return std::find( art_of_the_glaive_affected_list.begin(), art_of_the_glaive_affected_list.end(), ab::id ) !=
               art_of_the_glaive_affected_list.end();
      };

      if ( const auto& effect = p()->talent.aldrachi_reaver.art_of_the_glaive->effectN( idx );
           effect.ok() && in_whitelist() )
        add_parse_entry( ab::da_multiplier_effects ).set_buff( buff ).set_value( effect.percent() ).set_eff( &effect );
    };
    std::vector<int> art_of_the_glaive_glaive_flurry_affected_list  = { 199552, 200685, 391374, 391378, 210153,
                                                                        210155, 393054, 393055, 228478 };
    std::vector<int> art_of_the_glaive_rending_strike_affected_list = { 199547, 222031, 201428, 227518,
                                                                        203782, 225919, 225921 };
    // First empowered
    art_of_the_glaive_affected_list = art_of_the_glaive_glaive_flurry_affected_list;
    art_of_the_glaive_affected_list.insert( art_of_the_glaive_affected_list.end(),
                                            art_of_the_glaive_rending_strike_affected_list.begin(),
                                            art_of_the_glaive_rending_strike_affected_list.end() );
    art_of_the_glaive( 3, p()->buff.art_of_the_glaive_first );
    // Second empowered
    art_of_the_glaive_affected_list = art_of_the_glaive_glaive_flurry_affected_list;
    art_of_the_glaive( 4, p()->buff.art_of_the_glaive_second_glaive_flurry );
    art_of_the_glaive_affected_list = art_of_the_glaive_rending_strike_affected_list;
    art_of_the_glaive( 4, p()->buff.art_of_the_glaive_second_rending_strike );
    // End Art of the Glaive bullshittery

    // Fel-scarred
    ab::parse_effects( p()->buff.enduring_torment );
    ab::parse_effects( p()->buff.demonsurge_demonic );
    ab::parse_effects( p()->buff.demonsurge_hardcast );
    ab::parse_effects( p()->buff.demonsurge );
  }

  void apply_debuff_effects()
  {
    // Shared

    // Havoc
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::burning_wound ), p()->spec.burning_wound_debuff );
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::essence_break ), p()->spec.essence_break_debuff );
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::serrated_glaive ),
                              p()->talent.havoc.serrated_glaive->effectN( 1 ).trigger(),
                              p()->talent.havoc.serrated_glaive );

    // Vengeance
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::frailty ), p()->spec.frailty_debuff,
                              p()->talent.vengeance.vulnerability );

    // Vengeance Demon Hunter's DF S2 tier set spell data is baked into Fiery Brand's spell data at effect #4.
    // We exclude parsing effect #4 as that tier set is no longer active.
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::dots_t::fiery_brand ), p()->spec.fiery_brand_debuff,
                              effect_mask_t( true ).disable( 4 ), p()->talent.vengeance.fiery_demise );

    // Aldrachi Reaver
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::reavers_mark ), p()->hero_spec.reavers_mark );

    // Fel-scarred
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

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_da_multiplier( s );

    if ( affected_by.demonic_presence.direct )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.any_means_necessary.direct )
    {
      m *= 1.0 + p()->cache.mastery_value() * ( 1.0 + p()->mastery.any_means_necessary_tuning->effectN( 1 ).percent() );
    }

    // 2024-08-30 -- Some spells have full 100% mastery value from AMN.
    if ( affected_by.any_means_necessary_full.direct )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_ta_multiplier( s );

    if ( affected_by.demonic_presence.periodic )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.any_means_necessary.periodic )
    {
      m *= 1.0 + p()->cache.mastery_value() * ( 1.0 + p()->mastery.any_means_necessary_tuning->effectN( 2 ).percent() );
    }

    // 2024-08-30 -- Some spells have full 100% mastery value from AMN.
    if ( affected_by.any_means_necessary_full.periodic )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = ab::composite_crit_chance();

    if ( affected_by.chaos_theory && p()->buff.chaos_theory->up() )
    {
      const double bonus = p()->rng().range( p()->talent.havoc.chaos_theory->effectN( 1 ).percent(),
                                             p()->talent.havoc.chaos_theory->effectN( 2 ).percent() );
      c += bonus;
    }

    return c;
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

      // 2024-07-12 -- Shattered Destiny only accumulates while in Metamorphosis
      if ( p()->talent.havoc.shattered_destiny->ok() && p()->buff.metamorphosis->check() )
      {
        // 2024-07-12 -- If a cast costs Fury, it seems to use the base cost instead of the actual cost.
        resource_e cr  = ab::current_resource();
        const auto& bc = ab::base_costs[ cr ];
        auto base      = bc.base;

        p()->shattered_destiny_accumulator += base;
        const double threshold = p()->talent.havoc.shattered_destiny->effectN( 2 ).base_value();
        while ( p()->shattered_destiny_accumulator >= threshold )
        {
          p()->shattered_destiny_accumulator -= threshold;
          p()->buff.metamorphosis->extend_duration( p(),
                                                    p()->talent.havoc.shattered_destiny->effectN( 1 ).time_value() );
          p()->proc.shattered_destiny->occur();
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

protected:
  /// typedef for demon_hunter_action_t<action_base_t>
  using base_t = demon_hunter_action_t<Base>;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  using ab = parse_action_effects_t<Base>;
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
      sigil_cooldowns = { p->cooldown.sigil_of_flame, p->cooldown.sigil_of_spite, p->cooldown.sigil_of_misery,
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
      for ( unsigned i = 0; i < num_souls; i++ )
      {
        p()->proc.soul_fragment_from_soul_sigils->occur();
      }
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

struct demon_hunter_ranged_attack_t : public demon_hunter_action_t<ranged_attack_t>
{
  demon_hunter_ranged_attack_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                                util::string_view o = {} )
    : base_t( n, p, s, o )
  {
    special = true;
  }
};

template <demonsurge_ability ABILITY, typename BASE>
struct demonsurge_trigger_t : public BASE
{
  using base_t = demonsurge_trigger_t<ABILITY, BASE>;

  demonsurge_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( BASE::p()->active.demonsurge && BASE::p()->buff.demonsurge_abilities[ ABILITY ]->up() )
    {
      BASE::p()->buff.demonsurge_abilities[ ABILITY ]->expire();
      make_event<delayed_execute_event_t>(
          *BASE::sim, BASE::p(), BASE::p()->active.demonsurge, BASE::p()->target,
          timespan_t::from_millis( BASE::p()->hero_spec.demonsurge_trigger->effectN( 1 ).misc_value1() ) );
    }
  }
};

template <art_of_the_glaive_ability ABILITY, typename BASE>
struct art_of_the_glaive_trigger_t : public BASE
{
  using base_t = art_of_the_glaive_trigger_t<ABILITY, BASE>;

  timespan_t thrill_delay;

  art_of_the_glaive_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o ),
      // 2024-07-16 -- This is seems to be 700ms for everything but Death Sweep
      thrill_delay( 700_ms )
  {
  }

  void execute() override
  {
    BASE::execute();

    bool second_ability = false;
    if ( ABILITY == art_of_the_glaive_ability::GLAIVE_FLURRY &&
         BASE::p()->talent.aldrachi_reaver.fury_of_the_aldrachi->ok() && BASE::p()->buff.glaive_flurry->up() )
    {
      second_ability = !BASE::p()->buff.rending_strike->up();

      if ( BASE::p()->talent.aldrachi_reaver.fury_of_the_aldrachi->ok() )
      {
        BASE::p()->active.art_of_the_glaive->execute_on_target( BASE::target );
      }

      BASE::p()->buff.glaive_flurry->expire();
      make_event( *BASE::p()->sim, thrill_delay, [ this, second_ability ] {
        BASE::p()->buff.art_of_the_glaive_first->expire();
        BASE::p()->buff.art_of_the_glaive_second_glaive_flurry->expire();
        BASE::p()->buff.art_of_the_glaive_second_rending_strike->expire();
        if ( !second_ability )
        {
          BASE::p()->buff.art_of_the_glaive_second_rending_strike->trigger();
        }
      } );
    }
    else if ( ABILITY == art_of_the_glaive_ability::RENDING_STRIKE &&
              BASE::p()->talent.aldrachi_reaver.fury_of_the_aldrachi->ok() && BASE::p()->buff.rending_strike->up() )
    {
      second_ability = !BASE::p()->buff.glaive_flurry->up();

      if ( BASE::p()->talent.aldrachi_reaver.reavers_mark->ok() )
      {
        BASE::td( BASE::target )->debuffs.reavers_mark->trigger( second_ability ? 2 : 1 );
      }

      BASE::p()->buff.rending_strike->expire();
      make_event( *BASE::p()->sim, thrill_delay, [ this, second_ability ] {
        BASE::p()->buff.art_of_the_glaive_first->expire();
        BASE::p()->buff.art_of_the_glaive_second_glaive_flurry->expire();
        BASE::p()->buff.art_of_the_glaive_second_rending_strike->expire();
        if ( !second_ability )
        {
          BASE::p()->buff.art_of_the_glaive_second_glaive_flurry->trigger();
        }
      } );
    }

    if ( second_ability )
    {
      if ( BASE::p()->talent.aldrachi_reaver.thrill_of_the_fight->ok() )
      {
        make_event( *BASE::p()->sim, thrill_delay, [ this ] {
          BASE::p()->buff.thrill_of_the_fight_attack_speed->trigger();
          BASE::p()->buff.thrill_of_the_fight_damage->trigger();
        } );
      }
      if ( BASE::p()->talent.aldrachi_reaver.aldrachi_tactics->ok() )
      {
        BASE::p()->proc.soul_fragment_from_aldrachi_tactics->occur();
        BASE::p()->spawn_soul_fragment( soul_fragment::LESSER );
      }
    }
  }
};

template <typename BASE>
struct cycle_of_hatred_trigger_t : public BASE
{
  using base_t = cycle_of_hatred_trigger_t<BASE>;

  cycle_of_hatred_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o )
  {
  }

  virtual bool has_talents_for_cycle_of_hatred()
  {
    return BASE::p()->talent.havoc.cycle_of_hatred->ok();
  }

  void execute() override
  {
    BASE::execute();

    if ( !has_talents_for_cycle_of_hatred() )
      return;

    if ( !BASE::p()->cooldown.eye_beam->down() )
      return;

    timespan_t adjust_seconds = BASE::p()->talent.havoc.cycle_of_hatred->effectN( 1 ).time_value();
    BASE::p()->cooldown.eye_beam->adjust( -adjust_seconds );
  }
};

template <typename BASE>
struct amn_full_mastery_bug_t : public BASE
{
  using base_t = amn_full_mastery_bug_t<BASE>;

  amn_full_mastery_bug_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s ) : BASE( n, p, s )
  {
    // 2024-08-30 -- Demonsurge / Burning Blades gets a full 100% mastery buff from AMN instead of 80%
    if ( p->bugs )
    {
      if ( BASE::affected_by.any_means_necessary.direct )
      {
        BASE::affected_by.any_means_necessary.direct      = false;
        BASE::affected_by.any_means_necessary_full.direct = true;
      }
      if ( BASE::affected_by.any_means_necessary.periodic )
      {
        BASE::affected_by.any_means_necessary.periodic      = false;
        BASE::affected_by.any_means_necessary_full.periodic = true;
      }
    }
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
      : demon_hunter_heal_t( name, p, p->spec.feast_of_souls_heal )
    {
      dual = true;
    }
  };

  soul_cleave_heal_t( util::string_view name, demon_hunter_t* p, std::basic_string<char> reporting_name )
    : demon_hunter_heal_t( name, p, p->spec.soul_cleave )
  {
    background = dual  = true;
    name_str_reporting = reporting_name;

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
  frailty_heal_t( demon_hunter_t* p ) : demon_hunter_heal_t( "frailty_heal", p, p->spec.frailty_heal )
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
    for ( unsigned i = 0; i < num_souls; i++ )
    {
      p()->proc.soul_fragment_from_bulk_extraction->occur();
    }
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

// Chaotic Disposition ============================================================
struct chaotic_disposition_cb_t : public dbc_proc_callback_t
{
  struct chaotic_disposition_t : public demon_hunter_spell_t
  {
    chaotic_disposition_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.chaotic_disposition_damage )
    {
      min_travel_time = 0.1;
    }
  };

  uint32_t mask;

  chaotic_disposition_t* damage;
  double chance;
  size_t rolls;
  double damage_percent;

  chaotic_disposition_cb_t( demon_hunter_t* p, const special_effect_t& e )
    : dbc_proc_callback_t( p, e ),
      mask( dbc::get_school_mask( SCHOOL_CHROMATIC ) ),
      chance( p->talent.havoc.chaotic_disposition->effectN( 2 ).percent() / 100 ),
      rolls( as<size_t>( p->talent.havoc.chaotic_disposition->effectN( 1 ).base_value() ) ),
      damage_percent( p->talent.havoc.chaotic_disposition->effectN( 3 ).percent() )
  {
    deactivate();
    initialize();

    damage = p->get_background_action<chaotic_disposition_t>( "chaotic_disposition" );
  }

  void activate() override
  {
    if ( damage )
      dbc_proc_callback_t::activate();
  }

  void trigger( action_t* a, action_state_t* state ) override
  {
    if ( ( dbc::get_school_mask( state->action->school ) & mask ) != mask )
      return;

    if ( !damage )
      return;

    if ( state->action->id == damage->id )
      return;

    dbc_proc_callback_t::trigger( a, state );
  }

  void execute( action_t*, action_state_t* s ) override
  {
    if ( s->target->is_sleeping() )
      return;

    double da = s->result_amount;
    if ( da > 0 )
    {
      da *= damage_percent;

      for ( size_t i = 0; i < rolls; i++ )
      {
        if ( rng().roll( chance ) )
        {
          damage->execute_on_target( s->target, da );
        }
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
      energize_type                    = action_energize::ON_CAST;
      energize_resource                = effect.resource_gain_type();
      energize_amount                  = effect.resource( energize_resource );
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
    : demon_hunter_spell_t( "demon_spikes", p, p->spec.demon_spikes, options_str )
  {
    may_miss = harmful = false;
    use_off_gcd        = true;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    p()->buff.demon_spikes->trigger();
  }
};

// Retaliation ==============================================================

struct retaliation_t : public demon_hunter_spell_t
{
  retaliation_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_spell_t( name, p, p->spec.retaliation_damage )
  {
  }
};

// Disrupt ==================================================================

struct disrupt_t : public demon_hunter_spell_t
{
  disrupt_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "disrupt", p, p->spell.disrupt, options_str )
  {
    may_miss     = false;
    is_interrupt = true;

    if ( p->talent.demon_hunter.disrupting_fury->ok() )
    {
      const spelleffect_data_t& effect = p->talent.demon_hunter.disrupting_fury->effectN( 1 ).trigger()->effectN( 1 );
      energize_type                    = action_energize::ON_CAST;
      energize_resource                = effect.resource_gain_type();
      energize_amount                  = effect.resource( energize_resource );
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

struct eye_beam_base_t : public demon_hunter_spell_t
{
  struct eye_beam_tick_t : public demon_hunter_spell_t
  {
    eye_beam_tick_t( util::string_view name, demon_hunter_t* p, std::string reporting_name, const spell_data_t* s,
                     int reduced_targets )
      : demon_hunter_spell_t( name, p, s )
    {
      background = dual   = true;
      aoe                 = -1;
      name_str_reporting  = reporting_name;
      reduced_aoe_targets = reduced_targets;
    }

    double action_multiplier() const override
    {
      double m = demon_hunter_spell_t::action_multiplier();

      // 2024-07-07 -- Isolated Prey's effect on Eye Beam does not work on live nor beta
      if ( p()->talent.havoc.isolated_prey->ok() && !p()->bugs )
      {
        if ( targets_in_range_list( target_list() ).size() == 1 )
        {
          m *= 1.0 + p()->talent.havoc.isolated_prey->effectN( 2 ).percent();
        }
      }

      return m;
    }

    timespan_t execute_time() const override
    {
      // Eye Beam is applied via a player aura and experiences aura delay in applying damage tick events
      // Not a perfect implementation, but closer than the instant execution in current sims
      return rng().gauss( p()->sim->default_aura_delay );
    }
  };

  eye_beam_base_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : demon_hunter_spell_t( name, p, s, o )
  {
    may_miss            = false;
    channeled           = true;
    tick_on_application = false;
    cooldown            = p->cooldown.eye_beam;

    // 6/6/2020 - Override the lag handling for Eye Beam so that it doesn't use channeled ready behavior
    //            In-game tests have shown it is possible to cast after faster than the 250ms channel_lag using a
    //            nochannel macro
    ability_lag = p->world_lag;

    tick_action = p->get_background_action<eye_beam_tick_t>( name_str + "_tick", name_str, p->spec.eye_beam_damage,
                                                             as<int>( data().effectN( 5 ).base_value() ) );
    tick_action->stats = stats;

    // Add damage modifiers in eye_beam_tick_t, not here.
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

    // Since Demonic triggers Meta with 5s + hasted duration, need to extend by the hasted duration after have an
    // execute_state
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
  {
    return result_amount_type::DMG_DIRECT;
  }
};

struct eye_beam_t : public eye_beam_base_t
{
  eye_beam_t( demon_hunter_t* p, util::string_view options_str )
    : eye_beam_base_t( "eye_beam", p, p->talent.havoc.eye_beam, options_str )
  {
  }

  bool ready() override
  {
    if ( p()->buff.demonsurge_hardcast->check() )
    {
      return false;
    }
    return eye_beam_base_t::ready();
  }
};

struct abyssal_gaze_t : public demonsurge_trigger_t<demonsurge_ability::ABYSSAL_GAZE, eye_beam_base_t>
{
  abyssal_gaze_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "abyssal_gaze", p, p->hero_spec.abyssal_gaze, options_str )
  {
  }

  bool ready() override
  {
    if ( !p()->buff.demonsurge_hardcast->check() )
    {
      return false;
    }
    return base_t::ready();
  }
};

// Fel Barrage ==============================================================

struct fel_barrage_t : public demon_hunter_spell_t
{
  struct fel_barrage_tick_t : public demon_hunter_spell_t
  {
    fel_barrage_tick_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_spell_t( name, p, s )
    {
      background = dual   = true;
      aoe                 = -1;
      reduced_aoe_targets = as<int>( data().effectN( 2 ).base_value() );
    }
  };

  fel_barrage_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "fel_barrage", p, p->talent.havoc.fel_barrage, options_str )
  {
    may_miss     = false;
    dot_duration = timespan_t::zero();
    range        = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();
    set_target( p );  // Does not require a hostile target

    if ( !p->active.fel_barrage )
    {
      p->active.fel_barrage =
          p->get_background_action<fel_barrage_tick_t>( "fel_barrage_tick", data().effectN( 1 ).trigger() );
      p->active.fel_barrage->stats = stats;
    }
  }

  void execute() override
  {
    p()->buff.fel_barrage->trigger();
    demon_hunter_spell_t::execute();
  }
};

// Fel Devastation ==========================================================

struct fel_devastation_base_t : public demon_hunter_spell_t
{
  struct fel_devastation_tick_t : public demon_hunter_spell_t
  {
    fel_devastation_tick_t( util::string_view name, demon_hunter_t* p, std::string reporting_name,
                            const spell_data_t* s )
      : demon_hunter_spell_t( name, p, s )
    {
      background = dual  = true;
      aoe                = -1;
      name_str_reporting = reporting_name;
    }
  };

  heals::fel_devastation_heal_t* heal;
  bool benefits_from_dgb_cdr;

  fel_devastation_base_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : demon_hunter_spell_t( name, p, s, o ), heal( nullptr ), benefits_from_dgb_cdr( true )
  {
    may_miss            = false;
    channeled           = true;
    tick_on_application = false;

    if ( p->spec.fel_devastation_2->ok() )
    {
      heal = p->get_background_action<heals::fel_devastation_heal_t>( name_str + "_heal" );
    }

    tick_action =
        p->get_background_action<fel_devastation_tick_t>( name_str + "_tick", name_str, data().effectN( 1 ).trigger() );
    tick_action->stats = stats;
  }

  void execute() override
  {
    p()->trigger_demonic();
    demon_hunter_spell_t::execute();

    // Since Demonic triggers Meta with 5s + duration, need to extend by the duration after have an
    // execute_state
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

    if ( heal )
    {
      heal->set_target( player );
      heal->execute();
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

      if ( benefits_from_dgb_cdr )
      {
        p()->sim->print_debug( "{} rolled {}s of CDR on {} from Darkglare Boon", *p(), cdr_reduction.total_seconds(),
                               name_str );
        p()->cooldown.fel_devastation->adjust( -cdr_reduction );
      }
      p()->resource_gain( RESOURCE_FURY, fury_refund, p()->gain.darkglare_boon );
    }

    if ( p()->buff.tww1_vengeance_4pc->up() )
    {
      p()->buff.tww1_vengeance_4pc->expire();
    }
  }

  void tick( dot_t* d ) override
  {
    if ( heal )
    {
      heal->execute();  // Heal happens before damage
    }

    demon_hunter_spell_t::tick( d );
  }
};

struct fel_devastation_t : public fel_devastation_base_t
{
  fel_devastation_t( demon_hunter_t* p, util::string_view options_str )
    : fel_devastation_base_t( "fel_devastation", p, p->talent.vengeance.fel_devastation, options_str )
  {
  }

  bool ready() override
  {
    if ( p()->buff.demonsurge_hardcast->check() )
    {
      return false;
    }
    return fel_devastation_base_t::ready();
  }
};

struct fel_desolation_t : public demonsurge_trigger_t<demonsurge_ability::FEL_DESOLATION, fel_devastation_base_t>
{
  fel_desolation_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "fel_desolation", p, p->hero_spec.fel_desolation, options_str )
  {
    // 2024-07-07 -- Fel Desolation doesn't benefit from DGB CDR
    // 2024-07-07 -- Fel Desolation doesn't share a cooldown with Fel Devastation
    if ( p->bugs )
    {
      benefits_from_dgb_cdr = false;
    }
    else
    {
      cooldown = p->cooldown.fel_devastation;
    }
  }

  bool ready() override
  {
    if ( !p()->buff.demonsurge_hardcast->check() )
    {
      return false;
    }
    return base_t::ready();
  }
};

// Fel Eruption =============================================================

struct fel_eruption_t : public demon_hunter_spell_t
{
  fel_eruption_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "fel_eruption", p, p->spec.fel_eruption, options_str )
  {
  }
};

// Fiery Brand ==============================================================

struct fiery_brand_t : public demon_hunter_spell_t
{
  struct fiery_brand_state_t : public action_state_t
  {
    bool primary;

    fiery_brand_state_t( action_t* a, player_t* target ) : action_state_t( a, target ), primary( false )
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
      dot_behavior      = DOT_EXTEND;

      // Spread radius used for Burning Alive.
      if ( p->talent.vengeance.burning_alive->ok() )
      {
        radius = p->spec.burning_alive_controller->effectN( 1 ).radius_max();
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
      if ( !data().ok() )
        return nullptr;

      if ( !t )
        t = target;
      if ( !t )
        return nullptr;

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
      target_cache.is_valid          = false;
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

    if ( data().ok() )
    {
      dot_action = p->get_background_action<fiery_brand_dot_t>( "fiery_brand_dot" );
      add_child( dot_action );
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if ( result_is_miss( s->result ) )
      return;

    // Trigger the initial DoT action and set the primary flag for use with Burning Alive
    dot_action->set_target( s->target );
    fiery_brand_state_t* fb_state = debug_cast<fiery_brand_state_t*>( dot_action->get_state() );
    fb_state->target              = s->target;
    dot_action->snapshot_state( fb_state, result_amount_type::DMG_OVER_TIME );
    fb_state->primary = true;
    dot_action->schedule_execute( fb_state );
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !data().ok() )
      return nullptr;
    return dot_action->get_dot( t );
  }
};

// Glaive Tempest ===========================================================

struct glaive_tempest_t : public cycle_of_hatred_trigger_t<demon_hunter_spell_t>
{
  struct glaive_tempest_damage_t : public demon_hunter_attack_t
  {
    glaive_tempest_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->spec.glaive_tempest_damage )
    {
      background = dual = ground_aoe = true;
      aoe                            = -1;
      reduced_aoe_targets            = p->talent.havoc.glaive_tempest->effectN( 2 ).base_value();
    }
  };

  glaive_tempest_damage_t* glaive_tempest_mh;
  glaive_tempest_damage_t* glaive_tempest_oh;

  glaive_tempest_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "glaive_tempest", p, p->talent.havoc.glaive_tempest, options_str )
  {
    school            = SCHOOL_CHAOS;  // Reporting purposes only
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
    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( p()->x_position )
                                        .y( p()->y_position )
                                        .pulse_time( 500_ms )
                                        .hasted( ground_aoe_params_t::hasted_with::ATTACK_HASTE )
                                        .duration( data().duration() * p()->cache.attack_haste() )
                                        .action( action ),
                                    true );
  }

  void execute() override
  {
    base_t::execute();
    make_ground_aoe_event( glaive_tempest_mh );
    make_ground_aoe_event( glaive_tempest_oh );
  }
};

// Sigil of Flame / Sigil of Doom ===========================================

struct sigil_of_flame_damage_base_t : public demon_hunter_sigil_t
{
  sigil_of_flame_damage_base_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
    : demon_hunter_sigil_t( name, p, s, delay )
  {
    tick_on_application = false;
    dot_max_stack       = 1;
    dot_behavior        = dot_behavior_e::DOT_REFRESH_DURATION;

    if ( p->talent.demon_hunter.flames_of_fury->ok() )
    {
      energize_type     = action_energize::ON_HIT;
      energize_resource = RESOURCE_FURY;
      energize_amount   = p->talent.demon_hunter.flames_of_fury->effectN( 1 ).resource();
    }
  }

  void execute() override
  {
    demon_hunter_sigil_t::execute();
    if ( p()->talent.felscarred.student_of_suffering->ok() )
    {
      p()->buff.student_of_suffering->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_sigil_t::impact( s );

    // Sigil of Flame/Doom can apply Frailty if Frailty is talented
    if ( result_is_hit( s->result ) && p()->talent.vengeance.frailty->ok() )
    {
      td( s->target )->debuffs.frailty->trigger();
    }
  }

  timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
  {
    // 2023-10-21 -- Ascending Flame Sigil of Flame/Doom refreshes use normal DoT REFRESH_DURATION dot_behavior.
    if ( p()->talent.vengeance.ascending_flame->ok() )
    {
      return action_t::calculate_dot_refresh_duration( dot, triggered_duration );
    }
    // 2023-10-21 -- Non-Ascending Flame Sigil of Flame/Doom refreshes _truncate_ the existing DoT and apply a fresh
    // DoT.
    return triggered_duration - dot->time_to_next_full_tick();
  }
};

struct sigil_of_flame_damage_t : public sigil_of_flame_damage_base_t
{
  sigil_of_flame_damage_t( util::string_view name, demon_hunter_t* p, timespan_t delay )
    : sigil_of_flame_damage_base_t( name, p, p->spell.sigil_of_flame_damage, delay )
  {
  }

  void impact( action_state_t* s ) override
  {
    sigil_of_flame_damage_base_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs.sigil_of_flame->trigger();
    }
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t )
      t = target;
    if ( !t )
      return nullptr;
    return td( t )->dots.sigil_of_flame;
  }

  double composite_target_ta_multiplier( player_t* target ) const override
  {
    double ttm = demon_hunter_sigil_t::composite_target_ta_multiplier( target );

    double current_stack = td( target )->debuffs.sigil_of_flame->stack();
    ttm *= current_stack;

    return ttm;
  }
};

struct sigil_of_doom_damage_t : public sigil_of_flame_damage_base_t
{
  sigil_of_doom_damage_t( util::string_view name, demon_hunter_t* p, timespan_t delay )
    : sigil_of_flame_damage_base_t( name, p, p->hero_spec.sigil_of_doom_damage, delay )
  {
  }

  void impact( action_state_t* s ) override
  {
    sigil_of_flame_damage_base_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs.sigil_of_doom->trigger();
    }
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t )
      t = target;
    if ( !t )
      return nullptr;
    return td( t )->dots.sigil_of_doom;
  }

  double composite_target_ta_multiplier( player_t* target ) const override
  {
    double ttm = demon_hunter_sigil_t::composite_target_ta_multiplier( target );

    double current_stack = td( target )->debuffs.sigil_of_doom->stack();
    ttm *= current_stack;

    return ttm;
  }
};

struct sigil_of_flame_base_t : public demon_hunter_spell_t
{
  sigil_of_flame_damage_base_t* sigil;

  sigil_of_flame_base_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : demon_hunter_spell_t( name, p, s, o ), sigil( nullptr )
  {
    may_miss = false;
    cooldown = p->cooldown.sigil_of_flame;

    if ( p->spell.sigil_of_flame_fury->ok() )
    {
      energize_type     = action_energize::ON_CAST;
      energize_resource = RESOURCE_FURY;
      energize_amount   = p->spell.sigil_of_flame_fury->effectN( 1 ).resource();
    }

    if ( !p->active.frailty_heal )
    {
      p->active.frailty_heal = new heals::frailty_heal_t( p );
    }

    // Add damage modifiers in sigil_of_flame_damage_base_t, not here.
  }

  void init_finished() override
  {
    demon_hunter_spell_t::init_finished();
    harmful = !is_precombat;  // Do not count towards the precombat hostile action limit
  }

  bool usable_precombat() const override
  {
    return true;  // Has virtual travel time due to Sigil activation delay
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( sigil )
    {
      sigil->place_sigil( execute_state->target );
    }
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

  bool verify_actor_spec() const override
  {
    // Spell data still has a Vengeance Demon Hunter class association but it's baseline
    return true;
  }
};

struct sigil_of_flame_t : public sigil_of_flame_base_t
{
  sigil_of_flame_t( demon_hunter_t* p, util::string_view options_str )
    : sigil_of_flame_base_t( "sigil_of_flame", p, p->spell.sigil_of_flame, options_str )
  {
    sigil        = p->get_background_action<sigil_of_flame_damage_t>( "sigil_of_flame_damage", ground_aoe_duration );
    sigil->stats = stats;

    // Add damage modifiers in sigil_of_flame_damage_t, not here.
  }

  bool ready() override
  {
    if ( p()->buff.demonsurge_hardcast->check() )
    {
      return false;
    }
    return sigil_of_flame_base_t::ready();
  }
};

struct sigil_of_doom_t : public demonsurge_trigger_t<demonsurge_ability::SIGIL_OF_DOOM, sigil_of_flame_base_t>
{
  sigil_of_doom_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "sigil_of_doom", p, p->hero_spec.sigil_of_doom, options_str )
  {
    if ( p->hero_spec.sigil_of_doom_damage->ok() )
    {
      sigil        = p->get_background_action<sigil_of_doom_damage_t>( "sigil_of_doom_damage", ground_aoe_duration );
      sigil->stats = stats;
    }

    // Add damage modifiers in sigil_of_doom_damage_t, not here.
  }

  bool ready() override
  {
    if ( !p()->buff.demonsurge_hardcast->check() )
    {
      return false;
    }
    return base_t::ready();
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
      aoe  = -1;
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
    dual                 = true;
    tick_on_application  = false;

    tick_action = p->get_background_action<collective_anguish_tick_t>( "collective_anguish_tick" );
  }

  // Behaves as a channeled spell, although we can't set channeled = true since it is background
  void init() override
  {
    demon_hunter_spell_t::init();

    // Channeled dots get haste snapshotted in action_t::init() and we replicate that here
    update_flags &= ~STATE_HASTE;
  }
};

// Infernal Strike ==========================================================

struct infernal_strike_t : public demon_hunter_spell_t
{
  struct infernal_strike_impact_t : public demon_hunter_spell_t
  {
    infernal_strike_impact_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.infernal_strike_impact )
    {
      background = dual = true;
      aoe               = -1;
    }
  };

  infernal_strike_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "infernal_strike", p, p->spec.infernal_strike, options_str )
  {
    may_miss                = false;
    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;
    travel_speed            = 1.0;  // allows use precombat

    impact_action        = p->get_background_action<infernal_strike_impact_t>( "infernal_strike_impact" );
    impact_action->stats = stats;
  }

  // leap travel time, independent of distance
  timespan_t travel_time() const override
  {
    return 1_s;
  }
};

// Immolation Aura ==========================================================

struct immolation_aura_state_t : public action_state_t
{
  double growing_inferno_multiplier;
  buff_t* immolation_aura;

  immolation_aura_state_t( action_t* action, player_t* target )
    : action_state_t( action, target ), growing_inferno_multiplier( 1.0 ), immolation_aura( nullptr )
  {
  }

  void initialize() override
  {
    action_state_t::initialize();
    growing_inferno_multiplier = 1.0;
    immolation_aura            = nullptr;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );

    auto ias                   = debug_cast<const immolation_aura_state_t*>( s );
    growing_inferno_multiplier = ias->growing_inferno_multiplier;
    immolation_aura            = ias->immolation_aura;
  }

  double composite_da_multiplier() const override
  {
    return action_state_t::composite_da_multiplier() * growing_inferno_multiplier;
  }

  double composite_ta_multiplier() const override
  {
    return action_state_t::composite_ta_multiplier() * growing_inferno_multiplier;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );
    s << " growing_inferno_multiplier=" << growing_inferno_multiplier;
    return s;
  }
};

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
  private:
    using state_t = immolation_aura_state_t;

  public:
    bool initial;

    immolation_aura_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, bool initial )
      : demon_hunter_spell_t( name, p, s ), initial( initial )
    {
      background = dual   = true;
      aoe                 = -1;
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
        else  // DEMON_HUNTER_HAVOC
        {
          energize_amount = p->talent.havoc.burning_hatred->ok() ? data().effectN( 2 ).base_value() : 0;
        }
      }
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    state_t* cast_state( action_state_t* s )
    {
      return static_cast<state_t*>( s );
    }

    const state_t* cast_state( const action_state_t* s ) const
    {
      return static_cast<const state_t*>( s );
    }

    double composite_crit_chance() const override
    {
      double ccc = demon_hunter_spell_t::composite_crit_chance();

      if ( p()->talent.havoc.isolated_prey->ok() )
      {
        if ( targets_in_range_list( target_list() ).size() == 1 )
        {
          return 1.0;
        }
      }

      return ccc;
    }

    void accumulate_ragefire( immolation_aura_state_t* s );

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

        if ( p()->talent.vengeance.charred_flesh->ok() )
        {
          auto duration_extension        = p()->talent.vengeance.charred_flesh->effectN( 1 ).time_value();
          demon_hunter_td_t* target_data = td( s->target );
          if ( target_data->dots.fiery_brand->is_ticking() )
          {
            target_data->dots.fiery_brand->adjust_duration( duration_extension );
          }
          if ( target_data->dots.sigil_of_flame->is_ticking() )
          {
            target_data->dots.sigil_of_flame->adjust_duration( duration_extension );
            if ( duration_extension > timespan_t::zero() )
            {
              const auto& expiration = target_data->debuffs.sigil_of_flame->expiration;
              for ( auto& i : expiration )
              {
                i->reschedule( i->remains() + duration_extension );
                sim->print_log( "{} extends {} by {}. New expiration time: {}, remains={}", *p(),
                                *target_data->debuffs.sigil_of_flame, duration_extension, i->occurs(), i->remains() );
              }
            }
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

        accumulate_ragefire( cast_state( s ) );
      }
    }

    result_amount_type amount_type( const action_state_t*, bool ) const override
    {
      return initial ? result_amount_type::DMG_DIRECT : result_amount_type::DMG_OVER_TIME;
    }
  };

  // TODO: 2023-12-19: With the change from 30% to 25% proc chance, AFI seems to no longer be
  //                   a simple deck of cards system. Need to figure out exactly how it works
  //                   but until then we use a flat chance from spell data again.
  double afi_chance;

  immolation_aura_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "immolation_aura", p, p->spell.immolation_aura, options_str ),
      afi_chance( p->talent.havoc.a_fire_inside->effectN( 3 ).percent() )
  {
    may_miss     = false;
    dot_duration = timespan_t::zero();
    set_target( p );  // Does not require a hostile target

    apply_affecting_aura( p->spec.immolation_aura_cdr );
    apply_affecting_aura( p->talent.havoc.a_fire_inside );

    if ( p->specialization() == DEMON_HUNTER_VENGEANCE )
    {
      energize_amount = data().effectN( 3 ).base_value();
    }
    else
    {
      energize_amount = data().effectN( 2 ).base_value();
    }

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
    return true;  // Disables initial hit if used at time 0
  }

  void execute() override
  {
    p()->buff.immolation_aura->trigger();
    demon_hunter_spell_t::execute();

    if ( p()->talent.havoc.a_fire_inside->ok() && rng().roll( afi_chance ) )
      cooldown->reset( true, 1 );

    p()->trigger_demonsurge( demonsurge_ability::CONSUMING_FIRE );
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
      aoe               = -1;
    }
  };

  double landing_distance;
  timespan_t gcd_lag;

  metamorphosis_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "metamorphosis", p, p->spec.metamorphosis ), landing_distance( 0.0 )
  {
    add_option( opt_float( "landing_distance", landing_distance, 0.0, 40.0 ) );
    parse_options( options_str );

    may_miss     = false;
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
    else  // DEMON_HUNTER_VENGEANCE
    {
    }
  }

  // Meta leap travel time and self-pacify is a 1s hidden aura (201453) regardless of distance
  // This is affected by aura lag and will slightly delay execution of follow-up attacks
  // Not always relevant as GCD can be longer than the 1s + lag ability delay outside of lust
  void schedule_execute( action_state_t* s ) override
  {
    gcd_lag = rng().gauss( sim->gcd_lag );
    min_gcd = 1_s + gcd_lag;
    demon_hunter_spell_t::schedule_execute( s );
  }

  timespan_t travel_time() const override
  {
    if ( p()->specialization() == DEMON_HUNTER_HAVOC )
      return min_gcd;
    else  // DEMON_HUNTER_VENGEANCE
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

      for ( demonsurge_ability ability : demonsurge_havoc_abilities )
      {
        p()->buff.demonsurge_abilities[ ability ]->trigger();
      }
      p()->buff.demonsurge_demonic->trigger();
      p()->buff.demonsurge_hardcast->trigger();
      p()->buff.demonsurge->expire();

      // Buff is gained at the start of the leap.
      p()->buff.metamorphosis->extend_duration_or_trigger();
      p()->buff.inner_demon->trigger();

      if ( p()->talent.havoc.chaotic_transformation->ok() )
      {
        p()->cooldown.eye_beam->reset( false );
        p()->cooldown.blade_dance->reset( false );
      }

      if ( p()->talent.felscarred.violent_transformation->ok() )
      {
        p()->cooldown.immolation_aura->reset( false, -1 );
        p()->cooldown.sigil_of_flame->reset( false );
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
    else  // DEMON_HUNTER_VENGEANCE
    {
      for ( demonsurge_ability ability : demonsurge_vengeance_abilities )
      {
        p()->buff.demonsurge_abilities[ ability ]->trigger();
      }
      p()->buff.demonsurge_demonic->trigger();
      p()->buff.demonsurge_hardcast->trigger();
      p()->buff.metamorphosis->trigger();
      p()->buff.demonsurge->expire();

      if ( p()->talent.felscarred.violent_transformation->ok() )
      {
        p()->cooldown.fel_devastation->reset( false );
        p()->cooldown.sigil_of_flame->reset( false, -1 );
      }
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
      : event_t( *f->dh, time ), dh( f->dh ), frag( f ), expr( e )
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
    range                          = 5.0;  // Disallow use outside of melee.
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
    double dtm    = std::max( 0.0, frag->get_distance( p() ) - 6.0 );
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
              candidate       = frag;
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
              candidate       = frag;
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
              candidate       = frag;
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

struct spirit_bomb_base_t : public demon_hunter_spell_t
{
  struct spirit_bomb_damage_t : public demon_hunter_spell_t
  {
    spirit_bomb_damage_t( util::string_view name, demon_hunter_t* p, std::basic_string<char> reporting_name )
      : demon_hunter_spell_t( name, p, p->spec.spirit_bomb_damage )
    {
      background = dual   = true;
      aoe                 = -1;
      reduced_aoe_targets = p->talent.vengeance.spirit_bomb->effectN( 3 ).base_value();
      name_str_reporting  = reporting_name;
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      p()->buff.soul_furnace_damage_amp->expire();
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        td( s->target )->debuffs.frailty->trigger();
      }
    }
  };

  spirit_bomb_damage_t* damage;
  unsigned max_fragments_consumed;

  spirit_bomb_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view options_str )
    : demon_hunter_spell_t( n, p, s, options_str ),
      max_fragments_consumed( static_cast<unsigned>( data().effectN( 2 ).base_value() ) )
  {
    may_miss = proc = callbacks = false;

    damage        = p->get_background_action<spirit_bomb_damage_t>( name_str + "damage", name_str );
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
      damage_state->target         = target;
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

struct spirit_burst_t : public demonsurge_trigger_t<demonsurge_ability::SPIRIT_BURST, spirit_bomb_base_t>
{
  spirit_burst_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "spirit_burst", p, p->hero_spec.spirit_burst, options_str )
  {
  }

  bool ready() override
  {
    if ( !p()->buff.demonsurge_demonic->check() )
    {
      return false;
    }
    return spirit_bomb_base_t::ready();
  }
};

struct spirit_bomb_t : public spirit_bomb_base_t
{
  spirit_bomb_t( demon_hunter_t* p, util::string_view options_str )
    : spirit_bomb_base_t( "spirit_bomb", p, p->talent.vengeance.spirit_bomb, options_str )
  {
  }

  bool ready() override
  {
    if ( p()->buff.demonsurge_demonic->check() )
    {
      return false;
    }
    return spirit_bomb_base_t::ready();
  }
};

// Sigil of Spite ===========================================================

struct sigil_of_spite_t : public demon_hunter_spell_t
{
  struct sigil_of_spite_sigil_t : public demon_hunter_sigil_t
  {
    unsigned soul_fragments_to_spawn;

    sigil_of_spite_sigil_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
      : demon_hunter_sigil_t( name, p, s, delay ),
        soul_fragments_to_spawn( as<unsigned>( p->spell.sigil_of_spite->effectN( 3 ).base_value() ) )
    {
      reduced_aoe_targets = p->spell.sigil_of_spite->effectN( 1 ).base_value();
    }

    void execute() override
    {
      demon_hunter_sigil_t::execute();
      p()->spawn_soul_fragment( soul_fragment::LESSER, soul_fragments_to_spawn );
      for ( unsigned i = 0; i < soul_fragments_to_spawn; i++ )
      {
        p()->proc.soul_fragment_from_sigil_of_spite->occur();
      }
    }
  };

  sigil_of_spite_sigil_t* sigil;

  sigil_of_spite_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "sigil_of_spite", p, p->spell.sigil_of_spite, options_str ), sigil( nullptr )
  {
    if ( p->spell.sigil_of_spite->ok() )
    {
      sigil = p->get_background_action<sigil_of_spite_sigil_t>( "sigil_of_spite_sigil", p->spell.sigil_of_spite_damage,
                                                                ground_aoe_duration );
      sigil->stats = stats;
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    sigil->place_sigil( target );
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

// The Hunt =================================================================

struct the_hunt_t : public demon_hunter_spell_t
{
  struct the_hunt_damage_t : public demon_hunter_spell_t
  {
    struct the_hunt_dot_t : public demon_hunter_spell_t
    {
      the_hunt_dot_t( util::string_view name, demon_hunter_t* p )
        : demon_hunter_spell_t( name, p, p->spell.the_hunt->effectN( 1 ).trigger()->effectN( 4 ).trigger() )
      {
        dual = true;
        aoe  = as<int>( p->spell.the_hunt->effectN( 2 ).trigger()->effectN( 1 ).base_value() );
      }
    };

    the_hunt_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spell.the_hunt->effectN( 1 ).trigger() )
    {
      dual          = true;
      impact_action = p->get_background_action<the_hunt_dot_t>( "the_hunt_dot" );
    }
  };

  the_hunt_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "the_hunt", p, p->spell.the_hunt, options_str )
  {
    movement_directionality             = movement_direction_type::TOWARDS;
    impact_action                       = p->get_background_action<the_hunt_damage_t>( "the_hunt_damage" );
    impact_action->stats                = stats;
    impact_action->impact_action->stats = stats;
  }

  void execute() override
  {
    p()->buff.momentum->trigger();

    demon_hunter_spell_t::execute();

    p()->set_out_of_range( timespan_t::zero() );  // Cancel all other movement

    if ( p()->specialization() != DEMON_HUNTER_VENGEANCE )
    {
      p()->consume_nearby_soul_fragments( soul_fragment::LESSER );
    }

    p()->buff.reavers_glaive->trigger();
  }

  timespan_t travel_time() const override
  {
    return 100_ms;
  }

  // Bypass the normal demon_hunter_spell_t out of range and movement ready checks
  bool ready() override
  {
    // Not usable during the root effect of Stormeater's Boon
    if ( p()->buffs.stormeaters_boon && p()->buffs.stormeaters_boon->check() )
      return false;

    return spell_t::ready();
  }
};

struct sigil_of_misery_t : public demon_hunter_spell_t
{
  struct sigil_of_misery_sigil_t : public demon_hunter_sigil_t
  {
    sigil_of_misery_sigil_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
      : demon_hunter_sigil_t( name, p, s, delay )
    {
    }

    void execute() override
    {
      demon_hunter_sigil_t::execute();
    }
  };

  sigil_of_misery_sigil_t* sigil;

  sigil_of_misery_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "sigil_of_misery", p, p->spec.sigil_of_misery, options_str ), sigil( nullptr )
  {
    if ( data().ok() )
    {
      sigil        = p->get_background_action<sigil_of_misery_sigil_t>( "sigil_of_misery_sigil",
                                                                        p->spec.sigil_of_misery_debuff, ground_aoe_duration );
      sigil->stats = stats;
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    sigil->place_sigil( target );
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

struct sigil_of_silence_t : public demon_hunter_spell_t
{
  struct sigil_of_silence_sigil_t : public demon_hunter_sigil_t
  {
    sigil_of_silence_sigil_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
      : demon_hunter_sigil_t( name, p, s, delay )
    {
    }

    void execute() override
    {
      demon_hunter_sigil_t::execute();
    }
  };

  sigil_of_silence_sigil_t* sigil;

  sigil_of_silence_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "sigil_of_silence", p, p->spec.sigil_of_silence, options_str ), sigil( nullptr )
  {
    if ( data().ok() )
    {
      sigil = p->get_background_action<sigil_of_silence_sigil_t>(
          "sigil_of_silence_sigil", p->spec.sigil_of_silence_debuff, ground_aoe_duration );
      sigil->stats = stats;
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    sigil->place_sigil( target );
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

struct sigil_of_chains_t : public demon_hunter_spell_t
{
  struct sigil_of_chains_sigil_t : public demon_hunter_sigil_t
  {
    sigil_of_chains_sigil_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
      : demon_hunter_sigil_t( name, p, s, delay )
    {
    }

    void execute() override
    {
      demon_hunter_sigil_t::execute();
    }
  };

  sigil_of_chains_sigil_t* sigil;

  sigil_of_chains_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "sigil_of_chains", p, p->spec.sigil_of_chains, options_str ), sigil( nullptr )
  {
    if ( data().ok() )
    {
      sigil        = p->get_background_action<sigil_of_chains_sigil_t>( "sigil_of_chains_sigil",
                                                                        p->spec.sigil_of_chains_debuff, ground_aoe_duration );
      sigil->stats = stats;
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    sigil->place_sigil( target );
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

struct demonsurge_t : public amn_full_mastery_bug_t<demon_hunter_spell_t>
{
  demonsurge_t( util::string_view name, demon_hunter_t* p )
    : amn_full_mastery_bug_t( name, p, p->hero_spec.demonsurge_damage )
  {
    background = dual = true;
    aoe               = -1;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    p()->buff.demonsurge->trigger();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = demon_hunter_spell_t::composite_da_multiplier( s );

    // Focused Hatred increases Demonsurge damage when hitting only one target
    if ( p()->talent.felscarred.focused_hatred->ok() )
    {
      if ( p()->is_ptr() && s->n_targets <= 5 )
      {
        // 1 target is always effect 1 %
        // 2 target is effect 1 % - effect 2 %
        // 3 target is effect 1 % - (effect 2 % * 2)
        // etc up to 5 target
        auto num_target_reduction_percent =
            p()->talent.felscarred.focused_hatred->effectN( 2 ).percent() * ( s->n_targets - 1 );
        m *= 1.0 + ( p()->talent.felscarred.focused_hatred->effectN( 1 ).percent() - num_target_reduction_percent );
      }
      else if ( !p()->is_ptr() && s->n_targets == 1 )
      {
        m *= 1.0 + p()->talent.felscarred.focused_hatred->effectN( 1 ).percent();
      }
    }

    return m;
  }
};

}  // end namespace spells

// ==========================================================================
// Demon Hunter attacks
// ==========================================================================

namespace attacks
{

template <typename BASE>
struct burning_blades_trigger_t : public BASE
{
  using base_t = burning_blades_trigger_t<BASE>;

  burning_blades_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                            util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( !BASE::p()->talent.felscarred.burning_blades->ok() )
      return;

    if ( !action_t::result_is_hit( s->result ) )
      return;

    const double dot_damage = s->result_amount * BASE::p()->talent.felscarred.burning_blades->effectN( 1 ).percent();
    residual_action::trigger( BASE::p()->active.burning_blades, s->target, dot_damage );
  }
};

template <typename BASE>
struct soulscar_trigger_t : public BASE
{
  using base_t = soulscar_trigger_t<BASE>;

  soulscar_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                      util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( !BASE::p()->talent.havoc.soulscar->ok() )
      return;

    if ( !action_t::result_is_hit( s->result ) )
      return;

    const double dot_damage = s->result_amount * BASE::p()->talent.havoc.soulscar->effectN( 1 ).percent();
    residual_action::trigger( BASE::p()->active.soulscar, s->target, dot_damage );
  }
};

template <typename BASE>
struct felblade_trigger_t : public BASE
{
  using base_t = felblade_trigger_t<BASE>;

  felblade_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                      util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( !BASE::p()->talent.demon_hunter.felblade->ok() || !BASE::p()->rppm.felblade->trigger() )
      return;

    if ( action_t::result_is_miss( s->result ) )
      return;

    BASE::p()->proc.felblade_reset->occur();
    BASE::p()->cooldown.felblade->reset( true );
  }
};

// Auto Attack ==============================================================

struct auto_attack_damage_t : public burning_blades_trigger_t<demon_hunter_attack_t>
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

  auto_attack_damage_t( util::string_view name, demon_hunter_t* p, weapon_t* w,
                        const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, p, s )
  {
    school     = SCHOOL_PHYSICAL;
    special    = false;
    background = repeating = may_glance = may_crit = true;
    allow_class_ability_procs = not_a_proc = true;
    trigger_gcd                            = timespan_t::zero();
    weapon                                 = w;
    weapon_multiplier                      = 1.0;
    base_execute_time                      = weapon->swing_time;

    status.main_hand = status.off_hand = aa_contact::LOST_RANGE;

    if ( p->dual_wield() )
    {
      base_hit -= 0.19;
    }
  }

  double action_multiplier() const override
  {
    double m = base_t::action_multiplier();

    // Class Passives
    m *= 1.0 + p()->spec.havoc_demon_hunter->effectN( 8 ).percent();
    m *= 1.0 + p()->spec.vengeance_demon_hunter->effectN( 10 ).percent();

    return m;
  }

  void reset() override
  {
    base_t::reset();

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
    base_t::impact( s );

    trigger_demon_blades( s );
    if ( p()->talent.aldrachi_reaver.wounded_quarry->ok() && td( s->target )->debuffs.reavers_mark->up() )
    {
      p()->active.wounded_quarry->execute_on_target( s->target );
      // 2024-08-04 -- Chance seems to be 30% for Vengeance, 10% for Havoc
      if ( rng().roll( p()->hero_spec.wounded_quarry_proc_rate ) )
      {
        p()->proc.soul_fragment_from_wounded_quarry->occur();
        p()->spawn_soul_fragment( soul_fragment::LESSER );
      }
    }
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
    range       = 5;
    trigger_gcd = timespan_t::zero();

    p->melee_main_hand  = new auto_attack_damage_t( "auto_attack_mh", p, &( p->main_hand_weapon ) );
    p->main_hand_attack = p->melee_main_hand;

    p->melee_off_hand      = new auto_attack_damage_t( "auto_attack_oh", p, &( p->off_hand_weapon ) );
    p->off_hand_attack     = p->melee_off_hand;
    p->off_hand_attack->id = 1;
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

struct blade_dance_base_t
  : public cycle_of_hatred_trigger_t<
        art_of_the_glaive_trigger_t<art_of_the_glaive_ability::GLAIVE_FLURRY, demon_hunter_attack_t>>
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
                          const spell_data_t* first_blood_override = nullptr )
      : demon_hunter_attack_t( name, p, first_blood_override ? first_blood_override : eff.trigger() ),
        delay( timespan_t::from_millis( eff.misc_value1() ) ),
        trail_of_ruin_dot( nullptr ),
        last_attack( false ),
        from_first_blood( first_blood_override != nullptr )
    {
      background = dual   = true;
      aoe                 = ( from_first_blood ) ? 0 : -1;
      reduced_aoe_targets = p->find_spell( 199552 )->effectN( 1 ).base_value();  // Use first impact spell
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      demon_hunter_attack_t::available_targets( tl );

      if ( p()->talent.havoc.first_blood->ok() && !from_first_blood )
      {
        // Ensure the non-First Blood AoE spell doesn't hit the primary target
        range::erase_remove( tl, target );
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
    }

    void execute() override
    {
      demon_hunter_attack_t::execute();

      if ( last_attack && p()->buff.restless_hunter->check() )
      {
        // 2023-01-31 -- If Restless Hunter is triggered when the delayed final impact is queued, it does not fade
        //               Seems similar to some other 500ms buff protection in the game
        if ( sim->current_time() - p()->buff.restless_hunter->last_trigger_time() > 0.5_s )
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

  blade_dance_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view options_str,
                      buff_t* dodge_buff )
    : base_t( n, p, s, options_str ), dodge_buff( dodge_buff ), trail_of_ruin_dot( nullptr )
  {
    may_miss = false;
    cooldown = p->cooldown.blade_dance;  // Blade Dance/Death Sweep Category Cooldown
    range    = 5.0;                      // Disallow use outside of melee range.

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
    base_t::init_finished();

    for ( auto& attack : attacks )
    {
      attack->stats = stats;
    }

    if ( attacks.back() )
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

  double cost() const override
  {
    // TWW1 4pc % cost reduction results in a fractional fury cost, but testing shows rounding up
    return ceil( base_t::cost() );
  }

  void execute() override
  {
    // Blade Dance/Death Sweep Shared Category Cooldown
    cooldown->duration = ability_cooldown;

    base_t::execute();

    p()->buff.chaos_theory->trigger();

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

    if ( p()->set_bonuses.tww1_havoc_4pc->ok() )
    {
      p()->buff.tww1_havoc_4pc->expire();
    }
  }

  bool has_amount_result() const override
  {
    return true;
  }
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
    thrill_delay = timespan_t::from_millis( data().effectN( 5 ).misc_value1() + 1 );

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

    p()->trigger_demonsurge( demonsurge_ability::DEATH_SWEEP );
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

struct chaos_strike_base_t
  : public cycle_of_hatred_trigger_t<
        art_of_the_glaive_trigger_t<art_of_the_glaive_ability::RENDING_STRIKE, demon_hunter_attack_t>>
{
  struct chaos_strike_damage_t : public burning_blades_trigger_t<demon_hunter_attack_t>
  {
    timespan_t delay;
    chaos_strike_base_t* parent;
    bool may_refund;
    double refund_proc_chance;

    chaos_strike_damage_t( util::string_view name, demon_hunter_t* p, const spelleffect_data_t& eff,
                           chaos_strike_base_t* a )
      : base_t( name, p, eff.trigger() ),
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
      base_t::execute();

      if ( may_refund )
      {
        // Technically this appears to have a 0.5s ICD, but this is handled elsewhere
        // Onslaught can currently proc refunds due to being delayed by 600ms
        if ( p()->cooldown.chaos_strike_refund_icd->up() && p()->rng().roll( this->get_refund_proc_chance() ) )
        {
          p()->resource_gain( RESOURCE_FURY, p()->spec.chaos_strike_fury->effectN( 1 ).resource( RESOURCE_FURY ),
                              parent->gain );
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
      base_t::impact( s );

      // Relentless Onslaught cannot self-proc and is delayed by ~300ms after the normal OH impact
      if ( p()->talent.havoc.relentless_onslaught->ok() )
      {
        if ( result_is_hit( s->result ) && may_refund && !parent->from_onslaught )
        {
          double chance = p()->talent.havoc.relentless_onslaught->effectN( 1 ).percent();
          if ( p()->cooldown.relentless_onslaught_icd->up() && p()->rng().roll( chance ) )
          {
            attack_t* const relentless_onslaught = p()->buff.metamorphosis->check()
                                                       ? p()->active.relentless_onslaught_annihilation
                                                       : p()->active.relentless_onslaught;
            make_event<delayed_execute_event_t>( *sim, p(), relentless_onslaught, target, this->delay );
            p()->cooldown.relentless_onslaught_icd->start(
                p()->talent.havoc.relentless_onslaught->internal_cooldown() );
          }
        }
      }

      // TOCHECK -- Does this proc from Relentless Onslaught?
      // TOCHECK -- Does the applying Chaos Strike/Annihilation benefit from the debuff?
      if ( p()->talent.havoc.serrated_glaive->ok() )
      {
        td( s->target )->debuffs.serrated_glaive->trigger();
      }

      if ( p()->talent.aldrachi_reaver.warblades_hunger && p()->buff.warblades_hunger->up() )
      {
        p()->active.warblades_hunger->execute_on_target( target );
        p()->buff.warblades_hunger->expire();
      }
    }
  };

  std::vector<chaos_strike_damage_t*> attacks;
  bool from_onslaught;
  double tww1_reset_proc_chance;

  chaos_strike_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s,
                       util::string_view options_str = {} )
    : base_t( n, p, s, options_str ), from_onslaught( false ), tww1_reset_proc_chance( 0.0 )
  {
    if ( p->set_bonuses.tww1_havoc_4pc->ok() )
    {
      tww1_reset_proc_chance = p->set_bonuses.tww1_havoc_4pc->effectN( 1 ).percent();
    }
  }

  double cost() const override
  {
    if ( from_onslaught )
      return 0.0;

    return base_t::cost();
  }

  void init_finished() override
  {
    base_t::init_finished();

    // Use one stats object for all parts of the attack.
    for ( auto& attack : attacks )
    {
      attack->stats = stats;
    }
  }

  void execute() override
  {
    base_t::execute();

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
    if ( !from_onslaught && p()->rppm.demonic_appetite->trigger() )
    {
      p()->proc.demonic_appetite->occur();
      p()->spawn_soul_fragment( soul_fragment::LESSER );
    }

    if ( p()->buff.inner_demon->check() )
    {
      make_event<delayed_execute_event_t>( *sim, p(), p()->active.inner_demon, target, 1.25_s );
      p()->buff.inner_demon->expire();
    }

    // Note - cannot proc fury reduction buff if blade dance is not on cooldown
    // 2024-09-06 -- Cannot proc if Blade Dance has less than 3s left on CD
    if ( p()->set_bonuses.tww1_havoc_4pc->ok() && p()->cooldown.blade_dance->down() &&
         p()->cooldown.blade_dance->remains() >= 3_s && p()->rng().roll( tww1_reset_proc_chance ) )
    {
      p()->buff.tww1_havoc_4pc->trigger();
      p()->cooldown.blade_dance->reset( true );
    }
  }

  bool has_amount_result() const override
  {
    return true;
  }
};

struct chaos_strike_t : public chaos_strike_base_t
{
  chaos_strike_t( util::string_view name, demon_hunter_t* p, util::string_view options_str = {} )
    : chaos_strike_base_t( name, p, p->spec.chaos_strike, options_str )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_1", name ),
                                                                          data().effectN( 2 ), this ) );
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_2", name ),
                                                                          data().effectN( 3 ), this ) );
    }
  }

  void init() override
  {
    chaos_strike_base_t::init();

    if ( !from_onslaught && p()->active.relentless_onslaught )
    {
      add_child( p()->active.relentless_onslaught );
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

struct annihilation_t : public demonsurge_trigger_t<demonsurge_ability::ANNIHILATION, chaos_strike_base_t>
{
  annihilation_t( util::string_view name, demon_hunter_t* p, util::string_view options_str = {} )
    : base_t( name, p, p->spec.annihilation, options_str )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_1", name ),
                                                                          data().effectN( 2 ), this ) );
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_2", name ),
                                                                          data().effectN( 3 ), this ) );
    }
  }

  void init() override
  {
    chaos_strike_base_t::init();

    if ( !from_onslaught && p()->active.relentless_onslaught_annihilation )
    {
      add_child( p()->active.relentless_onslaught_annihilation );
    }
  }

  bool ready() override
  {
    if ( !p()->buff.metamorphosis->check() )
    {
      return false;
    }

    return base_t::ready();
  }
};

// Burning Wound ============================================================

struct burning_wound_t : public demon_hunter_spell_t
{
  burning_wound_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_spell_t( name, p, p->spec.burning_wound_debuff )
  {
    dual = true;
    aoe  = 0;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // This DoT has a limit 3 effect, oldest applications are removed first
      if ( as<int>( player->get_active_dots( get_dot() ) ) > data().max_targets() )
      {
        player_t* lowest_duration =
            *std::min_element( sim->target_non_sleeping_list.begin(), sim->target_non_sleeping_list.end(),
                               [ this, s ]( player_t* lht, player_t* rht ) {
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
        p()->sim->print_debug( "{} removes burning_wound on {} with duration {}", *p(), *lowest_duration,
                               tdata->dots.burning_wound->remains().total_seconds() );
        tdata->debuffs.burning_wound->cancel();
        tdata->dots.burning_wound->cancel();

        assert( as<int>( player->get_active_dots( get_dot() ) ) <= data().max_targets() );
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

struct demons_bite_t : public felblade_trigger_t<demon_hunter_attack_t>
{
  demons_bite_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "demons_bite", p, p->spec.demons_bite, options_str )
  {
    energize_delta = energize_amount * data().effectN( 3 ).m_delta();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = base_t::composite_energize_amount( s );

    if ( p()->talent.havoc.insatiable_hunger->ok() )
    {
      ea += static_cast<int>( p()->rng().range( p()->talent.havoc.insatiable_hunger->effectN( 3 ).base_value(),
                                                1 + p()->talent.havoc.insatiable_hunger->effectN( 4 ).base_value() ) );
    }
    if ( p()->talent.felscarred.demonsurge->ok() && p()->buff.metamorphosis->check() )
    {
      ea += as<int>( p()->spec.metamorphosis_buff->effectN( 11 ).base_value() );
    }

    return ea;
  }

  void execute() override
  {
    base_t::execute();

    if ( p()->buff.metamorphosis->check() )
    {
      p()->proc.demons_bite_in_meta->occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

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

struct demon_blades_t : public felblade_trigger_t<demon_hunter_attack_t>
{
  demon_blades_t( demon_hunter_t* p ) : base_t( "demon_blades", p, p->spec.demon_blades_damage )
  {
    background     = true;
    energize_delta = energize_amount * data().effectN( 2 ).m_delta();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = base_t::composite_energize_amount( s );

    if ( p()->talent.felscarred.demonsurge->ok() && p()->buff.metamorphosis->check() )
    {
      ea += as<int>( p()->spec.metamorphosis_buff->effectN( 10 ).base_value() );
    }

    return ea;
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

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
    aoe                 = -1;
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
      gain              = p->get_gain( "felblade" );
    }
  };

  unsigned max_fragments_consumed;

  felblade_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "felblade", p, p->talent.demon_hunter.felblade, options_str ),
      max_fragments_consumed(
          p->specialization() == DEMON_HUNTER_HAVOC && p->talent.aldrachi_reaver.warblades_hunger->ok()
              ? as<unsigned>( p->talent.aldrachi_reaver.warblades_hunger->effectN( 2 ).base_value() )
              : 0 )
  {
    may_block               = false;
    movement_directionality = movement_direction_type::TOWARDS;

    execute_action        = p->get_background_action<felblade_damage_t>( "felblade_damage" );
    execute_action->stats = stats;

    // Add damage modifiers in felblade_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();
    p()->set_out_of_range( timespan_t::zero() );  // Cancel all other movement
    if ( max_fragments_consumed > 0 )
    {
      event_t::cancel( p()->soul_fragment_pick_up );
      p()->consume_soul_fragments( soul_fragment::ANY, false, max_fragments_consumed );
    }
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
      background = dual   = true;
      aoe                 = -1;
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
    }
  };

  timespan_t gcd_lag;

  fel_rush_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_attack_t( "fel_rush", p, p->spec.fel_rush )
  {
    parse_options( options_str );

    may_miss = may_dodge = may_parry = may_block = false;
    min_gcd                                      = trigger_gcd;

    execute_action        = p->get_background_action<fel_rush_damage_t>( "fel_rush_damage" );
    execute_action->stats = stats;

    // Fel Rush does damage in a further line than it moves you
    base_teleport_distance                = execute_action->radius - 5;
    movement_directionality               = movement_direction_type::OMNI;
    p->buff.fel_rush_move->distance_moved = base_teleport_distance;

    // Add damage modifiers in fel_rush_damage_t, not here.
  }

  void execute() override
  {
    p()->buff.momentum->trigger();

    demon_hunter_attack_t::execute();

    if ( p()->buff.inertia_trigger->up() && p()->talent.havoc.inertia->ok() )
    {
      p()->buff.inertia_trigger->expire();
      p()->buff.inertia->trigger();
    }

    p()->buff.unbound_chaos->expire();

    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    p()->cooldown.movement_shared->start( timespan_t::from_seconds( 1.0 ) );

    p()->consume_nearby_soul_fragments( soul_fragment::LESSER );

    p()->buff.fel_rush_move->trigger();
  }

  void schedule_execute( action_state_t* s ) override
  {
    // Fel Rush's loss of control causes a GCD lag after the loss ends.
    // You get roughly 100ms in which to queue the next spell up correctly.
    // Calculate this once on schedule_execute since gcd() is called multiple times
    if ( sim->gcd_lag.mean > 100_ms )
      gcd_lag = rng().gauss( sim->gcd_lag.mean - 100_ms, sim->gcd_lag.stddev );
    else
      gcd_lag = 0_ms;

    if ( gcd_lag < 0_ms )
      gcd_lag = 0_ms;

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

struct fracture_t : public felblade_trigger_t<
                        art_of_the_glaive_trigger_t<art_of_the_glaive_ability::RENDING_STRIKE, demon_hunter_attack_t>>
{
  struct fracture_damage_t : public demon_hunter_attack_t
  {
    int soul_fragments_to_spawn;

    fracture_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, int i )
      : demon_hunter_attack_t( name, p, s ), soul_fragments_to_spawn( i )
    {
      dual     = true;
      may_miss = may_dodge = may_parry = false;
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      p()->spawn_soul_fragment( soul_fragment::LESSER, soul_fragments_to_spawn );
      for ( int i = 0; i < soul_fragments_to_spawn; i++ )
      {
        p()->proc.soul_fragment_from_fracture->occur();
      }
    }
  };

  fracture_damage_t *mh, *oh;

  fracture_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "fracture", p, p->talent.vengeance.fracture, options_str )
  {
    int number_of_soul_fragments_to_spawn = as<int>( data().effectN( 1 ).base_value() );
    // divide the number in 2 as half come from main hand, half come from offhand.
    int number_of_soul_fragments_to_spawn_per_hit = number_of_soul_fragments_to_spawn / 2;
    // handle leftover souls in the event that blizz ever changes Fracture to an odd number of souls generated
    int number_of_soul_fragments_to_spawn_leftover = number_of_soul_fragments_to_spawn % 2;

    int mh_soul_fragments_to_spawn =
        number_of_soul_fragments_to_spawn_per_hit + number_of_soul_fragments_to_spawn_leftover;
    int oh_soul_fragments_to_spawn = number_of_soul_fragments_to_spawn_per_hit;

    mh        = p->get_background_action<fracture_damage_t>( "fracture_mh", data().effectN( 2 ).trigger(),
                                                             mh_soul_fragments_to_spawn );
    oh        = p->get_background_action<fracture_damage_t>( "fracture_oh", data().effectN( 3 ).trigger(),
                                                             oh_soul_fragments_to_spawn );
    mh->stats = oh->stats = stats;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = base_t::composite_energize_amount( s );

    if ( p()->buff.metamorphosis->check() )
    {
      ea += p()->spec.metamorphosis_buff->effectN( 10 ).resource( RESOURCE_FURY );
    }

    return ea;
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    /*
     * logged event ordering for Fracture:
     * - cast Fracture (225919 - main hand spell ID)
     * - cast Fracture (263642 - container spell ID)
     * - cast Fracture (225921 - offhand spell ID)
     * - generate Soul Fragment from main hand hit
     * - generate Soul Fragment from offhand hit
     * - generate Soul Fragment if Metamorphosis is active
     */
    if ( result_is_hit( s->result ) )
    {
      mh->set_target( s->target );
      mh->execute();

      // offhand hit is ~150ms after cast
      make_event<delayed_execute_event_t>( *p()->sim, p(), oh, s->target,
                                           timespan_t::from_millis( data().effectN( 3 ).misc_value1() ) );

      if ( p()->buff.metamorphosis->check() )
      {
        // In all reviewed logs, it's always 500ms (based on Fires of Fel application)
        make_event( sim, 500_ms, [ this ]() {
          p()->spawn_soul_fragment( soul_fragment::LESSER );
          p()->proc.soul_fragment_from_meta->occur();
        } );
      }

      if ( p()->talent.aldrachi_reaver.warblades_hunger && p()->buff.warblades_hunger->up() )
      {
        p()->active.warblades_hunger->execute_on_target( target );
        p()->buff.warblades_hunger->expire();
      }
    }
  }
};

// Ragefire Talent ==========================================================

struct ragefire_t : public demon_hunter_spell_t
{
  ragefire_t( util::string_view name, demon_hunter_t* p ) : demon_hunter_spell_t( name, p, p->spec.ragefire_damage )
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

struct shear_t : public felblade_trigger_t<
                     art_of_the_glaive_trigger_t<art_of_the_glaive_ability::RENDING_STRIKE, demon_hunter_attack_t>>
{
  shear_t( demon_hunter_t* p, util::string_view options_str ) : base_t( "shear", p, p->spec.shear, options_str )
  {
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->spawn_soul_fragment( soul_fragment::LESSER );
      p()->proc.soul_fragment_from_shear->occur();

      if ( p()->buff.metamorphosis->check() )
      {
        p()->spawn_soul_fragment( soul_fragment::LESSER );
        p()->proc.soul_fragment_from_meta->occur();
      }

      if ( p()->talent.aldrachi_reaver.warblades_hunger && p()->buff.warblades_hunger->up() )
      {
        p()->active.warblades_hunger->execute_on_target( target );
        p()->buff.warblades_hunger->expire();
      }
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

    return ea;
  }

  bool verify_actor_spec() const override
  {
    if ( p()->talent.vengeance.fracture->ok() )
      return false;

    return demon_hunter_attack_t::verify_actor_spec();
  }
};

// Soul Cleave ==============================================================

struct soul_cleave_base_t
  : public art_of_the_glaive_trigger_t<art_of_the_glaive_ability::GLAIVE_FLURRY, demon_hunter_attack_t>
{
  struct soul_cleave_damage_t : public burning_blades_trigger_t<demon_hunter_attack_t>
  {
    soul_cleave_damage_t( util::string_view name, demon_hunter_t* p, std::basic_string<char> reporting_name,
                          const spell_data_t* s )
      : burning_blades_trigger_t( name, p, s )
    {
      dual               = true;
      name_str_reporting = reporting_name;
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      // Soul Cleave can apply Frailty if Void Reaver is talented
      if ( result_is_hit( s->result ) && p()->talent.vengeance.void_reaver->ok() )
      {
        td( s->target )->debuffs.frailty->trigger();
      }
    }

    void execute() override
    {
      base_t::execute();

      p()->buff.soul_furnace_damage_amp->expire();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = base_t::composite_da_multiplier( s );

      if ( s->chain_target == 0 && p()->talent.vengeance.focused_cleave->ok() )
      {
        m *= 1.0 + p()->talent.vengeance.focused_cleave->effectN( 1 ).percent();
      }

      return m;
    }
  };

  heals::soul_cleave_heal_t* heal;

  soul_cleave_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s,
                      util::string_view options_str = {} )
    : base_t( n, p, s, options_str ), heal( nullptr )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    attack_power_mod.direct = 0;  // This parent action deals no damage, parsed data is for the heal
    cooldown                = p->cooldown.soul_cleave;

    execute_action =
        p->get_background_action<soul_cleave_damage_t>( name_str + "_damage", name_str, data().effectN( 2 ).trigger() );
    execute_action->stats = stats;

    if ( p->spec.soul_cleave_2->ok() )
    {
      heal = p->get_background_action<heals::soul_cleave_heal_t>( name_str + "_heal", name_str );
    }

    if ( p->talent.vengeance.void_reaver->ok() && !p->active.frailty_heal )
    {
      p->active.frailty_heal = new heals::frailty_heal_t( p );
    }
    // Add damage modifiers in soul_cleave_damage_t, not here.
  }

  void execute() override
  {
    base_t::execute();

    if ( heal )
    {
      heal->set_target( player );
      heal->execute();
    }

    // Soul Cleave applies a stack of Frailty to the primary target if Soulcrush is talented,
    // doesn't need to hit.
    if ( p()->talent.vengeance.soulcrush->ok() )
    {
      td( target )->debuffs.frailty->trigger(
          timespan_t::from_seconds( p()->talent.vengeance.soulcrush->effectN( 2 ).base_value() ) );
    }

    // Soul fragments consumed are capped for Soul Cleave
    p()->consume_soul_fragments( soul_fragment::ANY, true, static_cast<unsigned>( data().effectN( 3 ).base_value() ) );

    // TWWBETA TOCHECK -- Is this flat % chance or something else (deck?)
    if ( p()->set_bonuses.tww1_vengeance_2pc->ok() &&
         p()->rng().roll( p()->set_bonuses.tww1_vengeance_2pc->effectN( 2 ).percent() ) )
    {
      unsigned soul_fragments_to_spawn = static_cast<unsigned>( data().effectN( 3 ).base_value() );
      p()->spawn_soul_fragment( soul_fragment::LESSER, soul_fragments_to_spawn );
      for ( unsigned i = 0; i < soul_fragments_to_spawn; i++ )
      {
        p()->proc.soul_fragment_from_vengeance_twws1_2pc->occur();
      }
    }
  }
};

struct soul_sunder_t : public demonsurge_trigger_t<demonsurge_ability::SOUL_SUNDER, soul_cleave_base_t>
{
  soul_sunder_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "soul_sunder", p, p->hero_spec.soul_sunder, options_str )
  {
  }

  bool ready() override
  {
    if ( !p()->buff.demonsurge_demonic->check() )
    {
      return false;
    }
    return base_t::ready();
  }
};

struct soul_cleave_t : public soul_cleave_base_t
{
  soul_cleave_t( demon_hunter_t* p, util::string_view options_str )
    : soul_cleave_base_t( "soul_cleave", p, p->spec.soul_cleave, options_str )
  {
  }

  bool ready() override
  {
    if ( p()->buff.demonsurge_demonic->check() )
    {
      return false;
    }
    return soul_cleave_base_t::ready();
  }
};

// Throw Glaive =============================================================

struct throw_glaive_t : public cycle_of_hatred_trigger_t<demon_hunter_attack_t>
{
  struct throw_glaive_damage_t : public soulscar_trigger_t<burning_blades_trigger_t<demon_hunter_attack_t>>
  {
    throw_glaive_damage_t( util::string_view name, demon_hunter_t* p )
      : base_t( name, p, p->spell.throw_glaive->effectN( 1 ).trigger() )
    {
      background = dual = true;
      radius            = 10.0;
    }

    void impact( action_state_t* state ) override
    {
      base_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( p()->spec.burning_wound_debuff->ok() )
        {
          p()->active.burning_wound->execute_on_target( state->target );
        }

        if ( p()->talent.havoc.serrated_glaive->ok() )
        {
          td( state->target )->debuffs.serrated_glaive->trigger();
        }
      }
    }
  };

  throw_glaive_damage_t* furious_throws;

  throw_glaive_t( util::string_view name, demon_hunter_t* p, util::string_view options_str )
    : base_t( name, p, p->spell.throw_glaive, options_str ), furious_throws( nullptr )
  {
    throw_glaive_damage_t* damage = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_damage" );

    execute_action        = damage;
    execute_action->stats = stats;

    if ( p->talent.havoc.furious_throws->ok() )
    {
      resource_current            = RESOURCE_FURY;
      base_costs[ RESOURCE_FURY ] = p->talent.havoc.furious_throws->effectN( 1 ).base_value();

      furious_throws = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_furious_throws" );

      add_child( furious_throws );
    }
  }

  void init() override
  {
    track_cd_waste = false;
    base_t::init();

    track_cd_waste = true;
    cd_wasted_exec =
        p()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>( "throw_glaive", p()->cd_waste_exec );
    cd_wasted_cumulative = p()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>(
        "throw_glaive", p()->cd_waste_cumulative );
    cd_wasted_iter =
        p()->template get_data_entry<simple_sample_data_t, simple_data_t>( "throw_glaive", p()->cd_waste_iter );
  }

  bool has_talents_for_cycle_of_hatred() override
  {
    return base_t::has_talents_for_cycle_of_hatred() && p()->talent.havoc.furious_throws->ok();
  }

  void execute() override
  {
    base_t::execute();

    if ( hit_any_target && furious_throws )
    {
      make_event<delayed_execute_event_t>( *sim, p(), furious_throws, target, 400_ms );
    }

    if ( td( target )->debuffs.serrated_glaive->up() )
    {
      p()->proc.throw_glaive_in_serrated_glaive->occur();
    }

    if ( p()->active.preemptive_strike )
    {
      p()->active.preemptive_strike->execute_on_target( target );
    }
  }

  bool ready() override
  {
    if ( p()->buff.reavers_glaive->up() )
    {
      return false;
    }

    return base_t::ready();
  }
};

// Reaver's Glaive ==========================================================
struct reavers_glaive_t : public soulscar_trigger_t<demon_hunter_attack_t>
{
  reavers_glaive_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "reavers_glaive", p, p->hero_spec.reavers_glaive, options_str )
  {
    if ( p->talent.aldrachi_reaver.keen_engagement->ok() )
    {
      energize_type     = action_energize::ON_CAST;
      energize_resource = data().effectN( 2 ).resource_gain_type();
      energize_amount   = p->talent.aldrachi_reaver.keen_engagement->effectN( 1 ).resource( energize_resource );
    }
    else
    {
      energize_type = action_energize::NONE;
    }
  }

  void execute() override
  {
    p()->buff.reavers_glaive->expire();

    base_t::execute();

    if ( p()->active.preemptive_strike )
    {
      p()->active.preemptive_strike->execute_on_target( target );
    }

    p()->buff.glaive_flurry->trigger();
    p()->buff.rending_strike->trigger();
    p()->buff.art_of_the_glaive_first->trigger();
  }

  bool ready() override
  {
    if ( !p()->buff.reavers_glaive->up() )
    {
      return false;
    }
    // 2024-07-11 -- Reaver's Glaive can't be cast unless a GCD is available, but doesn't trigger a GCD.
    if ( p()->gcd_ready > sim->current_time() )
    {
      return false;
    }

    return base_t::ready();
  }
};

// Soulscar =================================================================
struct soulscar_t : public residual_action::residual_periodic_action_t<demon_hunter_attack_t>
{
  soulscar_t( util::string_view name, demon_hunter_t* p ) : base_t( name, p, p->spec.soulscar_debuff )
  {
    dual = true;
  }

  void init() override
  {
    base_t::init();
    update_flags = 0;  // Snapshots on refresh, does not update dynamically
  }
};

// Burning Blades ===========================================================
struct burning_blades_t
  : public residual_action::residual_periodic_action_t<amn_full_mastery_bug_t<demon_hunter_spell_t>>
{
  burning_blades_t( util::string_view name, demon_hunter_t* p ) : base_t( name, p, p->hero_spec.burning_blades_debuff )
  {
    dual = true;
  }

  void init() override
  {
    base_t::init();
    update_flags = 0;  // Snapshots on refresh, does not update dynamically
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
      aoe               = -1;
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
    execute_action        = p->get_background_action<vengeful_retreat_damage_t>( "vengeful_retreat_damage" );
    execute_action->stats = stats;

    base_teleport_distance                        = VENGEFUL_RETREAT_DISTANCE;
    movement_directionality                       = movement_direction_type::OMNI;
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

    if ( p()->specialization() != DEMON_HUNTER_VENGEANCE )
    {
      p()->consume_nearby_soul_fragments( soul_fragment::LESSER );
    }

    if ( p()->talent.aldrachi_reaver.unhindered_assault->ok() )
    {
      p()->proc.felblade_reset->occur();
      p()->cooldown.felblade->reset( true );
    }
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

// Art of the Glaive ===================================================
struct art_of_the_glaive_t : public demon_hunter_attack_t
{
  struct art_of_the_glaive_damage_t : public demon_hunter_attack_t
  {
    timespan_t delay;

    art_of_the_glaive_damage_t( util::string_view name, demon_hunter_t* p, const spelleffect_data_t& eff,
                                std::basic_string<char> reporting_name )
      : demon_hunter_attack_t( name, p, eff.trigger() ), delay( timespan_t::from_millis( eff.misc_value1() ) )
    {
      background = dual  = true;
      aoe                = -1;
      name_str_reporting = reporting_name;
    }
  };

  std::vector<art_of_the_glaive_damage_t*> attacks;

  art_of_the_glaive_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_attack_t( name, p, p->hero_spec.art_of_the_glaive_damage )
  {
    background = dual = true;
    for ( const spelleffect_data_t& effect : data().effects() )
    {
      if ( effect.type() != E_TRIGGER_SPELL )
        continue;

      attacks.push_back( p->get_background_action<art_of_the_glaive_damage_t>(
          fmt::format( "art_of_the_glaive_{}", effect.index() ), effect, "art_of_the_glaive" ) );
    }
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

    // if glaive flurry is up and rending strike is not up
    // fury of the aldrachi causes art of the glaive to retrigger itself for 3 additional procs 300ms after initial
    // execution
    if ( p()->talent.aldrachi_reaver.fury_of_the_aldrachi->ok() && p()->buff.glaive_flurry->up() &&
         !p()->buff.rending_strike->up() )
    {
      make_event<delayed_execute_event_t>( *sim, p(), p()->active.art_of_the_glaive, target, 300_ms );
    }

    for ( auto attack : attacks )
    {
      make_event<delayed_execute_event_t>( *sim, p(), attack, target, attack->delay );
    }
  }
};

struct preemptive_strike_t : public demon_hunter_ranged_attack_t
{
  preemptive_strike_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_ranged_attack_t( name, p, p->talent.aldrachi_reaver.preemptive_strike->effectN( 1 ).trigger() )
  {
    background = dual = true;
  }

  // 2024-09-06 -- Preemptive Strike is very bugged and is using the following damage conversion process:
  //               weapon dps -> AP conversion without mastery -> AP coeff -> vers
  //               it also does not split AoE damage
  double calculate_direct_amount( action_state_t* state ) const override
  {
    if ( !p()->bugs )
    {
      return demon_hunter_ranged_attack_t::calculate_direct_amount( state );
    }

    double mh_wdps            = p()->main_hand_weapon.dps;
    double ap_conversion      = WEAPON_POWER_COEFFICIENT;
    double base_direct_amount = mh_wdps * ap_conversion;
    double ap_coeff           = data().effectN( 1 ).ap_coeff();
    double mult               = state->composite_da_multiplier();
    double amount             = base_direct_amount * ap_coeff * mult;

    if ( !sim->average_range )
      amount = floor( amount + rng().real() );

    if ( amount < 0 )
    {
      amount = 0;
    }

    if ( sim->debug )
    {
      sim->print_debug( "{} direct amount for {}: amount={} base={} mult={}", *p(), *this, amount, base_direct_amount,
                        mult );
    }

    if ( result_is_miss( state->result ) )
    {
      state->result_total = 0.0;
      return 0.0;
    }
    else
    {
      state->result_total = amount;
      return amount;
    }
  }
};

struct warblades_hunger_t : public demon_hunter_attack_t
{
  warblades_hunger_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_attack_t( name, p, p->hero_spec.warblades_hunger_damage )
  {
    background = dual = true;
  }
};

struct wounded_quarry_t : public demon_hunter_attack_t
{
  wounded_quarry_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_attack_t( name, p, p->hero_spec.wounded_quarry_damage )
  {
    background = dual = true;
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

  demon_hunter_buff_t( demon_hunter_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                       const item_t* item = nullptr )
    : BuffBase( &p, name, s, item )
  {
  }
  demon_hunter_buff_t( demon_hunter_td_t& td, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                       const item_t* item = nullptr )
    : BuffBase( td, name, s, item )
  {
  }

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
  struct immolation_aura_functional_buff_t : public demon_hunter_buff_t<buff_t>
  {
  private:
    using state_t = actions::spells::immolation_aura_state_t;

  public:
    double ragefire_accumulator;
    unsigned int ragefire_crit_accumulator;
    unsigned int growing_inferno_ticks;
    unsigned int growing_inferno_max_ticks;
    double growing_inferno_multiplier;

    immolation_aura_functional_buff_t( demon_hunter_t* p, std::string_view name )
      : base_t( *p, name, p->spell.immolation_aura ),
        ragefire_accumulator( 0.0 ),
        ragefire_crit_accumulator( 0 ),
        growing_inferno_ticks( 0 ),
        growing_inferno_max_ticks( 0 ),
        growing_inferno_multiplier( p->talent.havoc.growing_inferno->effectN( 1 ).percent() )
    {
      set_cooldown( timespan_t::zero() );
      apply_affecting_aura( p->spec.immolation_aura_3 );
      apply_affecting_aura( p->talent.vengeance.agonizing_flames );
      set_partial_tick( true );
      set_quiet( true );

      if ( p->talent.havoc.growing_inferno->ok() )
        growing_inferno_max_ticks = (int)( 10 / p->talent.havoc.growing_inferno->effectN( 1 ).percent() );

      set_tick_callback( [ this, p ]( buff_t*, int, timespan_t ) {
        ragefire_crit_accumulator = 0;

        state_t* s = static_cast<state_t*>( p->active.immolation_aura->get_state() );

        s->target                     = p->target;
        s->growing_inferno_multiplier = 1 + growing_inferno_ticks * growing_inferno_multiplier;
        s->immolation_aura            = this;

        p->active.immolation_aura->snapshot_state( s, p->active.immolation_aura->amount_type( s ) );
        p->active.immolation_aura->schedule_execute( s );

        if ( growing_inferno_ticks < growing_inferno_max_ticks )
          growing_inferno_ticks++;
      } );
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      demon_hunter_buff_t<buff_t>::start( stacks, value, duration );

      ragefire_accumulator      = 0.0;
      ragefire_crit_accumulator = 0;
      growing_inferno_ticks     = 1;

      if ( p()->active.immolation_aura_initial && p()->sim->current_time() > 0_s )
      {
        state_t* s = static_cast<state_t*>( p()->active.immolation_aura_initial->get_state() );

        s->target                     = p()->target;
        s->growing_inferno_multiplier = 1 + growing_inferno_ticks * growing_inferno_multiplier;
        s->immolation_aura            = this;

        p()->active.immolation_aura_initial->snapshot_state( s, p()->active.immolation_aura_initial->amount_type( s ) );
        p()->active.immolation_aura_initial->schedule_execute( s );
      }

      growing_inferno_ticks++;

      if ( p()->talent.havoc.unbound_chaos->ok() )
      {
        p()->buff.unbound_chaos->trigger();
        if ( p()->talent.havoc.inertia->ok() )
        {
          p()->buff.inertia_trigger->trigger();
        }
      }
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      demon_hunter_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

      if ( p()->talent.havoc.ragefire->ok() )
      {
        p()->active.ragefire->execute_on_target( p()->target, ragefire_accumulator );
        ragefire_accumulator      = 0;
        ragefire_crit_accumulator = 0;
      }
    }

    bool trigger( int stacks, double value, double chance, timespan_t duration ) override
    {
      // IA triggering multiple times fully resets the buff and triggers the instant damage again
      this->expire();
      return demon_hunter_buff_t<buff_t>::trigger( stacks, value, chance, duration );
    }
  };

  std::vector<immolation_aura_functional_buff_t*> immos;
  immolation_aura_buff_t( demon_hunter_t* p ) : base_t( *p, "immolation_aura", p->spell.immolation_aura ), immos()
  {
    set_cooldown( timespan_t::zero() );
    apply_affecting_aura( p->spec.immolation_aura_3 );
    apply_affecting_aura( p->talent.vengeance.agonizing_flames );
    set_tick_behavior( buff_tick_behavior::NONE );
    buff_period = 0_ms;

    set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS );

    if ( p->talent.vengeance.agonizing_flames->ok() )
    {
      add_invalidate( CACHE_RUN_SPEED );
    }

    if ( p->talent.demon_hunter.infernal_armor->ok() )
    {
      add_invalidate( CACHE_ARMOR );
    }

    if ( p->talent.havoc.a_fire_inside )
    {
      set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
      set_max_stack( 5 );
    }
    else
    {
      set_max_stack( 1 );
    }

    for ( int i = 0; i < max_stack(); i++ )
    {
      immos.push_back( new immolation_aura_functional_buff_t( p, fmt::format( "immolation_aura{}", i + 1 ) ) );
    }
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    // bool b = base_t::execute( stacks, value, chance, duration );
    int s = 0;
    if ( max_stack() == 1 )
    {
      immos[ 0 ]->trigger( 1, value, 1.0, duration );
      s++;
    }
    else
    {
      timespan_t min_remains                      = timespan_t::max();
      immolation_aura_functional_buff_t* min_immo = nullptr;

      for ( auto* immo : immos )
      {
        if ( immo->check() )
        {
          if ( immo->remains() < min_remains )
          {
            min_remains = immo->remains();
            min_immo    = immo;
          }
        }
        else
        {
          immo->trigger( 1, value, 1.0, duration );
          s++;
          if ( s >= stacks )
            break;
        }
      }

      if ( s < stacks )
      {
        min_immo->trigger( 1, value, 1.0, duration );
        s++;

        while ( s < stacks )
        {
          // Incredibly inefficient code to handle the absolute extreme of edge cases where soem
          min_remains = timespan_t::max();
          min_immo    = nullptr;

          for ( auto* immo : immos )
          {
            if ( immo->check() )
            {
              if ( immo->remains() < min_remains )
              {
                min_remains = immo->remains();
                min_immo    = immo;
              }
            }
          }
          min_immo->trigger( 1, value, 1.0, duration );
          s++;
        }
      }
    }

    if ( s > 0 )
      base_t::execute( s, value, duration );
  }
};

// Metamorphosis Buff =======================================================

struct metamorphosis_buff_t : public demon_hunter_buff_t<buff_t>
{
  metamorphosis_buff_t( demon_hunter_t* p ) : base_t( *p, "metamorphosis", p->spec.metamorphosis_buff )
  {
    set_cooldown( timespan_t::zero() );
    buff_period   = timespan_t::zero();
    tick_behavior = buff_tick_behavior::NONE;

    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      add_invalidate( CACHE_HASTE );
      add_invalidate( CACHE_LEECH );
    }
    else  // DEMON_HUNTER_VENGEANCE
    {
      set_default_value_from_effect_type( A_MOD_INCREASE_HEALTH_PERCENT );
      add_invalidate( CACHE_ARMOR );
    }

    if ( p->talent.demon_hunter.soul_rending->ok() )
    {
      add_invalidate( CACHE_LEECH );
    }

    if ( p->talent.felscarred.monster_rising->ok() )
    {
      add_invalidate( CACHE_AGILITY );
    }
  }

  void trigger_demonic()
  {
    if ( !p()->buff.metamorphosis->up() )
    {
      if ( p()->specialization() == DEMON_HUNTER_HAVOC )
      {
        p()->buff.demonsurge_abilities[ demonsurge_ability::ANNIHILATION ]->trigger();
        p()->buff.demonsurge_abilities[ demonsurge_ability::DEATH_SWEEP ]->trigger();
      }
      else
      {
        p()->buff.demonsurge_abilities[ demonsurge_ability::SOUL_SUNDER ]->trigger();
        p()->buff.demonsurge_abilities[ demonsurge_ability::SPIRIT_BURST ]->trigger();
      }
      p()->buff.demonsurge_demonic->trigger();
    }

    const timespan_t extend_duration = p()->talent.demon_hunter.demonic->effectN( 1 ).time_value();
    p()->buff.metamorphosis->extend_duration_or_trigger( extend_duration );
    p()->buff.inner_demon->trigger();
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    demon_hunter_buff_t<buff_t>::start( stacks, value, duration );

    if ( p()->specialization() == DEMON_HUNTER_VENGEANCE )
    {
      p()->metamorphosis_health = p()->max_health() * value;
      p()->stat_gain( STAT_MAX_HEALTH, p()->metamorphosis_health, (gain_t*)nullptr, (action_t*)nullptr, true );
    }

    if ( p()->talent.felscarred.monster_rising->ok() )
    {
      p()->buff.monster_rising->expire();
    }
    if ( p()->talent.felscarred.enduring_torment->ok() )
    {
      p()->buff.enduring_torment->expire();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    if ( p()->specialization() == DEMON_HUNTER_VENGEANCE )
    {
      p()->stat_loss( STAT_MAX_HEALTH, p()->metamorphosis_health, (gain_t*)nullptr, (action_t*)nullptr, true );
      p()->metamorphosis_health = 0;
    }

    if ( p()->talent.havoc.restless_hunter->ok() )
    {
      p()->cooldown.fel_rush->reset( false, 1 );
      p()->buff.restless_hunter->trigger();
    }

    if ( p()->talent.felscarred.monster_rising->ok() )
    {
      p()->buff.monster_rising->trigger();
    }
    if ( p()->talent.felscarred.enduring_torment->ok() )
    {
      p()->buff.enduring_torment->trigger();
    }

    auto demonsurge_spec_abilities =
        p()->specialization() == DEMON_HUNTER_HAVOC ? demonsurge_havoc_abilities : demonsurge_vengeance_abilities;
    for ( demonsurge_ability ability : demonsurge_spec_abilities )
    {
      p()->buff.demonsurge_abilities[ ability ]->expire();
    }
    p()->buff.demonsurge_demonic->expire();
    p()->buff.demonsurge_hardcast->expire();
  }
};

// Demon Spikes buff ========================================================

struct demon_spikes_t : public demon_hunter_buff_t<buff_t>
{
  const timespan_t max_duration;

  demon_spikes_t( demon_hunter_t* p )
    : base_t( *p, "demon_spikes", p->spec.demon_spikes_buff ),
      max_duration( base_buff_duration * 3 )  // Demon Spikes can only be extended to 3x its base duration
  {
    set_default_value_from_effect_type( A_MOD_PARRY_PERCENT );
    set_refresh_behavior( buff_refresh_behavior::EXTEND );
    apply_affecting_aura( p->talent.vengeance.extended_spikes );
    add_invalidate( CACHE_PARRY );
    add_invalidate( CACHE_ARMOR );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( duration == timespan_t::min() )
    {
      duration = buff_duration();
    }

    if ( remains() + buff_duration() > max_duration )
    {
      duration = max_duration - remains();
    }

    if ( duration <= timespan_t::zero() )
    {
      return false;
    }

    return demon_hunter_buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    if ( p()->talent.vengeance.calcified_spikes->ok() )
    {
      p()->buff.calcified_spikes->trigger();
    }
  }
};

struct calcified_spikes_t : public demon_hunter_buff_t<buff_t>
{
  calcified_spikes_t( demon_hunter_t* p ) : base_t( *p, "calcified_spikes", p->spec.calcified_spikes_buff )
  {
    auto max_stacks    = std::max( 1, as<int>( data().duration() / 1_s ) );
    auto default_value = data().effectN( 1 ).percent() / max_stacks;

    set_period( 1_s );
    set_reverse( true );
    set_max_stack( max_stacks );
    set_initial_stack( max_stacks );
    set_default_value( default_value );
  }
};

struct fel_barrage_buff_t : public demon_hunter_buff_t<buff_t>
{
  fel_barrage_buff_t( demon_hunter_t* p ) : base_t( *p, "fel_barrage", p->talent.havoc.fel_barrage )
  {
    set_cooldown( timespan_t::zero() );

    set_tick_callback( [ this, p ]( buff_t*, int, timespan_t ) {
      auto cost = p->active.fel_barrage->cost();
      if ( !p->resource_available( RESOURCE_FURY, cost ) )
      {
        // Separate the expiration event to happen immediately after tick processing
        make_event( *sim, 0_ms, [ this ]() { this->expire(); } );
        return;
      }
      p->active.fel_barrage->execute_on_target( p->target );
    } );
  }
};

}  // end namespace buffs

// Namespace Actions post buffs
namespace actions
{
namespace spells
{
void immolation_aura_t::immolation_aura_damage_t::accumulate_ragefire( immolation_aura_state_t* s )
{
  if ( !p()->talent.havoc.ragefire->ok() )
    return;

  assert( s->immolation_aura && "Immolation Aura state should contain the parent buff while executing damage." );

  if ( !( s->result_amount > 0 && s->result == RESULT_CRIT ) )
    return;

  buffs::immolation_aura_buff_t::immolation_aura_functional_buff_t* immo =
      static_cast<buffs::immolation_aura_buff_t::immolation_aura_functional_buff_t*>( s->immolation_aura );

  if ( immo->ragefire_crit_accumulator >= p()->talent.havoc.ragefire->effectN( 2 ).base_value() )
    return;

  const double multiplier = p()->talent.havoc.ragefire->effectN( 1 ).percent();
  immo->ragefire_accumulator += s->result_amount * multiplier;
  immo->ragefire_crit_accumulator++;
}
}  // namespace spells
}  // namespace actions

// ==========================================================================
// Misc. Events and Structs
// ==========================================================================

// Frailty event ========================================================

struct frailty_event_t : public event_t
{
  demon_hunter_t* dh;

  frailty_event_t( demon_hunter_t* p, bool initial = false ) : event_t( *p ), dh( p )
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
    assert( dh->frailty_accumulator >= 0.0 );

    if ( dh->frailty_accumulator > 0 )
    {
      action_t* a    = dh->active.frailty_heal;
      a->base_dd_min = a->base_dd_max = dh->frailty_accumulator;
      a->execute();

      dh->frailty_accumulator = 0.0;
    }

    dh->frailty_driver = make_event<frailty_event_t>( sim(), dh );
  }
};

movement_buff_t::movement_buff_t( demon_hunter_t* p, util::string_view name, const spell_data_t* spell_data,
                                  const item_t* item )
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
    debuffs.essence_break = make_buff( *this, "essence_break", p.spec.essence_break_debuff )
                                ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS )
                                ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                ->set_cooldown( timespan_t::zero() );

    debuffs.initiative_tracker =
        make_buff( *this, "initiative_tracker", p.talent.havoc.initiative )->set_duration( timespan_t::min() );

    debuffs.burning_wound = make_buff( *this, "burning_wound", p.spec.burning_wound_debuff )
                                ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS )
                                ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    dots.burning_wound = target->get_dot( "burning_wound", &p );
  }
  else  // DEMON_HUNTER_VENGEANCE
  {
    dots.fiery_brand = target->get_dot( "fiery_brand", &p );
    debuffs.frailty  = make_buff( *this, "frailty", p.spec.frailty_debuff )
                          ->set_default_value_from_effect( 1 )
                          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                          ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                          ->set_period( 0_ms )
                          ->apply_affecting_aura( p.talent.vengeance.soulcrush );
  }

  // TODO: make this conditional on hero spec
  debuffs.reavers_mark = make_buff( *this, "reavers_mark", p.hero_spec.reavers_mark )
                             ->set_default_value_from_effect( 1 )
                             ->set_max_stack( 2 )
                             ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                             ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  dots.sigil_of_flame = target->get_dot( "sigil_of_flame", &p );
  dots.sigil_of_doom  = target->get_dot( "sigil_of_doom", &p );
  dots.the_hunt       = target->get_dot( "the_hunt_dot", &p );

  debuffs.sigil_of_flame =
      make_buff( *this, "sigil_of_flame", p.spell.sigil_of_flame_damage )
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->set_stack_behavior( p.talent.vengeance.ascending_flame->ok() ? buff_stack_behavior::ASYNCHRONOUS
                                                                         : buff_stack_behavior::DEFAULT )
          ->apply_affecting_aura( p.talent.vengeance.ascending_flame )
          ->apply_affecting_aura( p.talent.vengeance.chains_of_anger );
  debuffs.sigil_of_doom =
      make_buff( *this, "sigil_of_doom", p.hero_spec.sigil_of_doom_damage )
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->set_stack_behavior( p.talent.vengeance.ascending_flame->ok() ? buff_stack_behavior::ASYNCHRONOUS
                                                                         : buff_stack_behavior::DEFAULT )
          ->apply_affecting_aura( p.talent.vengeance.ascending_flame )
          ->apply_affecting_aura( p.talent.vengeance.chains_of_anger );

  debuffs.serrated_glaive =
      make_buff( *this, "serrated_glaive", p.talent.havoc.serrated_glaive->effectN( 1 ).trigger() )
          ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
}

// ==========================================================================
// Demon Hunter Definitions
// ==========================================================================

demon_hunter_t::demon_hunter_t( sim_t* sim, util::string_view name, race_e r )
  : parse_player_effects_t( sim, DEMON_HUNTER, name, r ),
    melee_main_hand( nullptr ),
    melee_off_hand( nullptr ),
    next_fragment_spawn( 0 ),
    soul_fragments(),
    frailty_accumulator( 0.0 ),
    frailty_driver( nullptr ),
    shattered_destiny_accumulator( 0.0 ),
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

  if ( name == "soul_barrier" )
    return new soul_barrier_t( this, options_str );

  using namespace actions::spells;

  if ( name == "blur" )
    return new blur_t( this, options_str );
  if ( name == "bulk_extraction" )
    return new bulk_extraction_t( this, options_str );
  if ( name == "chaos_nova" )
    return new chaos_nova_t( this, options_str );
  if ( name == "consume_magic" )
    return new consume_magic_t( this, options_str );
  if ( name == "demon_spikes" )
    return new demon_spikes_t( this, options_str );
  if ( name == "disrupt" )
    return new disrupt_t( this, options_str );
  if ( name == "eye_beam" )
    return new eye_beam_t( this, options_str );
  if ( name == "abyssal_gaze" )
    return new abyssal_gaze_t( this, options_str );
  if ( name == "fel_barrage" )
    return new fel_barrage_t( this, options_str );
  if ( name == "fel_eruption" )
    return new fel_eruption_t( this, options_str );
  if ( name == "fel_devastation" )
    return new fel_devastation_t( this, options_str );
  if ( name == "fel_desolation" )
    return new fel_desolation_t( this, options_str );
  if ( name == "fiery_brand" )
    return new fiery_brand_t( "fiery_brand", this, options_str );
  if ( name == "glaive_tempest" )
    return new glaive_tempest_t( this, options_str );
  if ( name == "infernal_strike" )
    return new infernal_strike_t( this, options_str );
  if ( name == "immolation_aura" )
    return new immolation_aura_t( this, options_str );
  if ( name == "metamorphosis" )
    return new metamorphosis_t( this, options_str );
  if ( name == "pick_up_fragment" )
    return new pick_up_fragment_t( this, options_str );
  if ( name == "sigil_of_flame" )
    return new sigil_of_flame_t( this, options_str );
  if ( name == "sigil_of_doom" )
    return new sigil_of_doom_t( this, options_str );
  if ( name == "spirit_bomb" )
    return new spirit_bomb_t( this, options_str );
  if ( name == "spirit_burst" )
    return new spirit_burst_t( this, options_str );
  if ( name == "sigil_of_spite" )
    return new sigil_of_spite_t( this, options_str );
  if ( name == "the_hunt" )
    return new the_hunt_t( this, options_str );
  if ( name == "sigil_of_misery" )
    return new sigil_of_misery_t( this, options_str );
  if ( name == "sigil_of_silence" )
    return new sigil_of_silence_t( this, options_str );
  if ( name == "sigil_of_chains" )
    return new sigil_of_chains_t( this, options_str );

  using namespace actions::attacks;

  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "annihilation" )
    return new annihilation_t( "annihilation", this, options_str );
  if ( name == "blade_dance" )
    return new blade_dance_t( this, options_str );
  if ( name == "chaos_strike" )
    return new chaos_strike_t( "chaos_strike", this, options_str );
  if ( name == "essence_break" )
    return new essence_break_t( this, options_str );
  if ( name == "death_sweep" )
    return new death_sweep_t( this, options_str );
  if ( name == "demons_bite" )
    return new demons_bite_t( this, options_str );
  if ( name == "felblade" )
    return new felblade_t( this, options_str );
  if ( name == "fel_rush" )
    return new fel_rush_t( this, options_str );
  if ( name == "fracture" )
    return new fracture_t( this, options_str );
  if ( name == "shear" )
    return new shear_t( this, options_str );
  if ( name == "soul_cleave" )
    return new soul_cleave_t( this, options_str );
  if ( name == "soul_sunder" )
    return new soul_sunder_t( this, options_str );
  if ( name == "throw_glaive" )
    return new throw_glaive_t( "throw_glaive", this, options_str );
  if ( name == "vengeful_retreat" )
    return new vengeful_retreat_t( this, options_str );
  if ( name == "soul_carver" )
    return new soul_carver_t( this, options_str );
  if ( name == "reavers_glaive" )
    return new reavers_glaive_t( this, options_str );

  return base_t::create_action( name, options_str );
}

// demon_hunter_t::create_buffs =============================================

void demon_hunter_t::create_buffs()
{
  base_t::create_buffs();

  using namespace buffs;

  // General ================================================================

  buff.demon_soul           = make_buff( this, "demon_soul", spell.demon_soul );
  buff.empowered_demon_soul = make_buff( this, "empowered_demon_soul", spell.demon_soul_empowered );
  buff.immolation_aura      = make_buff<buffs::immolation_aura_buff_t>( this );
  buff.metamorphosis        = make_buff<buffs::metamorphosis_buff_t>( this );

  // Havoc ==================================================================

  buff.out_of_range = make_buff( this, "out_of_range", spell_data_t::nil() )->set_chance( 1.0 );

  buff.fel_rush_move = new movement_buff_t( this, "fel_rush_movement", spell_data_t::nil() );
  buff.fel_rush_move->set_chance( 1.0 )->set_duration( spec.fel_rush->gcd() );

  buff.vengeful_retreat_move = new movement_buff_t( this, "vengeful_retreat_movement", spell_data_t::nil() );
  buff.vengeful_retreat_move->set_chance( 1.0 )->set_duration( talent.demon_hunter.vengeful_retreat->duration() );

  buff.metamorphosis_move = new movement_buff_t( this, "metamorphosis_movement", spell_data_t::nil() );
  buff.metamorphosis_move->set_chance( 1.0 )->set_duration( 1_s );

  buff.blind_fury = make_buff( this, "blind_fury", talent.havoc.eye_beam )
                        ->set_default_value( talent.havoc.blind_fury->effectN( 3 ).resource( RESOURCE_FURY ) / 50 )
                        ->set_cooldown( timespan_t::zero() )
                        ->set_period( timespan_t::from_millis( 100 ) )  // Fake natural regeneration rate
                        ->set_tick_on_application( false )
                        ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
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

  buff.momentum = make_buff( this, "momentum", spec.momentum_buff );
  buff.momentum->set_refresh_duration_callback( []( const buff_t* b, timespan_t d ) {
    return std::min( b->remains() + d, 30_s );  // Capped to 30 seconds
  } );

  buff.inertia = make_buff( this, "inertia", spec.inertia_buff );
  buff.inertia->set_refresh_duration_callback( []( const buff_t* b, timespan_t d ) {
    return std::min( b->remains() + d, 10_s );  // Capped to 10 seconds
  } );
  buff.inertia_trigger = make_buff( this, "inertia_trigger", spell_data_t::nil() )->set_quiet( true );

  buff.inner_demon = make_buff( this, "inner_demon", spec.inner_demon_buff );

  buff.restless_hunter = make_buff( this, "restless_hunter", spec.restless_hunter_buff );

  buff.tactical_retreat = make_buff( this, "tactical_retreat", spec.tactical_retreat_buff )
                              ->set_default_value_from_effect_type( A_PERIODIC_ENERGIZE )
                              ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
                                resource_gain( RESOURCE_FURY, b->check_value(), gain.tactical_retreat );
                              } );

  buff.unbound_chaos = make_buff( this, "unbound_chaos", spec.unbound_chaos_buff )
                           ->set_default_value( talent.havoc.unbound_chaos->effectN( 2 ).percent() );

  buff.chaos_theory = make_buff( this, "chaos_theory", spec.chaos_theory_buff );

  buff.fel_barrage = new buffs::fel_barrage_buff_t( this );

  // Vengeance ==============================================================

  buff.demon_spikes = new buffs::demon_spikes_t( this );

  buff.calcified_spikes = new buffs::calcified_spikes_t( this );

  buff.painbringer = make_buff( this, "painbringer", spec.painbringer_buff )
                         ->set_default_value( talent.vengeance.painbringer->effectN( 1 ).percent() )
                         ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                         ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                         ->set_period( 0_ms );

  buff.soul_furnace_damage_amp =
      make_buff( this, "soul_furnace_damage_amp", spec.soul_furnace_damage_amp )->set_default_value_from_effect( 1 );
  buff.soul_furnace_stack = make_buff( this, "soul_furnace_stack", spec.soul_furnace_stack );

  buff.soul_fragments = make_buff( this, "soul_fragments", spec.soul_fragments_buff )->set_max_stack( 10 );

  buff.soul_barrier = make_buff<absorb_buff_t>( this, "soul_barrier", talent.vengeance.soul_barrier );
  buff.soul_barrier->set_absorb_source( get_stats( "soul_barrier" ) )
      ->set_absorb_gain( get_gain( "soul_barrier" ) )
      ->set_absorb_high_priority( true )  // TOCHECK
      ->set_cooldown( timespan_t::zero() );

  // Aldrachi Reaver ========================================================

  buff.reavers_glaive = make_buff( this, "reavers_glaive", hero_spec.reavers_glaive_buff )->set_quiet( true );
  buff.art_of_the_glaive =
      make_buff( this, "art_of_the_glaive", hero_spec.art_of_the_glaive_buff )
          ->set_default_value( specialization() == DEMON_HUNTER_HAVOC
                                   ? talent.aldrachi_reaver.art_of_the_glaive->effectN( 1 ).base_value()
                                   : talent.aldrachi_reaver.art_of_the_glaive->effectN( 2 ).base_value() )
          ->set_stack_change_callback( [ this ]( buff_t* b, int old, int new_ ) {
            // applying Reaver's Glaive only occurs upon gaining new stack on Art of the Glaive
            if ( new_ > old )
            {
              int target_stacks = static_cast<int>( b->default_value );
              if ( new_ >= target_stacks && !buff.reavers_glaive->check() &&
                   cooldown.art_of_the_glaive_consumption_icd->up() )
              {
                // use a cooldown to prevent multiple consumptions
                cooldown.art_of_the_glaive_consumption_icd->start( 100_ms );

                // using an event
                make_event( *sim, 0_ms, [ b, target_stacks, this ]() {
                  b->decrement( target_stacks );
                  buff.reavers_glaive->trigger();
                } );
              }
            }
          } );
  buff.glaive_flurry    = make_buff( this, "glaive_flurry", hero_spec.glaive_flurry );
  buff.rending_strike   = make_buff( this, "rending_strike", hero_spec.rending_strike );
  buff.warblades_hunger = make_buff( this, "warblades_hunger", hero_spec.warblades_hunger_buff );
  buff.thrill_of_the_fight_attack_speed =
      make_buff( this, "thrill_of_the_fight_attack_speed", hero_spec.thrill_of_the_fight_attack_speed_buff )
          ->set_default_value_from_effect_type( A_MOD_RANGED_AND_MELEE_AUTO_ATTACK_SPEED )
          ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );
  buff.thrill_of_the_fight_damage =
      make_buff( this, "thrill_of_the_fight_damage", hero_spec.thrill_of_the_fight_damage_buff );
  buff.art_of_the_glaive_first = make_buff( this, "art_of_the_glaive_first", talent.aldrachi_reaver.art_of_the_glaive )
                                     ->set_duration( buff.glaive_flurry->buff_duration() );
  buff.art_of_the_glaive_second_glaive_flurry =
      make_buff( this, "art_of_the_glaive_second_glaive_flurry", talent.aldrachi_reaver.art_of_the_glaive )
          ->set_duration( buff.glaive_flurry->buff_duration() );
  buff.art_of_the_glaive_second_rending_strike =
      make_buff( this, "art_of_the_glaive_second_rending_strike", talent.aldrachi_reaver.art_of_the_glaive )
          ->set_duration( buff.glaive_flurry->buff_duration() );

  // Fel-scarred ============================================================

  buff.enduring_torment = make_buff( this, "enduring_torment", hero_spec.enduring_torment_buff )
                              ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                              ->set_allow_precombat( true );
  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    buff.enduring_torment->set_default_value_from_effect_type( A_HASTE_ALL )->set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  }
  buff.monster_rising = make_buff( this, "monster_rising", hero_spec.monster_rising_buff )
                            ->set_default_value_from_effect_type( A_MOD_PERCENT_STAT )
                            ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
                            ->set_allow_precombat( true )
                            ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buff.pursuit_of_angryness =
      make_buff( this, "pursuit_of_angriness", talent.felscarred.pursuit_of_angriness )
          ->set_quiet( true )
          ->set_tick_zero( true )
          ->add_invalidate( CACHE_RUN_SPEED )
          ->set_tick_callback(
              [ this, speed_per_fury = talent.felscarred.pursuit_of_angriness->effectN( 1 ).percent() /
                                       talent.felscarred.pursuit_of_angriness->effectN( 1 ).base_value() ](
                  buff_t* b, int, timespan_t ) {
                // TOCHECK - Does this need to floor if it's not a whole number
                b->current_value = resources.current[ RESOURCE_FURY ] * speed_per_fury;
              } );
  buff.student_of_suffering =
      make_buff( this, "student_of_suffering", hero_spec.student_of_suffering_buff )
          ->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
          ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
          ->set_tick_behavior( buff_tick_behavior::REFRESH )
          ->set_tick_on_application( false )
          ->set_period( 2_s )
          ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
            resource_gain( RESOURCE_FURY, b->data().effectN( 2 ).trigger()->effectN( 1 ).base_value(),
                           gain.student_of_suffering );
          } );

  auto demonsurge_spec_abilities =
      specialization() == DEMON_HUNTER_HAVOC ? demonsurge_havoc_abilities : demonsurge_vengeance_abilities;
  for ( demonsurge_ability ability : demonsurge_spec_abilities )
  {
    buff.demonsurge_abilities[ ability ] = make_buff( this, demonsurge_ability_name( ability ), spell_data_t::nil() );
  }

  buff.demonsurge_demonic  = make_buff( this, "demonsurge_demonic", hero_spec.demonsurge_demonic_buff );
  buff.demonsurge_hardcast = make_buff( this, "demonsurge_hardcast", hero_spec.demonsurge_hardcast_buff );
  buff.demonsurge          = make_buff( this, "demonsurge", hero_spec.demonsurge_stacking_buff );

  // Set Bonus Items ========================================================

  buff.tww1_havoc_4pc =
      make_buff( this, "blade_rhapsody",
                 set_bonuses.tww1_havoc_4pc->ok() ? set_bonuses.tww1_havoc_4pc_buff : spell_data_t::not_found() )
          ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER );

  buff.tww1_vengeance_4pc = make_buff( this, "soulfuse",
                                       set_bonuses.tww1_vengeance_4pc->ok() ? set_bonuses.tww1_vengeance_4pc_buff
                                                                            : spell_data_t::not_found() )
                                ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC );
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
    cooldown_multiplier         = 1.0 / ( 1.0 + reduction_per_second );
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
  {
  }

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
  auto splits = util::string_split( name_str, "." );

  if ( ( splits.size() == 1 || splits.size() == 2 ) &&
       ( util::str_compare_ci( splits[ 0 ], "soul_fragments" ) ||
         util::str_compare_ci( splits[ 0 ], "greater_soul_fragments" ) ||
         util::str_compare_ci( splits[ 0 ], "lesser_soul_fragments" ) ||
         util::str_compare_ci( splits[ 0 ], "demon_soul_fragments" ) ) )
  {
    enum class soul_fragment_filter
    {
      ACTIVE,
      INACTIVE,
      TOTAL
    };

    struct soul_fragments_expr_t : public expr_t
    {
      demon_hunter_t* dh;
      soul_fragment type;
      soul_fragment_filter filter;

      soul_fragments_expr_t( demon_hunter_t* p, util::string_view n, soul_fragment t, soul_fragment_filter f )
        : expr_t( n ), dh( p ), type( t ), filter( f )
      {
      }

      double evaluate() override
      {
        switch ( filter )
        {
          case soul_fragment_filter::ACTIVE:
            return dh->get_active_soul_fragments( type );
          case soul_fragment_filter::INACTIVE:
            return dh->get_inactive_soul_fragments( type );
          case soul_fragment_filter::TOTAL:
            return dh->get_total_soul_fragments( type );
          default:
            return 0;
        }
      }
    };

    soul_fragment type = soul_fragment::LESSER;

    if ( util::str_compare_ci( splits[ 0 ], "soul_fragments" ) )
    {
      type = soul_fragment::ANY;
    }
    else if ( util::str_compare_ci( splits[ 0 ], "greater_soul_fragments" ) )
    {
      type = soul_fragment::ANY_GREATER;
    }
    else if ( util::str_compare_ci( splits[ 0 ], "demon_soul_fragments" ) )
    {
      type = soul_fragment::ANY_DEMON;
    }

    soul_fragment_filter filter = soul_fragment_filter::ACTIVE;

    if ( splits.size() == 2 )
    {
      if ( util::str_compare_ci( splits[ 1 ], "inactive" ) )
      {
        filter = soul_fragment_filter::INACTIVE;
      }
      else if ( util::str_compare_ci( splits[ 1 ], "total" ) )
      {
        filter = soul_fragment_filter::TOTAL;
      }
      else if ( !util::str_compare_ci( splits[ 1 ], "active" ) )
      {
        throw std::invalid_argument( fmt::format( "Unsupported soul_fragments filter '{}'.", splits[ 1 ] ) );
      }
    }

    return std::make_unique<soul_fragments_expr_t>( this, name_str, type, filter );
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
  add_option(
      opt_float( "soul_fragment_movement_consume_chance", options.soul_fragment_movement_consume_chance, 0, 1 ) );
  add_option( opt_float( "wounded_quarry_chance_vengeance", options.wounded_quarry_chance_vengeance, 0, 1 ) );
  add_option( opt_float( "wounded_quarry_chance_havoc", options.wounded_quarry_chance_havoc, 0, 1 ) );
}

// demon_hunter_t::create_pet ===============================================

pet_t* demon_hunter_t::create_pet( util::string_view pet_name, util::string_view /* pet_type */ )
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
  if ( main_hand_weapon.type == WEAPON_NONE || off_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
    {
      sim->errorf( "Player %s does not have a valid main-hand and off-hand weapon.", name() );
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
  resources.base[ RESOURCE_FURY ] += talent.felscarred.untethered_fury->effectN( 1 ).base_value();

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
  proc.delayed_aa_range               = get_proc( "delayed_aa_out_of_range" );
  proc.soul_fragment_greater          = get_proc( "soul_fragment_greater" );
  proc.soul_fragment_greater_demon    = get_proc( "soul_fragment_greater_demon" );
  proc.soul_fragment_empowered_demon  = get_proc( "soul_fragment_empowered_demon" );
  proc.soul_fragment_lesser           = get_proc( "soul_fragment_lesser" );
  proc.felblade_reset                 = get_proc( "felblade_reset" );
  proc.soul_fragment_from_soul_sigils = get_proc( "soul_fragment_from_soul_sigils" );

  // Havoc
  proc.demonic_appetite                = get_proc( "demonic_appetite" );
  proc.demons_bite_in_meta             = get_proc( "demons_bite_in_meta" );
  proc.chaos_strike_in_essence_break   = get_proc( "chaos_strike_in_essence_break" );
  proc.annihilation_in_essence_break   = get_proc( "annihilation_in_essence_break" );
  proc.blade_dance_in_essence_break    = get_proc( "blade_dance_in_essence_break" );
  proc.death_sweep_in_essence_break    = get_proc( "death_sweep_in_essence_break" );
  proc.chaos_strike_in_serrated_glaive = get_proc( "chaos_strike_in_serrated_glaive" );
  proc.annihilation_in_serrated_glaive = get_proc( "annihilation_in_serrated_glaive" );
  proc.throw_glaive_in_serrated_glaive = get_proc( "throw_glaive_in_serrated_glaive" );
  proc.shattered_destiny               = get_proc( "shattered_destiny" );
  proc.eye_beam_canceled               = get_proc( "eye_beam_canceled" );

  // Vengeance
  proc.soul_fragment_expire               = get_proc( "soul_fragment_expire" );
  proc.soul_fragment_overflow             = get_proc( "soul_fragment_overflow" );
  proc.soul_fragment_from_shear           = get_proc( "soul_fragment_from_shear" );
  proc.soul_fragment_from_fracture        = get_proc( "soul_fragment_from_fracture" );
  proc.soul_fragment_from_sigil_of_spite  = get_proc( "soul_fragment_from_sigil_of_spite" );
  proc.soul_fragment_from_fallout         = get_proc( "soul_fragment_from_fallout" );
  proc.soul_fragment_from_meta            = get_proc( "soul_fragment_from_meta" );
  proc.soul_fragment_from_bulk_extraction = get_proc( "soul_fragment_from_bulk_extraction" );

  // Aldrachi Reaver
  proc.soul_fragment_from_aldrachi_tactics = get_proc( "soul_fragment_from_aldrachi_tactics" );
  proc.soul_fragment_from_wounded_quarry   = get_proc( "soul_fragment_from_wounded_quarry" );

  // Fel-scarred

  // Set Bonuses
  proc.soul_fragment_from_vengeance_twws1_2pc = get_proc( "soul_fragment_from_vengeance_twws1_2pc" );
}

// demon_hunter_t::init_uptimes =============================================

void demon_hunter_t::init_uptimes()
{
  base_t::init_uptimes();
}

// demon_hunter_t::init_resources ===========================================

void demon_hunter_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.current[ RESOURCE_FURY ] = options.initial_fury;
  expected_max_health                = calculate_expected_max_health();
}

// demon_hunter_t::init_special_effects =====================================

void demon_hunter_t::init_special_effects()
{
  base_t::init_special_effects();
}

// demon_hunter_t::init_rng =================================================

void demon_hunter_t::init_rng()
{
  // RPPM objects

  // General
  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    rppm.felblade         = get_rppm( "felblade", spell.felblade_reset_havoc );
    rppm.demonic_appetite = get_rppm( "demonic_appetite", spec.demonic_appetite );
  }
  else  // DEMON_HUNTER_VENGEANCE
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
  spell.all_demon_hunter       = dbc::get_class_passive( *this, SPEC_NONE );
  spell.chaos_brand            = find_spell( 1490 );
  spell.critical_strikes       = find_spell( 221351 );
  spell.leather_specialization = find_specialization_spell( "Leather Specialization" );

  spell.demon_soul           = find_spell( 163073 );
  spell.demon_soul_empowered = find_spell( 347765 );
  spell.soul_fragment        = find_spell( 204255 );

  // Shared Abilities
  spell.disrupt           = find_class_spell( "Disrupt" );
  spell.immolation_aura   = find_class_spell( "Immolation Aura" );
  spell.immolation_aura_2 = find_rank_spell( "Immolation Aura", "Rank 2" );
  spell.spectral_sight    = find_class_spell( "Spectral Sight" );

  // Spec-Overriden Passives
  spec.demonic_wards       = find_specialization_spell( "Demonic Wards" );
  spec.demonic_wards_2     = find_rank_spell( "Demonic Wards", "Rank 2" );
  spec.demonic_wards_3     = find_rank_spell( "Demonic Wards", "Rank 3" );
  spec.immolation_aura_cdr = find_spell( 320378, DEMON_HUNTER_VENGEANCE );
  spec.thick_skin          = find_specialization_spell( "Thick Skin" );

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    spell.throw_glaive        = find_class_spell( "Throw Glaive" );
    spec.consume_soul_greater = find_spell( 178963 );
    spec.consume_soul_lesser  = spec.consume_soul_greater;
    spec.metamorphosis        = find_class_spell( "Metamorphosis" );
    spec.metamorphosis_buff   = spec.metamorphosis->effectN( 2 ).trigger();
  }
  else
  {
    spell.throw_glaive        = find_specialization_spell( "Throw Glaive" );
    spec.consume_soul_greater = find_spell( 210042 );
    spec.consume_soul_lesser  = find_spell( 203794 );
    spec.metamorphosis        = find_specialization_spell( "Metamorphosis" );
    spec.metamorphosis_buff   = spec.metamorphosis;
  }

  // Havoc Spells
  spec.havoc_demon_hunter = find_specialization_spell( "Havoc Demon Hunter" );

  spec.annihilation          = find_spell( 201427, DEMON_HUNTER_HAVOC );
  spec.blade_dance           = find_specialization_spell( "Blade Dance" );
  spec.blade_dance_2         = find_rank_spell( "Blade Dance", "Rank 2" );
  spec.blur                  = find_specialization_spell( "Blur" );
  spec.chaos_strike          = find_specialization_spell( "Chaos Strike" );
  spec.chaos_strike_fury     = find_spell( 193840, DEMON_HUNTER_HAVOC );
  spec.chaos_strike_refund   = find_spell( 197125, DEMON_HUNTER_HAVOC );
  spec.death_sweep           = find_spell( 210152, DEMON_HUNTER_HAVOC );
  spec.demons_bite           = find_spell( 162243, DEMON_HUNTER_HAVOC );
  spec.fel_rush              = find_specialization_spell( "Fel Rush" );
  spec.fel_rush_damage       = find_spell( 192611, DEMON_HUNTER_HAVOC );
  spec.immolation_aura_3     = find_rank_spell( "Immolation Aura", "Rank 3" );
  spec.fel_eruption          = find_specialization_spell( "Fel Eruption" );
  spec.demonic_appetite      = find_spell( 206478, DEMON_HUNTER_HAVOC );
  spec.demonic_appetite_fury = find_spell( 210041, DEMON_HUNTER_HAVOC );

  // Vengeance Spells
  spec.vengeance_demon_hunter = find_specialization_spell( "Vengeance Demon Hunter" );

  spec.demon_spikes        = find_specialization_spell( "Demon Spikes" );
  spec.infernal_strike     = find_specialization_spell( "Infernal Strike" );
  spec.soul_cleave         = find_specialization_spell( "Soul Cleave" );
  spec.shear               = find_specialization_spell( "Shear" );
  spec.soul_cleave_2       = find_rank_spell( "Soul Cleave", "Rank 2" );
  spec.riposte             = find_specialization_spell( "Riposte" );
  spec.soul_fragments_buff = find_spell( 203981, DEMON_HUNTER_VENGEANCE );

  // Masteries ==============================================================

  mastery.demonic_presence = find_mastery_spell( DEMON_HUNTER_HAVOC );
  mastery.fel_blood        = find_mastery_spell( DEMON_HUNTER_VENGEANCE );
  mastery.fel_blood_rank_2 = find_rank_spell( "Mastery: Fel Blood", "Rank 2" );

  // Talents ================================================================

  talent.demon_hunter.vengeful_retreat = find_talent_spell( talent_tree::CLASS, "Vengeful Retreat" );
  talent.demon_hunter.blazing_path     = find_talent_spell( talent_tree::CLASS, "Blazing Path" );
  talent.demon_hunter.sigil_of_misery  = find_talent_spell( talent_tree::CLASS, "Sigil of Misery" );

  talent.demon_hunter.unrestrained_fury     = find_talent_spell( talent_tree::CLASS, "Unrestrained Fury" );
  talent.demon_hunter.imprison              = find_talent_spell( talent_tree::CLASS, "Imprison" );
  talent.demon_hunter.shattered_restoration = find_talent_spell( talent_tree::CLASS, "Shattered Restoration" );

  talent.demon_hunter.vengeful_bonds           = find_talent_spell( talent_tree::CLASS, "Vengeful Bonds" );
  talent.demon_hunter.improved_disrupt         = find_talent_spell( talent_tree::CLASS, "Improved Disrupt" );
  talent.demon_hunter.bouncing_glaives         = find_talent_spell( talent_tree::CLASS, "Bouncing Glaives" );
  talent.demon_hunter.consume_magic            = find_talent_spell( talent_tree::CLASS, "Consume Magic" );
  talent.demon_hunter.improved_sigil_of_misery = find_talent_spell( talent_tree::CLASS, "Improved Sigil of Misery" );

  talent.demon_hunter.pursuit           = find_talent_spell( talent_tree::CLASS, "Pursuit" );
  talent.demon_hunter.disrupting_fury   = find_talent_spell( talent_tree::CLASS, "Disrupting Fury" );
  talent.demon_hunter.felblade          = find_talent_spell( talent_tree::CLASS, "Felblade" );
  talent.demon_hunter.swallowed_anger   = find_talent_spell( talent_tree::CLASS, "Swallowed Anger" );
  talent.demon_hunter.charred_warblades = find_talent_spell( talent_tree::CLASS, "Charred Warblades" );

  talent.demon_hunter.felfire_haste          = find_talent_spell( talent_tree::CLASS, "Felfire Haste" );
  talent.demon_hunter.master_of_the_glaive   = find_talent_spell( talent_tree::CLASS, "Master of the Glaive" );
  talent.demon_hunter.champion_of_the_glaive = find_talent_spell( talent_tree::CLASS, "Champion of the Glaive" );
  talent.demon_hunter.aura_of_pain           = find_talent_spell( talent_tree::CLASS, "Aura of Pain" );
  talent.demon_hunter.precise_sigils         = find_talent_spell( talent_tree::CLASS, "Precise Sigils" );
  talent.demon_hunter.lost_in_darkness       = find_talent_spell( talent_tree::CLASS, "Lost in Darkness" );

  talent.demon_hunter.chaos_nova      = find_talent_spell( talent_tree::CLASS, "Chaos Nova" );
  talent.demon_hunter.soul_rending    = find_talent_spell( talent_tree::CLASS, "Soul Rending" );
  talent.demon_hunter.infernal_armor  = find_talent_spell( talent_tree::CLASS, "Infernal Armor" );
  talent.demon_hunter.aldrachi_design = find_talent_spell( talent_tree::CLASS, "Aldrachi Design" );

  talent.demon_hunter.chaos_fragments      = find_talent_spell( talent_tree::CLASS, "Chaos Fragments" );
  talent.demon_hunter.illidari_knowledge   = find_talent_spell( talent_tree::CLASS, "Illidari Knowledge" );
  talent.demon_hunter.demonic              = find_talent_spell( talent_tree::CLASS, "Demonic" );
  talent.demon_hunter.will_of_the_illidari = find_talent_spell( talent_tree::CLASS, "Will of the Illidari" );
  talent.demon_hunter.live_by_the_glaive   = find_talent_spell( talent_tree::CLASS, "Live by the Glaive" );

  talent.demon_hunter.internal_struggle = find_talent_spell( talent_tree::CLASS, "Internal Struggle" );
  talent.demon_hunter.darkness          = find_talent_spell( talent_tree::CLASS, "Darkness" );
  talent.demon_hunter.soul_sigils       = find_talent_spell( talent_tree::CLASS, "Soul Sigils" );
  talent.demon_hunter.quickened_sigils  = find_talent_spell( talent_tree::CLASS, "Quickened Sigils" );

  talent.demon_hunter.erratic_felheart = find_talent_spell( talent_tree::CLASS, "Erratic Felheart" );
  talent.demon_hunter.long_night       = find_talent_spell( talent_tree::CLASS, "Long Night" );
  talent.demon_hunter.pitch_black      = find_talent_spell( talent_tree::CLASS, "Pitch Black" );
  talent.demon_hunter.rush_of_chaos    = find_talent_spell( talent_tree::CLASS, "Rush of Chaos" );
  talent.demon_hunter.demon_muzzle     = find_talent_spell( talent_tree::CLASS, "Demon Muzzle" );
  talent.demon_hunter.flames_of_fury   = find_talent_spell( talent_tree::CLASS, "Flames of Fury" );

  talent.demon_hunter.collective_anguish = find_talent_spell( talent_tree::CLASS, "Collective Anguish" );
  talent.demon_hunter.the_hunt           = find_talent_spell( talent_tree::CLASS, "The Hunt" );
  talent.demon_hunter.sigil_of_spite     = find_talent_spell( talent_tree::CLASS, "Sigil of Spite" );

  // Havoc Talents

  talent.havoc.eye_beam = find_talent_spell( talent_tree::SPECIALIZATION, "Eye Beam" );

  talent.havoc.critical_chaos    = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Chaos" );
  talent.havoc.insatiable_hunger = find_talent_spell( talent_tree::SPECIALIZATION, "Insatiable Hunger" );
  talent.havoc.demon_blades      = find_talent_spell( talent_tree::SPECIALIZATION, "Demon Blades" );
  talent.havoc.burning_hatred    = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Hatred" );

  talent.havoc.improved_fel_rush     = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Fel Rush" );
  talent.havoc.dash_of_chaos         = find_talent_spell( talent_tree::SPECIALIZATION, "Dash of Chaos" );
  talent.havoc.improved_chaos_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Chaos Strike" );
  talent.havoc.first_blood           = find_talent_spell( talent_tree::SPECIALIZATION, "First Blood" );
  talent.havoc.accelerated_blade     = find_talent_spell( talent_tree::SPECIALIZATION, "Accelerated Blade" );
  talent.havoc.demon_hide            = find_talent_spell( talent_tree::SPECIALIZATION, "Demon Hide" );

  talent.havoc.desperate_instincts = find_talent_spell( talent_tree::SPECIALIZATION, "Desperate Instincts" );
  talent.havoc.netherwalk          = find_talent_spell( talent_tree::SPECIALIZATION, "Netherwalk" );
  talent.havoc.deflecting_dance    = find_talent_spell( talent_tree::SPECIALIZATION, "Deflecting Dance" );
  talent.havoc.mortal_dance        = find_talent_spell( talent_tree::SPECIALIZATION, "Mortal Dance" );

  talent.havoc.initiative             = find_talent_spell( talent_tree::SPECIALIZATION, "Initiative" );
  talent.havoc.scars_of_suffering     = find_talent_spell( talent_tree::SPECIALIZATION, "Scars of Suffering" );
  talent.havoc.chaotic_transformation = find_talent_spell( talent_tree::SPECIALIZATION, "Chaotic Transformation" );
  talent.havoc.furious_throws         = find_talent_spell( talent_tree::SPECIALIZATION, "Furious Throws" );
  talent.havoc.trail_of_ruin          = find_talent_spell( talent_tree::SPECIALIZATION, "Trail of Ruin" );

  talent.havoc.unbound_chaos     = find_talent_spell( talent_tree::SPECIALIZATION, "Unbound Chaos" );
  talent.havoc.blind_fury        = find_talent_spell( talent_tree::SPECIALIZATION, "Blind Fury" );
  talent.havoc.looks_can_kill    = find_talent_spell( talent_tree::SPECIALIZATION, "Looks Can Kill" );
  talent.havoc.dancing_with_fate = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing with Fate" );
  talent.havoc.growing_inferno   = find_talent_spell( talent_tree::SPECIALIZATION, "Growing Inferno" );

  talent.havoc.tactical_retreat     = find_talent_spell( talent_tree::SPECIALIZATION, "Tactical Retreat" );
  talent.havoc.isolated_prey        = find_talent_spell( talent_tree::SPECIALIZATION, "Isolated Prey" );
  talent.havoc.furious_gaze         = find_talent_spell( talent_tree::SPECIALIZATION, "Furious Gaze" );
  talent.havoc.relentless_onslaught = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Onslaught" );
  talent.havoc.burning_wound        = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Wound" );

  talent.havoc.momentum        = find_talent_spell( talent_tree::SPECIALIZATION, "Momentum" );
  talent.havoc.inertia         = find_talent_spell( talent_tree::SPECIALIZATION, "Inertia" );
  talent.havoc.chaos_theory    = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Theory" );
  talent.havoc.restless_hunter = find_talent_spell( talent_tree::SPECIALIZATION, "Restless Hunter" );
  talent.havoc.inner_demon     = find_talent_spell( talent_tree::SPECIALIZATION, "Inner Demon" );
  talent.havoc.serrated_glaive = find_talent_spell( talent_tree::SPECIALIZATION, "Serrated Glaive" );
  talent.havoc.ragefire        = find_talent_spell( talent_tree::SPECIALIZATION, "Ragefire" );

  talent.havoc.know_your_enemy     = find_talent_spell( talent_tree::SPECIALIZATION, "Know Your Enemy" );
  talent.havoc.glaive_tempest      = find_talent_spell( talent_tree::SPECIALIZATION, "Glaive Tempest" );
  talent.havoc.cycle_of_hatred     = find_talent_spell( talent_tree::SPECIALIZATION, "Cycle of Hatred" );
  talent.havoc.soulscar            = find_talent_spell( talent_tree::SPECIALIZATION, "Soulscar" );
  talent.havoc.chaotic_disposition = find_talent_spell( talent_tree::SPECIALIZATION, "Chaotic Disposition" );

  talent.havoc.essence_break       = find_talent_spell( talent_tree::SPECIALIZATION, "Essence Break" );
  talent.havoc.fel_barrage         = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Barrage" );
  talent.havoc.shattered_destiny   = find_talent_spell( talent_tree::SPECIALIZATION, "Shattered Destiny" );
  talent.havoc.any_means_necessary = find_talent_spell( talent_tree::SPECIALIZATION, "Any Means Necessary" );
  talent.havoc.a_fire_inside       = find_talent_spell( talent_tree::SPECIALIZATION, "A Fire Inside" );

  // Vengeance Talents

  talent.vengeance.fel_devastation = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Devastation" );

  talent.vengeance.frailty     = find_talent_spell( talent_tree::SPECIALIZATION, "Frailty" );
  talent.vengeance.fiery_brand = find_talent_spell( talent_tree::SPECIALIZATION, "Fiery Brand" );

  talent.vengeance.perfectly_balanced_glaive =
      find_talent_spell( talent_tree::SPECIALIZATION, "Perfectly Balanced Glaive" );
  talent.vengeance.deflecting_spikes = find_talent_spell( talent_tree::SPECIALIZATION, "Deflecting Spikes" );
  talent.vengeance.ascending_flame   = find_talent_spell( talent_tree::SPECIALIZATION, "Ascending Flame" );

  talent.vengeance.shear_fury       = find_talent_spell( talent_tree::SPECIALIZATION, "Shear Fury" );
  talent.vengeance.fracture         = find_talent_spell( talent_tree::SPECIALIZATION, "Fracture" );
  talent.vengeance.calcified_spikes = find_talent_spell( talent_tree::SPECIALIZATION, "Calcified Spikes" );
  talent.vengeance.roaring_fire     = find_talent_spell( talent_tree::SPECIALIZATION, "Roaring Fire" );
  talent.vengeance.sigil_of_silence = find_talent_spell( talent_tree::SPECIALIZATION, "Sigil of Silence" );
  talent.vengeance.retaliation      = find_talent_spell( talent_tree::SPECIALIZATION, "Retaliation" );
  talent.vengeance.meteoric_strikes = find_talent_spell( talent_tree::SPECIALIZATION, "Meteoric Strikes" );

  talent.vengeance.spirit_bomb      = find_talent_spell( talent_tree::SPECIALIZATION, "Spirit Bomb" );
  talent.vengeance.feast_of_souls   = find_talent_spell( talent_tree::SPECIALIZATION, "Feast of Souls" );
  talent.vengeance.agonizing_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Agonizing Flames" );
  talent.vengeance.extended_spikes  = find_talent_spell( talent_tree::SPECIALIZATION, "Extended Spikes" );
  talent.vengeance.burning_blood    = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Blood" );
  talent.vengeance.soul_barrier     = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Barrier" );
  talent.vengeance.bulk_extraction  = find_talent_spell( talent_tree::SPECIALIZATION, "Bulk Extraction" );
  talent.vengeance.revel_in_pain    = find_talent_spell( talent_tree::SPECIALIZATION, "Revel in Pain" );

  talent.vengeance.void_reaver         = find_talent_spell( talent_tree::SPECIALIZATION, "Void Reaver" );
  talent.vengeance.fallout             = find_talent_spell( talent_tree::SPECIALIZATION, "Fallout" );
  talent.vengeance.ruinous_bulwark     = find_talent_spell( talent_tree::SPECIALIZATION, "Ruinous Bulwark" );
  talent.vengeance.volatile_flameblood = find_talent_spell( talent_tree::SPECIALIZATION, "Volatile Flameblood" );
  talent.vengeance.fel_flame_fortification =
      find_talent_spell( talent_tree::SPECIALIZATION, "Fel Flame Fortification" );

  talent.vengeance.soul_furnace    = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Furnace" );
  talent.vengeance.painbringer     = find_talent_spell( talent_tree::SPECIALIZATION, "Painbringer" );
  talent.vengeance.sigil_of_chains = find_talent_spell( talent_tree::SPECIALIZATION, "Sigil of Chains" );
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

  talent.vengeance.soulcrush          = find_talent_spell( talent_tree::SPECIALIZATION, "Soulcrush" );
  talent.vengeance.soul_carver        = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Carver" );
  talent.vengeance.last_resort        = find_talent_spell( talent_tree::SPECIALIZATION, "Last Resort" );
  talent.vengeance.darkglare_boon     = find_talent_spell( talent_tree::SPECIALIZATION, "Darkglare Boon" );
  talent.vengeance.down_in_flames     = find_talent_spell( talent_tree::SPECIALIZATION, "Down in Flames" );
  talent.vengeance.illuminated_sigils = find_talent_spell( talent_tree::SPECIALIZATION, "Illuminated Sigils" );

  // Hero Talents ===========================================================

  // Aldrachi Reaver talents
  talent.aldrachi_reaver.art_of_the_glaive = find_talent_spell( talent_tree::HERO, "Art of the Glaive" );

  talent.aldrachi_reaver.fury_of_the_aldrachi = find_talent_spell( talent_tree::HERO, "Fury of the Aldrachi" );
  talent.aldrachi_reaver.evasive_action       = find_talent_spell( talent_tree::HERO, "Evasive Action" );
  talent.aldrachi_reaver.unhindered_assault   = find_talent_spell( talent_tree::HERO, "Unhindered Assault" );
  talent.aldrachi_reaver.reavers_mark         = find_talent_spell( talent_tree::HERO, "Reaver's Mark" );

  talent.aldrachi_reaver.aldrachi_tactics     = find_talent_spell( talent_tree::HERO, "Aldrachi Tactics" );
  talent.aldrachi_reaver.army_unto_oneself    = find_talent_spell( talent_tree::HERO, "Army Unto Oneself" );
  talent.aldrachi_reaver.incorruptible_spirit = find_talent_spell( talent_tree::HERO, "Incorruptible Spirit" );
  talent.aldrachi_reaver.wounded_quarry       = find_talent_spell( talent_tree::HERO, "Wounded Quarry" );

  talent.aldrachi_reaver.incisive_blade    = find_talent_spell( talent_tree::HERO, "Incisive Blade" );
  talent.aldrachi_reaver.keen_engagement   = find_talent_spell( talent_tree::HERO, "Keen Engagement" );
  talent.aldrachi_reaver.preemptive_strike = find_talent_spell( talent_tree::HERO, "Preemptive Strike" );
  talent.aldrachi_reaver.warblades_hunger  = find_talent_spell( talent_tree::HERO, "Warblade's Hunger" );

  talent.aldrachi_reaver.thrill_of_the_fight = find_talent_spell( talent_tree::HERO, "Thrill of the Fight" );

  // Fel-Scarred talents
  talent.felscarred.demonsurge = find_talent_spell( talent_tree::HERO, "Demonsurge" );

  talent.felscarred.wave_of_debilitation  = find_talent_spell( talent_tree::HERO, "Wave of Debilitation" );
  talent.felscarred.pursuit_of_angriness  = find_talent_spell( talent_tree::HERO, "Pursuit of Angriness" );
  talent.felscarred.focused_hatred        = find_talent_spell( talent_tree::HERO, "Focused Hatred" );
  talent.felscarred.set_fire_to_the_pain  = find_talent_spell( talent_tree::HERO, "Set Fire to the Pain" );
  talent.felscarred.improved_soul_rending = find_talent_spell( talent_tree::HERO, "Improved Soul Rending" );

  talent.felscarred.burning_blades         = find_talent_spell( talent_tree::HERO, "Burning Blades" );
  talent.felscarred.violent_transformation = find_talent_spell( talent_tree::HERO, "Violent Transformation" );
  talent.felscarred.enduring_torment       = find_talent_spell( talent_tree::HERO, "Enduring Torment" );

  talent.felscarred.untethered_fury      = find_talent_spell( talent_tree::HERO, "Untethered Fury" );
  talent.felscarred.student_of_suffering = find_talent_spell( talent_tree::HERO, "Student of Suffering" );
  talent.felscarred.flamebound           = find_talent_spell( talent_tree::HERO, "Flamebound" );
  talent.felscarred.monster_rising       = find_talent_spell( talent_tree::HERO, "Monster Rising" );

  talent.felscarred.demonic_intensity = find_talent_spell( talent_tree::HERO, "Demonic Intensity" );

  // Class Background Spells
  spell.felblade_damage      = talent.demon_hunter.felblade->ok() ? find_spell( 213243 ) : spell_data_t::not_found();
  spell.felblade_reset_havoc = talent.demon_hunter.felblade->ok() ? find_spell( 236167 ) : spell_data_t::not_found();
  spell.felblade_reset_vengeance =
      talent.demon_hunter.felblade->ok() ? find_spell( 203557 ) : spell_data_t::not_found();
  spell.infernal_armor_damage =
      talent.demon_hunter.infernal_armor->ok() ? find_spell( 320334 ) : spell_data_t::not_found();
  spell.immolation_aura_damage = spell.immolation_aura_2->ok() ? find_spell( 258921 ) : spell_data_t::not_found();
  spell.sigil_of_flame_damage  = find_spell( 204598 );
  spell.sigil_of_flame_fury    = find_spell( 389787 );
  spell.the_hunt               = talent.demon_hunter.the_hunt;
  spec.sigil_of_misery_debuff =
      talent.demon_hunter.sigil_of_misery->ok() ? find_spell( 207685 ) : spell_data_t::not_found();

  // Spec Background Spells
  mastery.any_means_necessary = talent.havoc.any_means_necessary;
  mastery.any_means_necessary_tuning =
      talent.havoc.any_means_necessary->ok() ? find_spell( 394486 ) : spell_data_t::not_found();

  spec.burning_wound_debuff = talent.havoc.burning_wound->effectN( 1 ).trigger();
  spec.chaos_theory_buff    = talent.havoc.chaos_theory->ok() ? find_spell( 390195 ) : spell_data_t::not_found();
  spec.demon_blades_damage  = talent.havoc.demon_blades->effectN( 1 ).trigger();
  spec.essence_break_debuff = talent.havoc.essence_break->ok() ? find_spell( 320338 ) : spell_data_t::not_found();
  spec.eye_beam_damage      = talent.havoc.eye_beam->ok() ? find_spell( 198030 ) : spell_data_t::not_found();
  spec.furious_gaze_buff    = talent.havoc.furious_gaze->ok() ? find_spell( 343312 ) : spell_data_t::not_found();
  spec.first_blood_blade_dance_damage =
      talent.havoc.first_blood->ok() ? find_spell( 391374 ) : spell_data_t::not_found();
  spec.first_blood_blade_dance_2_damage =
      talent.havoc.first_blood->ok() ? find_spell( 391378 ) : spell_data_t::not_found();
  spec.first_blood_death_sweep_damage =
      talent.havoc.first_blood->ok() ? find_spell( 393055 ) : spell_data_t::not_found();
  spec.first_blood_death_sweep_2_damage =
      talent.havoc.first_blood->ok() ? find_spell( 393054 ) : spell_data_t::not_found();
  spec.glaive_tempest_damage = talent.havoc.glaive_tempest->ok() ? find_spell( 342857 ) : spell_data_t::not_found();
  spec.initiative_buff       = talent.havoc.initiative->ok() ? find_spell( 391215 ) : spell_data_t::not_found();
  spec.inner_demon_buff      = talent.havoc.inner_demon->ok() ? find_spell( 390145 ) : spell_data_t::not_found();
  spec.inner_demon_damage    = talent.havoc.inner_demon->ok() ? find_spell( 390137 ) : spell_data_t::not_found();
  spec.momentum_buff         = talent.havoc.momentum->ok() ? find_spell( 208628 ) : spell_data_t::not_found();
  spec.inertia_buff          = talent.havoc.inertia->ok() ? find_spell( 427641 ) : spell_data_t::not_found();
  spec.ragefire_damage       = talent.havoc.ragefire->ok() ? find_spell( 390197 ) : spell_data_t::not_found();
  spec.restless_hunter_buff  = talent.havoc.restless_hunter->ok() ? find_spell( 390212 ) : spell_data_t::not_found();
  spec.soulscar_debuff       = talent.havoc.soulscar->ok() ? find_spell( 390181 ) : spell_data_t::not_found();
  spec.tactical_retreat_buff = talent.havoc.tactical_retreat->ok() ? find_spell( 389890 ) : spell_data_t::not_found();
  spec.unbound_chaos_buff    = talent.havoc.unbound_chaos->ok() ? find_spell( 347462 ) : spell_data_t::not_found();
  spec.chaotic_disposition_damage =
      talent.havoc.chaotic_disposition->ok() ? find_spell( 428493 ) : spell_data_t::not_found();

  spec.demon_spikes_buff  = find_spell( 203819 );
  spec.fiery_brand_debuff = talent.vengeance.fiery_brand->ok() ? find_spell( 207771 ) : spell_data_t::not_found();
  spec.frailty_debuff     = talent.vengeance.frailty->ok() ? find_spell( 247456 ) : spell_data_t::not_found();
  spec.painbringer_buff   = talent.vengeance.painbringer->ok() ? find_spell( 212988 ) : spell_data_t::not_found();
  spec.calcified_spikes_buff =
      talent.vengeance.calcified_spikes->ok() ? find_spell( 391171 ) : spell_data_t::not_found();
  spec.soul_furnace_damage_amp = talent.vengeance.soul_furnace->ok() ? find_spell( 391172 ) : spell_data_t::not_found();
  spec.soul_furnace_stack      = talent.vengeance.soul_furnace->ok() ? find_spell( 391166 ) : spell_data_t::not_found();
  spec.retaliation_damage      = talent.vengeance.retaliation->ok() ? find_spell( 391159 ) : spell_data_t::not_found();
  spec.sigil_of_silence_debuff =
      talent.vengeance.sigil_of_silence->ok() ? find_spell( 204490 ) : spell_data_t::not_found();
  spec.sigil_of_chains_debuff =
      talent.vengeance.sigil_of_chains->ok() ? find_spell( 204843 ) : spell_data_t::not_found();
  spec.burning_alive_controller =
      talent.vengeance.burning_alive->ok() ? find_spell( 207760 ) : spell_data_t::not_found();
  spec.infernal_strike_impact = find_spell( 189112 );
  spec.spirit_bomb_damage     = talent.vengeance.spirit_bomb->ok() ? find_spell( 247455 ) : spell_data_t::not_found();
  spec.frailty_heal           = talent.vengeance.frailty->ok() ? find_spell( 227255 ) : spell_data_t::not_found();
  spec.feast_of_souls_heal  = talent.vengeance.feast_of_souls->ok() ? find_spell( 207693 ) : spell_data_t::not_found();
  spec.fel_devastation_2    = find_rank_spell( "Fel Devastation", "Rank 2" );
  spec.fel_devastation_heal = talent.vengeance.fel_devastation->ok() ? find_spell( 212106 ) : spell_data_t::not_found();

  // Hero spec background spells
  hero_spec.reavers_glaive =
      talent.aldrachi_reaver.art_of_the_glaive->ok() ? find_spell( 442294 ) : spell_data_t::not_found();
  hero_spec.reavers_mark = talent.aldrachi_reaver.reavers_mark->ok() ? find_spell( 442624 ) : spell_data_t::not_found();
  hero_spec.glaive_flurry =
      talent.aldrachi_reaver.art_of_the_glaive->ok() ? find_spell( 442435 ) : spell_data_t::not_found();
  hero_spec.rending_strike =
      talent.aldrachi_reaver.art_of_the_glaive->ok() ? find_spell( 442442 ) : spell_data_t::not_found();
  hero_spec.art_of_the_glaive_buff =
      talent.aldrachi_reaver.art_of_the_glaive->ok() ? find_spell( 444661 ) : spell_data_t::not_found();
  hero_spec.art_of_the_glaive_damage =
      talent.aldrachi_reaver.fury_of_the_aldrachi->ok() ? find_spell( 444810 ) : spell_data_t::not_found();
  hero_spec.warblades_hunger_buff =
      talent.aldrachi_reaver.warblades_hunger->ok() ? find_spell( 442503 ) : spell_data_t::not_found();
  hero_spec.warblades_hunger_damage =
      talent.aldrachi_reaver.warblades_hunger->ok() ? find_spell( 442507 ) : spell_data_t::not_found();
  hero_spec.wounded_quarry_damage =
      talent.aldrachi_reaver.wounded_quarry->ok() ? find_spell( 442808 ) : spell_data_t::not_found();
  hero_spec.thrill_of_the_fight_attack_speed_buff =
      talent.aldrachi_reaver.thrill_of_the_fight->ok() ? find_spell( 442695 ) : spell_data_t::not_found();
  hero_spec.thrill_of_the_fight_damage_buff =
      talent.aldrachi_reaver.thrill_of_the_fight->ok() ? find_spell( 442688 ) : spell_data_t::not_found();
  hero_spec.burning_blades_debuff =
      talent.felscarred.burning_blades->ok() ? find_spell( 453177 ) : spell_data_t::not_found();
  hero_spec.student_of_suffering_buff =
      talent.felscarred.student_of_suffering->ok() ? find_spell( 453239 ) : spell_data_t::not_found();
  hero_spec.monster_rising_buff =
      talent.felscarred.monster_rising->ok() ? find_spell( 452550 ) : spell_data_t::not_found();
  hero_spec.enduring_torment_buff =
      talent.felscarred.enduring_torment->ok() ? find_spell( 453314 ) : spell_data_t::not_found();
  hero_spec.demonsurge_demonic_buff =
      talent.felscarred.demonsurge->ok() ? find_spell( 452435 ) : spell_data_t::not_found();
  hero_spec.demonsurge_hardcast_buff =
      talent.felscarred.demonic_intensity->ok() ? find_spell( 452489 ) : spell_data_t::not_found();
  hero_spec.demonsurge_damage = talent.felscarred.demonsurge->ok() ? find_spell( 452416 ) : spell_data_t::not_found();
  hero_spec.demonsurge_stacking_buff =
      talent.felscarred.demonic_intensity->ok() ? find_spell( 452416 ) : spell_data_t::not_found();
  hero_spec.demonsurge_trigger = talent.felscarred.demonsurge->ok() ? find_spell( 453323 ) : spell_data_t::not_found();
  hero_spec.soul_sunder        = talent.felscarred.demonsurge->ok() ? find_spell( 452436 ) : spell_data_t::not_found();
  hero_spec.spirit_burst       = talent.vengeance.spirit_bomb->ok() && talent.felscarred.demonsurge->ok()
                                     ? find_spell( 452437 )
                                     : spell_data_t::not_found();
  hero_spec.sigil_of_doom =
      talent.felscarred.demonic_intensity->ok() ? find_spell( 452490 ) : spell_data_t::not_found();
  hero_spec.sigil_of_doom_damage =
      talent.felscarred.demonic_intensity->ok() ? find_spell( 462030 ) : spell_data_t::not_found();
  hero_spec.abyssal_gaze = talent.felscarred.demonic_intensity->ok() ? find_spell( 452497 ) : spell_data_t::not_found();
  hero_spec.fel_desolation =
      talent.felscarred.demonic_intensity->ok() ? find_spell( 452486 ) : spell_data_t::not_found();

  if ( talent.aldrachi_reaver.art_of_the_glaive->ok() )
  {
    hero_spec.reavers_glaive_buff =
        specialization() == DEMON_HUNTER_HAVOC ? find_spell( 444686 ) : find_spell( 444764 );
  }
  else
  {
    hero_spec.reavers_glaive_buff = spell_data_t::not_found();
  }

  hero_spec.wounded_quarry_proc_rate = specialization() == DEMON_HUNTER_HAVOC ? options.wounded_quarry_chance_havoc
                                                                              : options.wounded_quarry_chance_vengeance;

  // Sigil overrides for Precise/Concentrated Sigils
  std::vector<const spell_data_t*> sigil_overrides = { talent.demon_hunter.precise_sigils };
  spell.sigil_of_flame                             = find_spell_override( find_spell( 204596 ), sigil_overrides );
  spell.sigil_of_spite = find_spell_override( talent.demon_hunter.sigil_of_spite, sigil_overrides );
  spell.sigil_of_spite_damage =
      talent.demon_hunter.sigil_of_spite->ok() ? find_spell( 389860 ) : spell_data_t::not_found();
  spec.sigil_of_misery  = find_spell_override( talent.demon_hunter.sigil_of_misery, sigil_overrides );
  spec.sigil_of_silence = find_spell_override( talent.vengeance.sigil_of_silence, sigil_overrides );
  spec.sigil_of_chains  = find_spell_override( talent.vengeance.sigil_of_chains, sigil_overrides );

  if ( talent.demon_hunter.collective_anguish->ok() )
  {
    spell.collective_anguish = specialization() == DEMON_HUNTER_HAVOC ? find_spell( 393831 ) : find_spell( 391057 );
    spell.collective_anguish_damage =
        ( specialization() == DEMON_HUNTER_HAVOC ? spell.collective_anguish->effectN( 1 ).trigger()
                                                 : find_spell( 391058 ) );
  }
  else
  {
    spell.collective_anguish        = spell_data_t::not_found();
    spell.collective_anguish_damage = spell_data_t::not_found();
  }

  // Set Bonus Items ========================================================

  set_bonuses.tww1_havoc_2pc     = sets->set( DEMON_HUNTER_HAVOC, TWW1, B2 );
  set_bonuses.tww1_havoc_4pc     = sets->set( DEMON_HUNTER_HAVOC, TWW1, B4 );
  set_bonuses.tww1_vengeance_2pc = sets->set( DEMON_HUNTER_VENGEANCE, TWW1, B2 );
  set_bonuses.tww1_vengeance_4pc = sets->set( DEMON_HUNTER_VENGEANCE, TWW1, B4 );

  // Set Bonus Auxilliary ===================================================

  set_bonuses.tww1_havoc_4pc_buff = set_bonuses.tww1_havoc_4pc->ok() ? find_spell( 454628 ) : spell_data_t::not_found();
  set_bonuses.tww1_vengeance_4pc_buff =
      set_bonuses.tww1_vengeance_4pc->ok() ? find_spell( 454774 ) : spell_data_t::not_found();

  // Spell Initialization ===================================================

  using namespace actions::attacks;
  using namespace actions::spells;
  using namespace actions::heals;

  active.consume_soul_greater =
      new consume_soul_t( this, "consume_soul_greater", spec.consume_soul_greater, soul_fragment::GREATER );
  active.consume_soul_lesser =
      new consume_soul_t( this, "consume_soul_lesser", spec.consume_soul_lesser, soul_fragment::LESSER );

  active.burning_wound = get_background_action<burning_wound_t>( "burning_wound" );

  if ( talent.havoc.chaotic_disposition->ok() )
  {
    auto chaotic_disposition_effect          = new special_effect_t( this );
    chaotic_disposition_effect->name_str     = "chaotic_disposition";
    chaotic_disposition_effect->type         = SPECIAL_EFFECT_EQUIP;
    chaotic_disposition_effect->spell_id     = talent.havoc.chaotic_disposition->id();
    chaotic_disposition_effect->proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
    chaotic_disposition_effect->proc_chance_ = 1.0;  // 2023-11-14 -- Proc chance removed from talent spell
    special_effects.push_back( chaotic_disposition_effect );

    auto chaotic_disposition_cb = new chaotic_disposition_cb_t( this, *chaotic_disposition_effect );

    chaotic_disposition_cb->activate();
  }

  if ( talent.demon_hunter.collective_anguish->ok() )
  {
    active.collective_anguish = get_background_action<collective_anguish_t>( "collective_anguish" );
  }

  if ( talent.havoc.demon_blades->ok() )
  {
    active.demon_blades = new demon_blades_t( this );
  }
  if ( talent.havoc.relentless_onslaught->ok() )
  {
    auto relentless_onslaught_chaos_strike = get_background_action<chaos_strike_t>( "chaos_strike_onslaught" );
    relentless_onslaught_chaos_strike->from_onslaught = true;
    active.relentless_onslaught                       = relentless_onslaught_chaos_strike;

    auto relentless_onslaught_annihilation = get_background_action<annihilation_t>( "annihilation_onslaught" );
    relentless_onslaught_annihilation->from_onslaught = true;
    active.relentless_onslaught_annihilation          = relentless_onslaught_annihilation;
  }
  if ( talent.havoc.inner_demon->ok() )
  {
    active.inner_demon = get_background_action<inner_demon_t>( "inner_demon" );
  }
  if ( talent.havoc.ragefire->ok() )
  {
    active.ragefire = get_background_action<ragefire_t>( "ragefire" );
  }
  if ( talent.havoc.soulscar->ok() )
  {
    active.soulscar = get_background_action<soulscar_t>( "soulscar" );
  }

  if ( talent.vengeance.retaliation->ok() )
  {
    active.retaliation = get_background_action<retaliation_t>( "retaliation" );
  }

  if ( talent.aldrachi_reaver.fury_of_the_aldrachi->ok() )
  {
    active.art_of_the_glaive = get_background_action<art_of_the_glaive_t>( "art_of_the_glaive" );
  }
  if ( talent.aldrachi_reaver.preemptive_strike->ok() )
  {
    active.preemptive_strike = get_background_action<preemptive_strike_t>( "preemptive_strike" );
  }
  if ( talent.aldrachi_reaver.warblades_hunger->ok() )
  {
    active.warblades_hunger = get_background_action<warblades_hunger_t>( "warblades_hunger" );
  }
  if ( talent.aldrachi_reaver.wounded_quarry->ok() )
  {
    active.wounded_quarry = get_background_action<wounded_quarry_t>( "wounded_quarry" );
  }

  if ( talent.felscarred.burning_blades->ok() )
  {
    active.burning_blades = get_background_action<burning_blades_t>( "burning_blades" );
  }
  if ( talent.felscarred.demonsurge->ok() )
  {
    active.demonsurge = get_background_action<demonsurge_t>( "demonsurge" );
  }
}

// demon_hunter_t::init_finished ============================================

void demon_hunter_t::init_finished()
{
  player_t::init_finished();

  parse_player_effects();
}

// demon_hunter_t::validate_fight_style =====================================

bool demon_hunter_t::validate_fight_style( fight_style_e style ) const
{
  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    switch ( style )
    {
      case FIGHT_STYLE_DUNGEON_ROUTE:
      case FIGHT_STYLE_DUNGEON_SLICE:
        return false;
      default:
        return true;
    }
  }
  return true;
}

// demon_hunter_t::invalidate_cache =========================================

void demon_hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( talent.demon_hunter.pursuit->ok() )
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
      {
        invalidate_cache( CACHE_PARRY );
      }
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
  switch ( specialization() )
  {
    case DEMON_HUNTER_VENGEANCE:
      return demon_hunter_apl::food_vengeance( this );
    default:
      return demon_hunter_apl::food_havoc( this );
  }
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
  cooldown.consume_magic    = get_cooldown( "consume_magic" );
  cooldown.disrupt          = get_cooldown( "disrupt" );
  cooldown.sigil_of_spite   = get_cooldown( "sigil_of_spite" );
  cooldown.felblade         = get_cooldown( "felblade" );
  cooldown.fel_eruption     = get_cooldown( "fel_eruption" );
  cooldown.immolation_aura  = get_cooldown( "immolation_aura" );
  cooldown.the_hunt         = get_cooldown( "the_hunt" );
  cooldown.spectral_sight   = get_cooldown( "spectral_sight" );
  cooldown.sigil_of_flame   = get_cooldown( "sigil_of_flame" );
  cooldown.sigil_of_misery  = get_cooldown( "sigil_of_misery" );
  cooldown.throw_glaive     = get_cooldown( "throw_glaive" );
  cooldown.vengeful_retreat = get_cooldown( "vengeful_retreat" );
  cooldown.chaos_nova       = get_cooldown( "chaos_nova" );
  cooldown.metamorphosis    = get_cooldown( "metamorphosis" );

  // Havoc
  cooldown.blade_dance              = get_cooldown( "blade_dance" );
  cooldown.blur                     = get_cooldown( "blur" );
  cooldown.chaos_strike_refund_icd  = get_cooldown( "chaos_strike_refund_icd" );
  cooldown.essence_break            = get_cooldown( "essence_break" );
  cooldown.eye_beam                 = get_cooldown( "eye_beam" );
  cooldown.fel_barrage              = get_cooldown( "fel_barrage" );
  cooldown.fel_rush                 = get_cooldown( "fel_rush" );
  cooldown.netherwalk               = get_cooldown( "netherwalk" );
  cooldown.relentless_onslaught_icd = get_cooldown( "relentless_onslaught_icd" );
  cooldown.movement_shared          = get_cooldown( "movement_shared" );

  // Vengeance
  cooldown.demon_spikes            = get_cooldown( "demon_spikes" );
  cooldown.fiery_brand             = get_cooldown( "fiery_brand" );
  cooldown.sigil_of_chains         = get_cooldown( "sigil_of_chains" );
  cooldown.sigil_of_silence        = get_cooldown( "sigil_of_silence" );
  cooldown.fel_devastation         = get_cooldown( "fel_devastation" );
  cooldown.volatile_flameblood_icd = get_cooldown( "volatile_flameblood_icd" );
  cooldown.soul_cleave             = get_cooldown( "soul_cleave" );

  // Aldrachi Reaver
  cooldown.art_of_the_glaive_consumption_icd = get_cooldown( "art_of_the_glaive_consumption_icd" );

  // Fel-scarred
}

// demon_hunter_t::create_gains =============================================

void demon_hunter_t::create_gains()
{
  // General
  gain.miss_refund = get_gain( "miss_refund" );

  // Havoc
  gain.blind_fury       = get_gain( "blind_fury" );
  gain.tactical_retreat = get_gain( "tactical_retreat" );

  // Vengeance
  gain.metamorphosis       = get_gain( "metamorphosis" );
  gain.darkglare_boon      = get_gain( "darkglare_boon" );
  gain.volatile_flameblood = get_gain( "volatile_flameblood" );

  // Set Bonuses
  gain.seething_fury = get_gain( "seething_fury" );

  // Fel-scarred
  gain.student_of_suffering = get_gain( "student_of_suffering" );
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
  double a = base_t::composite_armor();

  // Mastery: Fel Blood increases Armor by Mastery * AGI and is doubled while Demon Spikes is active.
  const double mastery_value   = cache.mastery() * mastery.fel_blood->effectN( 1 ).mastery_value();
  const double fel_blood_value = cache.agility() * mastery_value * ( 1 + buff.demon_spikes->check() );
  a += fel_blood_value;

  return a;
}

// demon_hunter_t::composite_base_armor_multiplier ==========================

double demon_hunter_t::composite_base_armor_multiplier() const
{
  double am = base_t::composite_base_armor_multiplier();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    if ( buff.metamorphosis->check() )
    {
      am *= 1.0 + spec.metamorphosis_buff->effectN( 8 ).percent();
    }
  }

  return am;
}

// demon_hunter_t::composite_armor_multiplier ===============================

double demon_hunter_t::composite_armor_multiplier() const
{
  double am = base_t::composite_armor_multiplier();

  if ( buff.immolation_aura->check() )
  {
    am *= pow( 1.0 + talent.demon_hunter.infernal_armor->effectN( 1 ).percent(), buff.immolation_aura->check() );
  }

  return am;
}

// demon_hunter_t::composite_melee_haste  ===================================

double demon_hunter_t::composite_melee_haste() const
{
  double mh = base_t::composite_melee_haste();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    mh /= 1.0 + buff.metamorphosis->check_value();
  }

  return mh;
}

// demon_hunter_t::composite_spell_haste ====================================

double demon_hunter_t::composite_spell_haste() const
{
  double sh = base_t::composite_spell_haste();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    sh /= 1.0 + buff.metamorphosis->check_value();
  }

  return sh;
}

// demon_hunter_t::composite_player_multiplier ==============================

double demon_hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = base_t::composite_player_multiplier( school );

  return m;
}

// demon_hunter_t::composite_player_critical_damage_multiplier ==============

double demon_hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = base_t::composite_player_critical_damage_multiplier( s );

  if ( talent.havoc.know_your_enemy->ok() )
  {
    // 2022-11-28 -- Halving this value as it appears this still uses Modify Crit Damage Done% (163)
    //               However, it has been scripted to match the value of Spell Critical Bonus Multiplier (15)
    //               This does affect gear, however, so it is a player rather than spell modifier
    m *= 1.0 + talent.havoc.know_your_enemy->effectN( 2 ).percent() * cache.attack_crit_chance() * 0.5;
  }

  return m;
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

// demon_hunter_t::stacking_movement_modifier ===============================

double demon_hunter_t::stacking_movement_modifier() const
{
  double ms = base_t::stacking_movement_modifier();

  if ( talent.demon_hunter.pursuit->ok() )
  {
    ms += cache.mastery() * talent.demon_hunter.pursuit->effectN( 1 ).mastery_value();
  }

  if ( talent.felscarred.pursuit_of_angriness->ok() )
  {
    ms += buff.pursuit_of_angryness->value();
  }

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    ms += buff.immolation_aura->value();
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
  double prop_values  = 0;
  for ( auto& item : items )
  {
    if ( item.slot == SLOT_SHIRT || item.slot == SLOT_RANGED || item.slot == SLOT_TABARD || item.item_level() <= 0 )
    {
      continue;
    }

    const random_prop_data_t item_data = dbc->random_property( item.item_level() );
    int index                          = item_database::random_suffix_type( item.parsed.data );
    if ( item_data.p_epic[ 0 ] == 0 )
    {
      continue;
    }
    slot_weights += item_data.p_epic[ index ] / item_data.p_epic[ 0 ];

    if ( !item.active() )
    {
      continue;
    }

    prop_values += item_data.p_epic[ index ];
  }

  double expected_health = ( prop_values / slot_weights ) * 8.318556;
  expected_health += base.stats.attribute[ STAT_STAMINA ];
  expected_health *= 1 + matching_gear_multiplier( ATTR_STAMINA );
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
       dbc::is_school( school, SCHOOL_PHYSICAL ) && dt == result_amount_type::DMG_DIRECT &&
       s->action->result_is_hit( s->result ) )
  {
    for ( int i = 0; i < buff.immolation_aura->check(); i++ )
    {
      active.infernal_armor->set_target( s->action->player );
      active.infernal_armor->execute();
    }
  }

  if ( active.retaliation && buff.demon_spikes->check() && s->action->player->is_enemy() &&
       dt == result_amount_type::DMG_DIRECT && !s->action->special && s->action->result_is_hit( s->result ) )
  {
    active.retaliation->execute_on_target( s->action->player );
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

  // Cap starting fury
  double fury_cap     = 20.0;
  double current_fury = resources.current[ RESOURCE_FURY ];
  if ( in_boss_encounter && current_fury > fury_cap )
  {
    resources.current[ RESOURCE_FURY ] = fury_cap;
    sim->print_debug( "Fury for {} capped at combat start to {} (was {})", *this, fury_cap, current_fury );
  }

  if ( talent.felscarred.monster_rising->ok() )
  {
    buff.monster_rising->trigger();
  }
  if ( talent.felscarred.enduring_torment->ok() )
  {
    buff.enduring_torment->trigger();
  }
  if ( talent.felscarred.pursuit_of_angriness->ok() )
  {
    buff.pursuit_of_angryness->trigger();
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
  double amt = player_t::resource_gain( resource_type, amount, source, action );
  if ( resource_type == RESOURCE_FURY && talent.felscarred.pursuit_of_angriness->ok() )
  {
    invalidate_cache( CACHE_RUN_SPEED );
  }
  return amt;
}

double demon_hunter_t::resource_gain( resource_e resource_type, double amount, double delta_coeff, gain_t* source,
                                      action_t* action )
{
  double modified_amount =
      static_cast<int>( amount + rng().range( 0, 1 + ( amount * delta_coeff ) ) - ( ( amount * delta_coeff ) / 2.0 ) );
  return resource_gain( resource_type, modified_amount, source, action );
}

// demon_hunter_t::resource_loss ============================================

double demon_hunter_t::resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  double amt = player_t::resource_loss( resource_type, amount, source, action );
  if ( resource_type == RESOURCE_FURY && talent.felscarred.pursuit_of_angriness->ok() )
  {
    invalidate_cache( CACHE_RUN_SPEED );
  }
  return amt;
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
          sim->out_debug.printf( "%s adjusts %s temporary health. old=%.0f new=%.0f diff=%.0f", name(),
                                 buff.metamorphosis->name(), metamorphosis_health, new_health, diff );
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
      s->result_amount *=
          1.0 + spec.demonic_wards->effectN( 1 ).percent() + spec.demonic_wards_2->effectN( 1 ).percent();
    }

    if ( dbc::get_school_mask( school ) & SCHOOL_MASK_PHYSICAL )
    {
      s->result_amount *= 1.0 + talent.havoc.demon_hide->effectN( 2 ).percent();
    }
  }
  else  // DEMON_HUNTER_VENGEANCE
  {
    s->result_amount *= 1.0 + spec.demonic_wards->effectN( 1 ).percent() +
                        spec.demonic_wards_2->effectN( 1 ).percent() + spec.demonic_wards_3->effectN( 1 ).percent();

    s->result_amount *= 1.0 + buff.painbringer->check_stack_value();

    s->result_amount *= 1.0 + buff.calcified_spikes->check_stack_value();

    const demon_hunter_td_t* td = get_target_data( s->action->player );
    if ( td->dots.fiery_brand && td->dots.fiery_brand->is_ticking() )
    {
      s->result_amount *= 1.0 + spec.fiery_brand_debuff->effectN( 1 ).percent();
    }

    if ( td->debuffs.frailty->check() && talent.vengeance.void_reaver->ok() )
    {
      s->result_amount *= 1.0 + spec.frailty_debuff->effectN( 3 ).percent() * td->debuffs.frailty->check();
    }
  }
}

// demon_hunter_t::reset ====================================================

void demon_hunter_t::reset()
{
  base_t::reset();

  soul_fragment_pick_up         = nullptr;
  frailty_driver                = nullptr;
  exit_melee_event              = nullptr;
  next_fragment_spawn           = 0;
  metamorphosis_health          = 0;
  frailty_accumulator           = 0.0;
  shattered_destiny_accumulator = 0.0;

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
  if ( buff.out_of_range->check() && buff.out_of_range->remains() > timespan_t::zero() )
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
    if ( souls_consumed >= max )
      break;
  }

  if ( sim->debug )
  {
    sim->out_debug.printf( "%s consumes %u %ss. remaining=%u", name(), souls_consumed, get_soul_fragment_str( type ), 0,
                           get_total_soul_fragments( type ) );
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

// demon_hunter_t::get_inactive_soul_fragments ================================

unsigned demon_hunter_t::get_inactive_soul_fragments( soul_fragment type_mask ) const
{
  return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
                          [ &type_mask ]( unsigned acc, soul_fragment_t* frag ) {
                            return acc + ( frag->is_type( type_mask ) && !frag->active() );
                          } );
}

// demon_hunter_t::get_total_soul_fragments =================================

unsigned demon_hunter_t::get_total_soul_fragments( soul_fragment type_mask ) const
{
  if ( type_mask == soul_fragment::ANY )
  {
    return (unsigned)soul_fragments.size();
  }

  return std::accumulate(
      soul_fragments.begin(), soul_fragments.end(), 0,
      [ &type_mask ]( unsigned acc, soul_fragment_t* frag ) { return acc + frag->is_type( type_mask ); } );
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
          it->consume( true );

          if ( sim->debug )
          {
            sim->out_debug.printf( "%s consumes overflow fragment %ss. remaining=%u", name(),
                                   get_soul_fragment_str( soul_fragment::LESSER ),
                                   get_total_soul_fragments( soul_fragment::LESSER ) );
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
    case soul_fragment::GREATER:
      soul_proc = proc.soul_fragment_greater;
      break;
    case soul_fragment::GREATER_DEMON:
      soul_proc = proc.soul_fragment_greater_demon;
      break;
    case soul_fragment::EMPOWERED_DEMON:
      soul_proc = proc.soul_fragment_empowered_demon;
      break;
    default:
      soul_proc = proc.soul_fragment_lesser;
  }

  for ( unsigned i = 0; i < n; i++ )
  {
    soul_fragments.push_back( new soul_fragment_t( this, type, consume_on_activation ) );
    soul_proc->occur();
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

// demon_hunter_t::trigger_demonsurge =============================================

void demon_hunter_t::trigger_demonsurge( demonsurge_ability ability )
{
  if ( active.demonsurge && buff.demonsurge_abilities[ ability ]->up() )
  {
    buff.demonsurge_abilities[ ability ]->expire();
    make_event<delayed_execute_event_t>(
        *sim, this, active.demonsurge, target,
        timespan_t::from_millis( hero_spec.demonsurge_trigger->effectN( 1 ).misc_value1() ) );
  }
}

// demon_hunter_t::parse_player_effects() =========================================

void demon_hunter_t::parse_player_effects()
{
  // Shared
  parse_effects( talent.demon_hunter.soul_rending, talent.felscarred.improved_soul_rending );
  parse_effects( talent.demon_hunter.aldrachi_design );
  parse_effects( talent.demon_hunter.internal_struggle,
                 talent.demon_hunter.internal_struggle->effectN( 1 ).base_value() );
  parse_effects( spell.critical_strikes );

  // Havoc
  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    parse_effects( talent.havoc.scars_of_suffering );
    parse_effects( buff.blur );
  }

  // Vengeance
  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    parse_effects( buff.demon_spikes, talent.vengeance.deflecting_spikes->ok() ? effect_mask_t( true )
                                                                               : effect_mask_t( true ).disable( 1 ) );
    parse_effects( spec.riposte );
    parse_effects( spec.thick_skin );
    parse_effects( mastery.fel_blood_rank_2 );
  }

  // Aldrachi Reaver
  parse_effects( buff.thrill_of_the_fight_attack_speed );

  // Fel-scarred
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
    } );
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

// Reporting functions ======================================================

void demon_hunter_t::analyze( sim_t& sim )
{
  player_t::analyze( sim );
  // TODO: Fix reporting for container spells
}

// Utility functions ========================================================

// Utility function to search spell data for matching effect.
// NOTE: This will return the FIRST effect that matches parameters.
const spelleffect_data_t* demon_hunter_t::find_spelleffect( const spell_data_t* spell, effect_subtype_t subtype,
                                                            int misc_value, const spell_data_t* affected,
                                                            effect_type_t type )
{
  for ( size_t i = 1; i <= spell->effect_count(); i++ )
  {
    const auto& eff = spell->effectN( i );

    if ( affected->ok() && !affected->affected_by_all( eff ) )
      continue;

    if ( eff.type() == type && ( eff.subtype() == subtype || subtype == A_MAX ) &&
         ( eff.misc_value1() == misc_value || misc_value == 0 ) )
    {
      return &eff;
    }
  }

  return &spelleffect_data_t::nil();
}

// Return the appropriate spell when `base` is overriden to another spell by `passive`
const spell_data_t* demon_hunter_t::find_spell_override( const spell_data_t* base, const spell_data_t* passive )
{
  if ( !passive->ok() || !base->ok() )
    return base;

  auto id = as<unsigned>( find_spelleffect( passive, A_OVERRIDE_ACTION_SPELL, base->id() )->base_value() );
  if ( !id )
    return base;

  return find_spell( id );
}

const spell_data_t* demon_hunter_t::find_spell_override( const spell_data_t* base,
                                                         std::vector<const spell_data_t*> passives )
{
  if ( !base->ok() )
    return base;

  for ( auto passive : passives )
  {
    if ( !passive->ok() )
      continue;

    auto id = as<unsigned>( find_spelleffect( passive, A_OVERRIDE_ACTION_SPELL, base->id() )->base_value() );
    if ( id )
      return find_spell( id );
  }

  return base;
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

using namespace unique_gear;
using namespace actions::spells;
using namespace actions::attacks;

namespace items
{
}  // end namespace items

// MODULE INTERFACE ==================================================

class demon_hunter_module_t : public module_t
{
public:
  demon_hunter_module_t() : module_t( DEMON_HUNTER )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
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

    hotfix::register_spell( "Demon Hunter", "2023-10-23", "Manually set secondary Felblade level requirement.", 213243 )
        .field( "spell_level" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 16.0 )
        .verification_value( 50.0 );
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
