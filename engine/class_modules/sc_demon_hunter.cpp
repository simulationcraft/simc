// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action/parse_effects.hpp"
#include "class_modules/apl/apl_demon_hunter.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"

#include <valarray>

#include "simulationcraft.hpp"

namespace
{
// UNNAMED NAMESPACE

// Forward Declarations
class demon_hunter_t;
struct soul_fragment_t;

namespace actions::attacks
{
struct relentless_onslaught_t;
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

    // Annihilator
  } dots;

  struct debuffs_t
  {
    // Shared

    // Devourer
    buff_t* devourers_bite;

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

  void target_demise();
};

constexpr unsigned MAX_SOUL_FRAGMENTS          = 6;
constexpr unsigned HAVOC_MAX_SOUL_FRAGMENTS    = 5;
constexpr unsigned DEVOURER_MAX_SOUL_FRAGMENTS = 10;
constexpr double VENGEFUL_RETREAT_DISTANCE     = 20.0;

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
  SIGIL_OF_DOOM,
  CONSUMING_FIRE,
  ABYSSAL_GAZE,
  ANNIHILATION,
  DEATH_SWEEP,
  ENTER_META
};

const std::vector<demonsurge_ability> demonsurge_abilities{
    demonsurge_ability::SIGIL_OF_DOOM, demonsurge_ability::CONSUMING_FIRE, demonsurge_ability::ABYSSAL_GAZE,
    demonsurge_ability::ANNIHILATION, demonsurge_ability::DEATH_SWEEP };

std::string demonsurge_ability_action_name( demonsurge_ability ability )
{
  switch ( ability )
  {
    case demonsurge_ability::SIGIL_OF_DOOM:
      return "demonsurge_sigil_of_doom";
    case demonsurge_ability::CONSUMING_FIRE:
      return "demonsurge_consuming_fire";
    case demonsurge_ability::ABYSSAL_GAZE:
      return "demonsurge_abyssal_gaze";
    case demonsurge_ability::ANNIHILATION:
      return "demonsurge_annihilation";
    case demonsurge_ability::DEATH_SWEEP:
      return "demonsurge_death_sweep";
    case demonsurge_ability::ENTER_META:
      return "demonsurge_enter_meta";
    default:
      return "demonsurge_unknown";
  }
}

std::string demonsurge_ability_proc_name( demonsurge_ability ability )
{
  switch ( ability )
  {
    case demonsurge_ability::SIGIL_OF_DOOM:
      return "Demonsurge: Sigil of Doom";
    case demonsurge_ability::CONSUMING_FIRE:
      return "Demonsurge: Consuming Fire";
    case demonsurge_ability::ABYSSAL_GAZE:
      return "Demonsurge: Abyssal Gaze";
    case demonsurge_ability::ANNIHILATION:
      return "Demonsurge: Annihilation";
    case demonsurge_ability::DEATH_SWEEP:
      return "Demonsurge: Death Sweep";
    case demonsurge_ability::ENTER_META:
      return "Demonsurge: Volatile Instinct";
    default:
      return "demonsurge_unknown";
  }
}

enum voidsurge_ability
{
  PREDATORS_WAKE,
  PIERCE_THE_VEIL,
  REAPERS_TOLL,
  VOLATILE_INSTINCT
};

const std::vector<voidsurge_ability> voidsurge_abilities{
    voidsurge_ability::REAPERS_TOLL, voidsurge_ability::PREDATORS_WAKE, voidsurge_ability::PIERCE_THE_VEIL };

std::string voidsurge_ability_action_name( voidsurge_ability ability )
{
  switch ( ability )
  {
    case voidsurge_ability::PREDATORS_WAKE:
      return "voidsurge_predators_wake";
    case voidsurge_ability::PIERCE_THE_VEIL:
      return "voidsurge_pierce_the_veil";
    case voidsurge_ability::REAPERS_TOLL:
      return "voidsurge_reapers_toll";
    case voidsurge_ability::VOLATILE_INSTINCT:
      return "voidsurge_volatile_instinct";
    default:
      return "voidsurge_unknown";
  }
}

std::string voidsurge_ability_proc_name( voidsurge_ability ability )
{
  switch ( ability )
  {
    case voidsurge_ability::PREDATORS_WAKE:
      return "Voidsurge: Predator's Wake";
    case voidsurge_ability::PIERCE_THE_VEIL:
      return "Voidsurge: Pierce the Veil";
    case voidsurge_ability::REAPERS_TOLL:
      return "Voidsurge: Reaper's Toll";
    case voidsurge_ability::VOLATILE_INSTINCT:
      return "Voidsurge: Volatile Instinct";
    default:
      return "Voidsurge: Unknown";
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
  attack_t* melee_main_hand;
  attack_t* melee_off_hand;

  double metamorphosis_health;  // Vengeance temp health from meta;
  double expected_max_health;

  // Soul Fragments
  unsigned next_fragment_spawn;  // determines whether the next fragment spawn
  // on the left or right
  auto_dispose<std::vector<soul_fragment_t*>> soul_fragments;
  event_t* soul_fragment_pick_up;

  double frailty_accumulator;  // Frailty healing accumulator
  event_t* frailty_driver;

  double feed_the_demon_accumulator;
  double shattered_destiny_accumulator;

  double wounded_quarry_accumulator;
  player_t* last_reavers_mark_applied;

  event_t* exit_melee_event;  // Event to disable melee abilities mid-VR.

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* demon_soul;
    buff_t* empowered_demon_soul;
    buff_t* immolation_aura;
    buff_t* metamorphosis;
    buff_t* soul_fragments;

    // Devourer
    buff_t* reap;
    buff_t* feast_of_souls;
    buff_t* eradicate;
    buff_t* moment_of_craving;
    buff_t* emptiness;
    buff_t* collapsing_star_stack;
    buff_t* collapsing_star;
    buff_t* void_metamorphosis_stack;
    buff_t* rolling_torment;
    buff_t* impending_apocalypse;
    buff_t* hungering_slash;
    buff_t* voidstep;
    buff_t* voidrush;
    buff_t* entropy_out_of_combat;
    buff_t* entropy_in_combat;

    // Havoc
    buff_t* blind_fury;
    buff_t* blur;
    buff_t* chaos_theory;
    buff_t* death_sweep;
    buff_t* furious_gaze;
    buff_t* inertia;
    buff_t* inertia_trigger;  // hidden buff that determines if we can trigger inertia
    buff_t* initiative;
    buff_t* inner_demon;
    buff_t* exergy;
    buff_t* out_of_range;
    buff_t* tactical_retreat;
    buff_t* unbound_chaos;
    buff_t* cycle_of_hatred;
    buff_t* empowered_eye_beam;
    buff_t* eternal_hunt;

    movement_buff_t* fel_rush_move;
    movement_buff_t* vengeful_retreat_move;
    movement_buff_t* metamorphosis_move;

    // Vengeance
    buff_t* demon_spikes;
    buff_t* painbringer;
    absorb_buff_t* soul_barrier;
    buff_t* felfire_fist_in_combat;
    buff_t* felfire_fist_out_of_combat;
    buff_t* untethered_rage;
    buff_t* seething_anger;
    buff_t* fiery_brand;

    // Aldrachi Reaver
    buff_t* reavers_glaive;
    buff_t* art_of_the_glaive;
    buff_t* glaive_flurry;
    buff_t* rending_strike;
    buff_t* warblades_hunger;
    buff_t* thrill_of_the_fight_haste;
    buff_t* thrill_of_the_fight_damage;
    buff_t* art_of_the_glaive_first;
    buff_t* art_of_the_glaive_second_rending_strike;
    buff_t* art_of_the_glaive_second_glaive_flurry;

    // Annihilator
    buff_t* voidfall_building;
    buff_t* voidfall_spending;
    buff_t* voidfall_final_hour;
    buff_t* dark_matter;
    buff_t* doomsayer_in_combat;
    buff_t* doomsayer_out_of_combat;

    // Scarred
    buff_t* monster_rising;
    buff_t* student_of_suffering;
    buff_t* enduring_torment;
    buff_t* pursuit_of_angryness;  // passive periodic updater buff
    std::unordered_map<demonsurge_ability, buff_t*> demonsurge_abilities;
    std::unordered_map<voidsurge_ability, buff_t*> voidsurge_abilities;
    buff_t* demonsurge_demonsurge;
    buff_t* demonsurge_demonic_intensity;
    buff_t* demonsurge;
    buff_t* voidsurge;
    buff_t* volatile_instinct;

    // Set Bonuses
  } buff;

  // Talents
  struct talents_t
  {
    struct class_talents_t
    {
      player_talent_t vengeful_retreat;
      player_talent_t felblade;
      player_talent_t voidblade;
      player_talent_t sigil_of_misery;  // No Implementation

      player_talent_t vengeful_bonds;  // No Implementation
      player_talent_t unrestrained_fury;
      player_talent_t shattered_restoration;
      player_talent_t improved_sigil_of_misery;  // No Implementation

      player_talent_t bouncing_glaives;
      player_talent_t imprison;           // No Implementation
      player_talent_t charred_warblades;  // NYI

      player_talent_t chaos_nova;
      player_talent_t void_nova;  // NYI
      player_talent_t improved_disrupt;
      player_talent_t consume_magic;
      player_talent_t aldrachi_design;

      player_talent_t focused_ire;  // No Implementation
      player_talent_t master_of_the_glaive;
      player_talent_t champion_of_the_glaive;
      player_talent_t disrupting_fury;
      player_talent_t blazing_path;
      player_talent_t swallowed_anger;
      player_talent_t aura_of_pain;
      player_talent_t live_by_the_glaive;  // NYI

      player_talent_t pursuit;
      player_talent_t soul_rending;
      player_talent_t felfire_haste;  // NYI
      player_talent_t infernal_armor;
      player_talent_t burn_it_out;       // No Implementation
      player_talent_t soul_cleanse;      // No Implementation
      player_talent_t lost_in_darkness;  // No Implementation

      player_talent_t illidari_knowledge;
      player_talent_t felbound;
      player_talent_t guile;
      player_talent_t will_of_the_illidari;  // NYI

      player_talent_t internal_struggle;
      player_talent_t furious;
      player_talent_t remorseless;
      player_talent_t first_in_last_out;  // NYI

      player_talent_t erratic_felheart;
      player_talent_t final_breath;
      player_talent_t darkness;      // No Implementation
      player_talent_t demon_muzzle;  // No Implementation
      player_talent_t soul_splitter;

      player_talent_t wings_of_wrath;  // No Implementation
      player_talent_t long_night;      // No Implementation
      player_talent_t pitch_black;     // No Implementation
      player_talent_t demonic_resilience;
    } demon_hunter;

    struct devourer_talents_t
    {
      player_talent_t void_ray;

      player_talent_t soul_immolation;
      player_talent_t predators_thirst;

      player_talent_t tempered_soul;
      player_talent_t spontaneous_immolation;
      player_talent_t void_metamorphosis;
      player_talent_t feast_of_souls;

      player_talent_t scythes_embrace;
      player_talent_t duty_eternal;
      player_talent_t collapsing_star;
      player_talent_t moment_of_craving;

      player_talent_t gift_of_the_void;
      player_talent_t entropy;
      player_talent_t waste_not;
      player_talent_t soulshaper;

      player_talent_t singed_spirit;
      player_talent_t sweet_suffering;
      player_talent_t second_helping;
      player_talent_t umbral_blade;
      player_talent_t improved_consume;
      player_talent_t sweet_release;
      player_talent_t voidpurge;

      player_talent_t hungering_slash;
      player_talent_t voidrage;
      player_talent_t focused_ray;

      player_talent_t soulforged_blades;
      player_talent_t singular_strikes;
      player_talent_t demonic_instinct;
      player_talent_t devourers_edge;
      player_talent_t voidglare_boon;
      player_talent_t rolling_torment;

      player_talent_t voidrush;  // Partial implementation
      player_talent_t devourers_bite;
      player_talent_t impending_apocalypse;
      player_talent_t calamitous;
      player_talent_t star_fragments;

      player_talent_t the_hunt;
      player_talent_t emptiness;
      player_talent_t soul_glutton;
      player_talent_t eradicate;

      player_talent_t midnight1;
      player_talent_t midnight2;
      player_talent_t midnight3;
    } devourer;

    struct havoc_talents_t
    {
      player_talent_t eye_beam;

      player_talent_t critical_chaos;
      player_talent_t burning_hatred;

      player_talent_t dash_of_chaos;  // NYI
      player_talent_t improved_chaos_strike;
      player_talent_t first_blood;
      player_talent_t accelerated_blade;
      player_talent_t demon_hide;

      player_talent_t desperate_instincts;  // No Implementation
      player_talent_t netherwalk;           // No Implementation
      player_talent_t deflecting_dance;     // No Implementation
      player_talent_t mortal_dance;         // No Implementation

      player_talent_t initiative;
      player_talent_t scars_of_suffering;
      player_talent_t demonic;
      player_talent_t furious_throws;
      player_talent_t trail_of_ruin;

      player_talent_t tactical_retreat;
      player_talent_t blind_fury;
      player_talent_t chaotic_transformation;
      player_talent_t dancing_with_fate;
      player_talent_t growing_inferno;

      player_talent_t exergy;
      player_talent_t inertia;
      player_talent_t isolated_prey;
      player_talent_t furious_gaze;
      player_talent_t serrated_glaive;  // Partially implemented
      player_talent_t burning_wound;

      player_talent_t unbound_chaos;
      player_talent_t chaos_theory;
      player_talent_t inner_demon;
      player_talent_t the_hunt;
      player_talent_t relentless_onslaught;
      player_talent_t soulscar;
      player_talent_t ragefire;

      player_talent_t know_your_enemy;
      player_talent_t cycle_of_hatred;
      player_talent_t chaotic_disposition;

      player_talent_t essence_break;
      player_talent_t glaive_tempest;
      player_talent_t shattered_destiny;
      player_talent_t collective_anguish;
      player_talent_t screaming_brutality;
      player_talent_t a_fire_inside;

      player_talent_t eternal_hunt_1;
      player_talent_t eternal_hunt_2;
      player_talent_t eternal_hunt_3;
    } havoc;

    struct vengeance_talents_t
    {
      player_talent_t fel_devastation;

      player_talent_t spirit_bomb;
      player_talent_t fiery_brand;

      player_talent_t perfectly_balanced_glaive;
      player_talent_t quickened_sigils;
      player_talent_t ascending_flame;

      player_talent_t tempered_steel;
      player_talent_t calcified_spikes;
      player_talent_t roaring_fire;      // No Implementation
      player_talent_t sigil_of_silence;  // Partial Implementation
      player_talent_t retaliation;
      player_talent_t felfire_fist;

      player_talent_t sigil_of_spite;
      player_talent_t agonizing_flames;
      player_talent_t feed_the_demon;
      player_talent_t burning_blood;
      player_talent_t revel_in_pain;

      player_talent_t frailty;
      player_talent_t feast_of_souls;
      player_talent_t fallout;
      player_talent_t ruinous_bulwark;  // No Implementation
      player_talent_t volatile_flameblood;
      player_talent_t soul_barrier;
      player_talent_t soul_sigils;
      player_talent_t fel_flame_fortification;  // No Implementation

      player_talent_t void_reaver;
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
      player_talent_t vengeful_beast;
      player_talent_t charred_flesh;

      player_talent_t soulcrush;
      player_talent_t soul_carver;
      player_talent_t last_resort;  // NYI
      player_talent_t darkglare_boon;
      player_talent_t down_in_flames;

      player_talent_t untethered_rage_1;
      player_talent_t untethered_rage_2;
      player_talent_t untethered_rage_3;
    } vengeance;

    struct aldrachi_reaver_talents_t
    {
      player_talent_t art_of_the_glaive;

      player_talent_t fury_of_the_aldrachi;
      player_talent_t evasive_action;  // No Implementation
      player_talent_t unhindered_assault;
      player_talent_t reavers_mark;
      player_talent_t broken_spirit;

      player_talent_t aldrachi_tactics;
      player_talent_t army_unto_oneself;     // No Implementation
      player_talent_t incorruptible_spirit;  // No Implementation
      player_talent_t wounded_quarry;
      player_talent_t keen_edge;

      player_talent_t incisive_blade;
      player_talent_t keen_engagement;
      player_talent_t preemptive_strike;
      player_talent_t bladecraft;
      player_talent_t warblades_hunger;

      player_talent_t thrill_of_the_fight;
    } aldrachi_reaver;

    struct annihilator_talents_t
    {
      player_talent_t voidfall;

      player_talent_t swift_erasure;
      player_talent_t meteoric_rise;
      player_talent_t catastrophe;
      player_talent_t phase_shift;

      player_talent_t path_to_oblivion;  // No Implementation
      player_talent_t state_of_matter;   // No Implementation
      player_talent_t mass_acceleration;
      player_talent_t doomsayer;
      player_talent_t harness_the_cosmos;
      player_talent_t celestial_echoes;

      player_talent_t final_hour;
      player_talent_t meteoric_fall;
      player_talent_t dark_matter;
      player_talent_t otherworldly_focus;

      player_talent_t world_killer;
    } annihilator;

    struct scarred_talents_t
    {
      player_talent_t demonsurge;

      player_talent_t wave_of_debilitation;  // No Implementation
      player_talent_t pursuit_of_angriness;
      player_talent_t focused_hatred;
      player_talent_t set_fire_to_the_pain;  // No Implementation
      player_talent_t improved_soul_rending;

      player_talent_t burning_blades;
      player_talent_t violent_transformation;
      player_talent_t enduring_torment;

      player_talent_t untethered_fury;
      player_talent_t student_of_suffering;
      player_talent_t flamebound;
      player_talent_t monster_rising;

      player_talent_t blind_focus;
      player_talent_t undying_embers;
      player_talent_t volatile_instinct;

      player_talent_t demonic_intensity;
    } scarred;
  } talent;

  // Spell Data
  struct spells_t
  {
    // Core Class Spells
    const spell_data_t* chaos_brand;
    const spell_data_t* disrupt;
    const spell_data_t* immolation_aura;
    const spell_data_t* throw_glaive;
    const spell_data_t* spectral_sight;

    // Class Passives
    const spell_data_t* all_demon_hunter;
    const spell_data_t* critical_strikes;
    const spell_data_t* immolation_aura_2;
    const spell_data_t* leather_specialization;

    // Background Spells
    const spell_data_t* demon_soul;
    const spell_data_t* demon_soul_empowered;
    const spell_data_t* felblade_damage;
    const spell_data_t* immolation_aura_damage;
    const spell_data_t* infernal_armor_damage;
    const spell_data_t* soul_fragment;

    // Cross-Expansion Override Spells
  } spell;

  // Specialization Spells
  struct spec_t
  {
    // General
    const spell_data_t* consume_soul;
    const spell_data_t* consume_soul_greater_energize;
    const spell_data_t* consume_soul_greater_heal;
    const spell_data_t* consume_soul_lesser_energize;
    const spell_data_t* consume_soul_lesser_heal;
    const spell_data_t* demonic_wards;
    const spell_data_t* metamorphosis;
    const spell_data_t* metamorphosis_buff;
    const spell_data_t* shattered_souls;
    const spell_data_t* the_hunt;
    const spell_data_t* the_hunt_impact;
    const spell_data_t* the_hunt_dot;
    const spell_data_t* blur;

    // Devourer
    const spell_data_t* devourer_demon_hunter;
    const spell_data_t* consume;
    const spell_data_t* consume_energize;
    const spell_data_t* devour;
    const spell_data_t* devour_energize;
    const spell_data_t* voidblade;
    const spell_data_t* soul_immolation_energize;
    const spell_data_t* reap;
    const spell_data_t* reap_damage;
    const spell_data_t* reap_energize;
    const spell_data_t* feast_of_souls_buff;
    const spell_data_t* devourers_bite_debuff;
    const spell_data_t* void_metamorphosis;
    const spell_data_t* void_metamorphosis_stack;
    const spell_data_t* cull;
    const spell_data_t* cull_damage;
    const spell_data_t* eradicate;
    const spell_data_t* eradicate_buff;
    const spell_data_t* eradicate_damage;
    const spell_data_t* eradicate_damage_meta;
    const spell_data_t* void_ray_tick;
    const spell_data_t* void_ray_tick_meta;
    const spell_data_t* moment_of_craving_buff;
    const spell_data_t* void_buildup;
    const spell_data_t* voidglare_boon_energize;
    const spell_data_t* collapsing_star_damage;
    const spell_data_t* collapsing_star_spell;
    const spell_data_t* collapsing_star_buff;
    const spell_data_t* collapsing_star_stacking_buff;
    const spell_data_t* emptiness_buff;
    const spell_data_t* impending_apocalypse_buff;
    const spell_data_t* hungering_slash;
    const spell_data_t* hungering_slash_buff;
    const spell_data_t* hungering_slash_damage;
    const spell_data_t* hungering_slash_energize;
    const spell_data_t* voidstep;
    const spell_data_t* rolling_torment_buff;
    const spell_data_t* rolling_torment_energize;

    // Havoc
    const spell_data_t* havoc_demon_hunter;
    const spell_data_t* annihilation;
    const spell_data_t* blade_dance;
    const spell_data_t* chaos_strike;
    const spell_data_t* death_sweep;
    const spell_data_t* fel_rush;
    const spell_data_t* demonic_appetite;

    const spell_data_t* blade_dance_2;
    const spell_data_t* burning_wound_debuff;
    const spell_data_t* chaos_strike_fury;
    const spell_data_t* chaos_strike_refund;
    const spell_data_t* chaos_theory_buff;
    const spell_data_t* demon_blades;
    const spell_data_t* demon_blades_damage;
    const spell_data_t* essence_break_debuff;
    const spell_data_t* eye_beam_damage;
    const spell_data_t* fel_rush_damage;
    const spell_data_t* first_blood_blade_dance_damage;
    const spell_data_t* first_blood_blade_dance_2_damage;
    const spell_data_t* first_blood_death_sweep_damage;
    const spell_data_t* first_blood_death_sweep_2_damage;
    const spell_data_t* furious_gaze_buff;
    const spell_data_t* glaive_tempest;
    const spell_data_t* glaive_tempest_damage;
    const spell_data_t* immolation_aura_3;
    const spell_data_t* initiative_buff;
    const spell_data_t* inner_demon_buff;
    const spell_data_t* inner_demon_damage;
    const spell_data_t* exergy_buff;
    const spell_data_t* inertia_trigger_buff;
    const spell_data_t* inertia_buff;
    const spell_data_t* ragefire_damage;
    const spell_data_t* soulscar_debuff;
    const spell_data_t* tactical_retreat_buff;
    const spell_data_t* unbound_chaos_buff;
    const spell_data_t* cycle_of_hatred_buff;
    const spell_data_t* furious_throws_damage;
    const spell_data_t* collective_anguish;
    const spell_data_t* collective_anguish_damage;
    const spell_data_t* essence_break_proc_damage;
    const spell_data_t* empowered_eye_beam_buff;
    const spell_data_t* empowered_eye_beam_damage;
    const spell_data_t* eternal_hunt_buff;

    // Vengeance
    const spell_data_t* vengeance_demon_hunter;
    const spell_data_t* demon_spikes;
    const spell_data_t* infernal_strike;
    const spell_data_t* soul_cleave;
    const spell_data_t* fracture;
    const spell_data_t* sigil_of_flame;
    const spell_data_t* sigil_of_flame_damage;
    const spell_data_t* sigil_of_flame_fury;

    const spell_data_t* fiery_brand_debuff;
    const spell_data_t* frailty_debuff;
    const spell_data_t* riposte;
    const spell_data_t* thick_skin;
    const spell_data_t* demon_spikes_buff;
    const spell_data_t* painbringer_buff;
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
    const spell_data_t* felfire_fist_in_combat_buff;
    const spell_data_t* felfire_fist_out_of_combat_buff;
    const spell_data_t* sigil_of_spite_damage;
    const spell_data_t* untethered_rage_buff;
    const spell_data_t* seething_anger_buff;
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
    const spell_data_t* thrill_of_the_fight_haste_buff;
    const spell_data_t* thrill_of_the_fight_damage_buff;
    double wounded_quarry_proc_rate;

    // Annihilator
    const spell_data_t* voidfall_meteor;
    const spell_data_t* voidfall_building_buff;
    const spell_data_t* voidfall_spending_buff;
    const spell_data_t* voidfall_final_hour_buff;
    const spell_data_t* catastrophe_dot;
    const spell_data_t* dark_matter_buff;
    const spell_data_t* meteor_shower_driver;
    const spell_data_t* meteor_shower_damage;
    const spell_data_t* doomsayer_in_combat_buff;
    const spell_data_t* doomsayer_out_of_combat_buff;
    const spell_data_t* world_killer;

    // Scarred
    const spell_data_t* burning_blades_debuff;
    const spell_data_t* enduring_torment_buff;
    const spell_data_t* monster_rising_buff;
    const spell_data_t* student_of_suffering_buff;
    const spell_data_t* demonsurge_demonsurge_buff;
    const spell_data_t* demonsurge_placeholder_buff;
    const spell_data_t* demonsurge_demonic_intensity_buff;
    const spell_data_t* demonsurge_trigger;
    const spell_data_t* demonsurge_damage;
    const spell_data_t* demonsurge_stacking_buff;
    const spell_data_t* voidsurge_trigger;
    const spell_data_t* voidsurge_damage;
    const spell_data_t* voidsurge_stacking_buff;
    const spell_data_t* sigil_of_doom;
    const spell_data_t* sigil_of_doom_damage;
    const spell_data_t* abyssal_gaze;
    const spell_data_t* predators_wake;
    const spell_data_t* pierce_the_veil;
    const spell_data_t* reapers_toll;
    const spell_data_t* volatile_instinct;
    const spell_data_t* demonsurge_meta_trigger;
  } hero_spec;

  // Set Bonus effects
  struct set_bonuses_t
  {
    // Devourer
    const spell_data_t* stars_fury;  // MID1 Devourer 4pc Energize

    // Havoc

    // Vengeance
    const spell_data_t* mid1_vengeance_4pc;
    const spell_data_t* explosion_of_the_soul;
    // Auxilliary
  } set_bonuses;

  // Mastery Spells
  struct mastery_t
  {
    // Devourer
    const spell_data_t* monster_within;
    // Havoc
    const spell_data_t* a_fire_inside;
    const spell_data_t* demonic_presence;
    // Vengeance
    const spell_data_t* fel_blood;
    const spell_data_t* fel_blood_rank_2;
  } mastery;

  // Cooldowns
  struct cooldowns_t
  {
    // General
    cooldown_t* sigil_of_spite;
    cooldown_t* felblade;
    cooldown_t* immolation_aura;
    cooldown_t* the_hunt;
    cooldown_t* sigil_of_flame;
    cooldown_t* sigil_of_misery;
    cooldown_t* metamorphosis;
    cooldown_t* throw_glaive;
    cooldown_t* vengeful_retreat;
    cooldown_t* soul_splitter_icd;

    // Devourer
    cooldown_t* consume;
    cooldown_t* reap;
    cooldown_t* voidblade;
    cooldown_t* void_ray;
    cooldown_t* soul_immolation;

    // Havoc
    cooldown_t* blade_dance;
    cooldown_t* chaos_strike_refund_icd;
    cooldown_t* eye_beam;
    cooldown_t* fel_rush;
    cooldown_t* relentless_onslaught_icd;
    cooldown_t* fel_rush_vengeful_retreat_movement_shared;
    cooldown_t* felblade_vengeful_retreat_movement_shared;
    target_specific_cooldown_t* essence_break_proc_icd;

    // Vengeance
    cooldown_t* demon_spikes;
    cooldown_t* spirit_bomb;
    cooldown_t* fel_devastation;
    cooldown_t* sigil_of_chains;
    cooldown_t* sigil_of_silence;
    cooldown_t* volatile_flameblood_icd;
    cooldown_t* explosion_of_the_soul_icd;

    // Aldrachi Reaver
    cooldown_t* art_of_the_glaive_consumption_icd;
    cooldown_t* wounded_quarry_trigger_icd;

    // Scarred
    cooldown_t* predators_wake;
  } cooldown;

  // Gains
  struct gains_t
  {
    // General
    gain_t* miss_refund;
    gain_t* immolation_aura;

    // Devourer
    gain_t* voidglare_boon;
    gain_t* void_buildup;

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

    // Scarred
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
    proc_t* soul_fragment_from_soul_splitter;
    proc_t* unhindered_assault;
    proc_t* soul_fragment_from_soul_sigils;
    proc_t* soul_fragment_from_death;
    proc_t* soul_fragment_expire;
    proc_t* soul_fragment_overflow;

    // Devourer
    proc_t* spontaneous_immolation;
    proc_t* void_metamorphosis_stack_from_entropy;
    proc_t* soul_fragment_from_consume;
    proc_t* soul_fragment_from_devour;
    proc_t* soul_fragment_from_soul_immolation;
    proc_t* soul_fragment_from_shattered_souls;
    proc_t* soul_fragment_from_star_fragments;
    proc_t* soul_fragment_from_hungering_slash;
    proc_t* soul_fragment_from_reapers_toll;
    proc_t* soul_fragment_from_void_metamorphosis;
    proc_t* soul_fragment_from_entropy;
    std::unordered_map<std::string, proc_t*> shattered_souls;

    // Havoc
    proc_t* soul_fragment_from_demonic_appetite;
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
    proc_t* untethered_rage;
    proc_t* soul_fragment_from_fracture;
    proc_t* soul_fragment_from_fracture_meta;
    proc_t* soul_fragment_from_sigil_of_spite;
    proc_t* soul_fragment_from_soul_carver;
    proc_t* soul_fragment_from_fallout;
    proc_t* feed_the_demon;

    // Aldrachi Reaver
    proc_t* soul_fragment_from_aldrachi_tactics;
    proc_t* soul_fragment_from_wounded_quarry;
    proc_t* soul_fragment_from_broken_spirit;
    proc_t* wounded_quarry_accumulator_reset;

    // Annihilator
    proc_t* soul_fragment_from_meteoric_rise;
    proc_t* soul_fragment_from_world_killer;
    proc_t* voidfall_from_builder;
    proc_t* voidfall_from_meteoric_rise;
    proc_t* voidfall_from_mass_acceleration;

    // Scarred
    std::unordered_map<demonsurge_ability, proc_t*> demonsurge_abilities;
    std::unordered_map<voidsurge_ability, proc_t*> voidsurge_abilities;
    proc_t* undying_embers;

    // Set Bonuses
  } proc;

  // RPPM objects
  struct rppms_t
  {
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
    heal_t* consume_soul_greater         = nullptr;
    heal_t* consume_soul_lesser          = nullptr;
    heal_t* consume_soul_greater_demon   = nullptr;
    heal_t* consume_soul_empowered_demon = nullptr;
    spell_t* immolation_aura_tick        = nullptr;
    spell_t* immolation_aura_initial     = nullptr;
    spell_t* collective_anguish          = nullptr;

    // Devourer
    spell_t* void_buildup = nullptr;

    // Havoc
    spell_t* burning_wound                                         = nullptr;
    attack_t* demon_blades                                         = nullptr;
    spell_t* inner_demon                                           = nullptr;
    spell_t* ragefire                                              = nullptr;
    actions::attacks::relentless_onslaught_t* relentless_onslaught = nullptr;
    action_t* soulscar                                             = nullptr;
    attack_t* screaming_brutality_blade_dance_throw_glaive         = nullptr;
    attack_t* screaming_brutality_death_sweep_throw_glaive         = nullptr;
    attack_t* screaming_brutality_slash_proc_throw_glaive          = nullptr;
    attack_t* essence_break_proc                                   = nullptr;
    spell_t* glaive_tempest                                        = nullptr;

    // Vengeance
    spell_t* infernal_armor = nullptr;
    spell_t* retaliation    = nullptr;
    heal_t* frailty_heal    = nullptr;

    // Aldrachi Reaver
    attack_t* art_of_the_glaive = nullptr;
    attack_t* preemptive_strike = nullptr;
    attack_t* warblades_hunger  = nullptr;
    attack_t* wounded_quarry    = nullptr;

    // Annihilator
    spell_t* voidfall_meteor = nullptr;
    spell_t* catastrophe     = nullptr;
    spell_t* meteor_shower   = nullptr;
    spell_t* world_killer    = nullptr;

    // Scarred
    action_t* burning_blades = nullptr;
    action_t* demonsurge     = nullptr;
    action_t* voidsurge      = nullptr;

    // Sigils
    spell_t* sigil_of_flame = nullptr;
    spell_t* sigil_of_spite = nullptr;
  } active;

  // Pets
  struct pets_t
  {
  } pets;

  // Options
  struct demon_hunter_options_t
  {
    double initial_fury = 0;
    // Reset fury to soft cap at start of fight
    bool reset_fury_on_pull = true;
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
    // How many seconds that Vengeful Retreat locks out Felblade
    double felblade_lockout_from_vengeful_retreat = 0.6;
    bool enable_dungeon_slice                     = false;
    double proc_from_killing_blow_chance          = 0.4;
    int entropy_starting_souls                    = -1;
    int channel_tick_cutoff_benefit               = 2;
  } options;

  demon_hunter_t( sim_t* sim, util::string_view name, race_e r );

  // overridden player_t init functions
  void activate() override;
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
  void init_blizzard_action_list() override;
  void init_items() override;
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  std::string aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const override;
  void init_finished() override;
  bool validate_fight_style( fight_style_e style ) const override;
  bool validate_actor() override;
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
  double composite_armor_multiplier() const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_critical_damage_multiplier( const action_state_t*, school_e ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;

  // overridden player_t combat functions
  void assess_damage( school_e, result_amount_type, action_state_t* s ) override;
  void combat_begin() override;
  const demon_hunter_td_t* find_target_data( const player_t* target ) const override;
  demon_hunter_td_t* get_target_data( player_t* target ) const override;
  void interrupt() override;
  void arise() override;
  void regen( timespan_t periodicity ) override;
  double resource_gain( resource_e, double, gain_t* source = nullptr, action_t* action = nullptr ) override;
  double resource_gain( resource_e, double, double, gain_t* source = nullptr, action_t* action = nullptr );
  double resource_loss( resource_e, double, gain_t* source = nullptr, action_t* action = nullptr ) override;
  void recalculate_resource_max( resource_e, gain_t* source = nullptr ) override;
  void reset() override;
  void merge( player_t& other ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void analyze( sim_t& sim ) override;

  // custom demon_hunter_t functions
  const spell_data_t* conditional_spell_lookup( bool fn, int id ) const;
  const spell_data_t* talent_spell_lookup( player_talent_t t, int id ) const;
  const spell_data_t* spec_talent_spell_lookup( specialization_e s, const player_talent_t& t, int id ) const;
  void set_out_of_range( timespan_t duration );
  void adjust_movement();
  double calculate_expected_max_health() const;
  unsigned consume_soul_fragments( soul_fragment = soul_fragment::ANY, bool instant = true,
                                   unsigned max = MAX_SOUL_FRAGMENTS );
  unsigned consume_nearby_soul_fragments( soul_fragment = soul_fragment::ANY );
  unsigned get_active_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
  unsigned get_inactive_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
  unsigned get_total_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
  void activate_soul_fragment( soul_fragment_t* );
  void spawn_soul_fragment( proc_t*, soul_fragment, unsigned = 1, bool = false );
  void trigger_demonic() const;
  void trigger_demonsurge( demonsurge_ability, bool = true );
  void trigger_demonsurge( demonsurge_ability, timespan_t, bool = true );
  void trigger_voidsurge( voidsurge_ability, bool = true );
  void trigger_voidsurge( voidsurge_ability, timespan_t, bool = true );
  double get_target_reach() const
  {
    return options.target_reach >= 0 ? options.target_reach : sim->target->combat_reach;
  }
  void parse_player_effects();
  unsigned max_soul_fragments() const;

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

  struct fury_state_t final
  {
    timespan_t start_time;
    event_t* next_drain_event;
    int drain_stacks;
    demon_hunter_t* actor;
    double meta_drain_multiplier = 1.0;
    double initial_drain         = 15.0;
    double exp_factor            = 1.455;
    double exp_power             = 0.075;

    fury_state_t( demon_hunter_t* a )
      : start_time( timespan_t::min() ), next_drain_event( nullptr ), drain_stacks( 0 ), actor( a )
    {
    }

    void init()
    {
      if ( dh()->talent.devourer.soul_glutton.enabled() )
      {
        // Why separate multiplier - Easier to find stuff in the future.
        meta_drain_multiplier /= 1 + dh()->talent.devourer.soul_glutton->effectN( 2 ).percent();
      }

      dh()->sim->print_debug( "{} applying overall drain multiplier of {} to initial drain of {}, result: {}", *dh(),
                              meta_drain_multiplier, initial_drain, initial_drain * meta_drain_multiplier );

      dh()->sim->print_debug( "{} applying overall drain multiplier of {} to exp_factor of {}, result: {}", *dh(),
                              meta_drain_multiplier, exp_factor, exp_factor * meta_drain_multiplier );

      // Handled here to do less runtime math later - If formula changes, may need to move Drain Multiplier into the
      // calculation.
      initial_drain *= meta_drain_multiplier;
      exp_factor *= meta_drain_multiplier;
    }

    void clear_state();

    void stop();

    void reschedule_drain();

    void reset();

    demon_hunter_t* dh()
    {
      return actor;
    }

    demon_hunter_t* dh() const
    {
      return actor;
    }

    timespan_t time_to_next_tick( int stacks ) const;

    double base_fury_drain_per_second( int stacks ) const
    {
      return initial_drain + exp_factor * exp( exp_power * stacks );
    }

    double fury_drain_per_second( int stacks ) const;

    void drain();

    void start();

    void schedule_tick();

    struct drain_event_t : public event_t
    {
      demon_hunter_t* dh;
      timespan_t delta;
      drain_event_t( demon_hunter_t* p, timespan_t delta_time )
        : event_t( *p, delta_time ), dh( p ), delta( delta_time )
      {
      }

      const char* name() const override
      {
        return "Void-Buildup";
      }

      void execute() override
      {
        // Nullptr the event here as we know it is no longer needed. This guarantees it is not touched in any code by
        // accident
        dh->devourer_fury_state.next_drain_event = nullptr;
        dh->devourer_fury_state.drain();
      }
    };

  } devourer_fury_state;

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

  if ( dh->specialization() == DEMON_HUNTER_DEVOURER )
    return buff_t::trigger( s, v, c, d );

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

// Undying Embers Event definition ==========================================

struct undying_embers_event_t : public event_t
{
  action_t* action;
  action_state_t* state;

  undying_embers_event_t( demon_hunter_t* player, action_t* action, action_state_t* state )
    : event_t( *player, timespan_t::min() ), action( action ), state( state )
  {
  }

  ~undying_embers_event_t()
  {
    if ( state )
      action_state_t::release( state );
  }

  const char* name() const override
  {
    return "undying_embers_event";
  }

  void execute() override
  {
    action->trigger_dot( state );
    action_state_t::release( state );
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
    double velocity = dh->spec.consume_soul->missile_speed();
    if ( ( activation && consume_on_activation ) || velocity == 0 )
      return timespan_t::zero();

    if ( activation )
    {
      switch ( dh->specialization() )
      {
        case DEMON_HUNTER_DEVOURER:
          // 2025-10-08 -- Recent testing appears to show a roughly 0.23s activation time for Vengeance
          //               with some slight variance
          return dh->rng().gauss<230, 70>();
        case DEMON_HUNTER_HAVOC:
          // 2023-06-26 -- Recent testing appears to show a roughly fixed 1s activation time for Havoc
          return 1_s;
        case DEMON_HUNTER_VENGEANCE:
          // 2024-02-12 -- Recent testing appears to show a roughly 0.76s activation time for Vengeance
          //               with some slight variance
          return dh->rng().gauss<760, 120>();
        default:
          // cause it to fall down to velocity check
          break;
      }
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
    double distance = 0;
    double dist;
    switch ( dh->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        distance = 4.6066;
        break;
      case DEMON_HUNTER_HAVOC:
        distance = 4.6066;
        break;
      case DEMON_HUNTER_VENGEANCE:
        distance = 10.6066;
        break;
      default:
        break;
    }

    if ( is_type( soul_fragment::EMPOWERED_DEMON ) )
    {
      dist = 6.5;
      x    = dh->x_position;
      y    = dh->y_position;
    }
    else
    {
      x = dh->x_position + ( dh->next_fragment_spawn % 2 ? -distance : distance );
      y = dh->y_position + distance;

      // Calculate random offset, 2-5 yards from the base position.
      double r_min = 2.0;
      double r_max = 5.0;
      // Nornmalizing factor
      double a = 2.0 / ( r_max * r_max - r_min * r_min );
      // Calculate distance from origin using power-law distribution for
      // uniformity.
      dist = sqrt( 2.0 * dh->rng().real() / a + r_min * r_min );
    }
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

  void consume( bool instant = false )
  {
    assert( active() );
    timespan_t delay = get_travel_time();

    action_t* consume_action = nullptr;
    switch ( type )
    {
      case soul_fragment::EMPOWERED_DEMON:
        consume_action = dh->active.consume_soul_empowered_demon;
        break;
      case soul_fragment::GREATER_DEMON:
        consume_action = dh->active.consume_soul_greater_demon;
        break;
      case soul_fragment::LESSER:
        consume_action = dh->active.consume_soul_lesser;
        break;
      case soul_fragment::GREATER:
        consume_action = dh->active.consume_soul_greater;
        break;
      default:
        break;
    }
    assert( consume_action != nullptr );

    if ( instant || delay == 0_s )
    {
      consume_action->execute();
    }
    else
    {
      make_event<delayed_execute_event_t>( *dh->sim, dh, consume_action, dh, delay );
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
    // General

    // Havoc
    affect_flags a_fire_inside;
    affect_flags demonic_presence;

    bool chaos_theory        = false;
    bool chaotic_disposition = false;

    // Aldrachi Reaver
    bool reavers_mark = false;
  } affected_by;

  // This action will trigger on every execute -- does not behave identically to execute_action
  action_t* execute_energize_action;
  // This action will trigger on every impact -- does not behave identically to impact_action
  action_t* impact_energize_action;
  // This action will trigger on every tick -- does not behave identically to tick_action
  action_t* tick_energize_action;

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
          default:
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
      cd_wasted_iter( nullptr ),
      execute_energize_action( nullptr ),
      impact_energize_action( nullptr ),
      tick_energize_action( nullptr )
  {
    ab::parse_options( o );

    switch ( p->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        break;
      case DEMON_HUNTER_HAVOC:
        // Affect Flags
        parse_affect_flags( p->mastery.a_fire_inside, affected_by.a_fire_inside );
        parse_affect_flags( p->mastery.demonic_presence, affected_by.demonic_presence );

        if ( p->talent.havoc.chaos_theory->ok() )
        {
          affected_by.chaos_theory = ab::data().affected_by( p->spec.chaos_theory_buff->effectN( 1 ) );
        }

        if ( p->talent.havoc.chaotic_disposition->ok() )
        {
          uint32_t mask                   = dbc::get_school_mask( SCHOOL_CHROMATIC );
          affected_by.chaotic_disposition = ( dbc::get_school_mask( ab::school ) & mask ) == mask;
        }
        break;
      case DEMON_HUNTER_VENGEANCE:
        // Affect Flags
        break;
      default:
        break;
    }

    // Aldrachi Reaver
    if ( p->talent.aldrachi_reaver.reavers_mark->ok() )
    {
      affected_by.reavers_mark = ab::data().affected_by( p->hero_spec.reavers_mark->effectN( 1 ) );
    }
  }

  demon_hunter_t* dh()
  {
    return static_cast<demon_hunter_t*>( ab::player );
  }

  const demon_hunter_t* dh() const
  {
    return static_cast<const demon_hunter_t*>( ab::player );
  }

  demon_hunter_td_t* td( player_t* t ) const
  {
    return dh()->get_target_data( t );
  }

  void init() override
  {
    ab::init();

    apply_buff_effects();
    apply_debuff_effects();

    if ( track_cd_waste )
    {
      cd_wasted_exec =
          dh()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, dh()->cd_waste_exec );
      cd_wasted_cumulative = dh()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>(
          ab::name_str, dh()->cd_waste_cumulative );
      cd_wasted_iter =
          dh()->template get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, dh()->cd_waste_iter );
    }

    // Make sure background is set for triggered actions.
    // Leads to double-readying of the player otherwise.
    assert( ( !execute_energize_action || execute_energize_action->background ) &&
            "Execute energize action needs to be set to background." );
    assert( ( !tick_energize_action || tick_energize_action->background ) &&
            "Tick energize action needs to be set to background." );
    assert( ( !impact_energize_action || impact_energize_action->background ) &&
            "Impact energize action needs to be set to background." );
  }

  // Intended for parsing effects from buffs that have an allowlist of abilities
  // that they affect.
  void apply_buff_effects()
  {
    // Shared
    ab::parse_effects( dh()->buff.demon_soul );
    ab::parse_effects( dh()->buff.empowered_demon_soul );
    ab::parse_effects( dh()->mastery.monster_within, [ this ]( double v ) {
      if ( dh()->buff.enduring_torment->check() )
        v *= 1.0 + dh()->buff.enduring_torment->check_value();
      return v;
    } );

    effect_mask_t meta_mask = effect_mask_t( true );
    if ( dh()->specialization() == DEMON_HUNTER_VENGEANCE && !dh()->talent.vengeance.vengeful_beast->ok() )
    {
      meta_mask.disable( 14 );
    }
    ab::parse_effects( dh()->buff.metamorphosis, dh()->mastery.monster_within, meta_mask );

    // Devourer
    ab::parse_effects( dh()->buff.feast_of_souls );
    ab::parse_effects( dh()->buff.rolling_torment );
    ab::parse_effects( dh()->buff.impending_apocalypse );

    // Havoc
    ab::parse_effects( dh()->buff.exergy );
    ab::parse_effects( dh()->buff.inertia );
    ab::parse_effects( dh()->buff.empowered_eye_beam );

    // Vengeance

    // Aldrachi Reaver
    ab::parse_effects( dh()->buff.warblades_hunger );
    ab::parse_effects( dh()->buff.thrill_of_the_fight_damage );
    // Art of the Glaive empowered ability buffs GO
    std::vector<int> art_of_the_glaive_affected_list = {};
    auto art_of_the_glaive = [ &art_of_the_glaive_affected_list, this ]( int idx, buff_t* buff ) {
      auto in_whitelist = [ &art_of_the_glaive_affected_list, this ]() {
        return std::find( art_of_the_glaive_affected_list.begin(), art_of_the_glaive_affected_list.end(), ab::id ) !=
               art_of_the_glaive_affected_list.end();
      };

      if ( const auto& effect = dh()->talent.aldrachi_reaver.art_of_the_glaive->effectN( idx );
           effect.ok() && in_whitelist() )
      {
        auto added_entry = add_parse_entry( ab::da_multiplier_effects )
                               .set_buff( buff )
                               .set_value( effect.percent() )
                               .set_eff( &effect )
                               .print_debug( this );
      }
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
    art_of_the_glaive( 3, dh()->buff.art_of_the_glaive_first );
    // Second empowered
    art_of_the_glaive_affected_list = art_of_the_glaive_glaive_flurry_affected_list;
    art_of_the_glaive( 4, dh()->buff.art_of_the_glaive_second_glaive_flurry );
    art_of_the_glaive_affected_list = art_of_the_glaive_rending_strike_affected_list;
    art_of_the_glaive( 4, dh()->buff.art_of_the_glaive_second_rending_strike );
    // End Art of the Glaive bullshittery

    // Scarred
    ab::parse_effects( dh()->buff.enduring_torment );
    ab::parse_effects( dh()->buff.demonsurge_demonsurge );
    ab::parse_effects( dh()->buff.demonsurge_demonic_intensity );
    ab::parse_effects( dh()->buff.demonsurge );
    ab::parse_effects( dh()->buff.voidsurge );

    ab::parse_effects( dh()->talent.scarred.blind_focus, [ this ]( double v ) {
      if ( dh()->buff.metamorphosis->check() )
      {
        if ( dh()->specialization() == DEMON_HUNTER_DEVOURER )
        {
          v *= 1.0 + dh()->spec.void_metamorphosis->effectN( 16 ).percent();
        }
        else
        {
          v *= 1.0 + dh()->spec.metamorphosis_buff->effectN( 13 ).percent();
        }
      }
      return v;
    } );

    // Tier sets
  }

  void apply_debuff_effects()
  {
    // Shared

    // Devourer
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::devourers_bite ), dh()->spec.devourers_bite_debuff );

    // Havoc
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::burning_wound ), dh()->spec.burning_wound_debuff );
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::serrated_glaive ),
                              dh()->talent.havoc.serrated_glaive->effectN( 1 ).trigger(),
                              dh()->talent.havoc.serrated_glaive );

    // Vengeance
    if ( dh()->talent.vengeance.vulnerability->ok() )
    {
      ab::parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::frailty ), dh()->spec.frailty_debuff,
                                effect_mask_t( false ).enable( 4, 5 ),
                                dh()->talent.vengeance.vulnerability->effectN( 1 ).percent() +
                                    dh()->talent.vengeance.soulcrush->effectN( 4 ).percent() );
    }

    // Vengeance Demon Hunter's DF S2 tier set spell data is baked into Fiery Brand's spell data at effect #4.
    // We exclude parsing effect #4 as that tier set is no longer active.
    ab::parse_target_effects( d_fn( &demon_hunter_td_t::dots_t::fiery_brand ), dh()->spec.fiery_brand_debuff,
                              effect_mask_t( true ).disable( 4 ) );

    // Aldrachi Reaver

    // Scarred
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
      m *= 1.0 + dh()->cache.mastery_value();
    }

    if ( affected_by.a_fire_inside.direct )
    {
      m *= 1.0 + dh()->cache.mastery_value();
    }

    if ( affected_by.chaotic_disposition )
    {
      double chance         = dh()->talent.havoc.chaotic_disposition->effectN( 2 ).percent() / 100;
      size_t rolls          = as<size_t>( dh()->talent.havoc.chaotic_disposition->effectN( 1 ).base_value() );
      double damage_percent = dh()->talent.havoc.chaotic_disposition->effectN( 3 ).percent();

      double da = 0;
      for ( size_t i = 0; i < rolls; i++ )
      {
        if ( dh()->rng().roll( chance ) )
        {
          da += damage_percent;
        }
      }
      m *= 1.0 + da;
    }

    return m;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = ab::composite_target_da_multiplier( t );

    demon_hunter_td_t* target_data = td( t );
    if ( affected_by.reavers_mark && target_data->debuffs.reavers_mark->up() )
    {
      m *= 1.0 + target_data->debuffs.reavers_mark->check_stack_value();
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_ta_multiplier( s );

    if ( affected_by.demonic_presence.periodic )
    {
      m *= 1.0 + dh()->cache.mastery_value();
    }

    if ( affected_by.a_fire_inside.periodic )
    {
      m *= 1.0 + dh()->cache.mastery_value();
    }

    if ( affected_by.chaotic_disposition )
    {
      double chance         = dh()->talent.havoc.chaotic_disposition->effectN( 2 ).percent() / 100;
      size_t rolls          = as<size_t>( dh()->talent.havoc.chaotic_disposition->effectN( 1 ).base_value() );
      double damage_percent = dh()->talent.havoc.chaotic_disposition->effectN( 3 ).percent();

      double da = 0;
      for ( size_t i = 0; i < rolls; i++ )
      {
        if ( dh()->rng().roll( chance ) )
        {
          da += damage_percent;
        }
      }
      m *= 1.0 + da;
    }

    return m;
  }

  double composite_target_ta_multiplier( player_t* t ) const override
  {
    double m = ab::composite_target_ta_multiplier( t );

    demon_hunter_td_t* target_data = td( t );
    if ( affected_by.reavers_mark && target_data->debuffs.reavers_mark->up() )
    {
      m *= 1.0 + target_data->debuffs.reavers_mark->check_stack_value();
    }

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = ab::composite_crit_chance();

    if ( affected_by.chaos_theory && dh()->buff.chaos_theory->up() )
    {
      const double bonus = dh()->rng().range( dh()->talent.havoc.chaos_theory->effectN( 1 ).percent(),
                                              dh()->talent.havoc.chaos_theory->effectN( 2 ).percent() );
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
      ea = static_cast<int>( ea + dh()->rng().range( 0, 1 + energize_delta ) - ( energize_delta / 2.0 ) );
    }

    return ea;
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    accumulate_frailty( d->state );

    if ( tick_energize_action )
    {
      tick_energize_action->execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::result_is_hit( s->result ) )
    {
      accumulate_frailty( s );
      trigger_chaos_brand( s );
      trigger_initiative( s );

      if ( impact_energize_action )
      {
        impact_energize_action->execute();
      }
    }
  }

  void execute() override
  {
    ab::execute();

    if ( execute_energize_action )
    {
      execute_energize_action->execute();
    }
  }

  bool ready() override
  {
    if ( ( ab::execute_time() > timespan_t::zero() || ab::channeled ) && dh()->buff.out_of_range->check() )
    {
      return false;
    }

    if ( dh()->buff.out_of_range->check() && ab::range <= 5.0 )
    {
      return false;
    }

    if ( dh()->buff.fel_rush_move->check() )
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
      if ( dh()->sim->debug )
      {
        dh()->sim->out_debug.printf( "%s %s cooldown waste tracking waste=%.3f exec_time=%.3f", dh()->name(),
                                     ab::name(), time_, ab::time_to_execute.total_seconds() );
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
      dh()->resource_gain( ab::resource_current, ab::last_resource_cost * 0.80, dh()->gain.miss_refund );
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
      if ( dh()->talent.havoc.shattered_destiny->ok() && dh()->buff.metamorphosis->check() )
      {
        // 2024-07-12 -- If a cast costs Fury, it seems to use the base cost instead of the actual cost.
        resource_e cr  = ab::current_resource();
        const auto& bc = ab::base_costs[ cr ];
        auto base      = bc.base;

        dh()->shattered_destiny_accumulator += base;
        const double threshold = dh()->talent.havoc.shattered_destiny->effectN( 2 ).base_value();
        while ( dh()->shattered_destiny_accumulator >= threshold )
        {
          dh()->shattered_destiny_accumulator -= threshold;
          dh()->buff.metamorphosis->extend_duration( dh()->talent.havoc.shattered_destiny->effectN( 1 ).time_value() );
          dh()->proc.shattered_destiny->occur();
        }
      }

      if ( dh()->talent.vengeance.feed_the_demon->ok() )
      {
        resource_e cr  = ab::current_resource();
        const auto& bc = ab::base_costs[ cr ];
        auto base      = bc.base;

        dh()->feed_the_demon_accumulator += base;

        const double threshold = dh()->talent.vengeance.feed_the_demon->effectN( 1 ).base_value();
        while ( dh()->feed_the_demon_accumulator >= threshold )
        {
          dh()->feed_the_demon_accumulator -= threshold;
          dh()->cooldown.demon_spikes->adjust( -dh()->talent.vengeance.feed_the_demon->effectN( 2 ).time_value() );
          dh()->proc.feed_the_demon->occur();
        }
      }
    }
  }

  void accumulate_frailty( action_state_t* s )
  {
    if ( !dh()->talent.vengeance.frailty->ok() )
      return;

    if ( !( ab::harmful && s->result_amount > 0 ) )
      return;

    const demon_hunter_td_t* target_data = td( s->target );
    if ( !target_data->debuffs.frailty->up() )
      return;

    const double multiplier = target_data->debuffs.frailty->stack_value();
    dh()->frailty_accumulator += s->result_amount * multiplier;
  }

  void trigger_chaos_brand( action_state_t* s )
  {
    if ( ab::sim->overrides.chaos_brand )
      return;

    if ( ab::result_is_miss( s->result ) || s->result_amount == 0.0 )
      return;

    if ( s->target->debuffs.chaos_brand && dh()->spell.chaos_brand->ok() )
    {
      s->target->debuffs.chaos_brand->trigger();
    }
  }

  void trigger_initiative( action_state_t* s )
  {
    if ( ab::result_is_miss( s->result ) || s->result_amount == 0.0 )
      return;

    if ( !dh()->talent.havoc.initiative->ok() )
      return;

    if ( !s->target->is_enemy() || td( s->target )->debuffs.initiative_tracker->check() )
      return;

    dh()->buff.initiative->trigger();
    td( s->target )->debuffs.initiative_tracker->trigger();
  }

  bool trigger_untethered_rage( const int souls_consumed )
  {
    if ( souls_consumed <= 0 || !dh()->talent.vengeance.untethered_rage_1->ok() )
      return false;

    // TODO: base chance and growth factor are approximations, no spell data source
    double chance_to_proc = souls_consumed * 0.0075 * pow( 1.35, dh()->buff.seething_anger->stack() );
    if ( ab::rng().roll( chance_to_proc ) )
    {
      dh()->buff.seething_anger->expire();
      dh()->buff.untethered_rage->trigger();
      dh()->proc.untethered_rage->occur();
      return true;
    }

    if ( !dh()->buff.untethered_rage->up() && dh()->talent.vengeance.untethered_rage_3->ok() )
    {
      dh()->buff.seething_anger->trigger();
    }

    return false;
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

    BASE::dh()->trigger_demonsurge( ABILITY );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "demonsurge_available" ) )
    {
      if ( BASE::dh()->talent.scarred.demonsurge->ok() )
      {
        return make_fn_expr( name, [ this ]() { return BASE::dh()->buff.demonsurge_abilities[ ABILITY ]->check(); } );
      }
      return expr_t::create_constant( name, 0 );
    }

    return BASE::create_expression( name );
  }
};

template <voidsurge_ability ABILITY, typename BASE>
struct voidsurge_trigger_t : public BASE
{
  using base_t = voidsurge_trigger_t<ABILITY, BASE>;

  template <typename... Args>
  voidsurge_trigger_t( Args&&... args ) : BASE( std::forward<Args>( args )... )
  {
  }

  void execute() override
  {
    BASE::execute();

    BASE::dh()->trigger_voidsurge( ABILITY );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "voidsurge_available" ) )
    {
      if ( BASE::dh()->talent.scarred.demonsurge->ok() )
      {
        return make_fn_expr( name, [ this ]() { return BASE::dh()->buff.voidsurge_abilities[ ABILITY ]->check(); } );
      }
      return expr_t::create_constant( name, 0 );
    }

    return BASE::create_expression( name );
  }
};

template <art_of_the_glaive_ability ABILITY, typename BASE>
struct art_of_the_glaive_trigger_t : public BASE
{
  using base_t = art_of_the_glaive_trigger_t<ABILITY, BASE>;

  timespan_t thrill_delay;

  art_of_the_glaive_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o ),
      // 2025-07-18 -- Death sweep triggers this with the same delay as other abilities, also proccing the second
      // art of the glaive buff early before the final hit of death sweep
      thrill_delay( 700_ms )
  {
  }

  void execute() override
  {
    BASE::execute();

    bool second_ability = false;
    if ( ABILITY == art_of_the_glaive_ability::GLAIVE_FLURRY &&
         BASE::dh()->talent.aldrachi_reaver.fury_of_the_aldrachi->ok() && BASE::dh()->buff.glaive_flurry->up() )
    {
      second_ability = !BASE::dh()->buff.rending_strike->up();

      if ( BASE::dh()->talent.aldrachi_reaver.fury_of_the_aldrachi->ok() )
      {
        BASE::dh()->active.art_of_the_glaive->execute_on_target( BASE::target );
      }

      BASE::dh()->buff.glaive_flurry->expire();
      make_event( *BASE::dh()->sim, thrill_delay, [ this, second_ability ] {
        // 2025-07-19 -- Using glaive flurry as havoc causes all subsequent death sweeps and blade dances to
        // gain the first art of the glaive buff, even after all related reavers glaives buffs expire until
        // either chaos strike of annihilation are used.
        if ( BASE::dh()->specialization() != DEMON_HUNTER_HAVOC )
        {
          BASE::dh()->buff.art_of_the_glaive_first->expire();
        }
        BASE::dh()->buff.art_of_the_glaive_second_glaive_flurry->expire();
        BASE::dh()->buff.art_of_the_glaive_second_rending_strike->expire();
        if ( !second_ability )
        {
          BASE::dh()->buff.art_of_the_glaive_second_rending_strike->trigger();
        }
      } );
    }
    else if ( ABILITY == art_of_the_glaive_ability::RENDING_STRIKE &&
              BASE::dh()->talent.aldrachi_reaver.fury_of_the_aldrachi->ok() && BASE::dh()->buff.rending_strike->up() )
    {
      second_ability = !BASE::dh()->buff.glaive_flurry->up();

      int first_ability_amount = 1;
      int second_ability_amount =
          1 + as<int>( BASE::dh()->talent.aldrachi_reaver.reavers_mark->effectN( 2 ).base_value() );
      if ( BASE::dh()->talent.aldrachi_reaver.reavers_mark->ok() )
      {
        BASE::td( BASE::target )
            ->debuffs.reavers_mark->trigger( second_ability ? second_ability_amount : first_ability_amount );
      }
      // 2025-07-19 -- Consuming rending strike as havoc during a death sweep before the final hit causes the
      // final death sweep hit to also gain the second art of the glaive damage buff. This applies that buff
      // temporarily during the rending strike even to ensure the final death sweep hit also gains the buff.
      if ( ( BASE::dh()->specialization() == DEMON_HUNTER_HAVOC ) && second_ability )
      {
        BASE::dh()->buff.art_of_the_glaive_first->expire();
        BASE::dh()->buff.art_of_the_glaive_second_glaive_flurry->trigger();
      }

      BASE::dh()->buff.rending_strike->expire();
      make_event( *BASE::dh()->sim, thrill_delay, [ this, second_ability ] {
        BASE::dh()->buff.art_of_the_glaive_first->expire();
        BASE::dh()->buff.art_of_the_glaive_second_glaive_flurry->expire();
        BASE::dh()->buff.art_of_the_glaive_second_rending_strike->expire();
        if ( !second_ability )
        {
          BASE::dh()->buff.art_of_the_glaive_second_glaive_flurry->trigger();
        }
      } );
    }

    if ( second_ability )
    {
      if ( BASE::dh()->talent.aldrachi_reaver.thrill_of_the_fight->ok() )
      {
        BASE::dh()->buff.thrill_of_the_fight_haste->trigger();

        make_event( *BASE::dh()->sim, thrill_delay,
                    [ this ] { BASE::dh()->buff.thrill_of_the_fight_damage->trigger(); } );
      }
      if ( BASE::dh()->talent.aldrachi_reaver.aldrachi_tactics->ok() )
      {
        BASE::dh()->spawn_soul_fragment( BASE::dh()->proc.soul_fragment_from_aldrachi_tactics, soul_fragment::LESSER );
      }
    }
  }
};

template <typename BASE>
struct exergy_trigger_t : public BASE
{
  using base_t = exergy_trigger_t<BASE>;

  exergy_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o )
  {
  }

  virtual bool can_trigger_exergy()
  {
    return BASE::dh()->talent.havoc.exergy->ok();
  }

  void execute() override
  {
    if ( can_trigger_exergy() )
      BASE::dh()->buff.exergy->trigger();

    BASE::execute();
  }
};

template <typename BASE>
struct inertia_trigger_trigger_t : public BASE
{
  using base_t = inertia_trigger_trigger_t<BASE>;

  inertia_trigger_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o )
  {
  }

  virtual bool can_trigger_inertia_trigger()
  {
    return BASE::dh()->talent.havoc.inertia->ok();
  }

  void execute() override
  {
    BASE::execute();

    if ( can_trigger_inertia_trigger() )
      BASE::dh()->buff.inertia_trigger->trigger();
  }
};

template <typename BASE>
struct inertia_trigger_t : public BASE
{
  using base_t = inertia_trigger_t<BASE>;

  inertia_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o )
  {
  }

  virtual bool can_trigger_inertia()
  {
    return BASE::dh()->talent.havoc.inertia->ok() && BASE::dh()->buff.inertia_trigger->up();
  }

  void execute() override
  {
    BASE::execute();

    if ( can_trigger_inertia() )
    {
      BASE::dh()->buff.inertia_trigger->expire();
      BASE::dh()->buff.inertia->trigger();
    }
  }
};

template <typename BASE>
struct unbound_chaos_trigger_t : public BASE
{
  using base_t = unbound_chaos_trigger_t<BASE>;

  unbound_chaos_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o )
  {
  }

  virtual bool can_trigger_unbound_chaos()
  {
    return BASE::dh()->talent.havoc.unbound_chaos->ok();
  }

  void execute() override
  {
    BASE::execute();

    if ( can_trigger_unbound_chaos() )
    {
      BASE::dh()->buff.unbound_chaos->trigger();
    }
  }
};

template <typename BASE>
struct wounded_quarry_accumulator_t : public BASE
{
  using base_t = wounded_quarry_accumulator_t<BASE>;

  wounded_quarry_accumulator_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : BASE( n, p, s, o )
  {
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( !BASE::dh()->talent.aldrachi_reaver.wounded_quarry->ok() || !BASE::dh()->active.wounded_quarry )
      return;

    if ( BASE::school != SCHOOL_PHYSICAL )
      return;

    if ( s->target->is_sleeping() )
      return;

    if ( !BASE::dh()->last_reavers_mark_applied ||
         !BASE::dh()->get_target_data( BASE::dh()->last_reavers_mark_applied )->debuffs.reavers_mark->up() )
      return;

    double da = s->result_amount;
    if ( da > 0 )
    {
      da *= BASE::dh()->talent.aldrachi_reaver.wounded_quarry->effectN( 1 ).percent();
      BASE::dh()->wounded_quarry_accumulator += da;
      BASE::dh()->sim->print_debug( "{} accumulates Wounded Quarry from {} on target {}: da={} total={}",
                                    BASE::dh()->name(), s->action->name(), s->target->name(), da,
                                    BASE::dh()->wounded_quarry_accumulator );
      if ( BASE::dh()->cooldown.wounded_quarry_trigger_icd->up() )
      {
        BASE::dh()->sim->print_debug( "{} triggers Wounded Quarry from {} on target {}: {}", BASE::dh()->name(),
                                      s->action->name(), BASE::dh()->last_reavers_mark_applied->name(),
                                      BASE::dh()->wounded_quarry_accumulator );
        if ( s->target->debuffs.chaos_brand->up() )
        {
          BASE::dh()->wounded_quarry_accumulator *= 1.0 + BASE::dh()->spell.chaos_brand->effectN( 1 ).percent();
        }
        BASE::dh()->active.wounded_quarry->execute_on_target( BASE::dh()->last_reavers_mark_applied,
                                                              BASE::dh()->wounded_quarry_accumulator );
        BASE::dh()->wounded_quarry_accumulator = 0.0;
        // per dev communication, it's batched per second
        BASE::dh()->cooldown.wounded_quarry_trigger_icd->start( 1_s );
      }
    }
  }
};

template <typename BASE>
struct voidfall_building_trigger_t : public BASE
{
  using base_t = voidfall_building_trigger_t<BASE>;

  voidfall_building_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                               util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( !BASE::dh()->talent.annihilator.voidfall->ok() )
      return;

    if ( !BASE::rng().roll( BASE::dh()->talent.annihilator.voidfall->effectN( 3 ).percent() ) )
      return;

    // can't gain building while spending is up
    if ( BASE::dh()->buff.voidfall_spending->up() )
    {
      return;
    }

    BASE::dh()->proc.voidfall_from_builder->occur();
    BASE::dh()->buff.voidfall_building->trigger(
        as<int>( BASE::dh()->talent.annihilator.voidfall->effectN( 1 ).base_value() ) );
  }
};

template <typename BASE>
struct voidfall_spending_trigger_t : public BASE
{
  using base_t = voidfall_spending_trigger_t<BASE>;

  voidfall_spending_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                               util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( !BASE::dh()->talent.annihilator.voidfall->ok() )
      return;

    if ( !BASE::dh()->buff.voidfall_spending->up() )
      return;

    if ( BASE::dh()->buff.voidfall_spending->at_max_stacks() && BASE::dh()->talent.annihilator.meteoric_fall->ok() )
      return;

    BASE::dh()->sim->print_debug( "{} triggering Voidfall spending", BASE::dh()->name() );

    if ( BASE::dh()->buff.voidfall_spending->stack() == 1 && BASE::dh()->talent.annihilator.world_killer->ok() &&
         !BASE::dh()->talent.annihilator.meteoric_fall->ok() )
    {
      BASE::dh()->active.world_killer->execute_on_target( BASE::target );
    }
    else
    {
      BASE::dh()->active.voidfall_meteor->execute_on_target( BASE::target );
    }
  }
};

template <typename BASE>
struct meteoric_fall_trigger_t : public BASE
{
  using base_t = meteoric_fall_trigger_t<BASE>;

  meteoric_fall_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                           util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( !BASE::dh()->talent.annihilator.meteoric_fall->ok() )
      return;

    if ( !BASE::dh()->buff.voidfall_spending->at_max_stacks() )
      return;

    BASE::dh()->sim->print_debug( "{} triggering Meteoric Fall", BASE::dh()->name() );

    int stacks = BASE::dh()->buff.voidfall_spending->stack();
    if ( BASE::dh()->talent.annihilator.world_killer->ok() )
    {
      stacks -= 1;
    }

    for ( int i = 0; i < stacks; ++i )
    {
      make_event<delayed_execute_event_t>( *BASE::sim, BASE::dh(), BASE::dh()->active.voidfall_meteor, BASE::target,
                                           ( BASE::dh()->active.voidfall_meteor->travel_time() * i ) );
    }

    if ( BASE::dh()->talent.annihilator.world_killer->ok() )
    {
      make_event<delayed_execute_event_t>( *BASE::sim, BASE::dh(), BASE::dh()->active.world_killer, BASE::target,
                                           ( stacks * BASE::dh()->active.voidfall_meteor->travel_time() ) );
    }
  }
};

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

    if ( !BASE::dh()->talent.scarred.burning_blades->ok() )
      return;

    if ( !action_t::result_is_hit( s->result ) )
      return;

    const double dot_damage = s->result_amount * BASE::dh()->talent.scarred.burning_blades->effectN( 1 ).percent();
    residual_action::trigger( BASE::dh()->active.burning_blades, s->target, dot_damage );
  }
};

template <typename BASE>
struct catastrophe_trigger_t : public BASE
{
  using base_t = catastrophe_trigger_t<BASE>;

  catastrophe_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                         util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( !BASE::dh()->talent.annihilator.catastrophe->ok() )
      return;

    if ( !action_t::result_is_hit( s->result ) )
      return;

    const double dot_damage = s->result_amount * BASE::dh()->talent.annihilator.catastrophe->effectN( 1 ).percent();
    residual_action::trigger( BASE::dh()->active.catastrophe, s->target, dot_damage );
  }
};

template <typename BASE>
struct mass_acceleration_trigger_t : public BASE
{
  using base_t = mass_acceleration_trigger_t<BASE>;

  mass_acceleration_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                               util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( !BASE::dh()->talent.annihilator.mass_acceleration->ok() )
      return;

    BASE::dh()->proc.voidfall_from_mass_acceleration->occur();
    BASE::dh()->buff.voidfall_building->trigger(
        as<int>( BASE::dh()->talent.annihilator.mass_acceleration->effectN( 1 ).base_value() ) );

    switch ( BASE::dh()->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        BASE::dh()->cooldown.reap->reset( true );
        break;
      case DEMON_HUNTER_VENGEANCE:
        BASE::dh()->cooldown.spirit_bomb->reset( true );
        break;
      default:
        break;
    }
  }
};

template <typename BASE>
struct dark_matter_trigger_t : public BASE
{
  using base_t = dark_matter_trigger_t<BASE>;

  dark_matter_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                         util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( !BASE::dh()->talent.annihilator.dark_matter->ok() )
      return;

    if ( !BASE::dh()->buff.dark_matter->up() )
      return;

    BASE::dh()->buff.dark_matter->expire();
    BASE::dh()->active.meteor_shower->execute_on_target( BASE::target );
  }
};

template <typename BASE>
struct hungering_slash_trigger_t : BASE
{
  using base_t = hungering_slash_trigger_t<BASE>;

  hungering_slash_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                             util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( BASE::dh()->talent.devourer.hungering_slash->ok() )
      BASE::dh()->buff.hungering_slash->trigger();
  }
};

template <typename BASE>
struct voidrush_trigger_t : BASE
{
  using base_t = voidrush_trigger_t<BASE>;

  voidrush_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                      util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( BASE::dh()->talent.devourer.voidrush->ok() )
      BASE::dh()->buff.voidrush->trigger();
  }
};

template <typename BASE>
struct doomsayer_trigger_t : public BASE
{
  using base_t = doomsayer_trigger_t<BASE>;

  doomsayer_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                       util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( !BASE::dh()->talent.annihilator.doomsayer->ok() )
      return;

    if ( !( BASE::dh()->buff.doomsayer_in_combat->up() || BASE::dh()->buff.doomsayer_out_of_combat->up() ) )
      return;

    BASE::dh()->sim->print_debug( "{} triggering Doomsayer", BASE::dh()->name() );

    int meteors_to_trigger = as<int>( BASE::dh()->talent.annihilator.doomsayer->effectN( 1 ).base_value() );
    if ( BASE::dh()->talent.annihilator.world_killer->ok() )
    {
      meteors_to_trigger -= 1;
    }

    BASE::dh()->buff.doomsayer_in_combat->expire();
    BASE::dh()->buff.doomsayer_out_of_combat->expire();

    for ( int i = 0; i < meteors_to_trigger; ++i )
    {
      BASE::dh()->active.voidfall_meteor->execute_on_target( BASE::target );
    }
    if ( BASE::dh()->talent.annihilator.world_killer->ok() )
    {
      BASE::dh()->active.world_killer->execute_on_target( BASE::target );
    }
  }
};

template <typename BASE>
struct final_breath_trigger_t : public BASE
{
  using base_t = final_breath_trigger_t<BASE>;

  final_breath_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                          util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void last_tick( dot_t* d ) override
  {
    BASE::last_tick( d );

    if ( !BASE::dh()->talent.demon_hunter.final_breath->ok() )
      return;

    assert( BASE::tick_action != nullptr );

    if ( d->current_tick >= ( d->num_ticks() - BASE::dh()->options.channel_tick_cutoff_benefit ) )
    {
      action_state_t* tick_state = BASE::tick_action->get_state( BASE::tick_action->execute_state );
      if ( BASE::tick_action->pre_execute_state )
      {
        tick_state->copy_state( BASE::tick_action->pre_execute_state );
        action_state_t::release( BASE::tick_action->pre_execute_state );
      }

      tick_state->target = d->target;
      BASE::tick_action->set_target( d->target );

      if ( BASE::dynamic_tick_action == TICK_ACTION_UPDATE )
      {
        BASE::tick_action->update_state( tick_state, BASE::amount_type( tick_state, BASE::tick_action->direct_tick ) );
      }

      BASE::tick_action->snapshot_state( tick_state, result_amount_type::DMG_DIRECT );
      tick_state->da_multiplier *= BASE::dh()->talent.demon_hunter.final_breath->effectN( 1 ).percent();

      BASE::tick_action->schedule_execute( tick_state );
    }
  }
};

template <typename BASE>
struct student_of_suffering_trigger_t : public BASE
{
  using base_t = student_of_suffering_trigger_t<BASE>;

  student_of_suffering_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                                  util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  void execute() override
  {
    BASE::execute();

    if ( BASE::dh()->talent.scarred.student_of_suffering->ok() )
    {
      BASE::dh()->buff.student_of_suffering->trigger();
    }
  }
};

template <typename BASE>
struct otherworldly_focus_benefit_t : public BASE
{
  using base_t = otherworldly_focus_benefit_t<BASE>;

  otherworldly_focus_benefit_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                                util::string_view o = {} )
    : BASE( n, p, s, o )
  {
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = BASE::composite_da_multiplier( s );

    if ( BASE::dh()->talent.annihilator.otherworldly_focus->ok() )
    {
      // 1 target is always effect 1 %
      // 2 target is effect 1 % - effect 2 %
      // 3 target is effect 1 % - (effect 2 % * 2)
      // etc until reaching 0 benefit
      auto num_target_reduction_percent = BASE::dh()->talent.annihilator.otherworldly_focus->effectN( 2 ).percent() *
                                          ( std::max( std::min( s->n_targets - 1, 10U ), 0U ) );
      m *= 1.0 + std::max( 0.0, BASE::dh()->talent.annihilator.otherworldly_focus->effectN( 1 ).percent() -
                                    num_target_reduction_percent );
    }

    return m;
  }
};

template <typename BASE>
struct shattered_souls_trigger_t : public BASE
{
  using base_t = shattered_souls_trigger_t<BASE>;

  double shattered_souls_base_chance;
  proc_t* shattered_souls_proc;

  shattered_souls_trigger_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                             util::string_view o = {} )
    : BASE( n, p, s, o ), shattered_souls_base_chance( p->spec.shattered_souls->effectN( 1 ).percent() )
  {
    std::string proc_name = base_t::shattered_souls_ability_name();
    if ( !p->proc.shattered_souls[ proc_name ] )
    {
      p->proc.shattered_souls[ proc_name ] = p->get_proc( fmt::format( "Shattered Souls: {}", proc_name ) );
    }
    shattered_souls_proc = p->proc.shattered_souls[ proc_name ];
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( BASE::dh()->specialization() != DEMON_HUNTER_DEVOURER )
      return;

    if ( !BASE::result_is_hit( s->result ) || s->proc_type() == PROC1_PERIODIC || s->result_total <= 0 )
      return;

    double chance = shattered_souls_chance( s );
    if ( BASE::rng().roll( chance ) )
    {
      BASE::dh()->sim->print_debug( "{} proc-ed Shattered Souls with {} ({}) chance: {:.3f}", BASE::dh()->name(),
                                    BASE::name(), BASE::data().id(), chance );
      BASE::dh()->spawn_soul_fragment( BASE::dh()->proc.soul_fragment_from_shattered_souls, soul_fragment::LESSER, 1 );
      shattered_souls_proc->occur();
    }
  }

  virtual std::string shattered_souls_ability_name()
  {
    return BASE::name_str;
  }

  virtual double shattered_souls_chance( action_state_t* )
  {
    return shattered_souls_base_chance;
  }
};

struct demon_hunter_heal_t : public demon_hunter_action_t<heal_t>
{
  demon_hunter_heal_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                       util::string_view o = {} )
    : base_t( n, p, s, o )
  {
    harmful = false;
    target  = p;
  }
};

struct demon_hunter_spell_t : public wounded_quarry_accumulator_t<demon_hunter_action_t<spell_t>>
{
  demon_hunter_spell_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                        util::string_view o = {} )
    : base_t( n, p, s, o )
  {
  }
};

struct demon_hunter_energize_t : public demon_hunter_action_t<spell_t>
{
  demon_hunter_energize_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, p, s, "" )
  {
    may_miss = may_block = may_dodge = may_parry = callbacks = false;
    background = dual = true;
    energize_type     = action_energize::ON_CAST;
    target            = p;
    harmful           = false;
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
                          p->cooldown.sigil_of_silence, p->cooldown.sigil_of_chains };
      sigil_cooldown_adjust =
          -timespan_t::from_seconds( p->talent.vengeance.cycle_of_binding->effectN( 1 ).base_value() );
    }
  }

  void place_sigil( player_t* target )
  {
    make_event<ground_aoe_event_t>( *sim, dh(),
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

    if ( hit_any_target && dh()->talent.vengeance.soul_sigils->ok() )
    {
      unsigned num_souls = as<unsigned>( dh()->talent.vengeance.soul_sigils->effectN( 1 ).base_value() );
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_soul_sigils, soul_fragment::LESSER, num_souls, false );
    }
  }

  std::unique_ptr<expr_t> create_sigil_expression( util::string_view name );
};

struct demon_hunter_attack_t : public wounded_quarry_accumulator_t<demon_hunter_action_t<melee_attack_t>>
{
  demon_hunter_attack_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
                         util::string_view o = {} )
    : base_t( n, p, s, o )
  {
    special = true;
  }
};

struct demon_hunter_ranged_attack_t : public wounded_quarry_accumulator_t<demon_hunter_action_t<ranged_attack_t>>
{
  demon_hunter_ranged_attack_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil(),
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
    target     = p;

    bool is_greater = ( soul_fragment::ANY_GREATER & type ) == type;
    switch ( p->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        impact_energize_action = p->get_background_action<demon_hunter_energize_t>(
            fmt::format( "{}_energize", n ),
            is_greater ? p->spec.consume_soul_greater_energize : p->spec.consume_soul_lesser_energize );
        break;
      case DEMON_HUNTER_HAVOC:
        impact_energize_action = p->get_background_action<demon_hunter_energize_t>(
            fmt::format( "{}_energize", n ),
            is_greater ? p->spec.consume_soul_greater_energize : p->spec.consume_soul_lesser_energize );
        break;
      default:
        break;
    }
  }

  double calculate_heal( const action_state_t* ) const
  {
    switch ( dh()->specialization() )
    {
      case DEMON_HUNTER_VENGEANCE:
        if ( type == soul_fragment::LESSER )
        {
          // Vengeance-Specific Healing Logic
          // This is not in the heal data and they use SpellId 203783 to control the healing parameters
          return std::max(
              player->resources.max[ RESOURCE_HEALTH ] * vengeance_heal->effectN( 3 ).percent(),
              player->compute_incoming_damage( vengeance_heal_interval ) * vengeance_heal->effectN( 2 ).percent() );
        }
        // SOUL_FRAGMENT_GREATER for Vengeance uses AP mod calculations
        break;
      default:
        break;
    }
    return 0.0;
  }

  // this is for devourer/havoc
  double composite_pct_heal( const action_state_t* s ) const override
  {
    double pct = demon_hunter_heal_t::composite_pct_heal( s );

    if ( dh()->talent.demon_hunter.shattered_restoration->ok() )
    {
      pct *= 1.0 + dh()->talent.demon_hunter.shattered_restoration->effectN( 1 ).percent();
    }

    return pct;
  }

  // this is for vengeance
  double base_da_min( const action_state_t* s ) const override
  {
    double base_heal = calculate_heal( s );
    if ( dh()->talent.demon_hunter.shattered_restoration->ok() )
    {
      base_heal *= 1.0 + dh()->talent.demon_hunter.shattered_restoration->effectN( 1 ).percent();
    }
    return base_heal;
  }

  // this is for vengeance
  double base_da_max( const action_state_t* s ) const override
  {
    double base_heal = calculate_heal( s );
    if ( dh()->talent.demon_hunter.shattered_restoration->ok() )
    {
      base_heal *= 1.0 + dh()->talent.demon_hunter.shattered_restoration->effectN( 1 ).percent();
    }
    return base_heal;
  }

  // Handled in the delayed consume event
  timespan_t travel_time() const override
  {
    return 0_s;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_heal_t::impact( s );

    dh()->buff.painbringer->trigger();
    dh()->buff.art_of_the_glaive->trigger();

    dh()->buff.feast_of_souls->trigger();
    if ( !dh()->buff.metamorphosis->up() )
    {
      dh()->buff.void_metamorphosis_stack->trigger();
    }
    else
    {
      if ( dh()->talent.devourer.collapsing_star->ok() )
      {
        dh()->buff.collapsing_star_stack->trigger();
      }
      if ( dh()->talent.devourer.emptiness->ok() )
      {
        dh()->buff.emptiness->trigger();
      }
    }

    // Warblade's hunger currently applies an additional stack on first buff application
    if ( !dh()->buff.warblades_hunger->up() )
    {
      dh()->buff.warblades_hunger->trigger();
    }
    dh()->buff.warblades_hunger->trigger();

    if ( type == soul_fragment::GREATER_DEMON )
    {
      dh()->buff.demon_soul->trigger();
    }
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
    souls_consumed = dh()->consume_soul_fragments( soul_fragment::ANY, false );
    demon_hunter_action_t<absorb_t>::execute();
  }

  void impact( action_state_t* s ) override
  {
    dh()->buff.soul_barrier->trigger( 1, s->result_amount );
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

  soul_cleave_heal_t( util::string_view name, demon_hunter_t* p ) : demon_hunter_heal_t( name, p, p->spec.soul_cleave )
  {
    background = dual = true;
    target            = p;

    // Clear out the costs since this is just a copy of the damage spell
    base_costs.fill( 0 );
    max_base_costs.fill( 0 );

    if ( p->talent.vengeance.feast_of_souls->ok() )
    {
      execute_action = p->get_background_action<feast_of_souls_heal_t>( "feast_of_souls" );
    }
  }
};

// Frailty ==============================================================

struct frailty_heal_t : public demon_hunter_heal_t
{
  frailty_heal_t( util::string_view n, demon_hunter_t* p ) : demon_hunter_heal_t( n, p, p->spec.frailty_heal )
  {
    background = true;
    may_crit   = false;
    target     = p;
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

    dh()->buff.blur->trigger();
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
    dh()->buff.demon_spikes->trigger();
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

struct eye_beam_base_t : public student_of_suffering_trigger_t<final_breath_trigger_t<demon_hunter_spell_t>>
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

      if ( dh()->talent.havoc.isolated_prey->ok() && targets_in_range_list( target_list() ).size() == 1 )
      {
        m *= 1.0 + dh()->talent.havoc.isolated_prey->effectN( 2 ).percent();
      }

      return m;
    }

    timespan_t execute_time() const override
    {
      // Eye Beam is applied via a player aura and experiences aura delay in applying damage tick events
      // Not a perfect implementation, but closer than the instant execution in current sims
      return rng().gauss( dh()->sim->default_aura_delay );
    }
  };

  eye_beam_tick_t* tick;
  eye_beam_tick_t* empowered_tick;

  eye_beam_base_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : base_t( name, p, s, o )
  {
    may_miss            = false;
    channeled           = true;
    tick_on_application = false;
    cooldown            = p->cooldown.eye_beam;

    // 6/6/2020 - Override the lag handling for Eye Beam so that it doesn't use channeled ready behavior
    //            In-game tests have shown it is possible to cast after faster than the 250ms channel_lag using a
    //            nochannel macro
    ability_lag = p->world_lag;

    tick = p->get_background_action<eye_beam_tick_t>( name_str + "_tick", name_str, p->spec.eye_beam_damage,
                                                      as<int>( data().effectN( 5 ).base_value() ) );
    add_child( tick );

    if ( p->talent.havoc.eternal_hunt_1->ok() )
    {
      empowered_tick = p->get_background_action<eye_beam_tick_t>( name_str + "_empowered_tick", name_str,
                                                                  p->spec.empowered_eye_beam_damage,
                                                                  as<int>( data().effectN( 5 ).base_value() ) );
      add_child( empowered_tick );
    }

    // Add damage modifiers in eye_beam_tick_t, not here.
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );

    // If Eye Beam is canceled early, cancel Blind Fury and skip granting Furious Gaze
    // Collective Anguish is *not* canceled when early canceling Eye Beam, however
    if ( d->current_tick < d->num_ticks() )
    {
      dh()->buff.blind_fury->cancel();
      dh()->proc.eye_beam_canceled->occur();
    }

    if ( dh()->talent.havoc.furious_gaze->ok() &&
         d->current_tick >= ( d->num_ticks() - dh()->options.channel_tick_cutoff_benefit ) )
    {
      dh()->buff.furious_gaze->trigger();
    }

    if ( dh()->talent.havoc.eternal_hunt_3->ok() &&
         d->current_tick >= ( d->num_ticks() - dh()->options.channel_tick_cutoff_benefit ) )
    {
      dh()->buff.eternal_hunt->trigger();
    }

    if ( dh()->buff.empowered_eye_beam->up() )
    {
      dh()->buff.empowered_eye_beam->expire();
    }
  }

  void execute() override
  {
    // Trigger Meta before the execute so that the channel duration is affected by Meta haste
    dh()->trigger_demonic();

    tick_action = tick;
    if ( empowered_tick && dh()->buff.empowered_eye_beam->up() )
    {
      tick_action = empowered_tick;
    }

    base_t::execute();

    if ( dh()->talent.havoc.cycle_of_hatred->ok() )
    {
      dh()->buff.cycle_of_hatred->trigger();
    }

    timespan_t duration = composite_dot_duration( execute_state );

    // Since Demonic triggers Meta with 5s + hasted duration, need to extend by the hasted duration after have an
    // execute_state
    if ( dh()->talent.havoc.demonic->ok() )
    {
      dh()->buff.metamorphosis->extend_duration( duration );
    }

    if ( dh()->talent.havoc.blind_fury->ok() )
    {
      dh()->buff.blind_fury->trigger( duration );
    }

    // Collective Anguish
    if ( dh()->active.collective_anguish )
    {
      dh()->active.collective_anguish->set_target( target );
      dh()->active.collective_anguish->execute();
    }
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }

  timespan_t cooldown_base_duration( const cooldown_t& cd ) const override
  {
    return base_t::cooldown_base_duration( cd ) -
           timespan_t::from_millis( as<int>( dh()->buff.cycle_of_hatred->check_stack_value() ) );
  }
};

struct abyssal_gaze_t : public demonsurge_trigger_t<demonsurge_ability::ABYSSAL_GAZE, eye_beam_base_t>
{
  abyssal_gaze_t( demon_hunter_t* p ) : base_t( "abyssal_gaze", p, p->hero_spec.abyssal_gaze, "" )
  {
  }
};

struct eye_beam_t : public eye_beam_base_t
{
  abyssal_gaze_t* abyssal_gaze;
  double abyssal_gaze_cost;

  eye_beam_t( demon_hunter_t* p, util::string_view options_str )
    : eye_beam_base_t( "eye_beam", p, p->talent.havoc.eye_beam, options_str ),
      abyssal_gaze( nullptr ),
      abyssal_gaze_cost( 0 )
  {
    if ( p->talent.scarred.demonic_intensity->ok() )
    {
      abyssal_gaze      = new abyssal_gaze_t( p );
      abyssal_gaze_cost = abyssal_gaze->data().cost( POWER_FURY );
      add_child( abyssal_gaze );
    }
  }

  double cost() const override
  {
    if ( dh()->buff.demonsurge_demonic_intensity->check() )
    {
      return abyssal_gaze_cost;
    }
    return eye_beam_base_t::cost();
  }

  void execute() override
  {
    if ( dh()->buff.demonsurge_demonic_intensity->check() )
    {
      abyssal_gaze->execute_on_target( target );
      stats->add_execute( time_to_execute, target );
      return;
    }

    eye_beam_base_t::execute();
  }
};

// Fel Devastation ==========================================================

struct fel_devastation_t : public final_breath_trigger_t<demon_hunter_spell_t>
{
  struct fel_devastation_tick_t : public demon_hunter_spell_t
  {
    fel_devastation_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->talent.vengeance.fel_devastation->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe               = -1;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = demon_hunter_spell_t::composite_da_multiplier( s );

      if ( parent_dot )
      {
        m *= parent_dot->get_tick_factor();
      }

      return m;
    }
  };

  heals::fel_devastation_heal_t* heal;
  int soul_fragments_from_meteoric_rise;

  fel_devastation_t( demon_hunter_t* p, util::string_view o )
    : base_t( "fel_devastation", p, p->talent.vengeance.fel_devastation, o ),
      heal( nullptr ),
      soul_fragments_from_meteoric_rise( 0 )
  {
    may_miss            = false;
    channeled           = true;
    tick_on_application = false;
    cooldown            = p->cooldown.fel_devastation;

    // forces hasted cooldown if no Fel Devastation is in the APL for whatever reason
    if ( data().affected_by( p->spec.vengeance_demon_hunter->effectN( 4 ) ) )
    {
      cooldown->hasted = true;
    }

    if ( p->spec.fel_devastation_2->ok() )
    {
      heal = p->get_background_action<heals::fel_devastation_heal_t>( "fel_devastation_heal" );
    }

    tick_action = p->get_background_action<fel_devastation_tick_t>( "fel_devastation_tick" );
    add_child( tick_action );

    if ( p->talent.annihilator.meteoric_rise->ok() )
    {
      soul_fragments_from_meteoric_rise = as<int>( p->talent.annihilator.meteoric_rise->effectN( 4 ).base_value() );
    }
  }

  void execute() override
  {
    dh()->trigger_demonic();
    base_t::execute();

    if ( heal )
    {
      heal->set_target( player );
      heal->execute();
    }
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );

    if ( dh()->talent.vengeance.darkglare_boon->ok() )
    {
      // CDR reduction and Fury refund are separate rolls per Realz
      double base_cooldown = dh()->talent.vengeance.fel_devastation->cooldown().total_seconds();
      timespan_t minimum_cdr_reduction =
          timespan_t::from_seconds( dh()->talent.vengeance.darkglare_boon->effectN( 1 ).percent() * base_cooldown );
      timespan_t maximum_cdr_reduction =
          timespan_t::from_seconds( dh()->talent.vengeance.darkglare_boon->effectN( 2 ).percent() * base_cooldown );
      timespan_t cdr_reduction   = rng().range( minimum_cdr_reduction, maximum_cdr_reduction );
      double minimum_fury_refund = dh()->talent.vengeance.darkglare_boon->effectN( 3 ).base_value();
      double maximum_fury_refund = dh()->talent.vengeance.darkglare_boon->effectN( 4 ).base_value();
      double fury_refund         = rng().range( minimum_fury_refund, maximum_fury_refund );

      dh()->sim->print_debug( "{} rolled {}s of CDR on {} from Darkglare Boon", *dh(), cdr_reduction.total_seconds(),
                              name_str );
      dh()->cooldown.fel_devastation->adjust( -cdr_reduction );
      dh()->resource_gain( RESOURCE_FURY, fury_refund, dh()->gain.darkglare_boon );
    }
  }

  void tick( dot_t* d ) override
  {
    if ( heal )
    {
      heal->execute();  // Heal happens before damage
    }

    base_t::tick( d );

    if ( dh()->talent.annihilator.meteoric_rise->ok() )
    {
      int expected_before = ( d->current_tick - 1 ) * soul_fragments_from_meteoric_rise / d->num_ticks();
      int expected_after  = d->current_tick * soul_fragments_from_meteoric_rise / d->num_ticks();
      if ( expected_after > expected_before )
      {
        dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_meteoric_rise, soul_fragment::LESSER );
      }
    }
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
      if ( triggered_duration == dh()->spec.fiery_brand_debuff->duration() )
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
      if ( !t )
        t = target;

      return td( t )->dots.fiery_brand;
    }

    void tick( dot_t* d ) override
    {
      demon_hunter_spell_t::tick( d );

      trigger_burning_alive( d );
    }

    void trigger_burning_alive( dot_t* d )
    {
      if ( !dh()->talent.vengeance.burning_alive->ok() )
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
      player_t* target = targets[ static_cast<int>( dh()->rng().range( 0, static_cast<double>( targets.size() ) ) ) ];
      this->set_target( target );
      this->schedule_execute();

      // 2026-05-04 -- Burning Alive currently refreshes Fiery Brand on the player
      dh()->buff.fiery_brand->trigger();
    }
  };

  fiery_brand_dot_t* dot_action;

  fiery_brand_t( demon_hunter_t* p, util::string_view options_str = {} )
    : demon_hunter_spell_t( "fiery_brand", p, p->talent.vengeance.fiery_brand, options_str ), dot_action( nullptr )
  {
    use_off_gcd = true;

    dot_action = p->get_background_action<fiery_brand_dot_t>( "fiery_brand_dot" );
    add_child( dot_action );
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

    dh()->buff.fiery_brand->trigger();
  }

  dot_t* get_dot( player_t* t ) override
  {
    return dot_action->get_dot( t );
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
      aoe                            = -1;
      reduced_aoe_targets            = p->spec.glaive_tempest->effectN( 2 ).base_value();
    }
  };

  glaive_tempest_damage_t* glaive_tempest_mh;
  glaive_tempest_damage_t* glaive_tempest_oh;

  glaive_tempest_t( util::string_view n, demon_hunter_t* p ) : demon_hunter_spell_t( n, p, p->spec.glaive_tempest )
  {
    school            = SCHOOL_CHAOS;  // Reporting purposes only
    glaive_tempest_mh = p->get_background_action<glaive_tempest_damage_t>( fmt::format( "{}_mh", name() ) );
    glaive_tempest_oh = p->get_background_action<glaive_tempest_damage_t>( fmt::format( "{}_oh", name() ) );
    add_child( glaive_tempest_mh );
    add_child( glaive_tempest_oh );
  }

  void make_ground_aoe_event( glaive_tempest_damage_t* action )
  {
    // Has one initial period for 2*(6+1) ticks plus hasted duration and tick rate
    // However, hasted duration is snapshotted on cast, which is likely a bug
    // This allows it to sometimes tick more or less than the intended 14 times
    make_event<ground_aoe_event_t>( *sim, dh(),
                                    ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( dh()->x_position )
                                        .y( dh()->y_position )
                                        .pulse_time( 500_ms )
                                        .hasted( ground_aoe_params_t::hasted_with::ATTACK_HASTE )
                                        .duration( data().duration() * dh()->cache.attack_haste() )
                                        .action( action ),
                                    true );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    make_ground_aoe_event( glaive_tempest_mh );
    make_ground_aoe_event( glaive_tempest_oh );
  }
};

// Sigil of Flame ===========================================

struct sigil_of_flame_t : public demon_hunter_spell_t
{
  struct sigil_of_flame_damage_t : public demon_hunter_sigil_t
  {
    sigil_of_flame_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_sigil_t( name, p, p->spec.sigil_of_flame_damage, p->spec.sigil_of_flame->duration() )
    {
      tick_on_application = false;
      dot_behavior        = dot_behavior_e::DOT_REFRESH_DURATION;
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_sigil_t::impact( s );

      // Sigil of Flame can apply Frailty if Frailty is talented
      if ( result_is_hit( s->result ) && dh()->talent.vengeance.frailty->ok() )
      {
        td( s->target )->debuffs.frailty->trigger();
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
  };

  sigil_of_flame_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "sigil_of_flame", p, p->spec.sigil_of_flame, options_str )
  {
    if ( data().ok() && !p->active.sigil_of_flame )
    {
      p->active.sigil_of_flame = p->get_background_action<sigil_of_flame_damage_t>( "sigil_of_flame_damage" );
      add_child( p->active.sigil_of_flame );
    }

    may_miss = false;
    cooldown = p->cooldown.sigil_of_flame;

    if ( p->spec.sigil_of_flame_fury->ok() )
    {
      execute_energize_action =
          p->get_background_action<demon_hunter_energize_t>( "sigil_of_flame_fury", p->spec.sigil_of_flame_fury );
    }

    // Add damage modifiers in sigil_of_flame_damage_t, not here.
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

    debug_cast<demon_hunter_sigil_t*>( dh()->active.sigil_of_flame )->place_sigil( execute_state->target );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( auto e = debug_cast<demon_hunter_sigil_t*>( dh()->active.sigil_of_flame )->create_sigil_expression( name ) )
      return e;

    return demon_hunter_spell_t::create_expression( name );
  }
};

// Collective Anguish =======================================================

struct collective_anguish_t : public demon_hunter_spell_t
{
  struct collective_anguish_tick_t : public demon_hunter_spell_t
  {
    collective_anguish_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.collective_anguish_damage )
    {
      dual = true;
      aoe  = -1;
    }
  };

  collective_anguish_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_spell_t( name, p, p->spec.collective_anguish )
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

struct infernal_strike_t : public doomsayer_trigger_t<demon_hunter_spell_t>
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

  sigil_of_flame_t::sigil_of_flame_damage_t* felfire_fist;

  infernal_strike_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "infernal_strike", p, p->spec.infernal_strike, options_str ), felfire_fist( nullptr )
  {
    may_miss                = false;
    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;
    travel_speed            = 1.0;  // allows use precombat

    impact_action = p->get_background_action<infernal_strike_impact_t>( "infernal_strike_impact" );
    add_child( impact_action );

    if ( p->talent.vengeance.felfire_fist->ok() )
    {
      felfire_fist = p->get_background_action<sigil_of_flame_t::sigil_of_flame_damage_t>( "felfire_fist" );
      add_child( felfire_fist );
    }
  }

  // leap travel time, independent of distance
  timespan_t travel_time() const override
  {
    return 1_s;
  }

  void execute() override
  {
    base_t::execute();

    if ( felfire_fist && ( dh()->buff.felfire_fist_in_combat->up() || dh()->buff.felfire_fist_out_of_combat->up() ) )
    {
      felfire_fist->place_sigil( target );
    }
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
        switch ( p->specialization() )
        {
          case DEMON_HUNTER_HAVOC:
            energize_amount = p->talent.havoc.burning_hatred->ok() ? data().effectN( 2 ).base_value() : 0;
            break;
          case DEMON_HUNTER_VENGEANCE:
            energize_amount = data().effectN( 3 ).base_value();
            break;
          default:
            break;
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

      if ( dh()->talent.havoc.isolated_prey->ok() )
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
        bool spawn_fallout_soul = false;
        if ( dh()->talent.vengeance.fallout->ok() )
        {
          spawn_fallout_soul = s->n_targets == 1 || rng().roll( 0.60 );
        }

        if ( initial && spawn_fallout_soul )
        {
          dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_fallout, soul_fragment::LESSER, 1 );
        }

        if ( dh()->talent.vengeance.charred_flesh->ok() )
        {
          auto duration_extension        = dh()->talent.vengeance.charred_flesh->effectN( 1 ).time_value();
          demon_hunter_td_t* target_data = td( s->target );
          if ( target_data->dots.fiery_brand->is_ticking() )
          {
            target_data->dots.fiery_brand->adjust_duration( duration_extension );
          }
          if ( target_data->dots.sigil_of_flame->is_ticking() )
          {
            target_data->dots.sigil_of_flame->adjust_duration( duration_extension );
          }
        }

        if ( s->result == RESULT_CRIT && dh()->talent.vengeance.volatile_flameblood->ok() &&
             dh()->cooldown.volatile_flameblood_icd->up() )
        {
          dh()->resource_gain( RESOURCE_FURY, dh()->talent.vengeance.volatile_flameblood->effectN( 1 ).base_value(),
                               dh()->talent.vengeance.volatile_flameblood->effectN( 1 ).m_delta(),
                               dh()->gain.volatile_flameblood );
          dh()->cooldown.volatile_flameblood_icd->start(
              dh()->talent.vengeance.volatile_flameblood->internal_cooldown() );
        }

        accumulate_ragefire( cast_state( s ) );
      }
    }

    result_amount_type amount_type( const action_state_t*, bool ) const override
    {
      return initial ? result_amount_type::DMG_DIRECT : result_amount_type::DMG_OVER_TIME;
    }
  };

  struct ragefire_t : public demon_hunter_spell_t
  {
    ragefire_t( util::string_view name, demon_hunter_t* p ) : demon_hunter_spell_t( name, p, p->spec.ragefire_damage )
    {
      aoe = -1;
    }
  };

  immolation_aura_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "immolation_aura", p, p->spell.immolation_aura, options_str )
  {
    may_miss     = false;
    dot_duration = timespan_t::zero();
    set_target( p );  // Does not require a hostile target

    switch ( p->specialization() )
    {
      case DEMON_HUNTER_HAVOC:
        energize_amount = data().effectN( 2 ).base_value();
        break;
      case DEMON_HUNTER_VENGEANCE:
        energize_amount = data().effectN( 3 ).base_value();
        break;
      default:
        break;
    }

    if ( !p->active.immolation_aura_tick )
    {
      p->active.immolation_aura_tick = p->get_background_action<immolation_aura_damage_t>(
          "immolation_aura_tick", data().effectN( 1 ).trigger(), false );
      add_child( p->active.immolation_aura_tick );
    }

    if ( !p->active.immolation_aura_initial && p->spell.immolation_aura_damage->ok() )
    {
      p->active.immolation_aura_initial = p->get_background_action<immolation_aura_damage_t>(
          "immolation_aura_initial", p->spell.immolation_aura_damage, true );
      add_child( p->active.immolation_aura_initial );
    }

    if ( p->talent.demon_hunter.infernal_armor->ok() && !p->active.infernal_armor )
    {
      p->active.infernal_armor = p->get_background_action<infernal_armor_damage_t>( "infernal_armor" );
      add_child( p->active.infernal_armor );
    }

    if ( p->talent.havoc.ragefire->ok() && !p->active.ragefire )
    {
      p->active.ragefire = p->get_background_action<ragefire_t>( "ragefire" );
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
    dh()->buff.immolation_aura->trigger();
    demon_hunter_spell_t::execute();

    dh()->trigger_demonsurge( demonsurge_ability::CONSUMING_FIRE );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "demonsurge_available" ) )
    {
      if ( dh()->talent.scarred.demonsurge->ok() )
      {
        return make_fn_expr( name, [ this ]() {
          return dh()->buff.demonsurge_abilities[ demonsurge_ability::CONSUMING_FIRE ]->check();
        } );
      }
      return expr_t::create_constant( name, 0 );
    }

    return demon_hunter_spell_t::create_expression( name );
  }
};

// Metamorphosis ============================================================

struct metamorphosis_t : public mass_acceleration_trigger_t<demon_hunter_spell_t>
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
    : base_t( "metamorphosis", p, p->spec.metamorphosis ), landing_distance( 0.0 )
  {
    add_option( opt_float( "landing_distance", landing_distance, 0.0, 40.0 ) );
    parse_options( options_str );

    may_miss     = false;
    dot_duration = timespan_t::zero();

    switch ( p->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        break;
      case DEMON_HUNTER_HAVOC:
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
        break;
      case DEMON_HUNTER_VENGEANCE:
        break;
      default:
        break;
    }
  }

  // Meta leap travel time and self-pacify is a 1s hidden aura (201453) regardless of distance
  // This is affected by aura lag and will slightly delay execution of follow-up attacks
  // Not always relevant as GCD can be longer than the 1s + lag ability delay outside of lust
  void schedule_execute( action_state_t* s ) override
  {
    gcd_lag = rng().gauss( sim->gcd_lag );
    min_gcd = 1_s + gcd_lag;
    base_t::schedule_execute( s );
  }

  timespan_t travel_time() const override
  {
    switch ( dh()->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        return timespan_t::zero();
      case DEMON_HUNTER_HAVOC:
        return min_gcd;
      case DEMON_HUNTER_VENGEANCE:
        return timespan_t::zero();
      default:
        return timespan_t::zero();
    }
  }

  void execute() override
  {
    // Snapshot untethered_rage before base execute, since update_ready() expires the buff during base_t::execute()
    bool untethered = dh()->buff.untethered_rage->up();

    base_t::execute();

    switch ( dh()->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        for ( voidsurge_ability ability : voidsurge_abilities )
        {
          dh()->buff.voidsurge_abilities[ ability ]->trigger();
        }
        dh()->buff.demonsurge_demonsurge->trigger();
        dh()->buff.demonsurge_demonic_intensity->trigger();
        dh()->buff.demonsurge->expire();

        dh()->devourer_fury_state.start();

        dh()->buff.dark_matter->trigger();

        if ( dh()->talent.scarred.violent_transformation->ok() )
        {
          dh()->cooldown.voidblade->reset( true );
          dh()->cooldown.predators_wake->reset( true );
        }
        break;
      case DEMON_HUNTER_HAVOC:
        for ( demonsurge_ability ability : demonsurge_abilities )
        {
          dh()->buff.demonsurge_abilities[ ability ]->trigger();
        }
        dh()->buff.demonsurge_demonic_intensity->trigger();
        dh()->buff.demonsurge->expire();

        // Buff is gained at the start of the leap.
        dh()->buff.metamorphosis->extend_duration_or_trigger();

        if ( dh()->talent.havoc.chaotic_transformation->ok() )
        {
          dh()->cooldown.eye_beam->reset( false );
          dh()->cooldown.blade_dance->reset( false );
        }

        if ( dh()->talent.scarred.violent_transformation->ok() )
        {
          dh()->cooldown.immolation_aura->reset( false, -1 );
          dh()->cooldown.sigil_of_flame->reset( false );
        }

        // Cancel all previous movement events, as Metamorphosis is ground-targeted
        // If we are landing outside of point-blank range, trigger the movement buff
        dh()->set_out_of_range( timespan_t::zero() );
        if ( landing_distance > 0.0 )
        {
          dh()->buff.metamorphosis_move->distance_moved = landing_distance;
          dh()->buff.metamorphosis_move->trigger();
        }

        if ( dh()->talent.scarred.volatile_instinct->ok() )
        {
          dh()->trigger_demonsurge(
              demonsurge_ability::ENTER_META,
              timespan_t::from_millis( dh()->hero_spec.demonsurge_meta_trigger->effectN( 1 ).misc_value1() ), false );
        }
        break;
      case DEMON_HUNTER_VENGEANCE:
        if ( untethered )
        {
          // Untethered Rage uses a shorter duration, matching Demonic's extend_duration_or_trigger pattern
          timespan_t ur_duration =
              timespan_t::from_seconds( dh()->talent.vengeance.untethered_rage_1->effectN( 1 ).base_value() );
          dh()->buff.metamorphosis->extend_duration_or_trigger( ur_duration );
        }
        else
        {
          dh()->buff.metamorphosis->trigger();
        }
        dh()->buff.dark_matter->trigger();
        break;
      default:
        break;
    }
  }

  bool action_ready() override
  {
    if ( dh()->specialization() == DEMON_HUNTER_DEVOURER && !dh()->buff.void_metamorphosis_stack->at_max_stacks() )
    {
      return false;
    }
    return base_t::action_ready();
  }

  void update_ready( timespan_t cd_duration ) override
  {
    // Expiring untethered_rage removes the temporary max charge via adjust_max_charges, which also
    // loses a current charge. Skip base_t::update_ready to avoid consuming a real charge on top.
    if ( dh()->buff.untethered_rage->up() )
    {
      dh()->buff.untethered_rage->expire();
    }
    else
    {
      base_t::update_ready( cd_duration );
    }
  }

  void reset() override
  {
    // Reset max charges to initial value, since adjust_max_charges from untethered_rage can leave charges out of
    // sync when a previous iteration ends with the buff active.
    cooldown->charges = std::max( data().charges(), 1U );

    base_t::reset();
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
        frag->consume( false );
      }

      dh->soul_fragment_pick_up = nullptr;
    }
  };

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
    // Fragments have the following trigger radius:
    // Vengeance Lesser and Greater souls: 4 yards
    // Vengeance Greater Demon soul: 6 yards
    // Havoc Lesser and Greater souls: 8 yards
    // Havoc Greater Demon soul: 10 yards
    // Devourer Lesser and Greater souls: 4 yards
    double dtm;
    if ( frag->is_type( soul_fragment::EMPOWERED_DEMON ) )
    {
      dtm = std::max( 0.0, frag->get_distance( dh() ) - 6.0 );
    }
    else
    {
      switch ( dh()->specialization() )
      {
        case DEMON_HUNTER_DEVOURER:
          dtm = std::max( 0.0, frag->get_distance( dh() ) - 4.0 );
          break;
        case DEMON_HUNTER_HAVOC:
          dtm = std::max( 0.0, frag->get_distance( dh() ) - 8.0 );
          break;
        case DEMON_HUNTER_VENGEANCE:
          dtm = std::max( 0.0, frag->get_distance( dh() ) - 4.0 );
          break;
        default:
          dtm = std::max( 0.0, frag->get_distance( dh() ) - 6.0 );
          break;
      }
      if ( frag->is_type( soul_fragment::GREATER_DEMON ) )
      {
        dtm -= 2.0;
      }
    }
    timespan_t mt = timespan_t::from_seconds( dtm / dh()->cache.run_speed() );
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
        for ( auto frag : dh()->soul_fragments )
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
        for ( auto frag : dh()->soul_fragments )
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
        for ( auto frag : dh()->soul_fragments )
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

    assert( dh()->soul_fragment_pick_up == nullptr );
    dh()->soul_fragment_pick_up = make_event<pick_up_event_t>( *sim, frag, time, if_expr.get() );
  }

  bool ready() override
  {
    if ( dh()->soul_fragment_pick_up )
    {
      return false;
    }

    if ( !dh()->get_active_soul_fragments( type ) )
    {
      return false;
    }

    if ( !demon_hunter_spell_t::ready() )
    {
      return false;
    }

    // Not usable during the root effect of Stormeater's Boon
    if ( dh()->buffs.stormeaters_boon && dh()->buffs.stormeaters_boon->check() )
      return false;

    // Catch edge case where a fragment exists but we can't pick it up in time.
    return select_fragment() != nullptr;
  }
};

// Spirit Bomb ==============================================================

struct spirit_bomb_t : public demon_hunter_spell_t
{
  struct spirit_bomb_damage_t
    : public meteoric_fall_trigger_t<otherworldly_focus_benefit_t<dark_matter_trigger_t<demon_hunter_spell_t>>>
  {
    spirit_bomb_damage_t( util::string_view name, demon_hunter_t* p ) : base_t( name, p, p->spec.spirit_bomb_damage )
    {
      background = dual   = true;
      aoe                 = -1;
      reduced_aoe_targets = p->talent.vengeance.spirit_bomb->effectN( 3 ).base_value();
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      // Spirit Bomb can apply Frailty if Frailty is talented
      if ( result_is_hit( s->result ) && dh()->talent.vengeance.frailty->ok() )
      {
        td( s->target )->debuffs.frailty->trigger();
      }
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
    add_child( damage );

    if ( p->talent.annihilator.dark_matter->ok() && p->active.meteor_shower )
    {
      add_child( p->active.meteor_shower );
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    // Soul fragments consumed are capped for Spirit Bomb
    const int fragments_consumed = dh()->consume_soul_fragments( soul_fragment::ANY, true, max_fragments_consumed );
    if ( fragments_consumed > 0 )
    {
      damage->set_target( target );
      action_state_t* damage_state = damage->get_state();
      damage_state->target         = target;
      damage->snapshot_state( damage_state, result_amount_type::DMG_DIRECT );
      damage_state->da_multiplier *= 1.0 + ( data().effectN( 1 ).percent() * fragments_consumed );
      damage->schedule_execute( damage_state );
      damage->execute_event->reschedule( timespan_t::from_seconds( 1.0 ) );
    }

    trigger_untethered_rage( fragments_consumed );
  }

  bool action_ready() override
  {
    if ( dh()->get_active_soul_fragments() < 1 )
      return false;

    return demon_hunter_spell_t::action_ready();
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "max_souls_consumed" ) )
      return expr_t::create_constant( name, max_fragments_consumed );

    if ( util::str_compare_ci( name, "souls_consumed" ) )
      return make_fn_expr( name, [ this ]() {
        return std::min( dh()->get_active_soul_fragments( soul_fragment::ANY ), max_fragments_consumed );
      } );

    return demon_hunter_spell_t::create_expression( name );
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
        soul_fragments_to_spawn( as<unsigned>( p->talent.vengeance.sigil_of_spite->effectN( 3 ).base_value() ) )
    {
      reduced_aoe_targets = p->talent.vengeance.sigil_of_spite->effectN( 1 ).base_value();
    }

    void execute() override
    {
      demon_hunter_sigil_t::execute();
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_sigil_of_spite, soul_fragment::LESSER,
                                 soul_fragments_to_spawn );
    }
  };

  sigil_of_spite_t( demon_hunter_t* p, util::string_view options_str )
    : demon_hunter_spell_t( "sigil_of_spite", p, p->talent.vengeance.sigil_of_spite, options_str )
  {
    if ( data().ok() && !p->active.sigil_of_spite )
    {
      p->active.sigil_of_spite = p->get_background_action<sigil_of_spite_sigil_t>(
          "sigil_of_spite_sigil", p->spec.sigil_of_spite_damage, ground_aoe_duration );
      add_child( p->active.sigil_of_spite );
    }
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
    debug_cast<demon_hunter_sigil_t*>( dh()->active.sigil_of_spite )->place_sigil( target );
    dh()->buff.reavers_glaive->trigger();
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( auto e = debug_cast<demon_hunter_sigil_t*>( dh()->active.sigil_of_spite )->create_sigil_expression( name ) )
      return e;

    return demon_hunter_spell_t::create_expression( name );
  }
};

// The Hunt =================================================================

struct the_hunt_base_t
  : public voidrush_trigger_t<hungering_slash_trigger_t<
        unbound_chaos_trigger_t<inertia_trigger_trigger_t<exergy_trigger_t<demon_hunter_spell_t>>>>>
{
  struct the_hunt_dot_t : public demon_hunter_spell_t
  {
    the_hunt_dot_t( util::string_view n, demon_hunter_t* p )
      : demon_hunter_spell_t( fmt::format( "{}_dot", n ), p, p->spec.the_hunt_dot )
    {
      dual = true;
      aoe  = as<int>( p->spec.the_hunt->effectN( 2 ).trigger()->effectN( 1 ).base_value() );
    }
  };

  struct the_hunt_damage_t : public demon_hunter_spell_t
  {
    the_hunt_damage_t( util::string_view n, demon_hunter_t* p )
      : demon_hunter_spell_t( fmt::format( "{}_damage", n ), p, p->spec.the_hunt_impact )
    {
      dual          = true;
      impact_action = p->get_background_action<the_hunt_dot_t>( n );
      add_child( impact_action );
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      if ( s->chain_target == 0 && dh()->talent.devourer.devourers_bite->ok() )
      {
        td( target )->debuffs.devourers_bite->trigger();
      }

      if ( s->chain_target == 0 && dh()->talent.aldrachi_reaver.broken_spirit->ok() )
      {
        dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_broken_spirit, soul_fragment::LESSER );
      }

      if ( s->chain_target == 0 && dh()->specialization() == DEMON_HUNTER_DEVOURER &&
           dh()->talent.scarred.violent_transformation->ok() )
      {
        // only resets one charge of Soul Immo
        dh()->cooldown.soul_immolation->reset( true, 1 );
      }
    }
  };

  the_hunt_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : base_t( n, p, s, o )
  {
    movement_directionality = movement_direction_type::TOWARDS;
    impact_action           = p->get_background_action<the_hunt_damage_t>( n );
    add_child( impact_action );
  }

  void execute() override
  {
    base_t::execute();

    dh()->set_out_of_range( timespan_t::zero() );  // Cancel all other movement

    dh()->buff.reavers_glaive->trigger();

    dh()->buff.empowered_eye_beam->trigger();
  }

  timespan_t travel_time() const override
  {
    return 100_ms;
  }
};

struct predators_wake_t : public voidsurge_trigger_t<voidsurge_ability::PREDATORS_WAKE, the_hunt_base_t>
{
  predators_wake_t( demon_hunter_t* p, util::string_view o )
    : base_t( "predators_wake", p, p->hero_spec.predators_wake, o )
  {
  }

  bool action_ready() override
  {
    if ( !dh()->talent.scarred.demonic_intensity->ok() || !dh()->buff.metamorphosis->check() )
    {
      return false;
    }
    return base_t::action_ready();
  }
};

struct the_hunt_t : public the_hunt_base_t
{
  the_hunt_t( demon_hunter_t* p, util::string_view o ) : the_hunt_base_t( "the_hunt", p, p->spec.the_hunt, o )
  {
  }

  bool action_ready() override
  {
    if ( dh()->specialization() == DEMON_HUNTER_DEVOURER && dh()->talent.scarred.demonic_intensity->ok() &&
         dh()->buff.metamorphosis->check() )
    {
      return false;
    }
    return the_hunt_base_t::action_ready();
  }
};

struct surge_base_t : public demon_hunter_spell_t
{
  surge_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s ) : demon_hunter_spell_t( n, p, s )
  {
    background = dual   = true;
    aoe                 = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = demon_hunter_spell_t::composite_da_multiplier( s );

    // Focused Hatred increases _surge damage when hitting less than 6 targets
    if ( dh()->talent.scarred.focused_hatred->ok() && s->n_targets <= 5 )
    {
      // 1 target is always effect 1 %
      // 2 target is effect 1 % - effect 2 %
      // 3 target is effect 1 % - (effect 2 % * 2)
      // etc up to 5 target
      auto num_target_reduction_percent =
          dh()->talent.scarred.focused_hatred->effectN( 2 ).percent() * ( s->n_targets - 1 );
      m *= 1.0 + ( dh()->talent.scarred.focused_hatred->effectN( 1 ).percent() - num_target_reduction_percent );
    }

    return m;
  }
};

struct demonsurge_t : public surge_base_t
{
  demonsurge_t( util::string_view name, demon_hunter_t* p ) : surge_base_t( name, p, p->hero_spec.demonsurge_damage )
  {
  }

  void execute() override
  {
    surge_base_t::execute();
    dh()->buff.demonsurge->trigger();
  }
};

struct voidsurge_t : public surge_base_t
{
  voidsurge_t( util::string_view name, demon_hunter_t* p ) : surge_base_t( name, p, p->hero_spec.voidsurge_damage )
  {
  }

  void execute() override
  {
    surge_base_t::execute();
    dh()->buff.voidsurge->trigger();
  }
};

struct consume_base_t : public shattered_souls_trigger_t<voidfall_building_trigger_t<demon_hunter_spell_t>>
{
  proc_t* soul_fragment_generation_proc;

  consume_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : base_t( n, p, s, o ), soul_fragment_generation_proc( nullptr )
  {
    cooldown = p->cooldown.consume;
  }

  void execute() override
  {
    base_t::execute();

    if ( dh()->talent.devourer.predators_thirst->ok() )
    {
      dh()->spawn_soul_fragment( soul_fragment_generation_proc, soul_fragment::LESSER,
                                 as<unsigned int>( dh()->spec.shattered_souls->effectN( 3 ).base_value() ) );
    }
  }
};

struct devour_t : public consume_base_t
{
  timespan_t reap_cdr;

  devour_t( demon_hunter_t* p, util::string_view o ) : consume_base_t( "devour", p, p->spec.devour, o )
  {
    reap_cdr                = timespan_t::from_millis( p->spec.void_metamorphosis->effectN( 14 ).base_value() );
    execute_energize_action = p->get_background_action<demon_hunter_energize_t>(
        "devour_energize", p->spec.devour_energize->ok() ? p->spec.devour_energize : p->spec.consume_energize );
  }

  void init_finished() override
  {
    consume_base_t::init_finished();

    soul_fragment_generation_proc = dh()->proc.soul_fragment_from_devour;
  }

  void execute() override
  {
    consume_base_t::execute();

    dh()->cooldown.reap->adjust( -reap_cdr );
  }

  bool action_ready() override
  {
    if ( !dh()->buff.metamorphosis->check() )
    {
      return false;
    }
    return consume_base_t::action_ready();
  }
};

struct consume_t : public consume_base_t
{
  consume_t( demon_hunter_t* p, util::string_view o ) : consume_base_t( "consume", p, p->spec.consume, o )
  {
    execute_energize_action =
        p->get_background_action<demon_hunter_energize_t>( "consume_energize", p->spec.consume_energize );
  }

  void init_finished() override
  {
    consume_base_t::init_finished();

    soul_fragment_generation_proc = dh()->proc.soul_fragment_from_consume;
  }

  bool action_ready() override
  {
    if ( dh()->buff.metamorphosis->check() )
    {
      return false;
    }
    return consume_base_t::action_ready();
  }
};

struct voidblade_base_t : public voidrush_trigger_t<hungering_slash_trigger_t<demon_hunter_spell_t>>
{
  struct voidblade_damage_t : public burning_blades_trigger_t<shattered_souls_trigger_t<demon_hunter_spell_t>>
  {
    voidblade_damage_t( util::string_view name, demon_hunter_t* p ) : base_t( name, p, p->spec.voidblade )
    {
      background = dual = true;
      gain              = p->get_gain( "voidblade" );
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      if ( result_is_hit( s->result ) && dh()->talent.devourer.devourers_bite->ok() )
      {
        td( s->target )->debuffs.devourers_bite->trigger();
      }
    }
  };

  voidblade_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : base_t( n, p, s, o )
  {
    cooldown = p->cooldown.voidblade;

    may_block               = false;
    movement_directionality = movement_direction_type::TOWARDS;

    execute_action = p->get_background_action<voidblade_damage_t>( fmt::format( "{}_damage", n ) );
    add_child( execute_action );
  }

  bool action_ready() override
  {
    if ( dh()->buff.hungering_slash->check() )
    {
      return false;
    }

    return base_t::action_ready();
  }
};

struct pierce_the_veil_t : public voidsurge_trigger_t<voidsurge_ability::PIERCE_THE_VEIL, voidblade_base_t>
{
  pierce_the_veil_t( demon_hunter_t* p, util::string_view o )
    : base_t( "pierce_the_veil", p, p->hero_spec.pierce_the_veil, o )
  {
  }

  bool action_ready() override
  {
    if ( !dh()->buff.metamorphosis->check() )
    {
      return false;
    }

    return base_t::action_ready();
  }
};

struct voidblade_t : public voidblade_base_t
{
  voidblade_t( demon_hunter_t* p, util::string_view o )
    : voidblade_base_t( "voidblade", p, p->talent.demon_hunter.voidblade, o )
  {
  }

  bool action_ready() override
  {
    if ( dh()->talent.scarred.demonsurge->ok() && dh()->buff.metamorphosis->check() )
    {
      return false;
    }

    return voidblade_base_t::action_ready();
  }
};

struct void_buildup_t : public demon_hunter_spell_t
{
  void_buildup_t( util::string_view n, demon_hunter_t* p ) : demon_hunter_spell_t( n, p, p->spec.void_buildup, "" )
  {
    may_miss = may_block = may_dodge = may_parry = callbacks = false;
    background = dual = true;
    target            = p;

    resource_current            = RESOURCE_FURY;
    base_costs[ RESOURCE_FURY ] = -data().effectN( 1 ).resource( RESOURCE_FURY );
  }

  void consume_resource() override
  {
    // Bypass demon_hunter_spell_t::consume_resource as that does additional logic including refunds we do not wish to
    // apply here as we will be calling this without executing the action for performance reasons.
    spell_t::consume_resource();
  }
};

struct soul_immolation_heal_t : public demon_hunter_heal_t
{
  soul_immolation_heal_t( demon_hunter_t* p, util::string_view o )
    : demon_hunter_heal_t( "soul_immolation", p, p->talent.devourer.soul_immolation, o )
  {
    tick_energize_action = p->get_background_action<demon_hunter_energize_t>( fmt::format( "{}_energize", name() ),
                                                                              p->spec.soul_immolation_energize );
  }

  void execute() override
  {
    target = dh();
    demon_hunter_heal_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    assert( s->target == dh() );
    demon_hunter_heal_t::impact( s );
  }

  void tick( dot_t* d ) override
  {
    demon_hunter_heal_t::tick( d );

    // seems to spawn a soul fragment every other tick, starting with the first tick
    if ( d->current_tick % 2 == 0 )
    {
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_soul_immolation, soul_fragment::LESSER, 1 );
    }
  }

  void last_tick( dot_t* d ) override
  {
    demon_hunter_heal_t::last_tick( d );

    if ( dh()->talent.scarred.undying_embers->ok() &&
         rng().roll( dh()->talent.scarred.undying_embers->effectN( 2 ).percent() ) )
    {
      dh()->proc.undying_embers->occur();

      // retriggers the DoT but doesn't count as a cast/execute
      action_state_t* undying_embers_state = get_state();
      undying_embers_state->target         = d->state->target;
      snapshot_state( undying_embers_state, result_amount_type::HEAL_OVER_TIME );

      make_event<undying_embers_event_t>( *sim, dh(), this, undying_embers_state );
    }
  }
};

struct reap_base_t : public voidfall_spending_trigger_t<meteoric_fall_trigger_t<demon_hunter_spell_t>>
{
  struct reap_damage_t : public shattered_souls_trigger_t<demon_hunter_spell_t>
  {
    reap_damage_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s ) : base_t( n, p, s, "" )
    {
      background = dual = true;
    }
  };

  reap_damage_t* damage_action;

  reap_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o,
               const spell_data_t* damage_s, const spell_data_t* energize_s = nullptr )
    : base_t( n, p, s, o ), damage_action( nullptr )
  {
    cooldown = p->cooldown.reap;

    damage_action = p->get_background_action<reap_damage_t>( fmt::format( "{}_damage", n ), damage_s );
    add_child( damage_action );

    if ( p->talent.devourer.scythes_embrace->ok() && energize_s )
    {
      execute_energize_action =
          p->get_background_action<demon_hunter_energize_t>( fmt::format( "{}_energize", n ), energize_s );
    }
  }

  virtual unsigned int souls_to_consume() const
  {
    unsigned int souls = as<unsigned int>( dh()->spec.shattered_souls->effectN( 2 ).base_value() );
    if ( dh()->buff.moment_of_craving->up() )
    {
      souls += as<unsigned int>( dh()->buff.moment_of_craving->check_value() );
    }
    return souls;
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "max_souls_consumed" ) )
      return make_fn_expr( name, [ this ]() { return souls_to_consume(); } );

    if ( util::str_compare_ci( name, "souls_consumed" ) )
      return make_fn_expr( name, [ this ]() {
        return std::min( dh()->get_active_soul_fragments( soul_fragment::ANY ), souls_to_consume() );
      } );

    return base_t::create_expression( name );
  }

  void execute() override
  {
    dh()->buff.reap->trigger();

    base_t::execute();
    unsigned fragments_consumed = dh()->consume_soul_fragments( soul_fragment::LESSER, false, souls_to_consume() );

    // TOCHECK: This delay is a guess based on averages in logs as there is no spelldata
    make_event( *dh()->sim, 220_ms, [ this, fragments_consumed ] {
      damage_action->set_target( target );
      action_state_t* damage_state = damage_action->get_state();
      damage_state->target         = target;
      damage_action->snapshot_state( damage_state, result_amount_type::DMG_DIRECT );

      if ( dh()->talent.devourer.soulshaper->ok() )
      {
        damage_state->da_multiplier *=
            1.0 + fragments_consumed * dh()->talent.devourer.soulshaper->effectN( 1 ).percent();
      }
      damage_action->schedule_execute( damage_state );
    } );

    dh()->buff.moment_of_craving->expire();
  }
};

struct eradicate_t : public voidfall_spending_trigger_t<meteoric_fall_trigger_t<demon_hunter_spell_t>>
{
  struct eradicate_damage_t : public shattered_souls_trigger_t<demon_hunter_spell_t>
  {
    eradicate_damage_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s ) : base_t( n, p, s, "" )
    {
      background = dual = true;
    }
  };

  eradicate_damage_t* damage_action;
  eradicate_damage_t* damage_action_meta;

  eradicate_t( demon_hunter_t* p, util::string_view o )
    : base_t( "eradicate", p, p->spec.eradicate, o ), damage_action( nullptr ), damage_action_meta( nullptr )
  {
    cooldown = p->cooldown.reap;

    damage_action      = p->get_background_action<eradicate_damage_t>( "eradicate_reap", p->spec.eradicate_damage );
    damage_action->aoe = -1;
    damage_action->reduced_aoe_targets = p->spec.eradicate->effectN( 1 ).base_value();
    add_child( damage_action );

    if ( p->talent.devourer.void_metamorphosis->ok() )
    {
      damage_action_meta =
          p->get_background_action<eradicate_damage_t>( "eradicate_cull", p->spec.eradicate_damage_meta );
      damage_action_meta->aoe                 = -1;
      damage_action_meta->reduced_aoe_targets = p->spec.eradicate->effectN( 1 ).base_value();
      add_child( damage_action_meta );
    }

    if ( p->talent.devourer.scythes_embrace->ok() )
    {
      execute_energize_action =
          p->get_background_action<demon_hunter_energize_t>( "eradicate_energize", p->spec.reap_energize );
    }
  }

  virtual unsigned int souls_to_consume() const
  {
    unsigned int souls = as<unsigned int>( dh()->spec.shattered_souls->effectN( 2 ).base_value() );
    if ( dh()->buff.moment_of_craving->up() )
    {
      souls += as<unsigned int>( dh()->buff.moment_of_craving->check_value() );
    }
    return souls;
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "max_souls_consumed" ) )
      return make_fn_expr( name, [ this ]() { return souls_to_consume(); } );

    if ( util::str_compare_ci( name, "souls_consumed" ) )
      return make_fn_expr( name, [ this ]() {
        return std::min( dh()->get_active_soul_fragments( soul_fragment::ANY ), souls_to_consume() );
      } );

    return base_t::create_expression( name );
  }

  void execute() override
  {
    dh()->buff.reap->trigger();

    base_t::execute();

    unsigned fragments_consumed = dh()->consume_soul_fragments( soul_fragment::LESSER, false, souls_to_consume() );
    auto damage                 = dh()->buff.metamorphosis->up() ? damage_action_meta : damage_action;

    // TOCHECK: This delay is a guess based on averages in logs as there is no spelldata
    make_event( *dh()->sim, 220_ms, [ this, fragments_consumed, damage ] {
      damage->set_target( target );
      action_state_t* damage_state = damage->get_state();
      damage_state->target         = target;
      damage->snapshot_state( damage_state, result_amount_type::DMG_DIRECT );

      if ( dh()->talent.devourer.soulshaper->ok() )
      {
        damage_state->da_multiplier *=
            1.0 + fragments_consumed * dh()->talent.devourer.soulshaper->effectN( 1 ).percent();
      }

      damage->schedule_execute( damage_state );
    } );
    dh()->buff.moment_of_craving->expire();

    dh()->buff.eradicate->expire();
  }

  bool action_ready() override
  {
    if ( !dh()->buff.eradicate->check() )
    {
      return false;
    }

    return base_t::action_ready();
  }
};

struct cull_t : public reap_base_t
{
  cull_t( demon_hunter_t* p, util::string_view o )
    : reap_base_t( "cull", p, p->spec.cull, o, p->spec.cull_damage, p->spec.reap_energize )
  {
  }

  bool action_ready() override
  {
    if ( dh()->buff.eradicate->check() || !dh()->buff.metamorphosis->check() )
    {
      return false;
    }

    return reap_base_t::action_ready();
  }
};

struct reap_t : public reap_base_t
{
  reap_t( demon_hunter_t* p, util::string_view o )
    : reap_base_t( "reap", p, p->spec.reap, o, p->spec.reap_damage, p->spec.reap_energize )
  {
  }

  bool action_ready() override
  {
    if ( dh()->buff.eradicate->check() || dh()->buff.metamorphosis->check() )
    {
      return false;
    }

    return reap_base_t::action_ready();
  }
};

struct void_ray_t
  : public student_of_suffering_trigger_t<final_breath_trigger_t<doomsayer_trigger_t<demon_hunter_spell_t>>>
{
  struct void_ray_tick_t : public shattered_souls_trigger_t<demon_hunter_spell_t>
  {
    void_ray_tick_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s ) : base_t( n, p, s )
    {
      background = dual = true;
      aoe               = -1;

      shattered_souls_base_chance *= 1.0 + p->talent.devourer.waste_not->effectN( 1 ).percent();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = base_t::composite_da_multiplier( s );

      if ( dh()->talent.devourer.focused_ray->ok() &&
           s->n_targets <= as<unsigned int>( dh()->talent.devourer.focused_ray->effectN( 2 ).base_value() ) )
      {
        m *= 1.0 + dh()->talent.devourer.focused_ray->effectN( 1 ).percent();
      }

      return m;
    }

    timespan_t execute_time() const override
    {
      // Void Beam is applied via a player aura and experiences aura delay in applying damage tick events
      // Not a perfect implementation, but closer than the instant execution in current sims
      return rng().gauss( dh()->sim->default_aura_delay );
    }

    double cost() const override
    {
      // handled at the action level
      return 0;
    }

    double shattered_souls_chance( action_state_t* s ) override
    {
      double m = base_t::shattered_souls_chance( s );

      m /= s->n_targets;

      return m;
    }
  };

  void_ray_tick_t* tick;
  void_ray_tick_t* tick_meta;
  demon_hunter_energize_t* voidglare_boon_energize;

  void_ray_t( demon_hunter_t* p, util::string_view o )
    : base_t( "void_ray", p, p->talent.devourer.void_ray, o ), voidglare_boon_energize( nullptr )
  {
    may_miss            = false;
    channeled           = true;
    tick_on_application = false;

    // stealing this from eye beam
    ability_lag = p->world_lag;

    tick = p->get_background_action<void_ray_tick_t>( "void_ray_tick", p->spec.void_ray_tick );
    add_child( tick );

    tick_meta = p->get_background_action<void_ray_tick_t>( "void_ray_tick_meta", p->spec.void_ray_tick_meta );
    add_child( tick_meta );

    if ( p->talent.devourer.voidglare_boon->ok() )
    {
      voidglare_boon_energize = p->get_background_action<demon_hunter_energize_t>( "voidglare_boon_energize",
                                                                                   p->spec.voidglare_boon_energize );
    }

    // forces hasted cooldown due to the spell baseline having no cooldown duration
    if ( data().affected_by( p->spec.devourer_demon_hunter->effectN( 6 ) ) )
    {
      cooldown->hasted = true;
    }

    // Add damage modifiers in voidray_tick_t, not here.
  }

  void init() override
  {
    base_t::init();

    consume_per_tick_ = true;
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );

    if ( d->current_tick >= ( d->num_ticks() - dh()->options.channel_tick_cutoff_benefit ) )
    {
      if ( dh()->talent.devourer.eradicate->ok() )
      {
        dh()->buff.eradicate->trigger();
      }
      if ( dh()->talent.devourer.moment_of_craving->ok() )
      {
        dh()->buff.moment_of_craving->trigger();
        dh()->cooldown.reap->reset( true );
      }
      if ( voidglare_boon_energize )
      {
        voidglare_boon_energize->execute();
      }

      if ( dh()->talent.annihilator.meteoric_rise->ok() )
      {
        dh()->proc.voidfall_from_meteoric_rise->occur();
        dh()->buff.voidfall_building->trigger();
      }
    }

    dh()->devourer_fury_state.reschedule_drain();
  }

  void execute() override
  {
    tick_action        = dh()->buff.metamorphosis->up() ? tick_meta : tick;
    cooldown->duration = dh()->buff.metamorphosis->up() ? dh()->spec.void_metamorphosis->effectN( 8 ).time_value() +
                                                              dh()->talent.devourer.voidpurge->effectN( 1 ).time_value()
                                                        : dh()->talent.devourer.void_ray->cooldown();

    base_t::execute();
    dh()->devourer_fury_state.reschedule_drain();
  }

  bool action_ready() override
  {
    // Void Ray requires 100 Fury to cast but doesn't cost 100 Fury
    if ( dh()->buff.metamorphosis->up() || dh()->resource_available( RESOURCE_FURY, data().cost( POWER_FURY ) ) )
    {
      return base_t::action_ready();
    }
    return false;
  }

  double cost() const override
  {
    // Void Ray requires 100 Fury to cast but doesn't cost 100 Fury
    return 0;
  }

  double cost_per_tick( resource_e r ) const override
  {
    if ( r != RESOURCE_FURY )
    {
      return base_t::cost_per_tick( r );
    }

    // Void Ray costs 5 Fury per tick outside of Meta, 0 in Meta
    return dh()->buff.metamorphosis->check() ? dh()->spec.void_ray_tick_meta->cost( POWER_FURY )
                                             : dh()->spec.void_ray_tick->cost( POWER_FURY );
  }

  result_amount_type amount_type( const action_state_t*, bool ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }
};

struct collapsing_star_t : public demon_hunter_spell_t
{
  struct collapsing_star_damage_t
    : public shattered_souls_trigger_t<otherworldly_focus_benefit_t<dark_matter_trigger_t<demon_hunter_spell_t>>>
  {
    collapsing_star_damage_t( std::string_view n, demon_hunter_t* p ) : base_t( n, p, p->spec.collapsing_star_damage )
    {
      background = dual   = true;
      aoe                 = -1;
      reduced_aoe_targets = p->spec.collapsing_star_spell->effectN( 1 ).base_value();
    }

    double composite_crit_damage_bonus_multiplier() const override
    {
      auto cm = base_t::composite_crit_damage_bonus_multiplier();

      if ( dh()->talent.devourer.midnight2->ok() )
      {
        cm *= 1.0 + dh()->talent.devourer.midnight2->effectN( 3 ).percent() * dh()->cache.spell_crit_chance();
      }

      return cm;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = base_t::composite_da_multiplier( s );

      if ( s->chain_target == 0 )
      {
        m *= 1.0 + dh()->spec.collapsing_star_spell->effectN( 2 ).percent();
      }

      return m;
    }

    void execute() override
    {
      base_t::execute();

      if ( dh()->talent.devourer.star_fragments->ok() )
      {
        unsigned fragments_to_spawn = as<unsigned>( dh()->talent.devourer.star_fragments->effectN( 1 ).base_value() );
        dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_star_fragments, soul_fragment::LESSER,
                                   fragments_to_spawn );
      }

      if ( dh()->talent.devourer.voidrush->ok() )
      {
        dh()->cooldown.voidblade->adjust( -dh()->talent.devourer.voidrush->effectN( 1 ).time_value() );
      }
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      if ( dh()->talent.devourer.impending_apocalypse->ok() && s->chain_target == 0 )
      {
        make_event( *dh()->sim, 1.2_s, [ this ] { dh()->buff.impending_apocalypse->trigger(); } );
      }
    }
  };

  int soul_cost;
  collapsing_star_t( demon_hunter_t* p, util::string_view o )
    : demon_hunter_spell_t( "collapsing_star", p, p->spec.collapsing_star_spell, o ),
      soul_cost( as<int>( p->talent.devourer.collapsing_star->effectN( 1 ).base_value() ) )
  {
    execute_action = p->get_background_action<collapsing_star_damage_t>( "collapsing_star_damage" );
    add_child( execute_action );

    if ( p->sets->has_set_bonus( DEMON_HUNTER_DEVOURER, MID1, B4 ) )
    {
      execute_energize_action =
          p->get_background_action<demon_hunter_energize_t>( "stars_fury", p->set_bonuses.stars_fury );
    }

    if ( p->talent.annihilator.dark_matter->ok() && p->active.meteor_shower )
    {
      add_child( p->active.meteor_shower );
    }
  }

  void execute() override
  {
    dh()->buff.collapsing_star->expire();
    dh()->buff.collapsing_star_stack->decrement( soul_cost );
    demon_hunter_spell_t::execute();
  }

  bool action_ready() override
  {
    if ( !dh()->buff.collapsing_star->check() )
    {
      return false;
    }

    return demon_hunter_spell_t::action_ready();
  }
};

struct voidfall_meteor_base_t : public demon_hunter_spell_t
{
  struct voidfall_meteor_damage_t
    : public shattered_souls_trigger_t<otherworldly_focus_benefit_t<catastrophe_trigger_t<demon_hunter_spell_t>>>
  {
    voidfall_meteor_damage_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s ) : base_t( n, p, s )
    {
      background = dual   = true;
      aoe                 = -1;
      reduced_aoe_targets = p->talent.annihilator.voidfall->effectN( 2 ).base_value();
    }
  };

  voidfall_meteor_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s )
    : demon_hunter_spell_t( n, p, s )
  {
    impact_action = p->get_background_action<voidfall_meteor_damage_t>( fmt::format( "{}_damage", name() ),
                                                                        s->effectN( 2 ).trigger() );
    add_child( impact_action );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    dh()->buff.voidfall_spending->decrement();
  }
};

struct voidfall_meteor_t : public voidfall_meteor_base_t
{
  voidfall_meteor_t( util::string_view n, demon_hunter_t* p )
    : voidfall_meteor_base_t( n, p, p->hero_spec.voidfall_meteor )
  {
    if ( p->talent.annihilator.world_killer->ok() && p->active.world_killer )
    {
      add_child( p->active.world_killer );
    }
  }
};

struct world_killer_t : public voidfall_meteor_base_t
{
  world_killer_t( util::string_view n, demon_hunter_t* p ) : voidfall_meteor_base_t( n, p, p->hero_spec.world_killer )
  {
  }

  void impact( action_state_t* s ) override
  {
    voidfall_meteor_base_t::impact( s );

    switch ( dh()->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_world_killer, soul_fragment::LESSER,
                                   as<int>( dh()->talent.annihilator.world_killer->effectN( 4 ).base_value() ) );
        break;
      case DEMON_HUNTER_VENGEANCE:
        dh()->cooldown.metamorphosis->adjust(
            -timespan_t::from_seconds( as<int>( dh()->talent.annihilator.world_killer->effectN( 3 ).base_value() ) ) );
        break;
      default:
        break;
    }
  }
};

struct catastrophe_t : public residual_action::residual_periodic_action_t<demon_hunter_spell_t>
{
  catastrophe_t( util::string_view name, demon_hunter_t* p ) : base_t( name, p, p->hero_spec.catastrophe_dot )
  {
    dual = true;
  }

  void init() override
  {
    base_t::init();
    update_flags = 0;  // Snapshots on refresh, does not update dynamically
  }
};

struct meteor_shower_t : public demon_hunter_spell_t
{
  struct meteor_shower_damage_t
    : public shattered_souls_trigger_t<otherworldly_focus_benefit_t<catastrophe_trigger_t<demon_hunter_spell_t>>>
  {
    meteor_shower_damage_t( util::string_view n, demon_hunter_t* p ) : base_t( n, p, p->hero_spec.meteor_shower_damage )
    {
      background = dual = true;
      aoe               = -1;
    }

    void execute() override
    {
      base_t::execute();
    }
  };

  meteor_shower_damage_t* damage;

  meteor_shower_t( util::string_view n, demon_hunter_t* p )
    : demon_hunter_spell_t( n, p, p->hero_spec.meteor_shower_driver )
  {
    damage = p->get_background_action<meteor_shower_damage_t>( fmt::format( "{}_damage", name() ) );
    add_child( damage );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    int tick_count        = as<int>( dh()->talent.annihilator.dark_matter->effectN( 1 ).base_value() );
    timespan_t duration   = timespan_t::from_seconds( tick_count / 2 );
    timespan_t pulse_time = duration / tick_count;

    make_event<ground_aoe_event_t>( *sim, dh(),
                                    ground_aoe_params_t()
                                        .target( target )
                                        .x( target->x_position )
                                        .y( target->y_position )
                                        .pulse_time( pulse_time )
                                        .duration( duration )
                                        .action( damage ) );
  }
};

struct hungering_slash_base_t : public demon_hunter_spell_t
{
  struct hungering_slash_damage_t : public shattered_souls_trigger_t<burning_blades_trigger_t<demon_hunter_spell_t>>
  {
    int number_of_souls_to_spawn;
    proc_t* soul_generation_proc;

    hungering_slash_damage_t( util::string_view n, demon_hunter_t* p, int souls )
      : base_t( n, p, p->spec.hungering_slash_damage ),
        number_of_souls_to_spawn( souls ),
        soul_generation_proc( nullptr )
    {
      background = dual   = true;
      aoe                 = -1;
      reduced_aoe_targets = p->spec.hungering_slash->effectN( 2 ).base_value();
    }

    void execute() override
    {
      base_t::execute();

      dh()->spawn_soul_fragment( soul_generation_proc, soul_fragment::LESSER, number_of_souls_to_spawn );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = base_t::composite_da_multiplier( s );

      if ( dh()->talent.devourer.singular_strikes->ok() && s->chain_target == 0 )
      {
        m *= 1.0 + dh()->talent.devourer.singular_strikes->effectN( 2 ).percent();
      }

      return m;
    }
  };

  hungering_slash_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view o )
    : demon_hunter_spell_t( n, p, s, o )
  {
    execute_action = p->get_background_action<hungering_slash_damage_t>( fmt::format( "{}_damage", n ),
                                                                         as<int>( s->effectN( 1 ).base_value() ) );
    add_child( execute_action );
    execute_energize_action = p->get_background_action<demon_hunter_energize_t>( fmt::format( "{}_energize", n ),
                                                                                 p->spec.hungering_slash_energize );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    dh()->buff.hungering_slash->expire();
    dh()->buff.voidstep->trigger();
  }

  bool action_ready() override
  {
    if ( !dh()->buff.hungering_slash->check() )
    {
      return false;
    }

    return demon_hunter_spell_t::action_ready();
  }
};

struct reapers_toll_t : public voidsurge_trigger_t<voidsurge_ability::REAPERS_TOLL, hungering_slash_base_t>
{
  reapers_toll_t( demon_hunter_t* p, util::string_view o ) : base_t( "reapers_toll", p, p->hero_spec.reapers_toll, o )
  {
  }

  void init_finished() override
  {
    base_t::init_finished();

    debug_cast<hungering_slash_damage_t*>( execute_action )->soul_generation_proc =
        dh()->proc.soul_fragment_from_reapers_toll;
  }

  bool action_ready() override
  {
    if ( !dh()->buff.metamorphosis->check() )
    {
      return false;
    }
    return base_t::action_ready();
  }
};

struct hungering_slash_t : public hungering_slash_base_t
{
  hungering_slash_t( demon_hunter_t* p, util::string_view o )
    : hungering_slash_base_t( "hungering_slash", p, p->spec.hungering_slash, o )
  {
  }

  void init_finished() override
  {
    base_t::init_finished();

    debug_cast<hungering_slash_damage_t*>( execute_action )->soul_generation_proc =
        dh()->proc.soul_fragment_from_hungering_slash;
  }

  bool action_ready() override
  {
    if ( dh()->talent.scarred.demonsurge->ok() && dh()->buff.metamorphosis->check() )
    {
      return false;
    }
    return hungering_slash_base_t::action_ready();
  }
};

struct explosion_of_the_soul_t : public demon_hunter_spell_t
{
  explosion_of_the_soul_t( util::string_view n, demon_hunter_t* p )
    : demon_hunter_spell_t( n, p, p->set_bonuses.explosion_of_the_soul )
  {
    background = dual   = true;
    aoe                 = -1;
    reduced_aoe_targets = as<int>( p->set_bonuses.mid1_vengeance_4pc->effectN( 2 ).base_value() );
  }
};

struct rolling_torment_energize_t : demon_hunter_energize_t
{
  rolling_torment_energize_t( util::string_view n, demon_hunter_t* p )
    : demon_hunter_energize_t( n, p, p->spec.rolling_torment_energize )
  {
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double e = demon_hunter_energize_t::composite_energize_amount( s );

    int stacks = dh()->buff.collapsing_star_stack->check();

    // 2026-04-29 -- Celestial Echoes is additive, not multiplicative.
    return e * stacks + dh()->talent.annihilator.celestial_echoes->effectN( 2 ).base_value();
  }
};

}  // end namespace spells

// ==========================================================================
// Demon Hunter attacks
// ==========================================================================

namespace attacks
{

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

    if ( !BASE::dh()->talent.havoc.soulscar->ok() )
      return;

    if ( !action_t::result_is_hit( s->result ) )
      return;

    const double dot_damage = s->result_amount * BASE::dh()->talent.havoc.soulscar->effectN( 1 ).percent();
    residual_action::trigger( BASE::dh()->active.soulscar, s->target, dot_damage );
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

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = base_t::composite_target_da_multiplier( t );

    demon_hunter_td_t* target_data = td( t );
    if ( target_data->debuffs.reavers_mark->up() )
    {
      m *= 1.0 + target_data->debuffs.reavers_mark->check_stack_value();
    }

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
    if ( dh()->current.distance_to_move > 5 || dh()->buff.out_of_range->check() )
    {
      aa_contact c = aa_contact::LOST_RANGE;
      dh()->proc.delayed_aa_range->occur();

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
    dh()->main_hand_attack->set_target( target );
    if ( dh()->main_hand_attack->execute_event == nullptr )
    {
      dh()->main_hand_attack->schedule_execute();
    }

    dh()->off_hand_attack->set_target( target );
    if ( dh()->off_hand_attack->execute_event == nullptr )
    {
      dh()->off_hand_attack->schedule_execute();
    }
  }

  bool ready() override
  {
    // Range check
    if ( !demon_hunter_attack_t::ready() )
      return false;

    if ( dh()->main_hand_attack->execute_event == nullptr || dh()->off_hand_attack->execute_event == nullptr )
      return true;

    return false;
  }
};

// Blade Dance =============================================================

struct blade_dance_base_t
  : public art_of_the_glaive_trigger_t<art_of_the_glaive_ability::GLAIVE_FLURRY, demon_hunter_attack_t>
{
  struct trail_of_ruin_dot_t : public demon_hunter_spell_t
  {
    trail_of_ruin_dot_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->talent.havoc.trail_of_ruin->effectN( 1 ).trigger() )
    {
      background = dual = true;
    }
  };

  struct blade_dance_damage_first_blood_t : public burning_blades_trigger_t<demon_hunter_attack_t>
  {
    timespan_t delay;
    action_t* trail_of_ruin_dot;
    bool last_attack;

    blade_dance_damage_first_blood_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s,
                                      const spelleffect_data_t& eff )
      : base_t( n, p, s ),
        delay( timespan_t::from_millis( eff.misc_value1() ) ),
        trail_of_ruin_dot( nullptr ),
        last_attack( false )
    {
      background = dual = true;
      aoe               = 0;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = base_t::composite_da_multiplier( s );
      m *= 1.0 + dh()->talent.havoc.first_blood->effectN( 1 ).percent();
      return m;
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      if ( result_is_hit( s->result ) && td( s->target )->debuffs.essence_break->up() )
      {
        cooldown_t* tcd = dh()->cooldown.essence_break_proc_icd->get_cooldown( s->target );
        if ( tcd->up() )
        {
          dh()->active.essence_break_proc->execute_on_target( s->target );
          tcd->start();
        }
      }

      if ( last_attack )
      {
        if ( trail_of_ruin_dot )
        {
          trail_of_ruin_dot->execute_on_target( s->target );
        }
      }
    }
  };

  struct blade_dance_damage_t : public demon_hunter_attack_t
  {
    timespan_t delay;
    action_t* trail_of_ruin_dot;
    bool last_attack;
    unsigned glaive_tempest_targets;

    blade_dance_damage_t( util::string_view name, demon_hunter_t* p, const spelleffect_data_t& eff,
                          const spell_data_t* first_blood_override = nullptr )
      : demon_hunter_attack_t( name, p, first_blood_override ? first_blood_override : eff.trigger() ),
        delay( timespan_t::from_millis( eff.misc_value1() ) ),
        trail_of_ruin_dot( nullptr ),
        last_attack( false )
    {
      background = dual      = true;
      aoe                    = -1;
      reduced_aoe_targets    = p->find_spell( 199552 )->effectN( 1 ).base_value();  // Use first impact spell
      glaive_tempest_targets = as<unsigned>( p->talent.havoc.glaive_tempest->effectN( 2 ).base_value() );
      if ( p->talent.havoc.first_blood->ok() )
        target_filter_callback = secondary_targets_only();
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      if ( result_is_hit( s->result ) && td( s->target )->debuffs.essence_break->up() )
      {
        cooldown_t* tcd = dh()->cooldown.essence_break_proc_icd->get_cooldown( s->target );
        if ( tcd->up() )
        {
          dh()->active.essence_break_proc->execute_on_target( s->target );
          tcd->start();
        }
      }

      if ( last_attack )
      {
        if ( trail_of_ruin_dot )
        {
          trail_of_ruin_dot->execute_on_target( s->target );
        }

        // First Blood splits the primary target into a separate single-target hit,
        // so add 1 to account for it when checking the target threshold.
        if ( dh()->talent.havoc.glaive_tempest->ok() &&
             s->n_targets + ( dh()->talent.havoc.first_blood->ok() ? 1U : 0U ) >= glaive_tempest_targets &&
             dh()->resource_available( RESOURCE_FURY, dh()->talent.havoc.glaive_tempest->effectN( 1 ).base_value() ) &&
             s->chain_target == 0 )
        {
          dh()->active.glaive_tempest->execute_on_target( target );
        }
      }
    }
  };

  std::vector<blade_dance_damage_t*> attacks;
  std::vector<blade_dance_damage_first_blood_t*> first_blood_attacks;
  trail_of_ruin_dot_t* trail_of_ruin_dot;
  timespan_t ability_cooldown;

  blade_dance_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, util::string_view options_str )
    : base_t( n, p, s, options_str ), trail_of_ruin_dot( nullptr )
  {
    may_miss = false;
    cooldown = p->cooldown.blade_dance;  // Blade Dance/Death Sweep Category Cooldown
    range    = 5.0;                      // Disallow use outside of melee range.

    // forces hasted cooldown if no Blade Dance is in the APL for whatever reason
    if ( data().affected_by( p->spec.havoc_demon_hunter->effectN( 4 ) ) )
    {
      cooldown->hasted = true;
    }

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
      add_child( attack );
    }

    if ( attacks.back() )
    {
      attacks.back()->last_attack = true;

      // Trail of Ruin is added to the final hit in the attack list
      if ( dh()->talent.havoc.trail_of_ruin->ok() && trail_of_ruin_dot )
      {
        attacks.back()->trail_of_ruin_dot = trail_of_ruin_dot;
      }
    }

    if ( dh()->talent.havoc.first_blood->ok() )
    {
      for ( auto& attack : first_blood_attacks )
      {
        add_child( attack );
      }

      if ( first_blood_attacks.back() )
      {
        first_blood_attacks.back()->last_attack = true;

        // Trail of Ruin is added to the final hit in the attack list
        if ( dh()->talent.havoc.trail_of_ruin->ok() && trail_of_ruin_dot )
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

    dh()->buff.chaos_theory->trigger();

    // Metamorphosis benefit and Essence Break stats tracking
    if ( dh()->buff.metamorphosis->up() )
    {
      if ( td( target )->debuffs.essence_break->up() )
        dh()->proc.death_sweep_in_essence_break->occur();
    }
    else
    {
      if ( td( target )->debuffs.essence_break->up() )
        dh()->proc.blade_dance_in_essence_break->occur();
    }

    // Create Strike Events
    if ( dh()->talent.havoc.first_blood->ok() )
    {
      for ( auto& attack : first_blood_attacks )
      {
        make_event<delayed_execute_event_t>( *sim, dh(), attack, target, attack->delay );

        // TODO: (Topple) Clean up Screaming Brutality
        if ( dh()->talent.havoc.screaming_brutality->ok() )
        {
          double chance = dh()->talent.havoc.screaming_brutality->effectN( 2 ).percent();
          if ( rng().roll( chance ) )
          {
            make_event<delayed_execute_event_t>( *sim, dh(), dh()->active.screaming_brutality_slash_proc_throw_glaive,
                                                 target, attack->delay );
          }
        }
      }
    }
    if ( !dh()->talent.havoc.first_blood->ok() || dh()->sim->target_non_sleeping_list.size() > 1 )
    {
      for ( auto& attack : attacks )
      {
        make_event<delayed_execute_event_t>( *sim, dh(), attack, target, attack->delay );

        // TODO: (Topple) Clean up Screaming Brutality
        if ( dh()->talent.havoc.screaming_brutality->ok() && !dh()->talent.havoc.first_blood->ok() )
        {
          double chance = dh()->talent.havoc.screaming_brutality->effectN( 2 ).percent();
          if ( rng().roll( chance ) )
          {
            make_event<delayed_execute_event_t>( *sim, dh(), dh()->active.screaming_brutality_slash_proc_throw_glaive,
                                                 target, attack->delay );
          }
        }
      }
    }

    if ( dh()->talent.aldrachi_reaver.broken_spirit->ok() &&
         rng().roll( dh()->talent.aldrachi_reaver.broken_spirit->effectN( 4 ).percent() ) )
    {
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_broken_spirit, soul_fragment::LESSER );
    }

    // Eternal Hunt buff expires ~500ms after Blade Dance is used
    if ( dh()->buff.eternal_hunt->up() )
    {
      make_event( *dh()->sim, 500_ms, [ this ] {
        dh()->buff.eternal_hunt->expire();
        cooldown->reset( true );
      } );
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
    : blade_dance_base_t( "blade_dance", p, p->spec.blade_dance, options_str )
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
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_first_blood_t>(
          "blade_dance_first_blood", p->spec.first_blood_blade_dance_damage, data().effectN( 2 ) ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_first_blood_t>(
          "blade_dance_first_blood_2", p->spec.first_blood_blade_dance_damage, data().effectN( 3 ) ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_first_blood_t>(
          "blade_dance_first_blood_3", p->spec.first_blood_blade_dance_damage, data().effectN( 4 ) ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_first_blood_t>(
          "blade_dance_first_blood_4", p->spec.first_blood_blade_dance_2_damage, data().effectN( 5 ) ) );
    }

    if ( p->talent.havoc.screaming_brutality->ok() && p->active.screaming_brutality_blade_dance_throw_glaive )
      add_child( p->active.screaming_brutality_blade_dance_throw_glaive );
  }

  bool ready() override
  {
    if ( !blade_dance_base_t::ready() )
      return false;

    return !dh()->buff.metamorphosis->check();
  }

  void execute() override
  {
    blade_dance_base_t::execute();

    if ( dh()->talent.havoc.screaming_brutality->ok() && dh()->cooldown.throw_glaive->up() )
    {
      dh()->active.screaming_brutality_blade_dance_throw_glaive->execute_on_target( target );
    }
  }
};

// Death Sweep ==============================================================

struct death_sweep_t : public demonsurge_trigger_t<demonsurge_ability::DEATH_SWEEP, blade_dance_base_t>
{
  death_sweep_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "death_sweep", p, p->spec.death_sweep, options_str )
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
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_first_blood_t>(
          "death_sweep_first_blood", p->spec.first_blood_death_sweep_damage, data().effectN( 2 ) ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_first_blood_t>(
          "death_sweep_first_blood_2", p->spec.first_blood_death_sweep_damage, data().effectN( 3 ) ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_first_blood_t>(
          "death_sweep_first_blood_3", p->spec.first_blood_death_sweep_damage, data().effectN( 4 ) ) );
      first_blood_attacks.push_back( p->get_background_action<blade_dance_damage_first_blood_t>(
          "death_sweep_first_blood_4", p->spec.first_blood_death_sweep_2_damage, data().effectN( 5 ) ) );
    }

    if ( p->talent.havoc.screaming_brutality->ok() && p->active.screaming_brutality_death_sweep_throw_glaive )
      add_child( p->active.screaming_brutality_death_sweep_throw_glaive );
  }

  void execute() override
  {
    assert( dh()->buff.metamorphosis->check() );

    timespan_t ds_extension = timespan_t::from_millis( data().effectN( 5 ).misc_value1() );

    // If Metamorphosis has less than 950ms remaining, it gets extended so the whole Death Sweep happens during Meta.
    if ( dh()->buff.metamorphosis->remains_lt( ds_extension ) )
    {
      dh()->buff.metamorphosis->extend_duration( ds_extension - dh()->buff.metamorphosis->remains() );
    }

    base_t::execute();

    if ( dh()->talent.havoc.screaming_brutality->ok() && dh()->cooldown.throw_glaive->up() )
    {
      dh()->active.screaming_brutality_death_sweep_throw_glaive->execute_on_target( target );
    }
  }

  bool ready() override
  {
    if ( !base_t::ready() )
      return false;

    // Death Sweep can be queued in the last 250ms, so need to ensure meta is still up after that.
    return ( dh()->buff.metamorphosis->remains() > cooldown->queue_delay() );
  }
};

// Relentless Onslaught =====================================================

struct relentless_onslaught_t : public demon_hunter_spell_t
{
  attack_t* chaos_strike;
  attack_t* annihilation;

  relentless_onslaught_t( util::string_view n, demon_hunter_t* p, attack_t* cs, attack_t* anni )
    : demon_hunter_spell_t( n, p ), chaos_strike( cs ), annihilation( anni )
  {
    background = dual = true;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( dh()->buff.metamorphosis->up() )
    {
      annihilation->execute_on_target( target );
    }
    else
    {
      chaos_strike->execute_on_target( target );
    }

    dh()->cooldown.relentless_onslaught_icd->start( dh()->talent.havoc.relentless_onslaught->internal_cooldown() );
  }
};

// Chaos Strike =============================================================

struct chaos_strike_base_t
  : public art_of_the_glaive_trigger_t<art_of_the_glaive_ability::RENDING_STRIKE, demon_hunter_attack_t>
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

      if ( dh()->buff.chaos_theory->check() )
      {
        chance += dh()->buff.chaos_theory->data().effectN( 2 ).percent();
      }

      if ( dh()->talent.havoc.critical_chaos->ok() )
      {
        // DFALPHA TOCHECK -- Double check this uses the correct crit calculations
        chance += dh()->talent.havoc.critical_chaos->effectN( 2 ).percent() * dh()->cache.attack_crit_chance();
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
        if ( dh()->cooldown.chaos_strike_refund_icd->up() && dh()->rng().roll( this->get_refund_proc_chance() ) )
        {
          dh()->resource_gain( RESOURCE_FURY, dh()->spec.chaos_strike_fury->effectN( 1 ).resource( RESOURCE_FURY ),
                               parent->gain );
          dh()->cooldown.chaos_strike_refund_icd->start( dh()->spec.chaos_strike_refund->internal_cooldown() );
        }

        if ( dh()->talent.havoc.chaos_theory->ok() )
        {
          dh()->buff.chaos_theory->expire();
        }
      }
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      // Relentless Onslaught cannot self-proc and is delayed by ~300ms after the normal OH impact
      if ( dh()->talent.havoc.relentless_onslaught->ok() && result_is_hit( s->result ) && may_refund &&
           !parent->from_onslaught )
      {
        double chance = dh()->talent.havoc.relentless_onslaught->effectN( 1 ).percent();
        if ( dh()->cooldown.relentless_onslaught_icd->up() && dh()->rng().roll( chance ) )
        {
          make_event<delayed_execute_event_t>( *sim, dh(), dh()->active.relentless_onslaught, target, this->delay );
        }
      }

      // TOCHECK -- Does this proc from Relentless Onslaught?
      // TOCHECK -- Does the applying Chaos Strike/Annihilation benefit from the debuff?
      if ( dh()->talent.havoc.serrated_glaive->ok() )
      {
        td( s->target )->debuffs.serrated_glaive->trigger();
      }

      if ( dh()->talent.aldrachi_reaver.warblades_hunger && dh()->buff.warblades_hunger->up() )
      {
        dh()->active.warblades_hunger->execute_on_target( target );
        dh()->buff.warblades_hunger->expire();
      }

      if ( result_is_hit( s->result ) && td( s->target )->debuffs.essence_break->up() )
      {
        cooldown_t* tcd = dh()->cooldown.essence_break_proc_icd->get_cooldown( s->target );
        if ( tcd->up() )
        {
          dh()->active.essence_break_proc->execute_on_target( s->target );
          tcd->start();
        }
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
      add_child( attack );
    }
  }

  void execute() override
  {
    base_t::execute();

    // Create Strike Events
    for ( auto& attack : attacks )
    {
      make_event<delayed_execute_event_t>( *sim, dh(), attack, target, attack->delay );
    }

    // Metamorphosis benefit and Essence Break + Serrated Glaive stats tracking
    if ( dh()->buff.metamorphosis->up() )
    {
      if ( td( target )->debuffs.essence_break->up() )
        dh()->proc.annihilation_in_essence_break->occur();

      if ( td( target )->debuffs.serrated_glaive->up() )
        dh()->proc.annihilation_in_serrated_glaive->occur();
    }
    else
    {
      if ( td( target )->debuffs.essence_break->up() )
        dh()->proc.chaos_strike_in_essence_break->occur();

      if ( td( target )->debuffs.serrated_glaive->up() )
        dh()->proc.chaos_strike_in_serrated_glaive->occur();
    }

    // Demonic Appetite
    if ( dh()->rppm.demonic_appetite->trigger() )
    {
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_demonic_appetite, soul_fragment::LESSER );
    }

    if ( dh()->talent.aldrachi_reaver.broken_spirit->ok() &&
         rng().roll( dh()->talent.aldrachi_reaver.broken_spirit->effectN( 4 ).percent() ) )
    {
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_broken_spirit, soul_fragment::LESSER );
    }

    if ( dh()->buff.inner_demon->check() )
    {
      make_event<delayed_execute_event_t>( *sim, dh(), dh()->active.inner_demon, target, 1.25_s );
      dh()->buff.inner_demon->expire();
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

    if ( !from_onslaught && dh()->active.relentless_onslaught )
    {
      add_child( dh()->active.relentless_onslaught->chaos_strike );
    }
  }

  bool ready() override
  {
    if ( dh()->buff.metamorphosis->check() )
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

    if ( !from_onslaught && dh()->active.relentless_onslaught )
    {
      add_child( dh()->active.relentless_onslaught->annihilation );
    }
  }

  bool ready() override
  {
    if ( !dh()->buff.metamorphosis->check() )
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
        dh()->sim->print_debug( "{} removes burning_wound on {} with duration {}", *dh(), *lowest_duration,
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

// Demon Blades =============================================================

struct demon_blades_t : public demon_hunter_attack_t
{
  demon_blades_t( demon_hunter_t* p ) : demon_hunter_attack_t( "demon_blades", p, p->spec.demon_blades_damage )
  {
    background     = true;
    energize_delta = energize_amount * data().effectN( 2 ).m_delta();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = base_t::composite_energize_amount( s );

    if ( dh()->talent.scarred.demonsurge->ok() && dh()->buff.metamorphosis->check() )
    {
      ea += as<int>( dh()->spec.metamorphosis_buff->effectN( 10 ).base_value() );
    }

    return ea;
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( dh()->spec.burning_wound_debuff->ok() )
    {
      dh()->active.burning_wound->execute_on_target( s->target );
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

    add_child( p->active.essence_break_proc );
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // Debuff application appears to be delayed by 250-300ms according to logs
      buff_t* debuff = td( s->target )->debuffs.essence_break;
      make_event( *dh()->sim, 250_ms, [ debuff ] { debuff->trigger(); } );
    }
  }
};

// Felblade =================================================================
// TODO: Real movement stuff.

struct felblade_t : public inertia_trigger_t<demon_hunter_attack_t>
{
  struct felblade_damage_t : public demon_hunter_attack_t
  {
    felblade_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->spell.felblade_damage )
    {
      background = dual               = true;
      gain                            = p->get_gain( "felblade" );
      affected_by.chaotic_disposition = p->talent.havoc.chaotic_disposition->ok();
    }

    double action_multiplier() const override
    {
      double am = demon_hunter_attack_t::action_multiplier();

      am *= 1.0 + dh()->buff.unbound_chaos->value();

      return am;
    }
  };

  unsigned max_fragments_consumed;

  felblade_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "felblade", p, p->talent.demon_hunter.felblade, options_str ), max_fragments_consumed( 0 )
  {
    may_block               = false;
    movement_directionality = movement_direction_type::TOWARDS;

    execute_action = p->get_background_action<felblade_damage_t>( "felblade_damage" );
    add_child( execute_action );

    if ( p->specialization() == DEMON_HUNTER_HAVOC && p->talent.aldrachi_reaver.warblades_hunger->ok() )
    {
      max_fragments_consumed = as<unsigned>( p->talent.aldrachi_reaver.warblades_hunger->effectN( 2 ).base_value() );
    }

    // Add damage modifiers in felblade_damage_t, not here.
  }

  void execute() override
  {
    base_t::execute();
    dh()->set_out_of_range( timespan_t::zero() );  // Cancel all other movement
    if ( max_fragments_consumed > 0 )
    {
      event_t::cancel( dh()->soul_fragment_pick_up );
      dh()->consume_soul_fragments( soul_fragment::ANY, false, max_fragments_consumed );
    }
    dh()->buff.unbound_chaos->expire();
  }

  bool ready() override
  {
    // Felblade has a 1s cooldown triggered by Vengeful Retreat
    if ( dh()->cooldown.felblade_vengeful_retreat_movement_shared->down() )
      return false;

    return base_t::ready();
  }
};

// Fel Rush =================================================================

struct fel_rush_t : public inertia_trigger_t<demon_hunter_attack_t>
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
      double am = base_t::action_multiplier();

      am *= 1.0 + dh()->buff.unbound_chaos->value();

      return am;
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();
    }
  };

  timespan_t gcd_lag;

  fel_rush_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "fel_rush", p, p->spec.fel_rush, options_str )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    min_gcd                                      = trigger_gcd;

    execute_action = p->get_background_action<fel_rush_damage_t>( "fel_rush_damage" );
    add_child( execute_action );

    // Fel Rush does damage in a further line than it moves you
    base_teleport_distance                = execute_action->radius - 5;
    movement_directionality               = movement_direction_type::OMNI;
    p->buff.fel_rush_move->distance_moved = base_teleport_distance;

    // Add damage modifiers in fel_rush_damage_t, not here.
  }

  void execute() override
  {
    base_t::execute();

    dh()->buff.unbound_chaos->expire();

    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    dh()->cooldown.fel_rush_vengeful_retreat_movement_shared->start( timespan_t::from_seconds( 1.0 ) );

    dh()->consume_nearby_soul_fragments( soul_fragment::LESSER );

    dh()->buff.fel_rush_move->trigger();
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

    base_t::schedule_execute( s );
  }

  timespan_t gcd() const override
  {
    return base_t::gcd() + gcd_lag;
  }

  bool ready() override
  {
    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    if ( dh()->cooldown.fel_rush_vengeful_retreat_movement_shared->down() )
      return false;

    // Not usable during the root effect of Stormeater's Boon
    if ( dh()->buffs.stormeaters_boon && dh()->buffs.stormeaters_boon->check() )
      return false;

    return base_t::ready();
  }
};

// Fracture =================================================================

struct fracture_t : public voidfall_building_trigger_t<
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

      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_fracture, soul_fragment::LESSER,
                                 soul_fragments_to_spawn );
    }
  };

  fracture_damage_t *mh, *oh;
  spells::explosion_of_the_soul_t* explosion_of_the_soul;

  fracture_t( demon_hunter_t* p, util::string_view o )
    : base_t( "fracture", p, p->spec.fracture, o ), explosion_of_the_soul( nullptr )
  {
    int number_of_soul_fragments_to_spawn = as<int>( data().effectN( 1 ).base_value() );
    // divide the number in 2 as half come from main hand, half come from offhand.
    int number_of_soul_fragments_to_spawn_per_hit = number_of_soul_fragments_to_spawn / 2;
    // handle leftover souls in the event that blizz ever changes Fracture to an odd number of souls generated
    int number_of_soul_fragments_to_spawn_leftover = number_of_soul_fragments_to_spawn % 2;

    int mh_soul_fragments_to_spawn =
        number_of_soul_fragments_to_spawn_per_hit + number_of_soul_fragments_to_spawn_leftover;
    int oh_soul_fragments_to_spawn = number_of_soul_fragments_to_spawn_per_hit;

    mh = p->get_background_action<fracture_damage_t>( "fracture_mh", data().effectN( 2 ).trigger(),
                                                      mh_soul_fragments_to_spawn );
    add_child( mh );
    oh = p->get_background_action<fracture_damage_t>( "fracture_oh", data().effectN( 3 ).trigger(),
                                                      oh_soul_fragments_to_spawn );
    add_child( oh );

    if ( p->set_bonuses.mid1_vengeance_4pc->ok() )
    {
      explosion_of_the_soul = p->get_background_action<spells::explosion_of_the_soul_t>( "explosion_of_the_soul" );
      add_child( explosion_of_the_soul );
    }

    if ( p->talent.aldrachi_reaver.warblades_hunger->ok() )
    {
      add_child( p->active.warblades_hunger );
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = base_t::composite_energize_amount( s );

    if ( dh()->buff.metamorphosis->check() )
    {
      ea += dh()->spec.metamorphosis_buff->effectN( 10 ).resource( RESOURCE_FURY );
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
      make_event<delayed_execute_event_t>( *dh()->sim, dh(), oh, s->target,
                                           timespan_t::from_millis( data().effectN( 3 ).misc_value1() ) );

      if ( dh()->buff.metamorphosis->check() )
      {
        // In all reviewed logs, it's always 500ms (based on Fires of Fel application)
        make_event( sim, 500_ms, [ this ]() {
          dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_fracture_meta, soul_fragment::LESSER );
        } );
      }

      if ( dh()->talent.aldrachi_reaver.warblades_hunger && dh()->buff.warblades_hunger->up() )
      {
        dh()->active.warblades_hunger->execute_on_target( target );
        dh()->buff.warblades_hunger->expire();
      }

      if ( dh()->set_bonuses.mid1_vengeance_4pc->ok() &&
           rng().roll( dh()->set_bonuses.mid1_vengeance_4pc->effectN( 1 ).percent() ) &&
           dh()->cooldown.explosion_of_the_soul_icd->up() )
      {
        explosion_of_the_soul->execute_on_target( target );
        dh()->cooldown.explosion_of_the_soul_icd->start( dh()->set_bonuses.mid1_vengeance_4pc->internal_cooldown() );
      }
    }
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

// Soul Cleave ==============================================================

struct soul_cleave_t
  : public art_of_the_glaive_trigger_t<art_of_the_glaive_ability::GLAIVE_FLURRY, demon_hunter_attack_t>
{
  struct soul_cleave_damage_t
    : public voidfall_spending_trigger_t<meteoric_fall_trigger_t<burning_blades_trigger_t<demon_hunter_attack_t>>>
  {
    soul_cleave_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s ) : base_t( name, p, s )
    {
      background = dual = true;
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      // Soul Cleave can apply Frailty if Frailty is talented
      if ( result_is_hit( s->result ) && dh()->talent.vengeance.frailty->ok() )
      {
        td( s->target )->debuffs.frailty->trigger();
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = base_t::composite_da_multiplier( s );

      if ( s->chain_target == 0 && dh()->talent.vengeance.focused_cleave->ok() )
      {
        m *= 1.0 + dh()->talent.vengeance.focused_cleave->effectN( 1 ).percent();
      }

      return m;
    }
  };

  const spell_data_t* damage_spell;
  soul_cleave_damage_t* damage;
  heals::soul_cleave_heal_t* heal;
  unsigned max_fragments_consumed;

  soul_cleave_t( demon_hunter_t* p, util::string_view options_str = {} )
    : base_t( "soul_cleave", p, p->spec.soul_cleave, options_str ),
      damage_spell( data().effectN( 2 ).trigger() ),
      max_fragments_consumed( static_cast<unsigned>( data().effectN( 2 ).base_value() ) +
                              static_cast<unsigned>( data().effectN( 3 ).base_value() ) )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    attack_power_mod.direct = 0;  // This parent action deals no damage, parsed data is for the heal

    damage = p->get_background_action<soul_cleave_damage_t>( "soul_cleave_damage", damage_spell );
    add_child( damage );

    heal = p->get_background_action<heals::soul_cleave_heal_t>( "soul_cleave_heal" );

    if ( p->active.art_of_the_glaive )
    {
      add_child( p->active.art_of_the_glaive );
    }
    // Add damage modifiers in soul_cleave_damage_t, not here.
  }

  void execute() override
  {
    base_t::execute();

    heal->execute_on_target( player );

    // Soul fragments consumed are capped for Soul Cleave
    const int fragments_consumed = dh()->consume_soul_fragments( soul_fragment::ANY, true, max_fragments_consumed );
    damage->set_target( target );
    action_state_t* damage_state = damage->get_state();
    damage_state->target         = target;
    damage->snapshot_state( damage_state, result_amount_type::DMG_DIRECT );
    // Read per-fragment % from parent spell effectN(1), which Untethered Rage (1270448) effect#1 modifies.
    // The damage sub-spell 228478 effectN(3) has a stale base value (20) that UR2 does not reach.
    damage_state->da_multiplier *= 1.0 + ( data().effectN( 1 ).percent() * fragments_consumed );
    damage->schedule_execute( damage_state );
    damage->execute_event->reschedule( timespan_t::from_millis( data().effectN( 2 ).misc_value1() ) );

    trigger_untethered_rage( fragments_consumed );

    if ( dh()->talent.aldrachi_reaver.broken_spirit->ok() &&
         rng().roll( dh()->talent.aldrachi_reaver.broken_spirit->effectN( 3 ).percent() ) )
    {
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_broken_spirit, soul_fragment::LESSER );
    }
  }
};

// Throw Glaive =============================================================

struct throw_glaive_t : public demon_hunter_attack_t
{
  enum class glaive_source
  {
    THROWN                                = 0,
    SCREAMING_BRUTALITY_SLASH_PROC_THROW  = 1,
    SCREAMING_BRUTALITY_BLADE_DANCE_THROW = 2,
    SCREAMING_BRUTALITY_DEATH_SWEEP_THROW = 3
  };

  struct throw_glaive_damage_t : public soulscar_trigger_t<burning_blades_trigger_t<demon_hunter_attack_t>>
  {
    glaive_source source;

    throw_glaive_damage_t( util::string_view name, demon_hunter_t* p, glaive_source source = glaive_source::THROWN,
                           const spell_data_t* spell = nullptr )
      : base_t( name, p, spell ? spell : p->spell.throw_glaive->effectN( 1 ).trigger() ), source( source )
    {
      background = dual = true;
      radius            = 10.0;

      switch ( source )
      {
        case glaive_source::SCREAMING_BRUTALITY_SLASH_PROC_THROW:
          // slash procs have one multiplier
          base_multiplier *= p->talent.havoc.screaming_brutality->effectN( 1 ).percent();
          break;
        case glaive_source::SCREAMING_BRUTALITY_BLADE_DANCE_THROW:
        case glaive_source::SCREAMING_BRUTALITY_DEATH_SWEEP_THROW:
          // regular procs have a different multiplier
          base_multiplier *= p->talent.havoc.screaming_brutality->effectN( 3 ).percent();
          break;
        default:
          // this handles THROWN (multiplier of 1 by default) and any new sources
          break;
      }
    }

    void impact( action_state_t* state ) override
    {
      base_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        if ( dh()->spec.burning_wound_debuff->ok() )
        {
          dh()->active.burning_wound->execute_on_target( state->target );
        }

        if ( dh()->talent.havoc.serrated_glaive->ok() )
        {
          td( state->target )->debuffs.serrated_glaive->trigger();
        }
      }
    }
  };

  throw_glaive_damage_t* furious_throws;

  throw_glaive_t( util::string_view name, demon_hunter_t* p, util::string_view options_str,
                  glaive_source source = glaive_source::THROWN )
    : demon_hunter_attack_t( name, p, p->spell.throw_glaive, options_str ), furious_throws( nullptr )
  {
    throw_glaive_damage_t* damage;

    switch ( source )
    {
      case glaive_source::SCREAMING_BRUTALITY_SLASH_PROC_THROW:
        damage = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_damage_sb_slash_proc_throw", source );
        break;
      case glaive_source::SCREAMING_BRUTALITY_BLADE_DANCE_THROW:
        damage = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_damage_sb_bd_throw", source );
        break;
      case glaive_source::SCREAMING_BRUTALITY_DEATH_SWEEP_THROW:
        damage = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_damage_sb_ds_throw", source );
        break;
      default:
        damage = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_damage", source );
        break;
    }

    execute_action = damage;
    add_child( execute_action );

    if ( source == glaive_source::SCREAMING_BRUTALITY_BLADE_DANCE_THROW ||
         source == glaive_source::SCREAMING_BRUTALITY_DEATH_SWEEP_THROW )
    {
      cooldown->duration = 0_s;
      cooldown->charges  = 0;

      cooldown = p->cooldown.throw_glaive;
    }
    if ( source == glaive_source::SCREAMING_BRUTALITY_SLASH_PROC_THROW )
    {
      cooldown->duration = 0_s;
      cooldown->charges  = 0;
    }

    if ( p->talent.havoc.furious_throws->ok() )
    {
      if ( source == glaive_source::THROWN )
      {
        resource_current = RESOURCE_FURY;
      }
      else
      {
        base_costs[ RESOURCE_FURY ] = 0;
      }

      switch ( source )
      {
        case glaive_source::SCREAMING_BRUTALITY_SLASH_PROC_THROW:
          furious_throws = p->get_background_action<throw_glaive_damage_t>(
              "throw_glaive_furious_throws_sb_slash_proc_throw", source, p->spec.furious_throws_damage );
          break;
        case glaive_source::SCREAMING_BRUTALITY_BLADE_DANCE_THROW:
          furious_throws = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_furious_throws_sb_bd_throw",
                                                                            source, p->spec.furious_throws_damage );
          break;
        case glaive_source::SCREAMING_BRUTALITY_DEATH_SWEEP_THROW:
          furious_throws = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_furious_throws_sb_ds_throw",
                                                                            source, p->spec.furious_throws_damage );
          break;
        default:
          furious_throws = p->get_background_action<throw_glaive_damage_t>( "throw_glaive_furious_throws", source,
                                                                            p->spec.furious_throws_damage );
          break;
      }

      add_child( furious_throws );
    }
  }

  void init() override
  {
    track_cd_waste = false;
    demon_hunter_attack_t::init();

    track_cd_waste = true;
    cd_wasted_exec =
        dh()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>( "throw_glaive", dh()->cd_waste_exec );
    cd_wasted_cumulative = dh()->template get_data_entry<simple_sample_data_with_min_max_t, data_t>(
        "throw_glaive", dh()->cd_waste_cumulative );
    cd_wasted_iter =
        dh()->template get_data_entry<simple_sample_data_t, simple_data_t>( "throw_glaive", dh()->cd_waste_iter );
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( hit_any_target && furious_throws )
    {
      make_event<delayed_execute_event_t>( *sim, dh(), furious_throws, target, 400_ms );

      if ( dh()->active.preemptive_strike )
      {
        make_event<delayed_execute_event_t>( *sim, dh(), dh()->active.preemptive_strike, target, 400_ms );
      }
    }

    if ( td( target )->debuffs.serrated_glaive->up() )
    {
      dh()->proc.throw_glaive_in_serrated_glaive->occur();
    }

    if ( dh()->active.preemptive_strike )
    {
      dh()->active.preemptive_strike->execute_on_target( target );
    }
  }

  bool ready() override
  {
    if ( dh()->buff.reavers_glaive->up() )
    {
      return false;
    }

    return demon_hunter_attack_t::ready();
  }
};

// Reaver's Glaive ==========================================================
struct reavers_glaive_t : public soulscar_trigger_t<demon_hunter_attack_t>
{
  reavers_glaive_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "reavers_glaive", p, p->hero_spec.reavers_glaive, options_str )
  {
  }

  void execute() override
  {
    dh()->buff.reavers_glaive->expire();

    base_t::execute();

    if ( dh()->active.preemptive_strike )
    {
      dh()->active.preemptive_strike->execute_on_target( target );
    }

    dh()->buff.glaive_flurry->trigger();
    dh()->buff.rending_strike->trigger();
    dh()->buff.art_of_the_glaive_first->trigger();
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    dh()->buff.thrill_of_the_fight_damage->expire();
  }

  bool ready() override
  {
    if ( !dh()->buff.reavers_glaive->up() )
    {
      return false;
    }
    // 2024-07-11 -- Reaver's Glaive can't be cast unless a GCD is available, but doesn't trigger a GCD.
    if ( dh()->gcd_ready > sim->current_time() )
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

  double base_ta( const action_state_t* s ) const override
  {
    double amount = base_t::base_ta( s );

    if ( s->target->debuffs.chaos_brand->up() )
    {
      amount *= 1.0 + dh()->spell.chaos_brand->effectN( 1 ).percent();
    }

    // Currently double dips off Demon Hide
    if ( dh()->bugs && dh()->talent.havoc.demon_hide->ok() )
    {
      amount *= 1.0 + dh()->talent.havoc.demon_hide->effectN( 1 ).percent();
    }
    return amount;
  }
};

// Burning Blades ===========================================================
struct burning_blades_t : public residual_action::residual_periodic_action_t<demon_hunter_spell_t>
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

  double base_ta( const action_state_t* s ) const override
  {
    double amount = base_t::base_ta( s );

    // Burning Blades is supposed to benefit off Chaos Brand through a server side script,
    // but this only happens off pure auto attack and throw glaive refreshes. Refreshing
    // or applying burning blades with chaos strike does not all burning blades and any
    // future refreshes to benefit from chaos brand, although this is somewhat inconsisent.

    if ( !dh()->bugs && s->target->debuffs.chaos_brand->up() )
    {
      amount *= 1.0 + dh()->spell.chaos_brand->effectN( 1 ).percent();
    }

    return amount;
  }
};

// Vengeful Retreat =========================================================

struct vengeful_retreat_t
  : public unbound_chaos_trigger_t<inertia_trigger_trigger_t<exergy_trigger_t<demon_hunter_spell_t>>>
{
  struct voidstep_damage_t : public shattered_souls_trigger_t<demon_hunter_spell_t>
  {
    voidstep_damage_t( util::string_view n, demon_hunter_t* p )
      : base_t( n, p, p->spec.voidstep->effectN( 1 ).trigger() )
    {
      aoe = -1;
    }

    void execute() override
    {
      base_t::execute();

      dh()->buff.voidstep->expire();
    }
  };

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
      if ( dh()->talent.havoc.initiative->ok() )
      {
        for ( auto p : sim->target_non_sleeping_list )
        {
          td( p )->debuffs.initiative_tracker->expire();
        }
      }

      demon_hunter_spell_t::execute();

      dh()->buff.tactical_retreat->trigger();
    }
  };

  voidstep_damage_t* voidstep;

  vengeful_retreat_t( demon_hunter_t* p, util::string_view options_str )
    : base_t( "vengeful_retreat", p, p->talent.demon_hunter.vengeful_retreat, options_str ), voidstep( nullptr )
  {
    execute_action = p->get_background_action<vengeful_retreat_damage_t>( "vengeful_retreat_damage" );
    add_child( execute_action );

    // TODO: Remove or modify when category cooldowns are implemented/fixed
    cooldown->duration = data().category_cooldown();
    if ( data().affected_by( p->talent.havoc.tactical_retreat->effectN( 1 ) ) )
    {
      cooldown->duration += p->talent.havoc.tactical_retreat->effectN( 1 ).time_value();
    }

    base_teleport_distance                        = VENGEFUL_RETREAT_DISTANCE;
    movement_directionality                       = movement_direction_type::OMNI;
    p->buff.vengeful_retreat_move->distance_moved = base_teleport_distance;

    if ( p->talent.devourer.hungering_slash->ok() )
    {
      voidstep = p->get_background_action<voidstep_damage_t>( "voidstep" );
      add_child( voidstep );
    }
    // Add damage modifiers in vengeful_retreat_damage_t, not here.
  }

  void execute() override
  {
    // base_t::execute() will expire the voidstep buff so we do the damage before
    if ( dh()->buff.voidstep->up() )
    {
      voidstep->execute_on_target( target );
    }

    base_t::execute();

    // Fel Rush and VR share a 1 second GCD when one or the other is triggered
    dh()->cooldown.fel_rush_vengeful_retreat_movement_shared->start( 1_s );
    // Vengeful Retreat triggers a lockout for Felblade
    dh()->cooldown.felblade_vengeful_retreat_movement_shared->start(
        timespan_t::from_seconds( dh()->options.felblade_lockout_from_vengeful_retreat ) );
    dh()->buff.vengeful_retreat_move->trigger();

    if ( dh()->specialization() == DEMON_HUNTER_HAVOC )
    {
      dh()->consume_nearby_soul_fragments( soul_fragment::LESSER );
    }

    if ( dh()->talent.aldrachi_reaver.unhindered_assault->ok() )
    {
      dh()->proc.unhindered_assault->occur();
      dh()->cooldown.felblade->reset( true );
    }
  }

  bool ready() override
  {
    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    if ( dh()->cooldown.fel_rush_vengeful_retreat_movement_shared->down() )
      return false;

    // Not usable during the root effect of Stormeater's Boon
    if ( dh()->buffs.stormeaters_boon && dh()->buffs.stormeaters_boon->check() )
      return false;

    return base_t::ready();
  }

  void update_ready( timespan_t cd_duration ) override
  {
    // Decrementing a stack of Voidstep will consume a max charge. Consuming a max charge loses you a current
    // charge. Therefore update_ready needs to not be called in that case.
    if ( dh()->buff.voidstep->up() )
    {
      dh()->buff.voidstep->decrement();
    }
    else
    {
      base_t::update_ready( cd_duration );
    }
  }

  void reset() override
  {
    // Reset max charges to initial value, since it can get out of sync when previous iteration ends with charge-giving
    // buffs up. Do this before calling reset as that will also reset the cooldown.
    cooldown->charges = std::max( data().charges(), 1U );

    base_t::reset();
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
    add_child( impact_action );
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_soul_carver, soul_fragment::LESSER,
                               as<unsigned int>( data().effectN( 3 ).base_value() ) );
  }

  void tick( dot_t* d ) override
  {
    demon_hunter_attack_t::tick( d );

    dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_soul_carver, soul_fragment::LESSER,
                               as<unsigned int>( data().effectN( 4 ).base_value() ) );
  }
};

// Art of the Glaive ===================================================
struct art_of_the_glaive_t : public demon_hunter_attack_t
{
  struct fury_of_the_aldrachi_damage_t : public demon_hunter_attack_t
  {
    timespan_t delay;

    fury_of_the_aldrachi_damage_t( util::string_view name, demon_hunter_t* p, const spelleffect_data_t& eff )
      : demon_hunter_attack_t( name, p, eff.trigger() ), delay( timespan_t::from_millis( eff.misc_value1() ) )
    {
      background = dual   = true;
      aoe                 = -1;
      reduced_aoe_targets = as<int>( data().effectN( 2 ).base_value() );
    }
  };

  std::vector<fury_of_the_aldrachi_damage_t*> attacks;

  art_of_the_glaive_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_attack_t( name, p, p->hero_spec.art_of_the_glaive_damage )
  {
    background = dual = true;
    for ( const spelleffect_data_t& effect : data().effects() )
    {
      if ( effect.type() != E_TRIGGER_SPELL )
        continue;

      attacks.push_back( p->get_background_action<fury_of_the_aldrachi_damage_t>( "fury_of_the_aldrachi", effect ) );
    }
  }

  void init_finished() override
  {
    demon_hunter_attack_t::init_finished();

    // Use one stats object for all parts of the attack.
    for ( auto& attack : attacks )
    {
      add_child( attack );
    }
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    // if glaive flurry is up and rending strike is not up
    // fury of the aldrachi causes art of the glaive to retrigger itself for 3 additional procs 300ms after initial
    // execution
    if ( dh()->talent.aldrachi_reaver.fury_of_the_aldrachi->ok() && dh()->buff.glaive_flurry->up() &&
         !dh()->buff.rending_strike->up() )
    {
      make_event<delayed_execute_event_t>( *sim, dh(), dh()->active.art_of_the_glaive, target, 300_ms );

      // Bladecraft makes it trigger 6 more times
      if ( dh()->talent.aldrachi_reaver.bladecraft->ok() )
      {
        make_event<delayed_execute_event_t>( *sim, dh(), dh()->active.art_of_the_glaive, target, 300_ms );
        make_event<delayed_execute_event_t>( *sim, dh(), dh()->active.art_of_the_glaive, target, 300_ms );
      }
    }

    for ( auto attack : attacks )
    {
      make_event<delayed_execute_event_t>( *sim, dh(), attack, target, attack->delay );
    }
  }
};

struct preemptive_strike_t : public demon_hunter_ranged_attack_t
{
  preemptive_strike_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_ranged_attack_t( name, p, p->talent.aldrachi_reaver.preemptive_strike->effectN( 1 ).trigger() )
  {
    background = dual = true;
    aoe               = -1;
  }

  // 2025-02-19 -- Preemptive Strike does not hit the primary target
  std::vector<player_t*>& target_list() const override
  {
    std::vector<player_t*>& target_list = action_t::target_list();
    target_list.erase( std::remove( target_list.begin(), target_list.end(), target ), target_list.end() );

    return target_list;
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
  double chance;

  wounded_quarry_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_attack_t( name, p, p->hero_spec.wounded_quarry_damage )
  {
    chance = p->hero_spec.wounded_quarry_proc_rate;
    if ( p->bugs )
    {
      // 2025-02-23 -- WQ seems to proc things like Chaotic Disposition
      allow_class_ability_procs = true;
    }

    // WQ is affected by Havoc mastery
    if ( p->mastery.demonic_presence->ok() )
    {
      affected_by.demonic_presence.direct   = true;
      affected_by.demonic_presence.periodic = true;
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( rng().roll( chance ) )
    {
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_wounded_quarry, soul_fragment::LESSER );
    }
  }
};

struct essence_break_proc_t : public demon_hunter_attack_t
{
  essence_break_proc_t( util::string_view n, demon_hunter_t* p )
    : demon_hunter_attack_t( n, p, p->spec.essence_break_proc_damage )
  {
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
  demon_hunter_t* dh()
  {
    return static_cast<demon_hunter_t*>( BuffBase::source );
  }

  const demon_hunter_t* dh() const
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
      set_partial_tick( true );
      set_quiet( true );

      if ( p->talent.havoc.growing_inferno->ok() )
        growing_inferno_max_ticks = static_cast<int>( 10 / p->talent.havoc.growing_inferno->effectN( 1 ).percent() );

      set_tick_callback( [ this, p ]( buff_t*, int, timespan_t ) {
        ragefire_crit_accumulator = 0;

        state_t* s = static_cast<state_t*>( p->active.immolation_aura_tick->get_state() );

        p->active.immolation_aura_tick->set_target( p->target );

        s->target                     = p->target;
        s->growing_inferno_multiplier = 1 + growing_inferno_ticks * growing_inferno_multiplier;
        s->immolation_aura            = this;

        p->active.immolation_aura_tick->snapshot_state( s, p->active.immolation_aura_tick->amount_type( s ) );
        p->active.immolation_aura_tick->schedule_execute( s );

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

      if ( dh()->active.immolation_aura_initial && dh()->sim->current_time() > 0_s )
      {
        state_t* s = static_cast<state_t*>( dh()->active.immolation_aura_initial->get_state() );

        s->target = dh()->target;

        dh()->active.immolation_aura_initial->set_target( dh()->target );

        s->growing_inferno_multiplier = 1 + growing_inferno_ticks * growing_inferno_multiplier;
        s->immolation_aura            = this;

        dh()->active.immolation_aura_initial->snapshot_state( s,
                                                              dh()->active.immolation_aura_initial->amount_type( s ) );
        dh()->active.immolation_aura_initial->schedule_execute( s );
      }

      growing_inferno_ticks++;
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      demon_hunter_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

      if ( dh()->talent.havoc.ragefire->ok() )
      {
        make_event( *sim, 200_ms, [ this ] {
          dh()->active.ragefire->execute_on_target( dh()->target, ragefire_accumulator );
          ragefire_accumulator      = 0;
          ragefire_crit_accumulator = 0;
        } );
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
  double undying_embers_proc_chance;
  immolation_aura_buff_t( demon_hunter_t* p )
    : base_t( *p, "immolation_aura", p->spell.immolation_aura ), immos(), undying_embers_proc_chance( 0.0 )
  {
    set_cooldown( timespan_t::zero() );
    set_tick_behavior( buff_tick_behavior::NONE );
    disable_ticking( true );

    if ( p->talent.demon_hunter.infernal_armor->ok() )
    {
      add_invalidate( CACHE_ARMOR );
    }

    if ( p->talent.havoc.a_fire_inside->ok() )
    {
      set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
      set_max_stack( 5 );
    }
    else
    {
      set_max_stack( 1 );
    }

    if ( p->talent.scarred.undying_embers->ok() )
    {
      undying_embers_proc_chance = p->talent.scarred.undying_embers->effectN( 1 ).percent();
    }

    for ( int i = 0; i < max_stack(); i++ )
    {
      auto functional_buff = new immolation_aura_functional_buff_t( p, fmt::format( "immolation_aura{}", i + 1 ) );
      // this talent is a pain
      if ( p->talent.scarred.undying_embers->ok() )
      {
        functional_buff->set_expire_callback( [ this, p ]( buff_t*, int, timespan_t ) {
          if ( rng().roll( undying_embers_proc_chance ) )
          {
            p->proc.undying_embers->occur();
            // retriggers the buff but is not a cast
            make_event( sim, 300_ms, [ this ] { trigger(); } );
          }
        } );
      }
      immos.push_back( functional_buff );
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
  actions::demon_hunter_energize_t* rolling_torment_energize;

  metamorphosis_buff_t( demon_hunter_t* p )
    : base_t( *p, "metamorphosis", p->spec.metamorphosis_buff ), rolling_torment_energize( nullptr )
  {
    set_cooldown( timespan_t::zero() );

    if ( p->specialization() != DEMON_HUNTER_DEVOURER )
    {
      disable_ticking( true );
    }
    else
    {
      freeze_stacks = true;
      set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { dh()->devourer_fury_state.drain_stacks++; } );
    }
    // Spell 187827 has a Periodic Dummy effect (#7, 2s period) for visual/server logic that
    // SimC doesn't model. Without this override, init() -> set_period() detects the periodic
    // effect and sets refresh_behavior=TICK, but tick_behavior=NONE means no tick_event is
    // ever created. This mismatch causes an assertion failure when Meta is refreshed (e.g.
    // hardcasting Meta during an active Untethered Rage-procced Meta).
    set_refresh_behavior( buff_refresh_behavior::DURATION );

    switch ( p->specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        set_duration( timespan_t::zero() );
        set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
        break;
      case DEMON_HUNTER_HAVOC:
        demon_hunter_buff_t::set_default_value_from_effect_type( A_HASTE_ALL );
        add_invalidate( CACHE_HASTE );
        add_invalidate( CACHE_LEECH );
        break;
      case DEMON_HUNTER_VENGEANCE:
        demon_hunter_buff_t::set_default_value_from_effect_type( A_INCREASE_HEALTH_PCT );
        add_invalidate( CACHE_ARMOR );
        break;
      default:
        break;
    }

    if ( p->talent.demon_hunter.soul_rending->ok() )
    {
      add_invalidate( CACHE_LEECH );
    }

    if ( p->talent.devourer.rolling_torment->ok() )
    {
      rolling_torment_energize =
          p->get_background_action<actions::spells::rolling_torment_energize_t>( "rolling_torment_energize" );
    }
  }

  void trigger_demonic()
  {
    if ( dh()->specialization() != DEMON_HUNTER_HAVOC )
    {
      return;
    }

    if ( !dh()->buff.metamorphosis->up() )
    {
      dh()->buff.demonsurge_demonsurge->trigger();
      if ( dh()->talent.scarred.volatile_instinct->ok() )
      {
        dh()->trigger_demonsurge( demonsurge_ability::ENTER_META, false );
      }
    }
    dh()->buff.demonsurge_abilities[ demonsurge_ability::ANNIHILATION ]->trigger();
    dh()->buff.demonsurge_abilities[ demonsurge_ability::DEATH_SWEEP ]->trigger();

    const timespan_t extend_duration = dh()->talent.havoc.demonic->effectN( 1 ).time_value();
    dh()->buff.metamorphosis->extend_duration_or_trigger( extend_duration );
  }

  void extend_duration_or_trigger( timespan_t duration ) override
  {
    demon_hunter_buff_t::extend_duration_or_trigger( duration );

    dh()->buff.inner_demon->trigger();
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    demon_hunter_buff_t::start( stacks, value, duration );

    if ( dh()->specialization() == DEMON_HUNTER_VENGEANCE )
    {
      dh()->metamorphosis_health = dh()->max_health() * value;
      dh()->stat_gain( STAT_MAX_HEALTH, dh()->metamorphosis_health, (gain_t*)nullptr, (action_t*)nullptr, true );
    }

    if ( dh()->talent.devourer.void_metamorphosis->ok() )
    {
      dh()->buff.void_metamorphosis_stack->expire();
    }

    if ( dh()->talent.devourer.midnight3->ok() )
    {
      dh()->buff.collapsing_star->trigger();
      dh()->buff.collapsing_star_stack->trigger(
          as<int>( dh()->talent.devourer.collapsing_star->effectN( 1 ).base_value() ) );
      dh()->spawn_soul_fragment( dh()->proc.soul_fragment_from_void_metamorphosis, soul_fragment::LESSER,
                                 as<unsigned int>( dh()->talent.devourer.midnight3->effectN( 1 ).base_value() ) );
    }

    if ( ( sim->dbc->wowv() >= wowv_t( 12, 0, 1 ) || dh()->specialization() == DEMON_HUNTER_HAVOC ) &&
         dh()->talent.scarred.monster_rising->ok() )
    {
      dh()->buff.monster_rising->expire();
    }
    if ( ( sim->dbc->wowv() >= wowv_t( 12, 0, 1 ) || dh()->specialization() == DEMON_HUNTER_HAVOC ) &&
         dh()->talent.scarred.enduring_torment->ok() )
    {
      dh()->buff.enduring_torment->expire();
    }

    if ( dh()->specialization() == DEMON_HUNTER_DEVOURER && dh()->talent.scarred.volatile_instinct->ok() )
    {
      dh()->buff.volatile_instinct->trigger();
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( dh()->specialization() == DEMON_HUNTER_VENGEANCE )
    {
      dh()->stat_loss( STAT_MAX_HEALTH, dh()->metamorphosis_health, (gain_t*)nullptr, (action_t*)nullptr, true );
      dh()->metamorphosis_health = 0;
    }

    if ( dh()->talent.scarred.monster_rising->ok() )
    {
      dh()->buff.monster_rising->trigger();
    }
    if ( dh()->talent.scarred.enduring_torment->ok() )
    {
      dh()->buff.enduring_torment->trigger();
    }

    if ( dh()->specialization() == DEMON_HUNTER_DEVOURER )
    {
      dh()->resources.current[ RESOURCE_FURY ] = 0;
      dh()->buff.collapsing_star->expire();
      if ( dh()->talent.devourer.rolling_torment->ok() && dh()->buff.collapsing_star_stack->up() )
      {
        dh()->buff.rolling_torment->trigger( dh()->buff.collapsing_star_stack->stack() );
        rolling_torment_energize->execute_on_target( dh() );
      }
      dh()->buff.collapsing_star_stack->expire();
      dh()->buff.emptiness->expire();
      dh()->buff.impending_apocalypse->expire();
      event_t::cancel( dh()->devourer_fury_state.next_drain_event );
      dh()->devourer_fury_state.clear_state();
      dh()->cooldown.void_ray->reset( false );

      action_t* executing = dh()->executing;
      if ( executing &&
           ( executing->id == dh()->spec.devour->id() || executing->id == dh()->hero_spec.predators_wake->id() ) )
      {
        dh()->interrupt();
      }
    }

    for ( demonsurge_ability ability : demonsurge_abilities )
    {
      dh()->buff.demonsurge_abilities[ ability ]->expire();
    }
    for ( voidsurge_ability ability : voidsurge_abilities )
    {
      dh()->buff.voidsurge_abilities[ ability ]->expire();
    }
    dh()->buff.demonsurge->expire();
    dh()->buff.voidsurge->expire();
    dh()->buff.demonsurge_demonsurge->expire();
    dh()->buff.demonsurge_demonic_intensity->expire();
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
};

struct collapsing_star_stacking_t : public demon_hunter_buff_t<buff_t>
{
  int trigger_threshold;
  collapsing_star_stacking_t( demon_hunter_t* p )
    : base_t( *p, "collapsing_star_stacking", p->spec.collapsing_star_stacking_buff ),
      trigger_threshold( as<int>( p->talent.devourer.collapsing_star->effectN( 1 ).base_value() ) )
  {
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

    add_stack_change_callback( [ this ]( buff_t*, int old, int new_ ) {
      if ( new_ >= trigger_threshold && old < trigger_threshold )
      {
        this->dh()->buff.collapsing_star->trigger();
      }
    } );
  }
};

struct voidfall_building_buff_t : public demon_hunter_buff_t<buff_t>
{
  voidfall_building_buff_t( demon_hunter_t* p ) : base_t( *p, "voidfall_building", p->hero_spec.voidfall_building_buff )
  {
    base_t::set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN );
    disable_ticking( true );
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  }

  void bump( int stacks, double value ) override
  {
    base_t::bump( stacks, value );

    if ( at_max_stacks() )
    {
      make_event( *sim, [ this ] {
        expire();
        dh()->buff.voidfall_spending->trigger( max_stack() );
      } );
    }
  }
};

struct voidfall_spending_buff_t : public demon_hunter_buff_t<buff_t>
{
  voidfall_spending_buff_t( demon_hunter_t* p ) : base_t( *p, "voidfall_spending", p->hero_spec.voidfall_spending_buff )
  {
    disable_ticking( true );
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  }

  void decrement( int stacks, double value ) override
  {
    base_t::decrement( stacks, value );

    dh()->buff.voidfall_final_hour->trigger( stacks );
  }

  void expire( timespan_t d ) override
  {
    int stacks = check();

    base_t::expire( d );

    dh()->buff.voidfall_final_hour->trigger( stacks );
  }
};

struct student_of_suffering_buff_t : public demon_hunter_buff_t<buff_t>
{
  actions::demon_hunter_energize_t* energize;

  student_of_suffering_buff_t( demon_hunter_t* p )
    : base_t( *p, "student_of_suffering", p->hero_spec.student_of_suffering_buff )
  {
    energize = p->get_background_action<actions::demon_hunter_energize_t>(
        "student_of_suffering_energize", p->hero_spec.student_of_suffering_buff->effectN( 2 ).trigger() );

    set_default_value_from_effect_type( A_MOD_MASTERY_PCT );
    set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
    set_tick_behavior( buff_tick_behavior::REFRESH );
    set_tick_on_application( false );
    set_period( 2_s );

    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { energize->execute(); } );
  }
};

struct impending_apocalypse_buff_t : public demon_hunter_buff_t<buff_t>
{
  impending_apocalypse_buff_t( demon_hunter_t* p )
    : base_t( *p, "impending_apocalypse", p->spec.impending_apocalypse_buff )
  {
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
    set_tick_behavior( buff_tick_behavior::NONE );
    set_period( 0_ms );
  }

  void increment( int stacks, double value, timespan_t duration ) override
  {
    if ( !dh()->buff.metamorphosis->up() )
    {
      cancel();
      return;
    }

    base_t::increment( stacks, value, duration );
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
  if ( !dh()->talent.havoc.ragefire->ok() )
    return;

  assert( s->immolation_aura && "Immolation Aura state should contain the parent buff while executing damage." );

  if ( !( s->result_amount > 0 && s->result == RESULT_CRIT ) )
    return;

  buffs::immolation_aura_buff_t::immolation_aura_functional_buff_t* immo =
      static_cast<buffs::immolation_aura_buff_t::immolation_aura_functional_buff_t*>( s->immolation_aura );

  if ( immo->ragefire_crit_accumulator >= dh()->talent.havoc.ragefire->effectN( 2 ).base_value() )
    return;

  const double multiplier = dh()->talent.havoc.ragefire->effectN( 1 ).percent();
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
// Demon Hunter Proc Callbacks
// ==========================================================================
struct demon_hunter_proc_callback_t : public dbc_proc_callback_t
{
  demon_hunter_proc_callback_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
  {
    initialize();
    activate();
  }

  demon_hunter_t* dh() const
  {
    return debug_cast<demon_hunter_t*>( listener );
  }
  demon_hunter_t* dh()
  {
    return debug_cast<demon_hunter_t*>( listener );
  }
};

// ==========================================================================
// Targetdata Definitions
// ==========================================================================

demon_hunter_td_t::demon_hunter_td_t( player_t* target, demon_hunter_t& p )
  : actor_target_data_t( target, &p ), dots( dots_t() ), debuffs( debuffs_t() )
{
  switch ( p.specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      debuffs.devourers_bite = make_buff( *this, "devourers_bite", p.spec.devourers_bite_debuff )
                                   ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                                   ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                   ->disable_ticking( true );
      break;
    case DEMON_HUNTER_HAVOC:
      debuffs.essence_break = make_buff( *this, "essence_break", p.spec.essence_break_debuff )
                                  ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                  ->set_cooldown( timespan_t::zero() );

      debuffs.initiative_tracker =
          make_buff( *this, "initiative_tracker", p.talent.havoc.initiative )->set_duration( timespan_t::min() );

      debuffs.burning_wound = make_buff( *this, "burning_wound", p.spec.burning_wound_debuff )
                                  ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS )
                                  ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
      dots.burning_wound = target->get_dot( "burning_wound", &p );
      break;
    case DEMON_HUNTER_VENGEANCE:
      dots.fiery_brand = target->get_dot( "fiery_brand", &p );
      debuffs.frailty  = make_buff( *this, "frailty", p.spec.frailty_debuff )
                            ->set_default_value_from_effect( 1 )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                            ->disable_ticking( true );
      break;
    default:
      break;
  }

  // TODO: make this conditional on hero spec
  debuffs.reavers_mark =
      make_buff( *this, "reavers_mark", p.hero_spec.reavers_mark )
          ->set_default_value_from_effect( 1 )
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
          ->set_stack_change_callback( [ &p ]( buff_t* b, int, int new_ ) {
            if ( !new_ )
            {
              if ( p.active.wounded_quarry )
              {
                p.sim->print_debug( "{} triggers Wounded Quarry because Reaver's Mark was removed from target {}: {}",
                                    p.name(), p.last_reavers_mark_applied->name(), p.wounded_quarry_accumulator );
                // Apply Chaos Brand multiplier to match periodic discharge behavior (line ~2874)
                if ( p.last_reavers_mark_applied->debuffs.chaos_brand &&
                     p.last_reavers_mark_applied->debuffs.chaos_brand->up() )
                {
                  p.wounded_quarry_accumulator *= 1.0 + p.spell.chaos_brand->effectN( 1 ).percent();
                }
                p.active.wounded_quarry->execute_on_target( p.last_reavers_mark_applied, p.wounded_quarry_accumulator );
              }
              p.wounded_quarry_accumulator = 0.0;
              p.proc.wounded_quarry_accumulator_reset->occur();
              p.cooldown.wounded_quarry_trigger_icd->reset( false );
            }
            else
            {
              if ( p.last_reavers_mark_applied && p.last_reavers_mark_applied != b->player &&
                   p.get_target_data( p.last_reavers_mark_applied )->debuffs.reavers_mark->check() )
              {
                p.get_target_data( p.last_reavers_mark_applied )->debuffs.reavers_mark->expire();
              }
              p.last_reavers_mark_applied = b->player;
            }
          } );

  dots.sigil_of_flame = target->get_dot( "sigil_of_flame", &p );
  dots.sigil_of_doom  = target->get_dot( "sigil_of_doom", &p );
  dots.the_hunt       = target->get_dot( "the_hunt_dot", &p );

  debuffs.serrated_glaive =
      make_buff( *this, "serrated_glaive", p.talent.havoc.serrated_glaive->effectN( 1 ).trigger() )
          ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

// Spawn soul fragments from shattered souls proc using chance from sim options,
// Proc Soul Immolation reset on target death for Devourer using chance from sim options
// TODO: only spawn for actors killing blows for non-single actor batch

void demon_hunter_td_t::target_demise()
{
  if ( !( target->is_enemy() ) )
    return;
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source->sim->event_mgr.canceled )
    return;

  if ( dh().specialization() == DEMON_HUNTER_DEVOURER )
  {
    if ( dh().talent.devourer.spontaneous_immolation.enabled() &&
         dh().rng().roll( dh().options.proc_from_killing_blow_chance ) )
    {
      dh().cooldown.soul_immolation->reset( true );
      dh().proc.spontaneous_immolation->occur();
    }
  }
  if ( dh().rng().roll( dh().options.proc_from_killing_blow_chance ) )
  {
    dh().spawn_soul_fragment( dh().proc.soul_fragment_from_death, soul_fragment::GREATER );
    dh().proc.soul_fragment_from_death->occur();
  }
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
    feed_the_demon_accumulator( 0.0 ),
    shattered_destiny_accumulator( 0.0 ),
    wounded_quarry_accumulator( 0.0 ),
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
    options(),
    devourer_fury_state( this )
{
  create_cooldowns();
  create_gains();
  create_benefits();

  resource_regeneration = regen_type::DISABLED;

  sim->register_heartbeat_event_callback( [ this ]( sim_t* ) {
    if ( talent.devourer.entropy && !buff.entropy_out_of_combat->check() && !in_combat && !buff.metamorphosis->check() )
      buff.entropy_out_of_combat->trigger();
  } );
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
      return specialization() == DEMON_HUNTER_DEVOURER ? STAT_INTELLECT : STAT_AGILITY;
    case STAT_STR_INT:
      return specialization() == DEMON_HUNTER_DEVOURER ? STAT_INTELLECT : STAT_NONE;
    case STAT_STR_AGI:
      return specialization() == DEMON_HUNTER_DEVOURER ? STAT_NONE : STAT_AGILITY;
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
  if ( name == "fel_devastation" )
    return new fel_devastation_t( this, options_str );
  if ( name == "fiery_brand" )
    return new fiery_brand_t( this, options_str );
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
  if ( name == "spirit_bomb" )
    return new spirit_bomb_t( this, options_str );
  if ( name == "sigil_of_spite" )
    return new sigil_of_spite_t( this, options_str );
  if ( name == "the_hunt" )
    return new the_hunt_t( this, options_str );
  if ( name == "predators_wake" )
    return new predators_wake_t( this, options_str );
  if ( name == "devour" )
    return new devour_t( this, options_str );
  if ( name == "consume" )
    return new consume_t( this, options_str );
  if ( name == "voidblade" )
    return new voidblade_t( this, options_str );
  if ( name == "pierce_the_veil" )
    return new pierce_the_veil_t( this, options_str );
  if ( name == "soul_immolation" )
    return new soul_immolation_heal_t( this, options_str );
  if ( name == "eradicate" )
    return new eradicate_t( this, options_str );
  if ( name == "cull" )
    return new cull_t( this, options_str );
  if ( name == "reap" )
    return new reap_t( this, options_str );
  if ( name == "void_ray" )
    return new void_ray_t( this, options_str );
  if ( name == "collapsing_star" )
    return new collapsing_star_t( this, options_str );
  if ( name == "hungering_slash" )
    return new hungering_slash_t( this, options_str );
  if ( name == "reapers_toll" )
    return new reapers_toll_t( this, options_str );

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
  if ( name == "felblade" )
    return new felblade_t( this, options_str );
  if ( name == "fel_rush" )
    return new fel_rush_t( this, options_str );
  if ( name == "fracture" )
    return new fracture_t( this, options_str );
  if ( name == "soul_cleave" )
    return new soul_cleave_t( this, options_str );
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

// demon_hunter_t::activate =========================================
void demon_hunter_t::activate()
{
  base_t::activate();
  if ( specialization() == DEMON_HUNTER_DEVOURER )
  {
    register_on_combat_state_callback( [ this ]( player_t*, bool ) { devourer_fury_state.reschedule_drain(); } );

    if ( talent.devourer.entropy->ok() )
    {
      register_precombat_begin( [ this ]( player_t* ) {
        int starting_souls = options.entropy_starting_souls == -1
                                 ? as<int>( talent.devourer.entropy->effectN( 2 ).base_value() )
                                 : options.entropy_starting_souls;
        if ( starting_souls > 0 )
          buff.void_metamorphosis_stack->trigger( starting_souls );
      } );
    }
  }

  if ( talent.vengeance.felfire_fist->ok() )
  {
    register_on_combat_state_callback( [ this ]( player_t*, bool c ) {
      if ( c )
      {
        buff.felfire_fist_out_of_combat->expire();
        buff.felfire_fist_in_combat->trigger();
      }
      else
      {
        buff.felfire_fist_in_combat->expire();
        buff.felfire_fist_out_of_combat->trigger();
      }
    } );
  }

  if ( talent.annihilator.doomsayer->ok() )
  {
    register_on_combat_state_callback( [ this ]( player_t*, bool c ) {
      if ( c )
      {
        buff.doomsayer_out_of_combat->expire();
        buff.doomsayer_in_combat->trigger();
      }
      else
      {
        buff.doomsayer_in_combat->expire();
        buff.doomsayer_out_of_combat->trigger();
      }
    } );
  }

  if ( talent.devourer.entropy->ok() )
  {
    register_on_combat_state_callback( [ this ]( player_t*, bool c ) {
      if ( c )
      {
        buff.entropy_out_of_combat->expire();
        buff.entropy_in_combat->trigger();
      }
      else
        buff.entropy_in_combat->expire();
    } );
  }
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
  buff.soul_fragments       = make_buff( this, "soul_fragments", spec.soul_fragments_buff )
                            ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  // Devourer ===============================================================

  buff.reap           = make_buff( this, "reap", spec.reap );
  buff.feast_of_souls = make_buff( this, "feast_of_souls", spec.feast_of_souls_buff )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                            ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                            ->disable_ticking( true )
                            ->set_activated( true )
                            ->set_disable_async_expire_events_removal( true );

  buff.eradicate = make_buff( this, "eradicate", spec.eradicate_buff );
  buff.moment_of_craving =
      make_buff( this, "moment_of_craving", spec.moment_of_craving_buff )->set_default_value_from_effect( 1 );
  buff.void_metamorphosis_stack = make_buff( this, "void_metamorphosis_stack", spec.void_metamorphosis_stack )
                                      ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                                      ->disable_ticking( true );
  buff.rolling_torment = make_buff( this, "rolling_torment", spec.rolling_torment_buff )->disable_ticking( true );

  buff.emptiness = make_buff( this, "emptiness", spec.emptiness_buff )
                       ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                       ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  if ( spec.emptiness_buff->ok() )
  {
    buff.emptiness->set_default_value( talent.devourer.emptiness->effectN( 1 ).percent() /
                                       spec.emptiness_buff->max_stacks() );
  }

  buff.impending_apocalypse = make_buff<impending_apocalypse_buff_t>( this );

  buff.collapsing_star = make_buff( this, "collapsing_star", spec.collapsing_star_buff )
                             ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  buff.collapsing_star_stack = make_buff<collapsing_star_stacking_t>( this );

  buff.hungering_slash = make_buff( this, "hungering_slash", spec.hungering_slash_buff );

  buff.voidstep =
      make_buff( this, "voidstep", spec.voidstep )->set_stack_change_callback( [ this ]( buff_t*, int old, int cur ) {
        // adjust the max charges of vengeful retreat whenever voidstep is applied/removed.
        cooldown.vengeful_retreat->adjust_max_charges( cur - old );
      } );
  buff.voidstep->reactable = true;

  // TODO: Measure this slow duration instead of guessing.
  buff.voidrush =
      make_buff( this, "voidrush", talent.devourer.voidrush )
          ->set_duration( 0.5_s )
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->add_stack_change_callback( [ this ]( buff_t*, int, int ) { devourer_fury_state.reschedule_drain(); } );

  buff.entropy_out_of_combat =
      make_buff( this, "entropy_out_of_combat" )
          ->set_quiet( true )
          ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
          ->set_period( 1_s )
          ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
          ->set_tick_on_application( false )
          ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
            if ( buff.void_metamorphosis_stack->stack() < talent.devourer.entropy->effectN( 2 ).base_value() )
            {
              buff.void_metamorphosis_stack->trigger();
              proc.void_metamorphosis_stack_from_entropy->occur();
            }
          } );

  // Devourer spawns a Soul Fragment every 8s with Entropy talented

  buff.entropy_in_combat = make_buff( this, "entropy_in_combat" )
                               ->set_quiet( true )
                               ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
                               ->set_period( talent.devourer.entropy->effectN( 1 ).period() )
                               ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                               ->set_tick_on_application( true )
                               ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                                 spawn_soul_fragment( proc.soul_fragment_from_entropy, soul_fragment::LESSER, 1 );
                               } );

  // timespan_t initial_delay = timespan_t::from_millis( rng().range( 0, 5250 ) );

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

  buff.exergy = make_buff( this, "exergy", spec.exergy_buff );
  buff.exergy->set_refresh_duration_callback( []( const buff_t* b, timespan_t d ) {
    // TODO: Verify if this behavior is correct
    return std::min( b->remains() + d, 30_s );  // Capped to 30 seconds
  } );

  buff.inertia = make_buff( this, "inertia", spec.inertia_buff );
  buff.inertia->set_refresh_duration_callback( []( const buff_t* b, timespan_t d ) {
    return std::min( b->remains() + d, 10_s );  // Capped to 10 seconds
  } );
  buff.inertia_trigger = make_buff( this, "inertia_trigger", spec.inertia_trigger_buff );

  buff.inner_demon = make_buff( this, "inner_demon", spec.inner_demon_buff );

  buff.tactical_retreat = make_buff( this, "tactical_retreat", spec.tactical_retreat_buff )
                              ->set_default_value_from_effect_type( A_PERIODIC_ENERGIZE )
                              ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
                                resource_gain( RESOURCE_FURY, b->check_value(), gain.tactical_retreat );
                              } );

  buff.unbound_chaos = make_buff( this, "unbound_chaos", spec.unbound_chaos_buff )
                           ->set_default_value( talent.havoc.unbound_chaos->effectN( 2 ).percent() );

  buff.cycle_of_hatred = make_buff( this, "cycle_of_hatred", spec.cycle_of_hatred_buff )
                             ->set_default_value( talent.havoc.cycle_of_hatred->effectN( 1 ).base_value() );

  buff.chaos_theory = make_buff( this, "chaos_theory", spec.chaos_theory_buff );

  buff.empowered_eye_beam = make_buff( this, "empowered_eye_beam", spec.empowered_eye_beam_buff );

  buff.eternal_hunt = make_buff( this, "eternal_hunt", spec.eternal_hunt_buff );

  // Vengeance ==============================================================

  buff.demon_spikes = new buffs::demon_spikes_t( this );

  buff.painbringer = make_buff( this, "painbringer", spec.painbringer_buff )
                         ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
                         ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                         ->disable_ticking( true );

  buff.soul_barrier = make_buff<absorb_buff_t>( this, "soul_barrier", talent.vengeance.soul_barrier );
  buff.soul_barrier->set_absorb_source( get_stats( "soul_barrier" ) )
      ->set_absorb_gain( get_gain( "soul_barrier" ) )
      ->set_absorb_high_priority( true )  // TOCHECK
      ->set_cooldown( timespan_t::zero() );

  buff.felfire_fist_in_combat = make_buff( this, "felfire_fist_in_combat", spec.felfire_fist_in_combat_buff )
                                    ->set_quiet( true )
                                    ->set_allow_precombat( true );
  buff.felfire_fist_out_of_combat =
      make_buff( this, "felfire_fist_out_of_combat", spec.felfire_fist_out_of_combat_buff )
          ->set_quiet( true )
          ->set_allow_precombat( true );

  buff.untethered_rage = make_buff( this, "untethered_rage", spec.untethered_rage_buff )
                             ->set_stack_change_callback( [ this ]( buff_t*, int old, int cur ) {
                               // Grant/remove a temporary charge of Metamorphosis when Untethered Rage is gained/lost.
                               cooldown.metamorphosis->adjust_max_charges( cur - old );
                             } );
  buff.seething_anger =
      make_buff( this, "seething_anger", spec.seething_anger_buff )->set_default_value_from_effect( 1 );

  buff.fiery_brand = make_buff( this, "fiery_brand", spec.fiery_brand_debuff )
                         ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
                         ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                         ->disable_ticking( true );

  // Aldrachi Reaver ========================================================

  buff.reavers_glaive = make_buff( this, "reavers_glaive", hero_spec.reavers_glaive_buff )->set_quiet( true );

  double art_of_the_glaive_buff_value = 0;
  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
      art_of_the_glaive_buff_value = talent.aldrachi_reaver.art_of_the_glaive->effectN( 1 ).base_value();
      break;
    case DEMON_HUNTER_VENGEANCE:
      art_of_the_glaive_buff_value = talent.aldrachi_reaver.art_of_the_glaive->effectN( 2 ).base_value();
      break;
    default:
      break;
  }
  buff.art_of_the_glaive = make_buff( this, "art_of_the_glaive", hero_spec.art_of_the_glaive_buff )
                               ->set_default_value( art_of_the_glaive_buff_value )
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
  buff.warblades_hunger = make_buff( this, "warblades_hunger", hero_spec.warblades_hunger_buff )->set_max_stack( 6 );
  buff.thrill_of_the_fight_haste =
      make_buff( this, "thrill_of_the_fight_haste", hero_spec.thrill_of_the_fight_haste_buff )
          ->set_default_value_from_effect_type( A_HASTE_ALL )
          ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

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

  // Annihilator ============================================================

  buff.voidfall_building   = make_buff<voidfall_building_buff_t>( this );
  buff.voidfall_spending   = make_buff<voidfall_spending_buff_t>( this );
  buff.voidfall_final_hour = make_buff( this, "voidfall_final_hour", hero_spec.voidfall_final_hour_buff )
                                 ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
                                 ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                 ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                                 ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                                 ->disable_ticking( true );
  buff.dark_matter         = make_buff( this, "dark_matter", hero_spec.dark_matter_buff );
  buff.doomsayer_in_combat = make_buff( this, "doomsayer_in_combat", hero_spec.doomsayer_in_combat_buff )
                                 ->set_quiet( true )
                                 ->set_allow_precombat( true )
                                 ->set_duration_multiplier( talent.annihilator.doomsayer->effectN( 2 ).base_value() );
  buff.doomsayer_out_of_combat = make_buff( this, "doomsayer_out_of_combat", hero_spec.doomsayer_out_of_combat_buff )
                                     ->set_quiet( true )
                                     ->set_allow_precombat( true );

  // Scarred ============================================================

  buff.enduring_torment =
      make_buff( this, "enduring_torment", hero_spec.enduring_torment_buff )
          ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
          ->set_allow_precombat( true )
          ->set_quiet( true )
          ->set_default_value_from_effect_type( specialization() == DEMON_HUNTER_HAVOC ? A_HASTE_ALL
                                                                                       : A_ADD_PCT_LABEL_MODIFIER )
          ->set_pct_buff_type( specialization() == DEMON_HUNTER_HAVOC ? STAT_PCT_BUFF_HASTE : STAT_PCT_BUFF_MASTERY );

  buff.monster_rising = make_buff( this, "monster_rising", hero_spec.monster_rising_buff )
                            ->set_allow_precombat( true )
                            ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                            ->set_quiet( true );
  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      buff.monster_rising->set_default_value_from_effect( 2 )
          ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
          ->add_invalidate( CACHE_INTELLECT );
      break;
    case DEMON_HUNTER_HAVOC:
      buff.monster_rising->set_default_value_from_effect( 1 )
          ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
          ->add_invalidate( CACHE_AGILITY );
      break;
    default:
      break;
  }

  buff.pursuit_of_angryness =
      make_buff( this, "pursuit_of_angriness", talent.scarred.pursuit_of_angriness )
          ->set_quiet( true )
          ->set_tick_zero( true )
          ->set_tick_callback(
              [ this, speed_per_fury = talent.scarred.pursuit_of_angriness->effectN( 1 ).percent() /
                                       talent.scarred.pursuit_of_angriness->effectN( 1 ).base_value() ]( buff_t* b, int,
                                                                                                         timespan_t ) {
                // TOCHECK - Does this need to floor if it's not a whole number
                b->current_value = resources.current[ RESOURCE_FURY ] * speed_per_fury;
              } );
  buff.student_of_suffering = make_buff<student_of_suffering_buff_t>( this );

  for ( demonsurge_ability ability : demonsurge_abilities )
  {
    buff.demonsurge_abilities[ ability ] =
        make_buff( this, demonsurge_ability_action_name( ability ), hero_spec.demonsurge_placeholder_buff )
            ->set_quiet( true );
  }
  for ( voidsurge_ability ability : voidsurge_abilities )
  {
    buff.voidsurge_abilities[ ability ] =
        make_buff( this, voidsurge_ability_action_name( ability ), hero_spec.demonsurge_placeholder_buff )
            ->set_quiet( true );
  }

  buff.demonsurge_demonsurge = make_buff( this, "demonsurge_demonsurge", hero_spec.demonsurge_demonsurge_buff );
  buff.demonsurge_demonic_intensity =
      make_buff( this, "demonsurge_demonic_intensity", hero_spec.demonsurge_demonic_intensity_buff );
  buff.demonsurge        = make_buff( this, "demonsurge", hero_spec.demonsurge_stacking_buff );
  buff.voidsurge         = make_buff( this, "voidsurge", hero_spec.voidsurge_stacking_buff );
  buff.volatile_instinct = make_buff( this, "volatile_instinct", hero_spec.volatile_instinct );

  // Set Bonus Items ========================================================
}

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
        throw sc_invalid_apl_argument( fmt::format( "Unsupported soul_fragments filter '{}'.", splits[ 1 ] ) );
      }
    }

    return std::make_unique<soul_fragments_expr_t>( this, name_str, type, filter );
  }

  if ( name_str == "cooldown.bd_ds_shared.remains" )
  {
    return this->cooldown.blade_dance->create_expression( "remains" );
  }

  if ( util::str_compare_ci( name_str, "void_metamorphosis_base_drain_ps" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      return devourer_fury_state.base_fury_drain_per_second( devourer_fury_state.drain_stacks );
    } );
  }

  if ( util::str_compare_ci( name_str, "void_metamorphosis_drain_ps" ) )
  {
    return make_fn_expr( name_str, [ this ]() {
      return devourer_fury_state.fury_drain_per_second( devourer_fury_state.drain_stacks );
    } );
  }

  if ( splits.size() == 3 )
  {
    if ( util::str_compare_ci( splits[ 0 ], "buff" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "felfire_fist" ) )
      {
        if ( util::str_compare_ci( splits[ 2 ], "up" ) )
        {
          return make_fn_expr( name_str, [ this ]() {
            return buff.felfire_fist_in_combat->check() || buff.felfire_fist_out_of_combat->check();
          } );
        }
        if ( util::str_compare_ci( splits[ 2 ], "down" ) )
        {
          return make_fn_expr( name_str, [ this ]() {
            return !( buff.felfire_fist_in_combat->check() || buff.felfire_fist_out_of_combat->check() );
          } );
        }
      }

      if ( util::str_compare_ci( splits[ 1 ], "doomsayer" ) )
      {
        if ( util::str_compare_ci( splits[ 2 ], "up" ) )
        {
          return make_fn_expr( name_str, [ this ]() {
            return buff.doomsayer_in_combat->check() || buff.doomsayer_out_of_combat->check();
          } );
        }
        if ( util::str_compare_ci( splits[ 2 ], "down" ) )
        {
          return make_fn_expr( name_str, [ this ]() {
            return !( buff.doomsayer_in_combat->check() || buff.doomsayer_out_of_combat->check() );
          } );
        }
      }
    }

    if ( util::str_compare_ci( splits[ 0 ], "action" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "sigil_of_flame" ) )
      {
        auto sof_action = find_action( "sigil_of_flame" );

        if ( !sof_action )
          return expr_t::create_constant( name_str, false );

        auto expr = sof_action->create_expression( util::string_join( util::make_span( splits ).subspan( 2 ), "." ) );
        if ( expr )
          return expr;

        auto tail = name_str.substr( splits[ 0 ].length() + splits[ 1 ].length() + 2 );

        throw sc_invalid_apl_argument( fmt::format( "Unsupported sigil_of_flame expression '{}'.", tail ) );
      }

      if ( util::str_compare_ci( splits[ 1 ], "sigil_of_spite" ) )
      {
        auto sosp_action = find_action( "sigil_of_spite" );

        if ( !sosp_action )
          return expr_t::create_constant( name_str, false );

        auto expr = sosp_action->create_expression( util::string_join( util::make_span( splits ).subspan( 2 ), "." ) );
        if ( expr )
          return expr;

        auto tail = name_str.substr( splits[ 0 ].length() + splits[ 1 ].length() + 2 );

        throw sc_invalid_apl_argument( fmt::format( "Unsupported sigil_of_spite expression '{}'.", tail ) );
      }
    }
  }

  return player_t::create_expression( name_str );
}

// demon_hunter_t::create_options
// ==================================================

void demon_hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "demonhunter.target_reach", options.target_reach ) );
  add_option( opt_deprecated( "target_reach", "demonhunter.target_reach" ) );
  add_option( opt_float( "demonhunter.movement_direction_factor", options.movement_direction_factor, 1.0, 2.0 ) );
  add_option( opt_deprecated( "movement_direction_factor", "demonhunter.movement_direction_factor" ) );
  add_option( opt_float( "demonhunter.initial_fury", options.initial_fury, 0.0, 120 ) );
  add_option( opt_deprecated( "initial_fury", "demonhunter.initial_fury" ) );
  add_option( opt_bool( "demonhunter.reset_fury_on_pull", options.reset_fury_on_pull ) );
  add_option( opt_deprecated( "reset_fury_on_pull", "demonhunter.reset_fury_on_pull" ) );
  add_option( opt_float( "demonhunter.soul_fragment_movement_consume_chance",
                         options.soul_fragment_movement_consume_chance, 0, 1 ) );
  add_option(
      opt_deprecated( "soul_fragment_movement_consume_chance", "demonhunter.soul_fragment_movement_consume_chance" ) );
  add_option(
      opt_float( "demonhunter.wounded_quarry_chance_vengeance", options.wounded_quarry_chance_vengeance, 0, 1 ) );
  add_option( opt_deprecated( "wounded_quarry_chance_vengeance", "demonhunter.wounded_quarry_chance_vengeance" ) );
  add_option( opt_float( "demonhunter.wounded_quarry_chance_havoc", options.wounded_quarry_chance_havoc, 0, 1 ) );
  add_option( opt_deprecated( "wounded_quarry_chance_havoc", "demonhunter.wounded_quarry_chance_havoc" ) );
  add_option( opt_float( "demonhunter.felblade_lockout_from_vengeful_retreat",
                         options.felblade_lockout_from_vengeful_retreat, 0, 1 ) );
  add_option( opt_deprecated( "felblade_lockout_from_vengeful_retreat",
                              "demonhunter.felblade_lockout_from_vengeful_retreat" ) );
  add_option( opt_bool( "demonhunter.enable_dungeon_slice", options.enable_dungeon_slice ) );
  add_option( opt_deprecated( "enable_dungeon_slice", "demonhunter.enable_dungeon_slice" ) );
  add_option(
      opt_float( "demonhunter.proc_from_killing_blow_chance", options.proc_from_killing_blow_chance, 0.0, 1.0 ) );
  add_option( opt_deprecated( "proc_from_killing_blow_chance", "demonhunter.proc_from_killing_blow_chance" ) );
  add_option( opt_int( "demonhunter.entropy_starting_souls", options.entropy_starting_souls, -1, 50 ) );
  add_option( opt_deprecated( "entropy_starting_souls", "demonhunter.entropy_starting_souls" ) );
  add_option( opt_int( "demonhunter.channel_tick_cutoff_benefit", options.channel_tick_cutoff_benefit, 0, 10 ) );
  add_option( opt_deprecated( "channel_tick_cutoff_benefit", "demonhunter.channel_tick_cutoff_benefit" ) );

  add_option( opt_float( "demonhunter.void_metamorphosis_initial_drain", devourer_fury_state.initial_drain, 0, 100 ) );
  add_option( opt_deprecated( "void_metamorphosis_initial_drain", "demonhunter.void_metamorphosis_initial_drain" ) );
  add_option( opt_float( "demonhunter.void_metamorphosis_exp_factor", devourer_fury_state.exp_factor, 0, 100 ) );
  add_option( opt_deprecated( "void_metamorphosis_exp_factor", "demonhunter.void_metamorphosis_exp_factor" ) );
  add_option( opt_float( "demonhunter.void_metamorphosis_exp_power", devourer_fury_state.exp_power, 0, 100 ) );
  add_option( opt_deprecated( "void_metamorphosis_exp_power", "demonhunter.void_metamorphosis_exp_power" ) );
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
  // TODO: remove Devourer check once Devourer has a default loadout
  if ( ( main_hand_weapon.type == WEAPON_NONE || off_hand_weapon.type == WEAPON_NONE ) &&
       specialization() != DEMON_HUNTER_DEVOURER )
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

  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      demon_hunter_apl::devourer( this );
      break;
    case DEMON_HUNTER_HAVOC:
      demon_hunter_apl::havoc( this );
      break;
    case DEMON_HUNTER_VENGEANCE:
      demon_hunter_apl::vengeance( this );
      break;
    default:
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

// demon_hunter_t::init_base_stats ==========================================

void demon_hunter_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5.0;

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  base_t::init_base_stats();
}

// demon_hunter_t::init_procs ===============================================

void demon_hunter_t::init_procs()
{
  base_t::init_procs();

  // General
  proc.delayed_aa_range                 = get_proc( "Delayed AA out of range" );
  proc.soul_fragment_greater            = get_proc( "Soul Fragment (Greater)" );
  proc.soul_fragment_greater_demon      = get_proc( "Soul Fragment (Greater Demon)" );
  proc.soul_fragment_empowered_demon    = get_proc( "Soul Fragment (Empowered Demon)" );
  proc.soul_fragment_lesser             = get_proc( "Soul Fragment" );
  proc.soul_fragment_from_soul_splitter = get_proc( "Soul Fragment from Soul Splitter" );
  proc.soul_fragment_from_death         = get_proc( "Soul Fragment from Death" );
  proc.soul_fragment_expire             = get_proc( "Soul Fragment expiration" );
  proc.soul_fragment_overflow           = get_proc( "Soul Fragment overflow" );

  // Devourer
  proc.spontaneous_immolation                = get_proc( "Spontaneous Immolation" );
  proc.void_metamorphosis_stack_from_entropy = get_proc( "Void Metamorphosis Stack from Entropy" );
  proc.soul_fragment_from_consume            = get_proc( "Soul Fragment from Consume" );
  proc.soul_fragment_from_devour             = get_proc( "Soul Fragment from Devour" );
  proc.soul_fragment_from_soul_immolation    = get_proc( "Soul Fragment from Soul Immolation" );
  proc.soul_fragment_from_shattered_souls    = get_proc( "Soul Fragment from Shattered Souls" );
  proc.soul_fragment_from_star_fragments     = get_proc( "Soul Fragment from Star Fragments" );
  proc.soul_fragment_from_hungering_slash    = get_proc( "Soul Fragment from Hungering Slash" );
  proc.soul_fragment_from_reapers_toll       = get_proc( "Soul Fragment from Reaper's Toll" );
  proc.soul_fragment_from_void_metamorphosis = get_proc( "Soul Fragment from Void Metamorphosis" );
  proc.soul_fragment_from_entropy            = get_proc( "Soul Fragment from Entropy" );

  // Havoc
  proc.soul_fragment_from_demonic_appetite = get_proc( "Soul Fragment from Demonic Appetite" );
  proc.chaos_strike_in_essence_break       = get_proc( "Chaos Strike in Essence Break" );
  proc.annihilation_in_essence_break       = get_proc( "Annihilation in Essence Break" );
  proc.blade_dance_in_essence_break        = get_proc( "Blade Dance in Essence Break" );
  proc.death_sweep_in_essence_break        = get_proc( "Death Sweep in Essence Break" );
  proc.chaos_strike_in_serrated_glaive     = get_proc( "Chaos Strike in Serrated Glaive" );
  proc.annihilation_in_serrated_glaive     = get_proc( "Annihilation in Serrated Glaive" );
  proc.throw_glaive_in_serrated_glaive     = get_proc( "Throw Glaive in Serrated Glaive" );
  proc.shattered_destiny                   = get_proc( "Shattered Destiny" );
  proc.eye_beam_canceled                   = get_proc( "Eye Beam canceled" );

  // Vengeance
  proc.untethered_rage                   = get_proc( "Untethered Rage" );
  proc.soul_fragment_from_soul_sigils    = get_proc( "Soul Sigils" );
  proc.soul_fragment_from_fracture       = get_proc( "Soul Fragment from Fracture" );
  proc.soul_fragment_from_fracture_meta  = get_proc( "Soul Fragment from Fracture (Meta)" );
  proc.soul_fragment_from_sigil_of_spite = get_proc( "Soul Fragment from Sigil of Spite" );
  proc.soul_fragment_from_fallout        = get_proc( "Soul Fragment from Fallout" );
  proc.soul_fragment_from_soul_carver    = get_proc( "Soul Fragment from Soul Carver" );
  proc.feed_the_demon                    = get_proc( "Feed the Demon" );

  // Aldrachi Reaver
  proc.unhindered_assault                  = get_proc( "Unhindered Assault" );
  proc.soul_fragment_from_aldrachi_tactics = get_proc( "Soul Fragment from Aldrachi Tactics" );
  proc.soul_fragment_from_broken_spirit    = get_proc( "Soul Fragment from Broken Spirit" );
  proc.soul_fragment_from_wounded_quarry   = get_proc( "Soul Fragment from Wounded Quarry" );
  proc.wounded_quarry_accumulator_reset    = get_proc( "Wounded Quarry Accumulator Reset" );

  // Annihilator
  proc.soul_fragment_from_meteoric_rise = get_proc( "Soul Fragment from Meteoric Rise" );
  proc.soul_fragment_from_world_killer  = get_proc( "Soul Fragment from World Killer" );
  proc.voidfall_from_builder            = get_proc( "Voidfall from Builder" );
  proc.voidfall_from_mass_acceleration  = get_proc( "Voidfall from Mass Acceleration" );
  proc.voidfall_from_meteoric_rise      = get_proc( "Voidfall from Meteoric Rise" );

  // Scarred
  for ( demonsurge_ability ability : demonsurge_abilities )
  {
    proc.demonsurge_abilities[ ability ] = get_proc( demonsurge_ability_proc_name( ability ) );
  }
  proc.demonsurge_abilities[ demonsurge_ability::ENTER_META ] =
      get_proc( demonsurge_ability_proc_name( demonsurge_ability::ENTER_META ) );

  for ( voidsurge_ability ability : voidsurge_abilities )
  {
    proc.voidsurge_abilities[ ability ] = get_proc( voidsurge_ability_proc_name( ability ) );
  }
  proc.voidsurge_abilities[ voidsurge_ability::REAPERS_TOLL ] =
      get_proc( voidsurge_ability_proc_name( voidsurge_ability::REAPERS_TOLL ) );
  proc.voidsurge_abilities[ voidsurge_ability::VOLATILE_INSTINCT ] =
      get_proc( voidsurge_ability_proc_name( voidsurge_ability::VOLATILE_INSTINCT ) );
  proc.undying_embers = get_proc( "Undying Embers" );

  // Set Bonuses
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

  if ( options.initial_fury > 0.0 )
    resources.current[ RESOURCE_FURY ] = options.initial_fury;

  expected_max_health = calculate_expected_max_health();
}

// demon_hunter_t::init_special_effects =====================================

void demon_hunter_t::init_special_effects()
{
  base_t::init_special_effects();

  // Devourer

  // Havoc
  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    auto effect            = new special_effect_t( this );
    effect->name_str       = "demon_blades";
    effect->spell_id       = spec.demon_blades->id();
    effect->execute_action = active.demon_blades;
    special_effects.push_back( effect );

    new demon_hunter_proc_callback_t( *effect );
  }
}

// demon_hunter_t::init_rng =================================================

void demon_hunter_t::init_rng()
{
  // RPPM objects

  // General
  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      break;
    case DEMON_HUNTER_HAVOC:
      rppm.demonic_appetite = get_rppm( "demonic_appetite", spec.demonic_appetite );
      break;
    case DEMON_HUNTER_VENGEANCE:
      break;
    default:
      break;
  }

  player_t::init_rng();
}

// demon_hunter_t::init_scaling =============================================

void demon_hunter_t::init_scaling()
{
  base_t::init_scaling();

  if ( specialization() != DEMON_HUNTER_DEVOURER )
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
  spell.all_demon_hunter       = find_spell( dbc::get_class_aura_id( DEMON_HUNTER ) );
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
  spec.immolation_aura_cdr = find_spell( 320378, DEMON_HUNTER_VENGEANCE );
  spec.thick_skin          = find_specialization_spell( "Thick Skin" );

  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
      spell.throw_glaive                 = find_class_spell( "Throw Glaive" );
      spec.consume_soul                  = find_spell( 178963 );
      spec.consume_soul_greater_heal     = find_spell( 178963 );
      spec.consume_soul_greater_energize = spec.consume_soul_greater_heal->effectN( 2 ).trigger();
      spec.consume_soul_lesser_heal      = find_spell( 228532 );
      spec.consume_soul_lesser_energize  = spec.consume_soul_lesser_heal->effectN( 2 ).trigger();
      spec.metamorphosis                 = find_class_spell( "Metamorphosis" );
      spec.metamorphosis_buff            = spec.metamorphosis->effectN( 2 ).trigger();
      spec.soul_fragments_buff           = spell_data_t::not_found();
      spec.shattered_souls               = find_spell( 178940 );
      break;
    case DEMON_HUNTER_VENGEANCE:
      spell.throw_glaive                 = find_specialization_spell( "Throw Glaive" );
      spec.consume_soul                  = find_spell( 210042 );
      spec.consume_soul_greater_heal     = find_spell( 210042 );
      spec.consume_soul_greater_energize = spell_data_t::not_found();
      spec.consume_soul_lesser_heal      = find_spell( 203794 );
      spec.consume_soul_lesser_energize  = spell_data_t::not_found();
      spec.metamorphosis                 = find_specialization_spell( "Metamorphosis" );
      spec.metamorphosis_buff            = spec.metamorphosis;
      spec.soul_fragments_buff           = find_spell( 203981 );
      spec.shattered_souls               = find_spell( 178940 );
      break;
    case DEMON_HUNTER_DEVOURER:
      spell.throw_glaive                 = find_class_spell( "Throw Glaive" );
      spec.consume_soul                  = find_spell( 1223423 );
      spec.consume_soul_greater_heal     = find_spell( 1266301 );
      spec.consume_soul_greater_energize = find_spell( 1223628 );
      spec.consume_soul_lesser_heal      = find_spell( 1266301 );
      spec.consume_soul_lesser_energize  = find_spell( 1223628 );
      spec.metamorphosis                 = find_specialization_spell( "Void Metamorphosis" );
      spec.metamorphosis_buff            = spec.metamorphosis->effectN( 1 ).trigger();
      spec.soul_fragments_buff           = find_spell( 1245577 );
      spec.shattered_souls               = find_spell( 1227619 );
      break;
    default:
      break;
  }

  // Devourer Spells
  spec.devourer_demon_hunter    = find_specialization_spell( "Devourer Demon Hunter" );
  spec.consume                  = find_spell( 473662, DEMON_HUNTER_DEVOURER );
  spec.consume_energize         = find_spell( 1261710, DEMON_HUNTER_DEVOURER );
  spec.devour                   = find_spell( 1217610, DEMON_HUNTER_DEVOURER );
  spec.devour_energize          = find_spell( 1288123, DEMON_HUNTER_DEVOURER );
  spec.voidblade                = find_spell( 1245414, DEMON_HUNTER_DEVOURER );
  spec.soul_immolation_energize = find_spell( 1242475, DEMON_HUNTER_DEVOURER );
  spec.reap                     = find_spell( 1226019, DEMON_HUNTER_DEVOURER );
  spec.reap_damage              = find_spell( 1225823, DEMON_HUNTER_DEVOURER );
  spec.reap_energize            = find_spell( 1261679, DEMON_HUNTER_DEVOURER );
  spec.cull                     = find_spell( 1245453, DEMON_HUNTER_DEVOURER );
  spec.cull_damage              = find_spell( 1245455, DEMON_HUNTER_DEVOURER );

  // Havoc Spells
  spec.havoc_demon_hunter = find_specialization_spell( "Havoc Demon Hunter" );

  spec.annihilation        = find_spell( 201427, DEMON_HUNTER_HAVOC );
  spec.blade_dance         = find_specialization_spell( "Blade Dance" );
  spec.blade_dance_2       = find_rank_spell( "Blade Dance", "Rank 2" );
  spec.blur                = find_specialization_spell( "Blur" );
  spec.chaos_strike        = find_specialization_spell( "Chaos Strike" );
  spec.chaos_strike_fury   = find_spell( 193840, DEMON_HUNTER_HAVOC );
  spec.chaos_strike_refund = find_spell( 197125, DEMON_HUNTER_HAVOC );
  spec.death_sweep         = find_spell( 210152, DEMON_HUNTER_HAVOC );
  spec.fel_rush            = find_specialization_spell( "Fel Rush" );
  spec.fel_rush_damage     = find_spell( 192611, DEMON_HUNTER_HAVOC );
  spec.immolation_aura_3   = find_rank_spell( "Immolation Aura", "Rank 3" );
  spec.demonic_appetite    = find_spell( 206478, DEMON_HUNTER_HAVOC );

  // Vengeance Spells
  spec.vengeance_demon_hunter = find_specialization_spell( "Vengeance Demon Hunter" );

  spec.demon_spikes    = find_specialization_spell( "Demon Spikes" );
  spec.infernal_strike = find_specialization_spell( "Infernal Strike" );
  spec.soul_cleave     = find_specialization_spell( "Soul Cleave" );
  spec.fracture        = find_spell( 263642, DEMON_HUNTER_VENGEANCE );
  spec.riposte         = find_specialization_spell( "Riposte" );

  // Masteries ==============================================================

  mastery.monster_within   = find_mastery_spell( DEMON_HUNTER_DEVOURER );
  mastery.demonic_presence = find_mastery_spell( DEMON_HUNTER_HAVOC );
  mastery.fel_blood        = find_mastery_spell( DEMON_HUNTER_VENGEANCE );
  mastery.fel_blood_rank_2 = find_rank_spell( "Mastery: Fel Blood", "Rank 2" );

  // Talents ================================================================

  talent.demon_hunter.vengeful_retreat = find_talent_spell( talent_tree::CLASS, "Vengeful Retreat" );
  talent.demon_hunter.felblade         = find_talent_spell( talent_tree::CLASS, "Felblade" );
  talent.demon_hunter.voidblade        = find_talent_spell( talent_tree::CLASS, "Voidblade" );
  talent.demon_hunter.sigil_of_misery  = find_talent_spell( talent_tree::CLASS, "Sigil of Misery" );

  talent.demon_hunter.vengeful_bonds           = find_talent_spell( talent_tree::CLASS, "Vengeful Bonds" );
  talent.demon_hunter.unrestrained_fury        = find_talent_spell( talent_tree::CLASS, "Unrestrained Fury" );
  talent.demon_hunter.shattered_restoration    = find_talent_spell( talent_tree::CLASS, "Shattered Restoration" );
  talent.demon_hunter.improved_sigil_of_misery = find_talent_spell( talent_tree::CLASS, "Improved Sigil of Misery" );

  talent.demon_hunter.bouncing_glaives  = find_talent_spell( talent_tree::CLASS, "Bouncing Glaives" );
  talent.demon_hunter.imprison          = find_talent_spell( talent_tree::CLASS, "Imprison" );
  talent.demon_hunter.charred_warblades = find_talent_spell( talent_tree::CLASS, "Charred Warglaives" );

  talent.demon_hunter.chaos_nova       = find_talent_spell( talent_tree::CLASS, "Chaos Nova" );
  talent.demon_hunter.void_nova        = find_talent_spell( talent_tree::CLASS, "Void Nova" );
  talent.demon_hunter.improved_disrupt = find_talent_spell( talent_tree::CLASS, "Improved Disrupt" );
  talent.demon_hunter.consume_magic    = find_talent_spell( talent_tree::CLASS, "Consume Magic" );
  talent.demon_hunter.aldrachi_design  = find_talent_spell( talent_tree::CLASS, "Aldrachi Design" );

  talent.demon_hunter.focused_ire            = find_talent_spell( talent_tree::CLASS, "Master of the Glaive" );
  talent.demon_hunter.master_of_the_glaive   = find_talent_spell( talent_tree::CLASS, "Master of the Glaive" );
  talent.demon_hunter.champion_of_the_glaive = find_talent_spell( talent_tree::CLASS, "Champion of the Glaive" );
  talent.demon_hunter.disrupting_fury        = find_talent_spell( talent_tree::CLASS, "Disrupting Fury" );
  talent.demon_hunter.blazing_path           = find_talent_spell( talent_tree::CLASS, "Blazing Path" );
  talent.demon_hunter.swallowed_anger        = find_talent_spell( talent_tree::CLASS, "Swallowed Anger" );
  talent.demon_hunter.aura_of_pain           = find_talent_spell( talent_tree::CLASS, "Aura of Pain" );
  talent.demon_hunter.live_by_the_glaive     = find_talent_spell( talent_tree::CLASS, "Live by the Glaive" );

  talent.demon_hunter.pursuit          = find_talent_spell( talent_tree::CLASS, "Pursuit" );
  talent.demon_hunter.soul_rending     = find_talent_spell( talent_tree::CLASS, "Soul Rending" );
  talent.demon_hunter.felfire_haste    = find_talent_spell( talent_tree::CLASS, "Felfire Haste" );
  talent.demon_hunter.infernal_armor   = find_talent_spell( talent_tree::CLASS, "Infernal Armor" );
  talent.demon_hunter.burn_it_out      = find_talent_spell( talent_tree::CLASS, "Burn It Out" );
  talent.demon_hunter.soul_cleanse     = find_talent_spell( talent_tree::CLASS, "Soul Cleanse" );
  talent.demon_hunter.lost_in_darkness = find_talent_spell( talent_tree::CLASS, "Lost in Darkness" );

  talent.demon_hunter.illidari_knowledge   = find_talent_spell( talent_tree::CLASS, "Illidari Knowledge" );
  talent.demon_hunter.felbound             = find_talent_spell( talent_tree::CLASS, "Felbound" );
  talent.demon_hunter.guile                = find_talent_spell( talent_tree::CLASS, "Guile" );
  talent.demon_hunter.will_of_the_illidari = find_talent_spell( talent_tree::CLASS, "Will of the Illidari" );

  talent.demon_hunter.internal_struggle = find_talent_spell( talent_tree::CLASS, "Internal Struggle" );
  talent.demon_hunter.furious           = find_talent_spell( talent_tree::CLASS, "Furious" );
  talent.demon_hunter.remorseless       = find_talent_spell( talent_tree::CLASS, "Remorseless" );
  talent.demon_hunter.darkness          = find_talent_spell( talent_tree::CLASS, "Darkness" );

  talent.demon_hunter.erratic_felheart = find_talent_spell( talent_tree::CLASS, "Erratic Felheart" );
  talent.demon_hunter.final_breath     = find_talent_spell( talent_tree::CLASS, "Final Breath" );
  talent.demon_hunter.darkness         = find_talent_spell( talent_tree::CLASS, "Darkness" );
  talent.demon_hunter.demon_muzzle     = find_talent_spell( talent_tree::CLASS, "Demon Muzzle" );
  talent.demon_hunter.soul_splitter    = find_talent_spell( talent_tree::CLASS, "Soul Splitter" );

  talent.demon_hunter.wings_of_wrath     = find_talent_spell( talent_tree::CLASS, "Wings of Wrath" );
  talent.demon_hunter.long_night         = find_talent_spell( talent_tree::CLASS, "Long Night" );
  talent.demon_hunter.pitch_black        = find_talent_spell( talent_tree::CLASS, "Pitch Black" );
  talent.demon_hunter.demonic_resilience = find_talent_spell( talent_tree::CLASS, "Demonic Resilience" );

  // Devourer Talents

  talent.devourer.void_ray = find_talent_spell( talent_tree::SPECIALIZATION, "Void Ray" );

  talent.devourer.soul_immolation  = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Immolation" );
  talent.devourer.predators_thirst = find_talent_spell( talent_tree::SPECIALIZATION, "Predator's Thirst" );

  talent.devourer.tempered_soul          = find_talent_spell( talent_tree::SPECIALIZATION, "Tempered Soul" );
  talent.devourer.spontaneous_immolation = find_talent_spell( talent_tree::SPECIALIZATION, "Spontaneous Immolation" );
  talent.devourer.void_metamorphosis     = find_talent_spell( talent_tree::SPECIALIZATION, "Void Metamorphosis" );
  talent.devourer.feast_of_souls         = find_talent_spell( talent_tree::SPECIALIZATION, "Feast of Souls" );

  talent.devourer.scythes_embrace   = find_talent_spell( talent_tree::SPECIALIZATION, "Scythe's Embrace" );
  talent.devourer.duty_eternal      = find_talent_spell( talent_tree::SPECIALIZATION, "Duty Eternal" );
  talent.devourer.collapsing_star   = find_talent_spell( talent_tree::SPECIALIZATION, "Collapsing Star" );
  talent.devourer.moment_of_craving = find_talent_spell( talent_tree::SPECIALIZATION, "Moment of Craving" );

  talent.devourer.gift_of_the_void = find_talent_spell( talent_tree::SPECIALIZATION, "Gift of the Void" );
  talent.devourer.entropy          = find_talent_spell( talent_tree::SPECIALIZATION, "Entropy" );
  talent.devourer.waste_not        = find_talent_spell( talent_tree::SPECIALIZATION, "Waste Not" );
  talent.devourer.soulshaper       = find_talent_spell( talent_tree::SPECIALIZATION, "Soulshaper" );

  talent.devourer.singed_spirit    = find_talent_spell( talent_tree::SPECIALIZATION, "Singed Spirit" );
  talent.devourer.sweet_suffering  = find_talent_spell( talent_tree::SPECIALIZATION, "Sweet Suffering" );
  talent.devourer.second_helping   = find_talent_spell( talent_tree::SPECIALIZATION, "Second Helping" );
  talent.devourer.umbral_blade     = find_talent_spell( talent_tree::SPECIALIZATION, "Umbral Blade" );
  talent.devourer.improved_consume = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Consume" );
  talent.devourer.sweet_release    = find_talent_spell( talent_tree::SPECIALIZATION, "Sweet Release" );
  talent.devourer.voidpurge        = find_talent_spell( talent_tree::SPECIALIZATION, "Voidpurge" );

  talent.devourer.hungering_slash = find_talent_spell( talent_tree::SPECIALIZATION, "Hungering Slash" );
  talent.devourer.voidrage        = find_talent_spell( talent_tree::SPECIALIZATION, "Voidrage" );
  talent.devourer.focused_ray     = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Ray" );

  talent.devourer.soulforged_blades = find_talent_spell( talent_tree::SPECIALIZATION, "Soulforged Blades" );
  talent.devourer.singular_strikes  = find_talent_spell( talent_tree::SPECIALIZATION, "Singular Strikes" );
  talent.devourer.demonic_instinct  = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Instinct" );
  talent.devourer.devourers_edge    = find_talent_spell( talent_tree::SPECIALIZATION, "Devourer's Edge" );
  talent.devourer.voidglare_boon    = find_talent_spell( talent_tree::SPECIALIZATION, "Voidglare Boon" );
  talent.devourer.rolling_torment   = find_talent_spell( talent_tree::SPECIALIZATION, "Rolling Torment" );

  talent.devourer.voidrush             = find_talent_spell( talent_tree::SPECIALIZATION, "Voidrush" );
  talent.devourer.devourers_bite       = find_talent_spell( talent_tree::SPECIALIZATION, "Devourer's Bite" );
  talent.devourer.impending_apocalypse = find_talent_spell( talent_tree::SPECIALIZATION, "Impending Apocalypse" );
  talent.devourer.calamitous           = find_talent_spell( talent_tree::SPECIALIZATION, "Calamitous" );
  talent.devourer.star_fragments       = find_talent_spell( talent_tree::SPECIALIZATION, "Star Fragments" );

  talent.devourer.the_hunt     = find_talent_spell( talent_tree::SPECIALIZATION, "The Hunt" );
  talent.devourer.emptiness    = find_talent_spell( talent_tree::SPECIALIZATION, "Emptiness" );
  talent.devourer.soul_glutton = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Glutton" );
  talent.devourer.eradicate    = find_talent_spell( talent_tree::SPECIALIZATION, "Eradicate" );

  // Devouer Apex Talents
  talent.devourer.midnight1 = find_talent_spell( talent_tree::SPECIALIZATION, "Midnight", 1 );
  talent.devourer.midnight2 = find_talent_spell( talent_tree::SPECIALIZATION, "Midnight", 2 );
  talent.devourer.midnight3 = find_talent_spell( talent_tree::SPECIALIZATION, "Midnight", 3 );

  // Havoc Talents

  talent.havoc.eye_beam = find_talent_spell( talent_tree::SPECIALIZATION, "Eye Beam" );

  talent.havoc.critical_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Chaos" );
  talent.havoc.burning_hatred = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Hatred" );

  talent.havoc.dash_of_chaos         = find_talent_spell( talent_tree::SPECIALIZATION, "Dash of Chaos" );
  talent.havoc.improved_chaos_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Chaos Strike" );
  talent.havoc.first_blood           = find_talent_spell( talent_tree::SPECIALIZATION, "First Blood" );
  talent.havoc.accelerated_blade     = find_talent_spell( talent_tree::SPECIALIZATION, "Accelerated Blade" );
  talent.havoc.demon_hide            = find_talent_spell( talent_tree::SPECIALIZATION, "Demon Hide" );

  talent.havoc.desperate_instincts = find_talent_spell( talent_tree::SPECIALIZATION, "Desperate Instincts" );
  talent.havoc.netherwalk          = find_talent_spell( talent_tree::SPECIALIZATION, "Netherwalk" );
  talent.havoc.deflecting_dance    = find_talent_spell( talent_tree::SPECIALIZATION, "Deflecting Dance" );
  talent.havoc.mortal_dance        = find_talent_spell( talent_tree::SPECIALIZATION, "Mortal Dance" );

  talent.havoc.initiative         = find_talent_spell( talent_tree::SPECIALIZATION, "Initiative" );
  talent.havoc.scars_of_suffering = find_talent_spell( talent_tree::SPECIALIZATION, "Scars of Suffering" );
  talent.havoc.demonic            = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic" );
  talent.havoc.furious_throws     = find_talent_spell( talent_tree::SPECIALIZATION, "Furious Throws" );
  talent.havoc.trail_of_ruin      = find_talent_spell( talent_tree::SPECIALIZATION, "Trail of Ruin" );

  talent.havoc.tactical_retreat       = find_talent_spell( talent_tree::SPECIALIZATION, "Tactical Retreat" );
  talent.havoc.blind_fury             = find_talent_spell( talent_tree::SPECIALIZATION, "Blind Fury" );
  talent.havoc.chaotic_transformation = find_talent_spell( talent_tree::SPECIALIZATION, "Chaotic Transformation" );
  talent.havoc.dancing_with_fate      = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing with Fate" );
  talent.havoc.growing_inferno        = find_talent_spell( talent_tree::SPECIALIZATION, "Growing Inferno" );

  talent.havoc.exergy          = find_talent_spell( talent_tree::SPECIALIZATION, "Exergy" );
  talent.havoc.inertia         = find_talent_spell( talent_tree::SPECIALIZATION, "Inertia" );
  talent.havoc.isolated_prey   = find_talent_spell( talent_tree::SPECIALIZATION, "Isolated Prey" );
  talent.havoc.furious_gaze    = find_talent_spell( talent_tree::SPECIALIZATION, "Furious Gaze" );
  talent.havoc.serrated_glaive = find_talent_spell( talent_tree::SPECIALIZATION, "Serrated Glaive" );
  talent.havoc.burning_wound   = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Wound" );

  talent.havoc.unbound_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Unbound Chaos" );
  talent.havoc.chaos_theory  = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Theory" );
  talent.havoc.inner_demon   = find_talent_spell( talent_tree::SPECIALIZATION, "Inner Demon" );
  talent.havoc.the_hunt      = find_talent_spell( talent_tree::SPECIALIZATION, "The Hunt" );

  talent.havoc.relentless_onslaught = find_talent_spell( talent_tree::SPECIALIZATION, "Relentless Onslaught" );
  talent.havoc.soulscar             = find_talent_spell( talent_tree::SPECIALIZATION, "Soulscar" );
  talent.havoc.ragefire             = find_talent_spell( talent_tree::SPECIALIZATION, "Ragefire" );

  talent.havoc.know_your_enemy     = find_talent_spell( talent_tree::SPECIALIZATION, "Know Your Enemy" );
  talent.havoc.cycle_of_hatred     = find_talent_spell( talent_tree::SPECIALIZATION, "Cycle of Hatred" );
  talent.havoc.chaotic_disposition = find_talent_spell( talent_tree::SPECIALIZATION, "Chaotic Disposition" );

  talent.havoc.essence_break       = find_talent_spell( talent_tree::SPECIALIZATION, "Essence Break" );
  talent.havoc.glaive_tempest      = find_talent_spell( talent_tree::SPECIALIZATION, "Glaive Tempest" );
  talent.havoc.shattered_destiny   = find_talent_spell( talent_tree::SPECIALIZATION, "Shattered Destiny" );
  talent.havoc.collective_anguish  = find_talent_spell( talent_tree::SPECIALIZATION, "Collective Anguish" );
  talent.havoc.screaming_brutality = find_talent_spell( talent_tree::SPECIALIZATION, "Screaming Brutality" );
  talent.havoc.a_fire_inside       = find_talent_spell( talent_tree::SPECIALIZATION, "A Fire Inside" );

  talent.havoc.eternal_hunt_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Eternal Hunt", 1 );
  talent.havoc.eternal_hunt_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Eternal Hunt", 2 );
  talent.havoc.eternal_hunt_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Eternal Hunt", 3 );

  // Vengeance Talents

  talent.vengeance.fel_devastation = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Devastation" );

  talent.vengeance.spirit_bomb = find_talent_spell( talent_tree::SPECIALIZATION, "Spirit Bomb" );
  talent.vengeance.fiery_brand = find_talent_spell( talent_tree::SPECIALIZATION, "Fiery Brand" );

  talent.vengeance.perfectly_balanced_glaive =
      find_talent_spell( talent_tree::SPECIALIZATION, "Perfectly Balanced Glaive" );
  talent.vengeance.quickened_sigils = find_talent_spell( talent_tree::SPECIALIZATION, "Quickened Sigils" );
  talent.vengeance.ascending_flame  = find_talent_spell( talent_tree::SPECIALIZATION, "Ascending Flame" );

  talent.vengeance.tempered_steel   = find_talent_spell( talent_tree::SPECIALIZATION, "Tempered Steel" );
  talent.vengeance.calcified_spikes = find_talent_spell( talent_tree::SPECIALIZATION, "Calcified Spikes" );
  talent.vengeance.roaring_fire     = find_talent_spell( talent_tree::SPECIALIZATION, "Roaring Fire" );
  talent.vengeance.sigil_of_silence = find_talent_spell( talent_tree::SPECIALIZATION, "Sigil of Silence" );
  talent.vengeance.retaliation      = find_talent_spell( talent_tree::SPECIALIZATION, "Retaliation" );
  talent.vengeance.felfire_fist     = find_talent_spell( talent_tree::SPECIALIZATION, "Felfire Fist" );

  talent.vengeance.sigil_of_spite   = find_talent_spell( talent_tree::SPECIALIZATION, "Sigil of Spite" );
  talent.vengeance.agonizing_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Agonizing Flames" );
  talent.vengeance.feed_the_demon   = find_talent_spell( talent_tree::SPECIALIZATION, "Feed the Demon" );
  talent.vengeance.burning_blood    = find_talent_spell( talent_tree::SPECIALIZATION, "Burning Blood" );
  talent.vengeance.revel_in_pain    = find_talent_spell( talent_tree::SPECIALIZATION, "Revel in Pain" );

  talent.vengeance.frailty             = find_talent_spell( talent_tree::SPECIALIZATION, "Frailty" );
  talent.vengeance.feast_of_souls      = find_talent_spell( talent_tree::SPECIALIZATION, "Feast of Souls" );
  talent.vengeance.fallout             = find_talent_spell( talent_tree::SPECIALIZATION, "Fallout" );
  talent.vengeance.ruinous_bulwark     = find_talent_spell( talent_tree::SPECIALIZATION, "Ruinous Bulwark" );
  talent.vengeance.volatile_flameblood = find_talent_spell( talent_tree::SPECIALIZATION, "Volatile Flameblood" );
  talent.vengeance.soul_barrier        = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Barrier" );
  talent.vengeance.soul_sigils         = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Sigils" );
  talent.vengeance.fel_flame_fortification =
      find_talent_spell( talent_tree::SPECIALIZATION, "Fel Flame Fortification" );

  talent.vengeance.void_reaver     = find_talent_spell( talent_tree::SPECIALIZATION, "Void Reaver" );
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
  talent.vengeance.vengeful_beast = find_talent_spell( talent_tree::SPECIALIZATION, "Vengeful Beast" );
  talent.vengeance.charred_flesh  = find_talent_spell( talent_tree::SPECIALIZATION, "Charred Flesh" );

  talent.vengeance.soulcrush      = find_talent_spell( talent_tree::SPECIALIZATION, "Soulcrush" );
  talent.vengeance.soul_carver    = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Carver" );
  talent.vengeance.last_resort    = find_talent_spell( talent_tree::SPECIALIZATION, "Last Resort" );
  talent.vengeance.darkglare_boon = find_talent_spell( talent_tree::SPECIALIZATION, "Darkglare Boon" );
  talent.vengeance.down_in_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Down in Flames" );

  // Vengeance Apex Talents
  talent.vengeance.untethered_rage_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Untethered Rage", 1 );
  talent.vengeance.untethered_rage_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Untethered Rage", 2 );
  talent.vengeance.untethered_rage_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Untethered Rage", 3 );

  // Hero Talents ===========================================================

  // Aldrachi Reaver talents
  talent.aldrachi_reaver.art_of_the_glaive = find_talent_spell( talent_tree::HERO, "Art of the Glaive" );

  talent.aldrachi_reaver.fury_of_the_aldrachi = find_talent_spell( talent_tree::HERO, "Fury of the Aldrachi" );
  talent.aldrachi_reaver.evasive_action       = find_talent_spell( talent_tree::HERO, "Evasive Action" );
  talent.aldrachi_reaver.unhindered_assault   = find_talent_spell( talent_tree::HERO, "Unhindered Assault" );
  talent.aldrachi_reaver.reavers_mark         = find_talent_spell( talent_tree::HERO, "Reaver's Mark" );
  talent.aldrachi_reaver.broken_spirit        = find_talent_spell( talent_tree::HERO, "Broken Spirit" );

  talent.aldrachi_reaver.aldrachi_tactics     = find_talent_spell( talent_tree::HERO, "Aldrachi Tactics" );
  talent.aldrachi_reaver.army_unto_oneself    = find_talent_spell( talent_tree::HERO, "Army Unto Oneself" );
  talent.aldrachi_reaver.incorruptible_spirit = find_talent_spell( talent_tree::HERO, "Incorruptible Spirit" );
  talent.aldrachi_reaver.wounded_quarry       = find_talent_spell( talent_tree::HERO, "Wounded Quarry" );
  talent.aldrachi_reaver.keen_edge            = find_talent_spell( talent_tree::HERO, "Keen Edge" );

  talent.aldrachi_reaver.incisive_blade    = find_talent_spell( talent_tree::HERO, "Incisive Blade" );
  talent.aldrachi_reaver.keen_engagement   = find_talent_spell( talent_tree::HERO, "Keen Engagement" );
  talent.aldrachi_reaver.preemptive_strike = find_talent_spell( talent_tree::HERO, "Preemptive Strike" );
  talent.aldrachi_reaver.bladecraft        = find_talent_spell( talent_tree::HERO, "Bladecraft" );
  talent.aldrachi_reaver.warblades_hunger  = find_talent_spell( talent_tree::HERO, "Warblade's Hunger" );

  talent.aldrachi_reaver.thrill_of_the_fight = find_talent_spell( talent_tree::HERO, "Thrill of the Fight" );

  // Annihilator talents
  talent.annihilator.voidfall = find_talent_spell( talent_tree::HERO, "Voidfall" );

  talent.annihilator.swift_erasure = find_talent_spell( talent_tree::HERO, "Swift Erasure" );
  talent.annihilator.meteoric_rise = find_talent_spell( talent_tree::HERO, "Meteoric Rise" );
  talent.annihilator.catastrophe   = find_talent_spell( talent_tree::HERO, "Catastrophe" );
  talent.annihilator.phase_shift   = find_talent_spell( talent_tree::HERO, "Phase Shift" );

  talent.annihilator.path_to_oblivion   = find_talent_spell( talent_tree::HERO, "Path to Oblivion" );
  talent.annihilator.state_of_matter    = find_talent_spell( talent_tree::HERO, "State of Matter" );
  talent.annihilator.mass_acceleration  = find_talent_spell( talent_tree::HERO, "Mass Acceleration" );
  talent.annihilator.doomsayer          = find_talent_spell( talent_tree::HERO, "Doomsayer" );
  talent.annihilator.harness_the_cosmos = find_talent_spell( talent_tree::HERO, "Harness the Cosmos" );
  talent.annihilator.celestial_echoes   = find_talent_spell( talent_tree::HERO, "Celestial Echoes" );

  talent.annihilator.final_hour         = find_talent_spell( talent_tree::HERO, "Final Hour" );
  talent.annihilator.meteoric_fall      = find_talent_spell( talent_tree::HERO, "Meteoric Fall" );
  talent.annihilator.dark_matter        = find_talent_spell( talent_tree::HERO, "Dark Matter" );
  talent.annihilator.otherworldly_focus = find_talent_spell( talent_tree::HERO, "Otherworldly Focus" );

  talent.annihilator.world_killer = find_talent_spell( talent_tree::HERO, "World Killer" );

  // Scarred talents
  if ( specialization() == DEMON_HUNTER_HAVOC )
    talent.scarred.demonsurge = find_talent_spell( talent_tree::HERO, "Demonsurge" );
  else
    talent.scarred.demonsurge = find_talent_spell( talent_tree::HERO, "Voidsurge" );

  auto HT_FS = [ this ]( std::string_view n ) {
    return find_talent_spell( specialization() == DEMON_HUNTER_HAVOC ? HERO_FELSCARRED : HERO_VOID_SCARRED, n );
  };

  talent.scarred.wave_of_debilitation = HT_FS( "Wave of Debilitation" );

  talent.scarred.pursuit_of_angriness  = HT_FS( "Pursuit of Angriness" );
  talent.scarred.focused_hatred        = HT_FS( "Focused Hatred" );
  talent.scarred.set_fire_to_the_pain  = HT_FS( "Set Fire to the Pain" );
  talent.scarred.improved_soul_rending = HT_FS( "Improved Soul Rending" );

  talent.scarred.burning_blades         = HT_FS( "Burning Blades" );
  talent.scarred.violent_transformation = HT_FS( "Violent Transformation" );
  talent.scarred.enduring_torment       = HT_FS( "Enduring Torment" );

  talent.scarred.untethered_fury      = HT_FS( "Untethered Fury" );
  talent.scarred.student_of_suffering = HT_FS( "Student of Suffering" );
  talent.scarred.flamebound           = HT_FS( "Flamebound" );
  talent.scarred.monster_rising       = HT_FS( "Monster Rising" );

  talent.scarred.blind_focus       = HT_FS( "Blind Focus" );
  talent.scarred.undying_embers    = HT_FS( "Undying Embers" );
  talent.scarred.volatile_instinct = HT_FS( "Volatile Instinct" );

  talent.scarred.demonic_intensity = HT_FS( "Demonic Intensity" );

  // Class Background Spells
  spell.felblade_damage        = talent_spell_lookup( talent.demon_hunter.felblade, 213243 );
  spell.infernal_armor_damage  = talent_spell_lookup( talent.demon_hunter.infernal_armor, 320334 );
  spell.immolation_aura_damage = conditional_spell_lookup( spell.immolation_aura_2->ok(), 258921 );

  // Spec Background Spells
  spec.feast_of_souls_buff      = talent_spell_lookup( talent.devourer.feast_of_souls, 1232310 );
  spec.devourers_bite_debuff    = talent_spell_lookup( talent.devourer.devourers_bite, 1241532 );
  spec.void_metamorphosis       = talent_spell_lookup( talent.devourer.void_metamorphosis, 1217607 );
  spec.void_metamorphosis_stack = talent_spell_lookup( talent.devourer.void_metamorphosis, 1225789 );
  spec.eradicate                = talent_spell_lookup( talent.devourer.eradicate, 1225826 );
  spec.eradicate_damage         = talent_spell_lookup( talent.devourer.eradicate, 1225827 );
  spec.eradicate_damage_meta    = talent_spell_lookup( talent.devourer.eradicate, 1279200 );
  spec.eradicate_buff           = talent_spell_lookup( talent.devourer.eradicate, 1239524 );
  spec.void_ray_tick            = talent_spell_lookup( talent.devourer.void_ray, 1213649 );
  spec.void_ray_tick_meta       = talent_spell_lookup( talent.devourer.void_ray, 1214595 );
  spec.moment_of_craving_buff   = talent_spell_lookup( talent.devourer.moment_of_craving, 1238495 );
  spec.void_buildup             = talent_spell_lookup( talent.devourer.void_metamorphosis, 473671 );
  spec.voidglare_boon_energize  = talent_spell_lookup( talent.devourer.voidglare_boon, 1241922 );
  spec.collapsing_star_damage =
      conditional_spell_lookup( talent.devourer.collapsing_star->ok() || talent.devourer.midnight3->ok(), 1221162 );
  spec.collapsing_star_spell =
      conditional_spell_lookup( talent.devourer.collapsing_star->ok() || talent.devourer.midnight3->ok(), 1221150 );
  spec.collapsing_star_buff =
      conditional_spell_lookup( talent.devourer.collapsing_star->ok() || talent.devourer.midnight3->ok(), 1221171 );
  spec.collapsing_star_stacking_buff =
      conditional_spell_lookup( talent.devourer.collapsing_star->ok() || talent.devourer.midnight3->ok(), 1227702 );
  spec.emptiness_buff            = talent_spell_lookup( talent.devourer.emptiness, 1242504 );
  spec.impending_apocalypse_buff = talent_spell_lookup( talent.devourer.impending_apocalypse, 1227338 );
  spec.hungering_slash           = talent_spell_lookup( talent.devourer.hungering_slash, 1239123 );
  spec.hungering_slash_buff      = talent_spell_lookup( talent.devourer.hungering_slash, 1239525 );
  spec.hungering_slash_damage    = talent_spell_lookup( talent.devourer.hungering_slash, 1239127 );
  spec.hungering_slash_energize  = talent_spell_lookup( talent.devourer.hungering_slash, 1239507 );
  spec.voidstep                  = talent_spell_lookup( talent.devourer.hungering_slash, 1223157 );
  spec.rolling_torment_buff      = talent_spell_lookup( talent.devourer.rolling_torment, 1244235 );
  spec.rolling_torment_energize  = talent_spell_lookup( talent.devourer.rolling_torment, 1277769 );

  mastery.a_fire_inside = talent.havoc.a_fire_inside->effectN( 6 ).trigger();

  spec.burning_wound_debuff                      = talent.havoc.burning_wound->effectN( 1 ).trigger();
  spec.chaos_theory_buff                         = talent_spell_lookup( talent.havoc.chaos_theory, 390195 );
  spec.demon_blades                              = find_spell( 203555, DEMON_HUNTER_HAVOC );
  spec.demon_blades_damage                       = spec.demon_blades->effectN( 1 ).trigger();
  spec.essence_break_debuff                      = talent_spell_lookup( talent.havoc.essence_break, 320338 );
  cooldown.essence_break_proc_icd->base_duration = spec.essence_break_debuff->internal_cooldown();
  spec.eye_beam_damage                           = talent_spell_lookup( talent.havoc.eye_beam, 198030 );
  spec.furious_gaze_buff                         = talent_spell_lookup( talent.havoc.furious_gaze, 343312 );
  spec.first_blood_blade_dance_damage            = talent_spell_lookup( talent.havoc.first_blood, 391374 );
  spec.first_blood_blade_dance_2_damage          = talent_spell_lookup( talent.havoc.first_blood, 391378 );
  spec.first_blood_death_sweep_damage            = talent_spell_lookup( talent.havoc.first_blood, 393055 );
  spec.first_blood_death_sweep_2_damage          = talent_spell_lookup( talent.havoc.first_blood, 393054 );
  spec.glaive_tempest                            = talent_spell_lookup( talent.havoc.glaive_tempest, 342817 );
  spec.glaive_tempest_damage                     = talent_spell_lookup( talent.havoc.glaive_tempest, 342857 );
  spec.initiative_buff                           = talent_spell_lookup( talent.havoc.initiative, 391215 );
  spec.inner_demon_buff                          = talent_spell_lookup( talent.havoc.inner_demon, 390145 );
  spec.inner_demon_damage                        = talent_spell_lookup( talent.havoc.inner_demon, 390137 );
  spec.exergy_buff                               = talent_spell_lookup( talent.havoc.exergy, 208628 );
  spec.inertia_trigger_buff                      = talent_spell_lookup( talent.havoc.inertia, 1215159 );
  spec.inertia_buff                              = talent_spell_lookup( talent.havoc.inertia, 427641 );
  spec.ragefire_damage                           = talent_spell_lookup( talent.havoc.ragefire, 390197 );
  spec.soulscar_debuff                           = talent_spell_lookup( talent.havoc.soulscar, 390181 );
  spec.tactical_retreat_buff                     = talent_spell_lookup( talent.havoc.tactical_retreat, 389890 );
  spec.unbound_chaos_buff                        = talent_spell_lookup( talent.havoc.unbound_chaos, 347462 );
  spec.cycle_of_hatred_buff                      = talent_spell_lookup( talent.havoc.cycle_of_hatred, 1214887 );
  spec.furious_throws_damage                     = talent_spell_lookup( talent.havoc.furious_throws, 393035 );
  spec.collective_anguish                        = talent_spell_lookup( talent.havoc.collective_anguish, 393831 );
  spec.collective_anguish_damage                 = spec.collective_anguish->effectN( 1 ).trigger();
  spec.essence_break_proc_damage                 = talent_spell_lookup( talent.havoc.essence_break, 1245759 );
  spec.empowered_eye_beam_buff                   = talent_spell_lookup( talent.havoc.eternal_hunt_1, 1271144 );
  spec.empowered_eye_beam_damage                 = talent_spell_lookup( talent.havoc.eternal_hunt_1, 1287949 );
  spec.eternal_hunt_buff                         = talent_spell_lookup( talent.havoc.eternal_hunt_3, 1271092 );

  spec.demon_spikes_buff               = find_spell( 203819, DEMON_HUNTER_VENGEANCE );
  spec.sigil_of_flame                  = find_spell( 204596, DEMON_HUNTER_VENGEANCE );
  spec.sigil_of_flame_damage           = find_spell( 204598, DEMON_HUNTER_VENGEANCE );
  spec.sigil_of_flame_fury             = find_spell( 389787, DEMON_HUNTER_VENGEANCE );
  spec.fiery_brand_debuff              = talent_spell_lookup( talent.vengeance.fiery_brand, 207771 );
  spec.frailty_debuff                  = talent_spell_lookup( talent.vengeance.frailty, 247456 );
  spec.painbringer_buff                = talent_spell_lookup( talent.vengeance.painbringer, 212988 );
  spec.retaliation_damage              = talent_spell_lookup( talent.vengeance.retaliation, 391159 );
  spec.sigil_of_silence_debuff         = talent_spell_lookup( talent.vengeance.sigil_of_silence, 204490 );
  spec.sigil_of_chains_debuff          = talent_spell_lookup( talent.vengeance.sigil_of_chains, 204843 );
  spec.burning_alive_controller        = talent_spell_lookup( talent.vengeance.burning_alive, 207760 );
  spec.infernal_strike_impact          = find_spell( 189112, DEMON_HUNTER_VENGEANCE );
  spec.spirit_bomb_damage              = talent_spell_lookup( talent.vengeance.spirit_bomb, 247455 );
  spec.frailty_heal                    = talent_spell_lookup( talent.vengeance.frailty, 227255 );
  spec.feast_of_souls_heal             = talent_spell_lookup( talent.vengeance.feast_of_souls, 207693 );
  spec.fel_devastation_2               = find_rank_spell( "Fel Devastation", "Rank 2" );
  spec.fel_devastation_heal            = talent_spell_lookup( talent.vengeance.fel_devastation, 212106 );
  spec.felfire_fist_in_combat_buff     = talent_spell_lookup( talent.vengeance.felfire_fist, 1265759 );
  spec.felfire_fist_out_of_combat_buff = talent_spell_lookup( talent.vengeance.felfire_fist, 1265751 );
  spec.untethered_rage_buff            = talent_spell_lookup( talent.vengeance.untethered_rage_1, 1270476 );
  spec.seething_anger_buff             = talent_spell_lookup( talent.vengeance.untethered_rage_3, 1270547 );

  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      spec.the_hunt        = talent.devourer.the_hunt;
      spec.the_hunt_impact = spec.the_hunt->effectN( 1 ).trigger();
      spec.the_hunt_dot    = spec.the_hunt_impact->effectN( 4 ).trigger();
      break;
    case DEMON_HUNTER_HAVOC:
      spec.the_hunt        = talent.havoc.the_hunt;
      spec.the_hunt_impact = spec.the_hunt->effectN( 1 ).trigger();
      spec.the_hunt_dot    = spec.the_hunt_impact->effectN( 4 ).trigger();
      break;
    case DEMON_HUNTER_VENGEANCE:
      spec.the_hunt        = spell_data_t::not_found();
      spec.the_hunt_impact = spell_data_t::not_found();
      spec.the_hunt_dot    = spell_data_t::not_found();
      break;
    default:
      break;
  }

  // Hero spec background spells
  hero_spec.reavers_mark                   = talent_spell_lookup( talent.aldrachi_reaver.reavers_mark, 442624 );
  hero_spec.glaive_flurry                  = talent_spell_lookup( talent.aldrachi_reaver.art_of_the_glaive, 442435 );
  hero_spec.rending_strike                 = talent_spell_lookup( talent.aldrachi_reaver.art_of_the_glaive, 442442 );
  hero_spec.art_of_the_glaive_buff         = talent_spell_lookup( talent.aldrachi_reaver.art_of_the_glaive, 444661 );
  hero_spec.art_of_the_glaive_damage       = talent_spell_lookup( talent.aldrachi_reaver.fury_of_the_aldrachi, 444810 );
  hero_spec.warblades_hunger_buff          = talent_spell_lookup( talent.aldrachi_reaver.warblades_hunger, 442503 );
  hero_spec.warblades_hunger_damage        = talent_spell_lookup( talent.aldrachi_reaver.warblades_hunger, 442507 );
  hero_spec.wounded_quarry_damage          = talent_spell_lookup( talent.aldrachi_reaver.wounded_quarry, 442808 );
  hero_spec.thrill_of_the_fight_haste_buff = talent_spell_lookup( talent.aldrachi_reaver.thrill_of_the_fight, 442688 );
  hero_spec.thrill_of_the_fight_damage_buff = talent_spell_lookup( talent.aldrachi_reaver.thrill_of_the_fight, 442695 );
  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
      hero_spec.reavers_glaive           = talent_spell_lookup( talent.aldrachi_reaver.art_of_the_glaive, 442294 );
      hero_spec.reavers_glaive_buff      = talent_spell_lookup( talent.aldrachi_reaver.art_of_the_glaive, 444686 );
      hero_spec.wounded_quarry_proc_rate = options.wounded_quarry_chance_havoc;
      break;
    case DEMON_HUNTER_VENGEANCE:
      hero_spec.reavers_glaive           = talent_spell_lookup( talent.aldrachi_reaver.art_of_the_glaive, 1283344 );
      hero_spec.reavers_glaive_buff      = talent_spell_lookup( talent.aldrachi_reaver.art_of_the_glaive, 444764 );
      hero_spec.wounded_quarry_proc_rate = options.wounded_quarry_chance_vengeance;
      break;
    default:
      hero_spec.reavers_glaive_buff      = spell_data_t::not_found();
      hero_spec.wounded_quarry_proc_rate = 0;
      break;
  }

  hero_spec.voidfall_building_buff       = talent_spell_lookup( talent.annihilator.voidfall, 1256301 );
  hero_spec.voidfall_spending_buff       = talent_spell_lookup( talent.annihilator.voidfall, 1256302 );
  hero_spec.voidfall_final_hour_buff     = talent_spell_lookup( talent.annihilator.final_hour, 1256322 );
  hero_spec.dark_matter_buff             = talent_spell_lookup( talent.annihilator.dark_matter, 1256308 );
  hero_spec.doomsayer_in_combat_buff     = talent_spell_lookup( talent.annihilator.doomsayer, 1265768 );
  hero_spec.doomsayer_out_of_combat_buff = talent_spell_lookup( talent.annihilator.doomsayer, 1264087 );
  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      hero_spec.voidfall_meteor      = talent_spell_lookup( talent.annihilator.voidfall, 1256304 );
      hero_spec.catastrophe_dot      = talent_spell_lookup( talent.annihilator.catastrophe, 1256676 );
      hero_spec.meteor_shower_driver = talent_spell_lookup( talent.annihilator.dark_matter, 1264126 );
      hero_spec.meteor_shower_damage = hero_spec.meteor_shower_driver->effectN( 1 ).trigger();
      hero_spec.world_killer         = talent_spell_lookup( talent.annihilator.world_killer, 1256618 );
      break;
    case DEMON_HUNTER_VENGEANCE:
      hero_spec.voidfall_meteor      = talent_spell_lookup( talent.annihilator.voidfall, 1256303 );
      hero_spec.catastrophe_dot      = talent_spell_lookup( talent.annihilator.catastrophe, 1256667 );
      hero_spec.meteor_shower_driver = talent_spell_lookup( talent.annihilator.dark_matter, 1264128 );
      hero_spec.meteor_shower_damage = hero_spec.meteor_shower_driver->effectN( 1 ).trigger();
      hero_spec.world_killer         = talent_spell_lookup( talent.annihilator.world_killer, 1256616 );
      break;
    default:
      hero_spec.voidfall_meteor      = spell_data_t::not_found();
      hero_spec.catastrophe_dot      = spell_data_t::not_found();
      hero_spec.meteor_shower_driver = spell_data_t::not_found();
      hero_spec.meteor_shower_damage = spell_data_t::not_found();
      hero_spec.world_killer         = spell_data_t::not_found();
      break;
  }

  hero_spec.student_of_suffering_buff = talent_spell_lookup( talent.scarred.student_of_suffering, 453239 );
  hero_spec.monster_rising_buff       = talent_spell_lookup( talent.scarred.monster_rising, 452550 );
  hero_spec.enduring_torment_buff     = talent_spell_lookup( talent.scarred.enduring_torment, 453314 );
  // 2026-03-04 -- This only buffs Devourer spells and Havoc doesn't get a baseline damage buff from Demonsurge.
  hero_spec.demonsurge_demonsurge_buff =
      spec_talent_spell_lookup( DEMON_HUNTER_DEVOURER, talent.scarred.demonsurge, 452435 );
  hero_spec.demonsurge_placeholder_buff = talent_spell_lookup( talent.scarred.demonsurge, 452443 );
  hero_spec.demonsurge_trigger = spec_talent_spell_lookup( DEMON_HUNTER_HAVOC, talent.scarred.demonsurge, 453323 );
  hero_spec.demonsurge_damage  = hero_spec.demonsurge_trigger->effectN( 1 ).trigger();
  hero_spec.demonsurge_stacking_buff = hero_spec.demonsurge_damage;
  hero_spec.voidsurge_trigger = spec_talent_spell_lookup( DEMON_HUNTER_DEVOURER, talent.scarred.demonsurge, 1246161 );
  hero_spec.voidsurge_damage  = hero_spec.voidsurge_trigger->effectN( 1 ).trigger();
  hero_spec.voidsurge_stacking_buff = hero_spec.voidsurge_damage;
  hero_spec.sigil_of_doom = spec_talent_spell_lookup( DEMON_HUNTER_HAVOC, talent.scarred.demonic_intensity, 452490 );
  hero_spec.sigil_of_doom_damage =
      spec_talent_spell_lookup( DEMON_HUNTER_HAVOC, talent.scarred.demonic_intensity, 462030 );
  hero_spec.abyssal_gaze = spec_talent_spell_lookup( DEMON_HUNTER_HAVOC, talent.scarred.demonic_intensity, 452497 );
  hero_spec.predators_wake =
      conditional_spell_lookup( specialization() == DEMON_HUNTER_DEVOURER && talent.devourer.the_hunt->ok() &&
                                    talent.scarred.demonic_intensity->ok(),
                                1259431 );
  hero_spec.pierce_the_veil = spec_talent_spell_lookup( DEMON_HUNTER_DEVOURER, talent.scarred.demonsurge, 1245483 );
  hero_spec.reapers_toll    = spec_talent_spell_lookup( DEMON_HUNTER_DEVOURER, talent.scarred.demonsurge, 1245470 );
  hero_spec.volatile_instinct =
      spec_talent_spell_lookup( DEMON_HUNTER_DEVOURER, talent.scarred.volatile_instinct, 1272462 );
  hero_spec.demonsurge_meta_trigger =
      spec_talent_spell_lookup( DEMON_HUNTER_HAVOC, talent.scarred.volatile_instinct, 1238696 );

  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      hero_spec.burning_blades_debuff             = talent_spell_lookup( talent.scarred.burning_blades, 1245654 );
      hero_spec.demonsurge_demonic_intensity_buff = talent_spell_lookup( talent.scarred.demonic_intensity, 1245496 );
      break;
    case DEMON_HUNTER_HAVOC:
      hero_spec.burning_blades_debuff             = talent_spell_lookup( talent.scarred.burning_blades, 453177 );
      hero_spec.demonsurge_demonic_intensity_buff = talent_spell_lookup( talent.scarred.demonic_intensity, 452489 );
      break;
    default:
      hero_spec.burning_blades_debuff             = spell_data_t::not_found();
      hero_spec.demonsurge_demonic_intensity_buff = spell_data_t::not_found();
      break;
  }

  spec.sigil_of_spite_damage = talent_spell_lookup( talent.vengeance.sigil_of_spite, 389860 );
  spec.sigil_of_silence      = talent.vengeance.sigil_of_silence;
  spec.sigil_of_chains       = talent.vengeance.sigil_of_chains;

  // Set Bonus Items ========================================================

  set_bonuses.mid1_vengeance_4pc = sets->set( DEMON_HUNTER_VENGEANCE, MID1, B4 );

  // Set Bonus Auxilliary ===================================================
  set_bonuses.stars_fury            = conditional_spell_lookup( sets->has_set_bonus( DEMON_HUNTER_DEVOURER, MID1, B4 ),
                                                                1271663 );  // Stars' Fury (set bonus)
  set_bonuses.explosion_of_the_soul = conditional_spell_lookup( set_bonuses.mid1_vengeance_4pc->ok(), 1276488 );

  // Wounded Quarry (442808) is affected by Demon Hide.
  register_passive_affect_list( talent.havoc.demon_hide,
                                affect_list_t( 1, 3 ).add_spell( hero_spec.wounded_quarry_damage->id() ) );

  // 2026-04-29 -- Celestial Echoes is additive, not multiplicative.
  register_passive_affect_list( talent.annihilator.celestial_echoes,
                                affect_list_t( 2 ).remove_spell( spec.rolling_torment_energize->id() ) );

  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      deregister_passive_effect( talent.scarred.untethered_fury->effectN( 1 ) );
      break;
    case DEMON_HUNTER_HAVOC:
    case DEMON_HUNTER_VENGEANCE:
      deregister_passive_effect( talent.scarred.untethered_fury->effectN( 2 ) );
      break;
    default:
      break;
  }

  // Reap variants and The Hunt are not hasted
  register_passive_affect_list( spec.devourer_demon_hunter, affect_list_t( 6 )
                                                                .remove_spell( spec.reap->id() )
                                                                .remove_spell( spec.cull->id() )
                                                                .remove_spell( spec.eradicate->id() )
                                                                .remove_spell( spec.the_hunt->id() )
                                                                .remove_spell( hero_spec.pierce_the_veil->id() ) );

  // Blind Focus is done via parse_effects
  deregister_passive_spell( talent.scarred.blind_focus );

  // Critical Chaos eff#2 (dummy script) overwrites the value of eff#1 (add flat: proc chance)
  deregister_passive_effect( talent.havoc.critical_chaos->effectN( 1 ) );

  // TODO: Check if this still behaves as described in `composite_player_critical_damage_multiplier`
  deregister_passive_spell( talent.havoc.know_your_enemy );
  deregister_passive_spell( talent.havoc.tactical_retreat );

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();
  parse_raid_buffs();

  // Spell Initialization ===================================================

  using namespace actions::attacks;
  using namespace actions::spells;
  using namespace actions::heals;

  active.consume_soul_lesser =
      new consume_soul_t( this, "consume_soul_lesser", spec.consume_soul_lesser_heal, soul_fragment::LESSER );
  active.consume_soul_greater =
      new consume_soul_t( this, "consume_soul_greater", spec.consume_soul_greater_heal, soul_fragment::GREATER );
  active.consume_soul_greater_demon = new consume_soul_t(
      this, "consume_soul_greater_demon", spec.consume_soul_greater_heal, soul_fragment::GREATER_DEMON );
  active.consume_soul_empowered_demon = new consume_soul_t(
      this, "consume_soul_empowered_demon", spec.consume_soul_greater_heal, soul_fragment::EMPOWERED_DEMON );

  if ( talent.devourer.void_metamorphosis->ok() )
  {
    active.void_buildup = get_background_action<void_buildup_t>( "void_buildup" );
  }

  // this can't be conditional because APL uses `active_dot.burning_wound` which looks for an
  // action
  active.burning_wound = get_background_action<burning_wound_t>( "burning_wound" );

  if ( talent.havoc.collective_anguish->ok() )
  {
    active.collective_anguish = get_background_action<collective_anguish_t>( "collective_anguish" );
  }

  if ( spec.demon_blades_damage->ok() )
  {
    active.demon_blades = new demon_blades_t( this );
  }
  if ( talent.havoc.relentless_onslaught->ok() )
  {
    auto cs            = get_background_action<chaos_strike_t>( "chaos_strike_onslaught" );
    cs->from_onslaught = true;

    auto anni            = get_background_action<annihilation_t>( "annihilation_onslaught" );
    anni->from_onslaught = true;

    active.relentless_onslaught = get_background_action<relentless_onslaught_t>( "relentless_onslaught", cs, anni );
  }
  if ( talent.havoc.inner_demon->ok() )
  {
    active.inner_demon = get_background_action<inner_demon_t>( "inner_demon" );
  }
  if ( talent.havoc.soulscar->ok() )
  {
    active.soulscar = get_background_action<soulscar_t>( "soulscar" );
  }
  if ( talent.havoc.screaming_brutality->ok() )
  {
    active.screaming_brutality_blade_dance_throw_glaive = get_background_action<throw_glaive_t>(
        "throw_glaive_sb_bd_throw", "", throw_glaive_t::glaive_source::SCREAMING_BRUTALITY_BLADE_DANCE_THROW );
    active.screaming_brutality_death_sweep_throw_glaive = get_background_action<throw_glaive_t>(
        "throw_glaive_sb_ds_throw", "", throw_glaive_t::glaive_source::SCREAMING_BRUTALITY_DEATH_SWEEP_THROW );
    active.screaming_brutality_slash_proc_throw_glaive = get_background_action<throw_glaive_t>(
        "throw_glaive_sb_slash_proc_throw", "", throw_glaive_t::glaive_source::SCREAMING_BRUTALITY_SLASH_PROC_THROW );
  }
  if ( talent.havoc.essence_break->ok() )
  {
    active.essence_break_proc = get_background_action<essence_break_proc_t>( "essence_break_proc_damage" );
  }
  if ( talent.havoc.glaive_tempest->ok() )
  {
    active.glaive_tempest = get_background_action<glaive_tempest_t>( "glaive_tempest" );
  }

  if ( talent.vengeance.retaliation->ok() )
  {
    active.retaliation = get_background_action<retaliation_t>( "retaliation" );
  }
  if ( talent.vengeance.frailty->ok() )
  {
    active.frailty_heal = get_background_action<frailty_heal_t>( "frailty" );
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

  if ( talent.annihilator.world_killer->ok() )
  {
    active.world_killer = get_background_action<world_killer_t>( "world_killer" );
  }
  if ( talent.annihilator.voidfall->ok() )
  {
    active.voidfall_meteor = get_background_action<voidfall_meteor_t>( "voidfall_meteor" );
  }
  if ( talent.annihilator.catastrophe->ok() )
  {
    active.catastrophe = get_background_action<catastrophe_t>( "catastrophe" );
  }
  if ( talent.annihilator.dark_matter->ok() )
  {
    active.meteor_shower = get_background_action<meteor_shower_t>( "meteor_shower" );
  }

  if ( talent.scarred.burning_blades->ok() )
  {
    active.burning_blades = get_background_action<burning_blades_t>( "burning_blades" );
  }
  if ( talent.scarred.demonsurge->ok() )
  {
    switch ( specialization() )
    {
      case DEMON_HUNTER_DEVOURER:
        active.voidsurge = get_background_action<voidsurge_t>( "voidsurge" );
        break;
      case DEMON_HUNTER_HAVOC:
        active.demonsurge = get_background_action<demonsurge_t>( "demonsurge" );
        break;
      default:
        break;
    }
  }

  if ( specialization() == DEMON_HUNTER_DEVOURER )
  {
    devourer_fury_state.init();
  }
}

void demon_hunter_t::init_blizzard_action_list()
{
  action_priority_list_t* default_ = get_action_priority_list( "default" );
  default_->add_action( "auto_attack" );  // Add before generating the other actions so its always the highest priority
  player_t::init_blizzard_action_list();

  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      break;
    case DEMON_HUNTER_HAVOC:
      cooldowns->add_action( "metamorphosis" );
      break;
    case DEMON_HUNTER_VENGEANCE:
      cooldowns->add_action( "metamorphosis" );
      break;
    default:
      break;
  }
}

std::vector<std::string> demon_hunter_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  if ( spell_id == 1217605 )
  {
    return { "metamorphosis" };
  }

  if ( spell_id == 203782 || spell_id == 203783 )
  {
    return { "fracture" };
  }

  return player_t::action_names_from_spell_id( spell_id );
}

std::string demon_hunter_t::aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const
{
  std::string aura_expr = player_t::aura_expr_from_spell_id( spell_id, on_self );

  if ( aura_expr == "buff.void_metamorphosis" )
  {
    return "buff.metamorphosis";
  }
  if ( aura_expr == "buff.soul_immolation" )
  {
    return "dot.soul_immolation";
  }

  return aura_expr;
}

void demon_hunter_t::init_items()
{
  player_t::init_items();
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
#ifdef NDEBUG
  if ( style == FIGHT_STYLE_DUNGEON_SLICE && !options.enable_dungeon_slice )
  {
    throw sc_invalid_fight_style(
        "Dungeon Slice is disabled for Demon Hunter. To force enable, use enable_dungeon_slice=1 option." );
    return false;
  }
#endif

  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      return style == FIGHT_STYLE_PATCHWERK || style == FIGHT_STYLE_DUNGEON_ROUTE ||
             style == FIGHT_STYLE_CASTING_PATCHWERK || style == FIGHT_STYLE_HECTIC_ADD_CLEAVE;
    case DEMON_HUNTER_HAVOC:
      return style == FIGHT_STYLE_PATCHWERK || style == FIGHT_STYLE_DUNGEON_ROUTE ||
             style == FIGHT_STYLE_CASTING_PATCHWERK || style == FIGHT_STYLE_HECTIC_ADD_CLEAVE;
    case DEMON_HUNTER_VENGEANCE:
      return style == FIGHT_STYLE_PATCHWERK || style == FIGHT_STYLE_DUNGEON_ROUTE ||
             style == FIGHT_STYLE_CASTING_PATCHWERK || style == FIGHT_STYLE_HECTIC_ADD_CLEAVE;
    default:
      return false;
  }
}

// demon_hunter_t::validate_actor =====================================

bool demon_hunter_t::validate_actor()
{
  return player_t::validate_actor();
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
    case DEMON_HUNTER_DEVOURER:
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
    case DEMON_HUNTER_DEVOURER:
      return ROLE_SPELL;
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
    case DEMON_HUNTER_DEVOURER:
      return demon_hunter_apl::flask_devourer( this );
    case DEMON_HUNTER_HAVOC:
      return demon_hunter_apl::flask_havoc( this );
    case DEMON_HUNTER_VENGEANCE:
      return demon_hunter_apl::flask_vengeance( this );
    default:
      return "disabled";
  }
}

// demon_hunter_t::default_potion ==================================================

std::string demon_hunter_t::default_potion() const
{
  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      return demon_hunter_apl::potion_devourer( this );
    case DEMON_HUNTER_HAVOC:
      return demon_hunter_apl::potion_havoc( this );
    case DEMON_HUNTER_VENGEANCE:
      return demon_hunter_apl::potion_vengeance( this );
    default:
      return "disabled";
  }
}

// demon_hunter_t::default_food ====================================================

std::string demon_hunter_t::default_food() const
{
  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      return demon_hunter_apl::food_devourer( this );
    case DEMON_HUNTER_HAVOC:
      return demon_hunter_apl::food_havoc( this );
    case DEMON_HUNTER_VENGEANCE:
      return demon_hunter_apl::food_vengeance( this );
    default:
      return "disabled";
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
    case DEMON_HUNTER_DEVOURER:
      return demon_hunter_apl::temporary_enchant_devourer( this );
    case DEMON_HUNTER_HAVOC:
      return demon_hunter_apl::temporary_enchant_havoc( this );
    case DEMON_HUNTER_VENGEANCE:
      return demon_hunter_apl::temporary_enchant_vengeance( this );
    default:
      return "disabled";
  }
}

// ==========================================================================
// custom demon_hunter_t init functions
// ==========================================================================

// demon_hunter_t::create_cooldowns =========================================

void demon_hunter_t::create_cooldowns()
{
  // General
  cooldown.sigil_of_spite    = get_cooldown( "sigil_of_spite" );
  cooldown.felblade          = get_cooldown( "felblade" );
  cooldown.immolation_aura   = get_cooldown( "immolation_aura" );
  cooldown.the_hunt          = get_cooldown( "the_hunt" );
  cooldown.sigil_of_flame    = get_cooldown( "sigil_of_flame" );
  cooldown.sigil_of_misery   = get_cooldown( "sigil_of_misery" );
  cooldown.throw_glaive      = get_cooldown( "throw_glaive" );
  cooldown.vengeful_retreat  = get_cooldown( "vengeful_retreat" );
  cooldown.metamorphosis     = get_cooldown( "metamorphosis" );
  cooldown.soul_splitter_icd = get_cooldown( "soul_splitter_icd" );

  // Devourer
  cooldown.consume         = get_cooldown( "consume" );
  cooldown.reap            = get_cooldown( "reap" );
  cooldown.voidblade       = get_cooldown( "voidblade" );
  cooldown.void_ray        = get_cooldown( "void_ray" );
  cooldown.soul_immolation = get_cooldown( "soul_immolation" );

  // Havoc
  cooldown.blade_dance                               = get_cooldown( "blade_dance" );
  cooldown.chaos_strike_refund_icd                   = get_cooldown( "chaos_strike_refund_icd" );
  cooldown.eye_beam                                  = get_cooldown( "eye_beam" );
  cooldown.fel_rush                                  = get_cooldown( "fel_rush" );
  cooldown.relentless_onslaught_icd                  = get_cooldown( "relentless_onslaught_icd" );
  cooldown.fel_rush_vengeful_retreat_movement_shared = get_cooldown( "fel_rush_vengeful_retreat_movement_shared" );
  cooldown.felblade_vengeful_retreat_movement_shared = get_cooldown( "felblade_vengeful_retreat_movement_shared" );
  cooldown.essence_break_proc_icd                    = get_target_specific_cooldown( "essence_break_proc_icd" );

  // Vengeance
  cooldown.demon_spikes              = get_cooldown( "demon_spikes" );
  cooldown.spirit_bomb               = get_cooldown( "spirit_bomb" );
  cooldown.sigil_of_chains           = get_cooldown( "sigil_of_chains" );
  cooldown.sigil_of_silence          = get_cooldown( "sigil_of_silence" );
  cooldown.fel_devastation           = get_cooldown( "fel_devastation" );
  cooldown.volatile_flameblood_icd   = get_cooldown( "volatile_flameblood_icd" );
  cooldown.explosion_of_the_soul_icd = get_cooldown( "explosion_of_the_soul_icd" );

  // Aldrachi Reaver
  cooldown.art_of_the_glaive_consumption_icd = get_cooldown( "art_of_the_glaive_consumption_icd" );
  cooldown.wounded_quarry_trigger_icd        = get_cooldown( "wounded_quarry_trigger_icd" );

  // Scarred
  cooldown.predators_wake = get_cooldown( "predators_wake" );
}

// demon_hunter_t::create_gains =============================================

void demon_hunter_t::create_gains()
{
  // General
  gain.miss_refund = get_gain( "miss_refund" );

  // Devourer
  gain.voidglare_boon = get_gain( "voidglare_boon" );
  gain.void_buildup   = get_gain( "void_buildup" );

  // Havoc
  gain.blind_fury       = get_gain( "blind_fury" );
  gain.tactical_retreat = get_gain( "tactical_retreat" );

  // Vengeance
  gain.metamorphosis       = get_gain( "metamorphosis" );
  gain.darkglare_boon      = get_gain( "darkglare_boon" );
  gain.volatile_flameblood = get_gain( "volatile_flameblood" );

  // Set Bonuses
  gain.seething_fury = get_gain( "seething_fury" );

  // Scarred
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

// demon_hunter_t::composite_armor_multiplier ===============================

double demon_hunter_t::composite_armor_multiplier() const
{
  double am = base_t::composite_armor_multiplier();

  if ( buff.immolation_aura->check() )
  {
    am *= pow( 1.0 + spell.immolation_aura->effectN( 5 ).percent(), buff.immolation_aura->check() );
  }

  return am;
}

// demon_hunter_t::composite_player_multiplier ==============================

double demon_hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = base_t::composite_player_multiplier( school );

  return m;
}

// demon_hunter_t::composite_player_critical_damage_multiplier ==============

double demon_hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const
{
  double m = base_t::composite_player_critical_damage_multiplier( s, school );

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
  return base_t::matching_gear_multiplier( attr );
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
  if ( options.reset_fury_on_pull && in_boss_encounter && current_fury > fury_cap )
  {
    resources.current[ RESOURCE_FURY ] = fury_cap;
    sim->print_debug( "Fury for {} capped at combat start to {} (was {})", *this, fury_cap, current_fury );
  }
}

// demon_hunter_t::interrupt ================================================

void demon_hunter_t::interrupt()
{
  event_t::cancel( soul_fragment_pick_up );
  base_t::interrupt();
}

// demon_hunter_t::arise ====================================================

void demon_hunter_t::arise()
{
  base_t::arise();

  if ( talent.vengeance.felfire_fist->ok() )
  {
    buff.felfire_fist_out_of_combat->trigger();
  }

  if ( talent.annihilator.doomsayer->ok() )
  {
    buff.doomsayer_out_of_combat->trigger();
  }

  if ( talent.scarred.monster_rising->ok() )
  {
    buff.monster_rising->trigger();
  }
  if ( talent.scarred.enduring_torment->ok() )
  {
    buff.enduring_torment->trigger();
  }
  if ( talent.scarred.pursuit_of_angriness->ok() )
  {
    buff.pursuit_of_angryness->trigger();
  }
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
  if ( resource_type == RESOURCE_FURY && talent.scarred.pursuit_of_angriness->ok() )
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
  if ( resource_type == RESOURCE_FURY && talent.scarred.pursuit_of_angriness->ok() )
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
  feed_the_demon_accumulator    = 0.0;
  shattered_destiny_accumulator = 0.0;
  wounded_quarry_accumulator    = 0.0;
  last_reavers_mark_applied     = nullptr;

  for ( size_t i = 0; i < soul_fragments.size(); i++ )
  {
    delete soul_fragments[ i ];
  }

  soul_fragments.clear();
  devourer_fury_state.reset();
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
      buff.out_of_range->extend_duration( duration - buff.out_of_range->remains() );
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

unsigned demon_hunter_t::consume_soul_fragments( soul_fragment type, bool instant, unsigned max )
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
    it->consume( instant );
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
    return static_cast<unsigned>( soul_fragments.size() );
  }

  return std::accumulate(
      soul_fragments.begin(), soul_fragments.end(), 0,
      [ &type_mask ]( unsigned acc, soul_fragment_t* frag ) { return acc + frag->is_type( type_mask ); } );
}

void demon_hunter_t::fury_state_t::start()
{
  assert( !next_drain_event );

  dh()->buff.metamorphosis->trigger();

  dh()->resource_gain( RESOURCE_FURY, 200.00, dh()->gain.metamorphosis );

  start_time = actor->sim->current_time();
  schedule_tick();
}

void demon_hunter_t::fury_state_t::schedule_tick()
{
  assert( !next_drain_event );

  next_drain_event = make_event<drain_event_t>( *actor->sim, actor, time_to_next_tick( drain_stacks ) );
}

double demon_hunter_t::fury_state_t::fury_drain_per_second( int stacks ) const
{
  double drain = base_fury_drain_per_second( stacks );

  bool has_reduced_drain = !dh()->in_combat || dh()->buff.voidrush->check() || dh()->buffs.stunned->check() ||
                           ( dh()->executing && dh()->executing->id == dh()->spec.collapsing_star_spell->id() ) ||
                           ( dh()->channeling && dh()->channeling->id == dh()->talent.devourer.void_ray->id() );

  if ( has_reduced_drain )
  {
    // Guess
    drain *= 0.15;
  }

  if ( drain_stacks < 1 )
  {
    // Slow after meta cast
    drain = 15;
  }

  return drain;
}

timespan_t demon_hunter_t::fury_state_t::time_to_next_tick( int stacks ) const
{
  // 2 as it currently drains 2 per event.
  // TODO: Don't hardcode this.
  return 2.0_s / fury_drain_per_second( stacks );
}

void demon_hunter_t::fury_state_t::reschedule_drain()
{
  if ( !next_drain_event )
    return;

  double percent_remaining = 1.0 - next_drain_event->remains() / static_cast<drain_event_t*>( next_drain_event )->delta;

  auto new_time = time_to_next_tick( drain_stacks ) * percent_remaining;

  if ( new_time < next_drain_event->remains() )
  {
    event_t::cancel( next_drain_event );
    next_drain_event = make_event<drain_event_t>( *actor->sim, actor, new_time );
  }
  else
  {
    next_drain_event->reschedule( new_time );
  }
}

void demon_hunter_t::fury_state_t::clear_state()
{
  drain_stacks = 0;
  start_time   = timespan_t::min();
}

void demon_hunter_t::fury_state_t::stop()
{
  dh()->buff.metamorphosis->expire();
}

void demon_hunter_t::fury_state_t::reset()
{
  event_t::cancel( next_drain_event );
  stop();
}

void demon_hunter_t::fury_state_t::drain()
{
  dh()->active.void_buildup->stats->add_execute( 0_s, dh() );
  dh()->active.void_buildup->consume_resource();

  if ( dh()->resources.current[ RESOURCE_FURY ] <= 0.0 )
  {
    bool cannot_end_meta = ( dh()->channeling && dh()->channeling->id == dh()->talent.devourer.void_ray->id() ) ||
                           ( dh()->executing && dh()->executing->id == dh()->talent.devourer.collapsing_star->id() );

    if ( !cannot_end_meta )
    {
      stop();
      return;
    }
  }

  schedule_tick();
}

// demon_hunter_t::activate_soul_fragment ===================================

void demon_hunter_t::activate_soul_fragment( soul_fragment_t* frag )
{
  buff.soul_fragments->trigger();

  // If we spawn a fragment with this flag, instantly consume it
  if ( frag->consume_on_activation )
  {
    frag->consume( true );
    return;
  }

  auto max_soul_frags = max_soul_fragments();

  if ( frag->type == soul_fragment::LESSER )
  {
    unsigned active_fragments = get_active_soul_fragments( frag->type );
    if ( active_fragments > max_soul_frags )
    {
      // Find and delete the oldest active fragment of this type.
      for ( auto& it : soul_fragments )
      {
        if ( it->is_type( soul_fragment::LESSER ) && it->active() )
        {
          it->consume( false );

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

    assert( get_active_soul_fragments( soul_fragment::LESSER ) <= max_soul_frags );
  }
}

// demon_hunter_t::spawn_soul_fragment ======================================

void demon_hunter_t::spawn_soul_fragment( proc_t* source_proc, soul_fragment type, unsigned n,
                                          bool consume_on_activation )
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
      break;
  }

  for ( unsigned i = 0; i < n; i++ )
  {
    soul_fragments.push_back( new soul_fragment_t( this, type, consume_on_activation ) );
    source_proc->occur();
    soul_proc->occur();
  }

  sim->print_log( "{} creates {} {}. active={} total={}", *this, n, get_soul_fragment_str( type ),
                  get_active_soul_fragments( type ), get_total_soul_fragments( type ) );

  if ( talent.demon_hunter.soul_splitter->ok() && cooldown.soul_splitter_icd->up() &&
       rng().roll( talent.demon_hunter.soul_splitter->effectN( 1 ).percent() ) )
  {
    soul_fragments.push_back( new soul_fragment_t( this, soul_fragment::LESSER, consume_on_activation ) );
    proc.soul_fragment_lesser->occur();
    proc.soul_fragment_from_soul_splitter->occur();

    sim->print_log( "{} creates an additional {} from Soul Splitter. active={} total={}", *this,
                    get_soul_fragment_str( type ), get_active_soul_fragments( type ),
                    get_total_soul_fragments( type ) );
    cooldown.soul_splitter_icd->start( talent.demon_hunter.soul_splitter->internal_cooldown() );
  }
}

// demon_hunter_t::trigger_demonic ==========================================

void demon_hunter_t::trigger_demonic() const
{
  if ( !talent.havoc.demonic->ok() )
    return;

  debug_cast<buffs::metamorphosis_buff_t*>( buff.metamorphosis )->trigger_demonic();
}

// demon_hunter_t::trigger_demonsurge =============================================

void demon_hunter_t::trigger_demonsurge( const demonsurge_ability ability, const bool check_buff )
{
  timespan_t delay;

  // TOCHECK: Death sweep currently uses a 700 ms delay, while all other abilities use 450 ms delay.
  switch ( ability )
  {
    case demonsurge_ability::DEATH_SWEEP:
      delay = timespan_t::from_millis( hero_spec.demonsurge_meta_trigger->effectN( 1 ).misc_value1() );
      break;
    default:
      delay = timespan_t::from_millis( hero_spec.demonsurge_trigger->effectN( 1 ).misc_value1() );
      break;
  }
  trigger_demonsurge( ability, delay, check_buff );
}

void demon_hunter_t::trigger_demonsurge( const demonsurge_ability ability, timespan_t delay, const bool check_buff )
{
  if ( active.demonsurge && ( !check_buff || buff.demonsurge_abilities[ ability ]->up() ) )
  {
    if ( check_buff )
    {
      buff.demonsurge_abilities[ ability ]->expire();
    }
    proc.demonsurge_abilities[ ability ]->occur();
    make_event<delayed_execute_event_t>( *sim, this, active.demonsurge, target, delay );
  }
}

// demon_hunter_t::trigger_voidsurge ==============================================

void demon_hunter_t::trigger_voidsurge( const voidsurge_ability ability, const bool check_buff )
{
  trigger_voidsurge( ability, timespan_t::from_millis( hero_spec.voidsurge_trigger->effectN( 1 ).misc_value1() ),
                     check_buff );
}

void demon_hunter_t::trigger_voidsurge( const voidsurge_ability ability, timespan_t delay, const bool check_buff )
{
  if ( active.voidsurge && ( !check_buff || buff.voidsurge_abilities[ ability ]->up() ) )
  {
    if ( check_buff )
    {
      buff.voidsurge_abilities[ ability ]->expire();
    }
    proc.voidsurge_abilities[ ability ]->occur();
    make_event<delayed_execute_event_t>( *sim, this, active.voidsurge, target, delay );

    if ( buff.volatile_instinct->up() )
    {
      proc.voidsurge_abilities[ VOLATILE_INSTINCT ]->occur();
      make_event<delayed_execute_event_t>( *sim, this, active.voidsurge, target, delay );

      buff.volatile_instinct->expire();
    }
  }
}

// demon_hunter_t::parse_player_effects() =========================================

void demon_hunter_t::parse_player_effects()
{
  // Shared
  parse_effects( talent.demon_hunter.illidari_knowledge, PARSE_PASSIVE );
  parse_effects( talent.demon_hunter.pursuit );
  parse_effects( talent.havoc.demon_hide, PARSE_PASSIVE );
  parse_effects( spec.demonic_wards, PARSE_PASSIVE );

  // Devourer
  if ( specialization() == DEMON_HUNTER_DEVOURER )
  {
    parse_effects( buff.metamorphosis );
    parse_effects( buff.blur );
  }

  // Havoc
  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    parse_effects( buff.metamorphosis );
    parse_effects( buff.blur );
  }

  // Vengeance
  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    parse_effects( buff.metamorphosis );
    parse_effects( buff.demon_spikes );
    parse_effects( buff.seething_anger );
    parse_effects( mastery.fel_blood_rank_2 );
    parse_effects( buff.fiery_brand );
    parse_effects( buff.painbringer );
    parse_effects( buff.immolation_aura );

    if ( talent.vengeance.void_reaver.ok() )
    {
      parse_target_effects( d_fn( &demon_hunter_td_t::debuffs_t::frailty ), spec.frailty_debuff );
    }

  }

  // Aldrachi Reaver
  parse_effects( buff.thrill_of_the_fight_haste );

  // Annihilator
  parse_effects( buff.voidfall_building );
  parse_effects( buff.voidfall_spending );
  parse_effects( buff.voidfall_final_hour );

  // Scarred
  parse_effects( buff.pursuit_of_angryness, USE_CURRENT );

  // Set Bonuses
}

// demon_hunter_t::max_soul_fragments =============================================

unsigned demon_hunter_t::max_soul_fragments() const
{
  switch ( specialization() )
  {
    case DEMON_HUNTER_DEVOURER:
      return DEVOURER_MAX_SOUL_FRAGMENTS;
    case DEMON_HUNTER_HAVOC:
      return HAVOC_MAX_SOUL_FRAGMENTS;
    case DEMON_HUNTER_VENGEANCE:
      return MAX_SOUL_FRAGMENTS;
    default:
      return 0;
  }
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
      if ( dh()->sim->current_time() < sigil_activates )
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

// Reporting functions ======================================================

void demon_hunter_t::analyze( sim_t& sim )
{
  player_t::analyze( sim );
  // TODO: Fix reporting for container spells
}

// Utility functions ========================================================

const spell_data_t* demon_hunter_t::conditional_spell_lookup( bool fn, int id ) const
{
  if ( !fn )
  {
    return spell_data_t::not_found();
  }
  return find_spell( id );
}

const spell_data_t* demon_hunter_t::talent_spell_lookup( player_talent_t t, int id ) const
{
  return conditional_spell_lookup( t->ok(), id );
}

const spell_data_t* demon_hunter_t::spec_talent_spell_lookup( const specialization_e s, const player_talent_t& t,
                                                              const int id ) const
{
  return conditional_spell_lookup( specialization() == s && t->ok(), id );
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

  static void cdwaste_table_header( report::sc_html_stream& os )
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

  static void cdwaste_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void cdwaste_table_contents( report::sc_html_stream& os ) const
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

  void shattered_souls_piechart( report::sc_html_stream& os ) const
  {
    highchart::pie_chart_t shattered_souls_sources( highchart::build_id( p, "shattered_souls_source" ), *p.sim );
    shattered_souls_sources.set_title( "Shattered Souls Sources" );
    shattered_souls_sources.set( "plotOptions.pie.dataLabels.format", "{point.name}: {point.percentage:.1f}%" );

    std::vector<proc_t*> shattered_souls_procs( p.proc.shattered_souls.size() );
    std::transform( p.proc.shattered_souls.begin(), p.proc.shattered_souls.end(), shattered_souls_procs.begin(),
                    []( auto entry ) { return entry.second; } );

    double sum = 0.0;

    // Get total amount of Shattered Souls
    for ( const auto entry : shattered_souls_procs )
    {
      sum += entry->count.mean();
    }

    // Sort the dataset so it looks better in the chart
    range::sort( shattered_souls_procs, []( const auto& left, const auto& right ) {
      if ( left->count.mean() == right->count.mean() )
      {
        return left->name_str < right->name_str;
      }

      return left->count.mean() > right->count.mean();
    } );

    // Populate the pie chart with each entry
    range::for_each( shattered_souls_procs, [ this, &shattered_souls_sources, sum ]( const auto& entry ) {
      if ( entry->count.mean() == 0.0 )
        return;

      std::string prefix = "Shattered Souls: ";
      color::rgb color   = color::school_color( SCHOOL_SHADOW );

      js::sc_js_t e;
      e.set( "color", color.str() );
      e.set( "y", util::round( ( entry->count.mean() / sum ) * 100, p.sim->report_precision ) );
      e.set( "name", report_decorators::decorate_html_string(
                         util::encode_html( entry->name_str.substr( prefix.length() ) ), color ) );

      shattered_souls_sources.add( "series.0.data", e );
    } );
    shattered_souls_sources.set( "series.0.name", "Percentage" );

    os << shattered_souls_sources.to_target_div();
    p.sim->add_chart_data( shattered_souls_sources );
  }

  void soul_fragment_generation_piechart( report::sc_html_stream& os ) const
  {
    highchart::pie_chart_t soul_fragment_generation_sources( highchart::build_id( p, "soul_fragment_sources" ),
                                                             *p.sim );
    soul_fragment_generation_sources.set_title( "Soul Fragment Sources" );
    soul_fragment_generation_sources.set( "plotOptions.pie.dataLabels.format",
                                          "{point.name}: {point.percentage:.1f}%" );

    std::vector<proc_t*> soul_fragment_generation_procs{
        // general
        p.proc.soul_fragment_from_soul_splitter,
        p.proc.soul_fragment_from_death,

        // devourer
        p.proc.soul_fragment_from_consume,
        p.proc.soul_fragment_from_devour,
        p.proc.soul_fragment_from_soul_immolation,
        p.proc.soul_fragment_from_shattered_souls,
        p.proc.soul_fragment_from_star_fragments,
        p.proc.soul_fragment_from_hungering_slash,
        p.proc.soul_fragment_from_reapers_toll,
        p.proc.soul_fragment_from_void_metamorphosis,
        p.proc.soul_fragment_from_entropy,

        // havoc
        p.proc.soul_fragment_from_demonic_appetite,

        // vengeance
        p.proc.soul_fragment_from_fracture,
        p.proc.soul_fragment_from_fracture_meta,
        p.proc.soul_fragment_from_sigil_of_spite,
        p.proc.soul_fragment_from_soul_carver,
        p.proc.soul_fragment_from_fallout,

        // aldrachi reaver
        p.proc.soul_fragment_from_aldrachi_tactics,
        p.proc.soul_fragment_from_wounded_quarry,
        p.proc.soul_fragment_from_broken_spirit,

        // annihilator
        p.proc.soul_fragment_from_meteoric_rise,
        p.proc.soul_fragment_from_world_killer,

        // scarred
    };

    double sum = 0.0;

    // Get total amount of Shattered Souls
    for ( const auto entry : soul_fragment_generation_procs )
    {
      sum += entry->count.mean();
    }

    // Sort the dataset so it looks better in the chart
    range::sort( soul_fragment_generation_procs, []( const auto& left, const auto& right ) {
      if ( left->count.mean() == right->count.mean() )
      {
        return left->name_str < right->name_str;
      }

      return left->count.mean() > right->count.mean();
    } );

    // Populate the pie chart with each entry
    range::for_each( soul_fragment_generation_procs,
                     [ this, &soul_fragment_generation_sources, sum ]( const auto& entry ) {
                       if ( entry->count.mean() == 0.0 )
                         return;

                       std::string prefix = "Soul Fragment from ";
                       color::rgb color   = color::school_color( SCHOOL_SHADOW );

                       js::sc_js_t e;
                       e.set( "color", color.str() );
                       e.set( "y", util::round( ( entry->count.mean() / sum ) * 100, p.sim->report_precision ) );
                       e.set( "name", report_decorators::decorate_html_string(
                                          util::encode_html( entry->name_str.substr( prefix.length() ) ), color ) );

                       soul_fragment_generation_sources.add( "series.0.data", e );
                     } );
    soul_fragment_generation_sources.set( "series.0.name", "Percentage" );

    os << soul_fragment_generation_sources.to_target_div();
    p.sim->add_chart_data( soul_fragment_generation_sources );
  }

  void html_customsection_soul_fragment_generation( report::sc_html_stream& os ) const
  {
    os << "<div class=\"player-section custom_section\">\n"
          "<h3 class=\"toggle open\">Soul Fragment Generation</h3>\n"
          "<div class=\"toggle-content\">\n";

    soul_fragment_generation_piechart( os );
    if ( p.specialization() == DEMON_HUNTER_DEVOURER )
      shattered_souls_piechart( os );

    os << "</div>\n"
          "</div>\n";
  }

  void html_customsection_cd_waste( report::sc_html_stream& os ) const
  {
    os << "<div class=\"player-section custom_section\">\n";
    if ( !p.cd_waste_exec.empty() )
    {
      os << "<h3 class=\"toggle open\">Cooldown Waste Details</h3>\n"
         << "<div class=\"toggle-content\">\n";

      cdwaste_table_header( os );
      cdwaste_table_contents( os );
      cdwaste_table_footer( os );

      os << "</div>\n";
      os << "<div class=\"clear\"></div>\n";
    }
    os << "</div>\n";
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.sim->report_details == 0 )
      return;

    html_customsection_cd_waste( os );

    html_customsection_soul_fragment_generation( os );
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
    hotfix::register_effect( "Demon Hunter", "2025-03-26",
                             "Collapsing star still only does 50% additional damage to primary target", 1290193 )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 50.0 )
        .verification_value( 75.0 );
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
