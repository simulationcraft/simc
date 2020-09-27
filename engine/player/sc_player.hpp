// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "actor.hpp"
#include "sc_enums.hpp"
#include "player_talent_points.hpp"
#include "player_resources.hpp"
#include "gear_stats.hpp"
#include "rating.hpp"
#include "weapon.hpp"
#include "runeforge_data.hpp"
#include "effect_callbacks.hpp"
#include "util/plot_data.hpp"
#include "player_collected_data.hpp"
#include "player_processed_report_information.hpp"
#include "player_stat_cache.hpp"
#include "scaling_metric_data.hpp"
#include "util/cache.hpp"
#include "dbc/item_database.hpp"
#include "assessor.hpp"
#include <map>
#include <set>

struct absorb_buff_t;
struct action_t;
struct action_state_t;
struct action_priority_list_t;
struct action_callback_t;
struct action_variable_t;
struct actor_target_data_t;
struct attack_t;
class azerite_essence_t;
class azerite_power_t;
class conduit_data_t;
class dbc_t;
struct benefit_t;
struct item_t;
struct buff_t;
struct cooldown_t;
struct dot_t;
struct event_t;
struct expr_t;
struct gain_t;
struct instant_absorb_t;
struct sample_data_helper_t;
struct option_t;
struct pet_t;
struct player_description_t;
struct player_report_extension_t;
struct player_scaling_t;
struct proc_t;
struct real_ppm_t;
struct set_bonus_t;
class shuffled_rng_t;
struct special_effect_t;
struct spelleffect_data_t;
struct stat_buff_t;
struct stats_t;
struct uptime_t;
namespace azerite {
    class azerite_state_t;
    class azerite_essence_state_t;
}
namespace rng {
    struct rng_t;
}
namespace io {
  class ofstream;
}
namespace report {
  using sc_html_stream = io::ofstream;
}
namespace js {
    struct JsonOutput;
}
namespace covenant {
  class covenant_state_t;
}

/* Player Report Extension
 * Allows class modules to write extension to the report sections based on the dynamic class of the player.
 *
 * To add sort functionaliy to custom tables:
 *  1) add the 'sort' class to the table: <table="sc sort">
 *    a) optionally add 'even' or 'odd' to automatically stripe the table: <table="sc sort even">
 *  2) wrap <thead> around all header rows: <thead><tr><th> ... </th></tr></thead>
 *  3) add the 'toggle-sort' class to each header to be sorted: <th class="toggle-sort"> ... </th>
 *    a) default sort behavior is descending numerical sort
 *    b) add 'data-sortdir="asc" to sort ascending: <th class="toggle-sort" data-sortdir="asc"> ... </th>
 *    c) add 'data-sorttype="alpha" to sort alphabetically
 */
struct player_report_extension_t
{
public:
  virtual ~player_report_extension_t()
  {

  }
  virtual void html_customsection(report::sc_html_stream&)
  {

  }
};

struct player_t : public actor_t
{
  static const int default_level = 60;

  // static values
  player_e type;
  player_t* parent; // corresponding player in main thread
  int index;
  int creation_iteration; // The iteration when this actor was created, -1 for "init"
  size_t actor_index;
  int actor_spawn_index; // a unique identifier for each arise() of the actor
  // (static) attributes - things which should not change during combat
  race_e       race;
  role_e       role;
  int          true_level; /* The character's true level. If the outcome would change when the character's level is
                           scaled down (such as when timewalking) then use the level() method instead. */
  int          party;
  int          ready_type;
  specialization_e  _spec;
  bool         bugs; // If true, include known InGame mechanics which are probably the cause of a bug and not inteded
  int          disable_hotfixes;
  bool         scale_player;
  double       death_pct; // Player will die if he has equal or less than this value as health-pct
  double       height; // Actor height, only used for enemies. Affects the travel distance calculation for spells.
  double       combat_reach; // AKA hitbox size, for enemies.
  profile_source profile_source_;
  player_t*    default_target;

  // dynamic attributes - things which change during combat
  player_t*   target;
  bool        initialized;
  bool        potion_used;

  std::string talents_str, id_str, target_str;
  std::string region_str, server_str, origin_str;
  std::string race_str, professions_str, position_str;
  enum timeofday_e { NIGHT_TIME, DAY_TIME, } timeofday; // Specify InGame time of day to determine Night Elf racial
  enum zandalari_loa_e {AKUNDA, BWONSAMDI, GONK, KIMBUL, KRAGWA, PAKU} zandalari_loa; //Specify which loa zandalari has chosen to determine racial
  enum vulpera_tricks_e { CORROSIVE, FLAMES, SHADOWS, HEALING, HOLY } vulpera_tricks; //Specify which trick to use for vulpera bag of tricks

  // GCD Related attributes
  timespan_t  gcd_ready, base_gcd, min_gcd; // When is GCD ready, default base and minimum GCD times.
  gcd_haste_type gcd_type; // If the current GCD is hasted, what haste type is used
  double gcd_current_haste_value; // The currently used haste value for GCD speedup

  timespan_t started_waiting;
  std::vector<pet_t*> pet_list;
  std::vector<pet_t*> active_pets;
  std::vector<absorb_buff_t*> absorb_buff_list;
  std::map<unsigned,instant_absorb_t> instant_absorb_list;

  int         invert_scaling;

  // Reaction
  timespan_t  reaction_offset, reaction_max, reaction_mean, reaction_stddev, reaction_nu;
  // Latency
  timespan_t  world_lag, world_lag_stddev;
  timespan_t  brain_lag, brain_lag_stddev;
  bool        world_lag_override, world_lag_stddev_override;
  timespan_t  cooldown_tolerance_;

  // Data access
  std::unique_ptr<dbc_t> dbc;
  const dbc_override_t*  dbc_override;

  // Option Parsing
  std::vector<std::unique_ptr<option_t>> options;

  // Stat Timelines to Display
  std::vector<stat_e> stat_timelines;

  // Talent Parsing
  player_talent_points_t talent_points;
  std::string talent_overrides_str;

  // Profs
  std::array<int, PROFESSION_MAX> profession;

  /// Azerite state object
  std::unique_ptr<azerite::azerite_state_t> azerite;

  /// Azerite essence state object
  std::unique_ptr<azerite::azerite_essence_state_t> azerite_essence;

  /// Covenant state object
  std::unique_ptr<covenant::covenant_state_t> covenant;

  // TODO: FIXME, these stats should not be increased by scale factor deltas
  struct base_initial_current_t
  {
    base_initial_current_t();

    gear_stats_t stats;

    double spell_power_per_intellect, spell_power_per_attack_power, spell_crit_per_intellect;
    double attack_power_per_strength, attack_power_per_agility, attack_crit_per_agility, attack_power_per_spell_power;
    double dodge_per_agility, parry_per_strength;
    double health_per_stamina;
    std::array<double, SCHOOL_MAX> resource_reduction;
    double miss, dodge, parry, block;
    double hit, expertise;
    double spell_crit_chance, attack_crit_chance, block_reduction, mastery;
    double skill, skill_debuff, distance;
    double distance_to_move;
    double moving_away;
    movement_direction_type movement_direction;
    double armor_coeff;
    bool sleeping;
    rating_t rating;

    std::array<double, ATTRIBUTE_MAX> attribute_multiplier;
    double spell_power_multiplier, attack_power_multiplier, base_armor_multiplier, armor_multiplier;
    position_e position;

    friend void format_to( const base_initial_current_t&, fmt::format_context::iterator );
  }
  base, // Base values, from some database or overridden by user
  initial, // Base + Passive + Gear (overridden or items) + Player Enchants + Global Enchants
  current; // Current values, reset to initial before every iteration

  /// Passive combat rating multipliers
  rating_t passive_rating_multiplier;

  gear_stats_t passive; // Passive stats from various passive auras (and similar effects)

  timespan_t last_cast;

  // Defense Mechanics
  struct diminishing_returns_constants_t
  {
    double horizontal_shift = 0.0;
    double block_vertical_stretch = 0.0;
    double vertical_stretch = 0.0;
    double dodge_factor = 1.0;
    double parry_factor = 1.0;
    double miss_factor = 1.0;
    double block_factor = 1.0;
  } def_dr;

  // Weapons
  weapon_t main_hand_weapon;
  weapon_t off_hand_weapon;

  // Main, offhand, and ranged attacks
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  // Current attack speed (needed for dynamic attack speed adjustments)
  double current_attack_speed;

  player_resources_t resources;

  // Events
  action_t* executing;
  action_t* queueing;
  action_t* channeling;
  action_t* strict_sequence; // Strict sequence of actions currently being executed
  event_t* readying;
  event_t* off_gcd;
  event_t* cast_while_casting_poll_event; // Periodically check for something to do while casting
  std::vector<const cooldown_t*> off_gcd_cd, off_gcd_icd;
  std::vector<const cooldown_t*> cast_while_casting_cd, cast_while_casting_icd;
  timespan_t off_gcd_ready;
  timespan_t cast_while_casting_ready;
  bool in_combat;
  bool action_queued;
  bool first_cast;
  action_t* last_foreground_action;
  std::vector<action_t*> prev_gcd_actions;
  std::vector<action_t*> off_gcdactions; // Returns all off gcd abilities used since the last gcd.

  // Delay time used by "cast_delay" expression to determine when an action
  // can be used at minimum after a spell cast has finished, including GCD
  timespan_t cast_delay_reaction;
  timespan_t cast_delay_occurred;

  // Callbacks
  effect_callbacks_t<action_callback_t> callbacks;
  auto_dispose< std::vector<special_effect_t*> > special_effects;
  std::vector<std::function<void(player_t*)> > callbacks_on_demise;
  std::vector<std::function<void(void)>> callbacks_on_arise;

  // Action Priority List
  auto_dispose< std::vector<action_t*> > action_list;
  /// Actions that have an action-specific dynamic targeting
  std::set<action_t*> dynamic_target_action_list;
  std::string action_list_str;
  std::string choose_action_list;
  std::string action_list_skip;
  std::string modify_action;
  std::string use_apl;
  bool use_default_action_list;
  auto_dispose< std::vector<dot_t*> > dot_list;
  auto_dispose< std::vector<action_priority_list_t*> > action_priority_list;
  std::vector<action_t*> precombat_action_list;
  action_priority_list_t* active_action_list;
  action_priority_list_t* default_action_list;
  action_priority_list_t* active_off_gcd_list;
  action_priority_list_t* active_cast_while_casting_list;
  action_priority_list_t* restore_action_list;
  std::unordered_map<std::string, std::string> alist_map;
  std::string action_list_information; // comment displayed in profile
  bool no_action_list_provided;
  std::unordered_map<std::string, std::string> apl_variable_map;

  bool quiet;
  // Reporting
  std::unique_ptr<player_report_extension_t> report_extension;
  timespan_t arise_time;
  timespan_t iteration_fight_length;
  timespan_t iteration_waiting_time, iteration_pooling_time;
  int iteration_executed_foreground_actions;
  std::array< double, RESOURCE_MAX > iteration_resource_lost, iteration_resource_gained;
  double rps_gain, rps_loss;
  std::string tmi_debug_file_str;
  double tmi_window;

  auto_dispose< std::vector<buff_t*> > buff_list;
  auto_dispose< std::vector<proc_t*> > proc_list;
  auto_dispose< std::vector<gain_t*> > gain_list;
  auto_dispose< std::vector<stats_t*> > stats_list;
  auto_dispose< std::vector<benefit_t*> > benefit_list;
  auto_dispose< std::vector<uptime_t*> > uptime_list;
  auto_dispose< std::vector<cooldown_t*> > cooldown_list;
  auto_dispose< std::vector<real_ppm_t*> > rppm_list;
  auto_dispose< std::vector<shuffled_rng_t*> > shuffled_rng_list;
  std::vector<cooldown_t*> dynamic_cooldown_list;
  std::array< std::vector<plot_data_t>, STAT_MAX > dps_plot_data;
  std::vector<std::vector<plot_data_t> > reforge_plot_data;
  auto_dispose< std::vector<sample_data_helper_t*> > sample_data_list;

  // All Data collected during / end of combat
  player_collected_data_t collected_data;

  // Damage
  double iteration_dmg, priority_iteration_dmg, iteration_dmg_taken; // temporary accumulators
  double dpr;
  struct incoming_damage_entry_t {
    timespan_t time;
    double amount;
    school_e school;
  };
  std::vector<incoming_damage_entry_t> incoming_damage; // for tank active mitigation conditionals

  // Heal
  double iteration_heal, iteration_heal_taken, iteration_absorb, iteration_absorb_taken; // temporary accumulators
  double hpr;
  std::vector<unsigned> absorb_priority; // for strict sequence absorbs

  player_processed_report_information_t report_information;

  void sequence_add( const action_t* a, const player_t* target, timespan_t ts );
  void sequence_add_wait( timespan_t amount, timespan_t ts );

  // Gear
  std::string items_str, meta_gem_str, potion_str, flask_str, food_str, rune_str;
  std::vector<item_t> items;
  gear_stats_t gear, enchant; // Option based stats
  gear_stats_t total_gear; // composite of gear, enchant and for non-pets sim -> enchant
  std::unique_ptr<set_bonus_t> sets;
  meta_gem_e meta_gem;
  bool matching_gear;
  bool karazhan_trinkets_paired;
  std::unique_ptr<cooldown_t> item_cooldown;
  cooldown_t* legendary_tank_cloak_cd; // non-Null if item available


  // Warlord's Unseeing Eye (6.2 Trinket)
  double warlords_unseeing_eye;
  stats_t* warlords_unseeing_eye_stats;

  // Misc Multipliers
  // auto attack multiplier (for Jeweled Signet of Melandrus and similar effects)
  double auto_attack_multiplier;
  // Prepatch Insignia of the Grand Army flat dmg multiplier
  double insignia_of_the_grand_army_multiplier;

  // Scale Factors
  std::unique_ptr<player_scaling_t> scaling;

  // Movement & Position
  double base_movement_speed;
  double passive_modifier; // _PASSIVE_ movement speed modifiers
  double x_position, y_position, default_x_position, default_y_position;

  struct consumables_t {
    buff_t* flask;
    stat_buff_t* guardian_elixir;
    stat_buff_t* battle_elixir;
    buff_t* food;
    buff_t* augmentation;
  } consumables;

  struct buffs_t
  {
    buff_t* angelic_feather;
    buff_t* beacon_of_light;
    buff_t* blood_fury;
    buff_t* body_and_soul;
    buff_t* damage_done;
    buff_t* darkflight;
    buff_t* devotion_aura;
    buff_t* entropic_embrace;
    buff_t* exhaustion;
    buff_t* guardian_spirit;
    buff_t* blessing_of_sacrifice;
    buff_t* mongoose_mh;
    buff_t* mongoose_oh;
    buff_t* nitro_boosts;
    buff_t* pain_suppression;
    buff_t* movement;
    buff_t* stampeding_roar;
    buff_t* shadowmeld;
    buff_t* windwalking_movement_aura;
    buff_t* stoneform;
    buff_t* stunned;
    std::array<buff_t*, 4> ancestral_call;
    buff_t* fireblood;
    buff_t* embrace_of_paku;

    buff_t* berserking;
    buff_t* bloodlust;

    buff_t* cooldown_reduction;
    buff_t* amplification;
    buff_t* amplification_2;

    // Legendary meta stuff
    buff_t* courageous_primal_diamond_lucidity;
    buff_t* tempus_repit;
    buff_t* fortitude;

    buff_t* archmages_greater_incandescence_str;
    buff_t* archmages_greater_incandescence_agi;
    buff_t* archmages_greater_incandescence_int;
    buff_t* archmages_incandescence_str;
    buff_t* archmages_incandescence_agi;
    buff_t* archmages_incandescence_int;
    buff_t* legendary_aoe_ring; // Legendary ring buff.
    buff_t* legendary_tank_buff;

    // T17 LFR stuff
    buff_t* surge_of_energy;
    buff_t* natures_fury;
    buff_t* brute_strength;

    // 7.0 trinket proxy buffs
    buff_t* incensed;
    buff_t* taste_of_mana; // Gnawed Thumb Ring buff

    // 7.0 Legendaries
    buff_t* aggramars_stride;

    // 7.1
    buff_t* temptation; // Ring that goes on a 5 minute cd if you use it too much.
    buff_t* norgannons_foresight; //Legendary item that allows movement for 5 seconds if you stand still for 8.
    buff_t* norgannons_foresight_ready;
    buff_t* nefarious_pact; // Whispers in the dark good buff
    buff_t* devils_due; // Whispers in the dark bad buff

    // 6.2 trinket proxy buffs
    buff_t* naarus_discipline; // Priest-Discipline Boss 13 T18 trinket
    buff_t* tyrants_immortality; // Tyrant's Decree trinket proc
    buff_t* tyrants_decree_driver; // Tyrant's Decree trinket driver

    buff_t* fel_winds; // T18 LFR Plate Melee Attack Speed buff
    buff_t* demon_damage_buff; // 6.2.3 Heirloom trinket demon damage buff

    // Darkmoon Faire versatility food
    buff_t* dmf_well_fed;

    // 8.0
    buff_t* galeforce_striking; // Gale-Force Striking weapon enchant
    buff_t* torrent_of_elements; // Torrent of Elements weapon enchant

    // 8.0 - Leyshock's Grand Compendium stat buffs
    buff_t* leyshock_crit;
    buff_t* leyshock_haste;
    buff_t* leyshock_mastery;
    buff_t* leyshock_versa;

    // Azerite power
    buff_t* normalization_increase;

    // Uldir
    buff_t* reorigination_array;

    /// 8.2 Azerite Essences
    stat_buff_t* memory_of_lucid_dreams;
    stat_buff_t* lucid_dreams; // Versatility Buff from Rank 3
    buff_t* reckless_force; // The Unbound Force minor - crit chance
    buff_t* reckless_force_counter; // The Unbound Force minor - max 20 stack counter
    stat_buff_t* lifeblood; // Worldvein Resonance - grant primary stat per shard, max 4
    buff_t* seething_rage; // Blood of the Enemy major - 25% crit dam
    stat_buff_t* reality_shift; // Ripple in Space minor - primary stat on moving 25yds
    buff_t* guardian_of_azeroth; // Condensed Life-Force major - R3 stacking haste on pet cast

    // 8.2 misc
    buff_t* damage_to_aberrations; // Benthic belt special effect
    buff_t* fathom_hunter; // Follower themed Benthic boots special effect
    buff_t* delirious_frenzy; // Dream's End 1H STR axe attack speed buff
    buff_t* bioelectric_charge; // Diver's Folly 1H AGI axe buff to store damage
    buff_t* razor_coral; // Ashvane's Razor Coral trinket crit rating buff

    // 9.0 class buffs
    buff_t* focus_magic; // Mage talent
    buff_t* power_infusion; // Priest spell

    // 9.0 Soulbinds
    buff_t* invigorating_herbs;       // night_fae/niya/tools - proc on direct heal
    buff_t* redirected_anima_stacks;  // night_fae/niya/grove_invigoration - counter procced via rppm
    buff_t* field_of_blossoms;        // night_fae/dreamweaver - buff procced on covenant ability use
    buff_t* social_butterfly;         // night_fae/dreamweaver - periodic buff when 2+ allies are nearby
    buff_t* first_strike;             // night_fae/korayn - crit buff when first hitting enemy
    buff_t* wild_hunt_tactics;        // night_fae/korayn - dummy buff used to quickly check if soulbind is enabled
    buff_t* thrill_seeker;            // venthyr/nadjia - counter every 2s
    buff_t* euphoria;                 // venthyr/nadjia/thill_seeker - haste buff
    buff_t* wasteland_propriety;      // venthyr/theotar - vers buff on covenant cast
    buff_t* built_for_war;            // venthyr/draven - stacking buff when above 80% hp
    buff_t* superior_tactics;         // venthyr/draven - crit buff on interrupt
    buff_t* let_go_of_the_past;       // kyrian/pelagos - vers buff on diff spell
    buff_t* combat_meditation;        // kyrian/pelagos - mast buff on covenant cast
    buff_t* pointed_courage;          // kyrian/kleia - crit buff for every nearby enemy or ally
    buff_t* hammer_of_genesis;        // kyrian/mikanikos - haste on hitting new enemy
    buff_t* brons_call_to_action;     // kyrian/mikanikos - bron's counter
    buff_t* gnashing_chompers;        // necrolord/emeni - haste on enemy death
    buff_t* embody_the_construct;     // necrolord/emeni - cast counter
    buff_t* marrowed_gemstone_charging;     // necrolord/heirmir - crit counter
    buff_t* marrowed_gemstone_enhancement;  // necrolord/heirmir - crit proc after 10 crits

    // 9.0 Enchants and Consumables
    buff_t* celestial_guidance;

    // 9.0 Runecarves
    buff_t* norgannons_sagacity_stacks;  // stacks on every cast
    buff_t* norgannons_sagacity;         // consume stacks to allow casting while moving
  } buffs;

  struct debuffs_t
  {
    buff_t* bleeding;
    buff_t* casting;
    buff_t* flying;
    buff_t* forbearance;
    buff_t* invulnerable;
    buff_t* vulnerable;
    buff_t* damage_taken;

    // WoD debuffs
    buff_t* mortal_wounds;

    // BfA Raid Damage Modifier Debuffs
    buff_t* chaos_brand;  // Demon Hunter
    buff_t* mystic_touch; // Monk
  } debuffs;

  struct external_buffs_t
  {
    bool focus_magic;
    std::vector<timespan_t> power_infusion;
  } external_buffs;

  struct gains_t
  {
    gain_t* arcane_torrent;
    gain_t* endurance_of_niuzao;
    std::array<gain_t*, RESOURCE_MAX> resource_regen;
    gain_t* health;
    gain_t* mana_potion;
    gain_t* restore_mana;
    gain_t* touch_of_the_grave;
    gain_t* vampiric_embrace;
    gain_t* warlords_unseeing_eye;
    gain_t* embrace_of_bwonsamdi;

    gain_t* leech;
  } gains;

  struct spells_t
  {
    action_t* leech;
  } spells;

  struct procs_t
  {
    proc_t* parry_haste;
  } procs;

  struct uptimes_t
  {
    uptime_t* primary_resource_cap;
  } uptimes;

  struct racials_t
  {
    const spell_data_t* quickness;
    const spell_data_t* command;
    const spell_data_t* arcane_acuity;
    const spell_data_t* heroic_presence;
    const spell_data_t* might_of_the_mountain;
    const spell_data_t* expansive_mind;
    const spell_data_t* nimble_fingers;
    const spell_data_t* time_is_money;
    const spell_data_t* the_human_spirit;
    const spell_data_t* touch_of_elune;
    const spell_data_t* brawn;
    const spell_data_t* endurance;
    const spell_data_t* viciousness;
    const spell_data_t* magical_affinity;
    const spell_data_t* mountaineer;
    const spell_data_t* brush_it_off;
  } racials;

  struct passives_t
  {
    double amplification_1;
    double amplification_2;
  } passive_values;

  bool active_during_iteration;
  const spelleffect_data_t* _mastery; // = find_mastery_spell( specialization() ) -> effectN( 1 );
  player_stat_cache_t cache;
  auto_dispose<std::vector<action_variable_t*>> variables;
  std::vector<std::string> action_map;

  regen_type resource_regeneration;

  /// Last iteration time regeneration occurred. Set at player_t::arise()
  timespan_t last_regen;

  /// A list of CACHE_x enumerations (stats) that affect the resource regeneration of the actor.
  std::vector<bool> regen_caches;

  /// Flag to indicate if any pets require dynamic regneration. Initialized in player_t::init().
  bool dynamic_regen_pets;

  /// Visited action lists, needed for call_action_list support. Reset by player_t::execute_action().
  uint64_t visited_apls_;

  /// Internal counter for action priority lists, used to set action_priority_list_t::internal_id for lists.
  unsigned action_list_id_;

  /// Current execution type
  execute_type current_execute_type;


  using resource_callback_function_t = std::function<void()>;

private:
  /// Flag to activate/deactive resource callback checks. Motivation: performance.
  bool has_active_resource_callbacks;

  struct resource_callback_entry_t
  {
    resource_e resource;
    double value;
    bool is_pct;
    bool fire_once;
    bool is_consumed;
    resource_callback_function_t callback;
  };
  std::vector<resource_callback_entry_t> resource_callbacks;

  /// Per-player custom dbc data
  std::unique_ptr<dbc_override_t> dbc_override_;

public:



  player_t( sim_t* sim, player_e type, util::string_view name, race_e race_e );
  ~player_t() override;

  // Static methods
  static player_t* create( sim_t* sim, const player_description_t& );
  static bool _is_enemy( player_e t ) { return t == ENEMY || t == ENEMY_ADD || t == ENEMY_ADD_BOSS || t == TANK_DUMMY; }
  static bool _is_sleeping( const player_t* t ) { return t -> current.sleeping; }


  // Overrides
  const char* name() const override
  { return name_str.c_str(); }


  // Normal methods
  void init_character_properties();
  double get_stat_value(stat_e);
  void stat_gain( stat_e stat, double amount, gain_t* g = nullptr, action_t* a = nullptr, bool temporary = false );
  void stat_loss( stat_e stat, double amount, gain_t* g = nullptr, action_t* a = nullptr, bool temporary = false );
  void create_talents_numbers();
  void create_talents_armory();
  void create_talents_wowhead();
  void clear_action_priority_lists() const;
  void copy_action_priority_list( util::string_view old_list, util::string_view new_list );
  void change_position( position_e );
  void register_resource_callback(resource_e resource, double value, resource_callback_function_t callback,
      bool use_pct, bool fire_once = true);
  bool add_action( util::string_view action, util::string_view options = "", util::string_view alist = "default" );
  bool add_action( const spell_data_t* s, util::string_view options = "", util::string_view alist = "default" );
  void add_option( std::unique_ptr<option_t> o );
  void parse_talents_numbers( util::string_view talent_string );
  bool parse_talents_armory( util::string_view talent_string );
  bool parse_talents_armory2( util::string_view talent_string );

  bool is_moving() const;
  double composite_block_dr( double extra_block ) const;
  bool is_pet() const { return type == PLAYER_PET || type == PLAYER_GUARDIAN || type == ENEMY_ADD || type == ENEMY_ADD_BOSS; }
  bool is_enemy() const { return _is_enemy( type ); }
  bool is_boss() const { return type == ENEMY || type == ENEMY_ADD_BOSS; }
  bool is_add() const { return type == ENEMY_ADD || type == ENEMY_ADD_BOSS; }
  bool is_sleeping() const { return _is_sleeping( this ); }
  bool is_my_pet( const player_t* t ) const;
  bool in_gcd() const;
  bool recent_cast() const;
  bool dual_wield() const
  { return main_hand_weapon.type != WEAPON_NONE && off_hand_weapon.type != WEAPON_NONE; }
  bool has_shield_equipped() const;
  /// Figure out if healing should be recorded
  bool record_healing() const;
  specialization_e specialization() const
  { return _spec; }
  const char* primary_tree_name() const;
  timespan_t total_reaction_time();
  double avg_item_level() const;
  double get_attribute( attribute_e a ) const
  { return util::floor( composite_attribute( a ) * composite_attribute_multiplier( a ) ); }
  double strength() const
  { return get_attribute( ATTR_STRENGTH ); }
  double agility() const
  { return get_attribute( ATTR_AGILITY ); }
  double stamina() const
  { return get_attribute( ATTR_STAMINA ); }
  double intellect() const
  { return get_attribute( ATTR_INTELLECT ); }
  double spirit() const
  { return get_attribute( ATTR_SPIRIT ); }
  double mastery_coefficient() const;
  double get_player_distance( const player_t& ) const;
  double get_ground_aoe_distance( const action_state_t& ) const;
  double get_position_distance( double m = 0, double v = 0 ) const;
  double compute_incoming_damage( timespan_t interval) const;
  double compute_incoming_magic_damage( timespan_t interval ) const;
  double calculate_time_to_bloodlust() const;
  slot_e parent_item_slot( const item_t& item ) const;
  slot_e child_item_slot( const item_t& item ) const;
  /// Actor-specific cooldown tolerance for queueable actions
  timespan_t cooldown_tolerance() const;
  position_e position() const
  { return current.position; }


  pet_t* cast_pet();
  const pet_t* cast_pet() const;

  azerite_power_t find_azerite_spell( util::string_view name, bool tokenized = false ) const;
  azerite_power_t find_azerite_spell( unsigned power_id ) const;
  azerite_essence_t find_azerite_essence( util::string_view name, bool tokenized = false ) const;
  azerite_essence_t find_azerite_essence( unsigned power_id ) const;

  item_runeforge_t find_runeforge_legendary( util::string_view name, bool tokenized = false ) const;

  conduit_data_t find_conduit_spell( util::string_view name ) const;
  const spell_data_t* find_soulbind_spell( util::string_view name ) const;
  const spell_data_t* find_covenant_spell( util::string_view name ) const;

  const spell_data_t* find_racial_spell( util::string_view name, race_e s = RACE_NONE ) const;
  const spell_data_t* find_class_spell( util::string_view name, specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_rank_spell( util::string_view name, util::string_view desc, specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_pet_spell( util::string_view name ) const;
  const spell_data_t* find_talent_spell( util::string_view name, specialization_e s = SPEC_NONE, bool name_tokenized = false, bool check_validity = true ) const;
  const spell_data_t* find_specialization_spell( util::string_view name, specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_specialization_spell( util::string_view name, util::string_view desc, specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_specialization_spell( unsigned spell_id, specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_mastery_spell( specialization_e s ) const;
  const spell_data_t* find_spell( util::string_view name, specialization_e s = SPEC_NONE ) const;
  const spell_data_t* find_spell( unsigned int id, specialization_e s ) const;
  const spell_data_t* find_spell( unsigned int id ) const;

  pet_t*      find_pet( util::string_view name ) const;
  item_t*     find_item_by_name( util::string_view name );
  item_t*     find_item_by_id( unsigned id );
  item_t*     find_item_by_use_effect_name( util::string_view name );
  action_t*   find_action( util::string_view ) const;
  cooldown_t* find_cooldown( util::string_view name ) const;
  dot_t*      find_dot     ( util::string_view name, player_t* source ) const;
  stats_t*    find_stats   ( util::string_view name ) const;
  gain_t*     find_gain    ( util::string_view name ) const;
  proc_t*     find_proc    ( util::string_view name ) const;
  benefit_t*  find_benefit ( util::string_view name ) const;
  uptime_t*   find_uptime  ( util::string_view name ) const;
  sample_data_helper_t* find_sample_data( util::string_view name ) const;
  action_priority_list_t* find_action_priority_list( util::string_view name ) const;
  int find_action_id( util::string_view name ) const;

  cooldown_t* get_cooldown( util::string_view name, action_t* action = nullptr );
  real_ppm_t* get_rppm    ( util::string_view );
  real_ppm_t* get_rppm    ( util::string_view, const spell_data_t* data, const item_t* item = nullptr );
  real_ppm_t* get_rppm    ( util::string_view, double freq, double mod = 1.0, unsigned s = RPPM_NONE );
  shuffled_rng_t* get_shuffled_rng( util::string_view name, int success_entries = 0, int total_entries = 0);
  dot_t*      get_dot     ( util::string_view name, player_t* source );
  gain_t*     get_gain    ( util::string_view name );
  proc_t*     get_proc    ( util::string_view name );
  stats_t*    get_stats   ( util::string_view name, action_t* action = nullptr );
  benefit_t*  get_benefit ( util::string_view name );
  uptime_t*   get_uptime  ( util::string_view name );
  sample_data_helper_t* get_sample_data( util::string_view name );
  action_priority_list_t* get_action_priority_list( util::string_view name, util::string_view comment = {} );
  int get_action_id( util::string_view name );


  // Virtual methods
  virtual void invalidate_cache( cache_e c );
  virtual void init();
  virtual void override_talent( util::string_view override_str );
  virtual void init_meta_gem();
  virtual void init_resources( bool force = false );
  virtual std::string init_use_item_actions( const std::string& append = std::string() );
  virtual std::string init_use_profession_actions( const std::string& append = std::string() );
  virtual std::string init_use_racial_actions( const std::string& append = std::string() );
  virtual std::vector<std::string> get_item_actions( const std::string& options = std::string() );
  virtual std::vector<std::string> get_profession_actions();
  virtual std::vector<std::string> get_racial_actions();
  virtual void init_target();
  virtual void init_race();
  virtual void init_talents();
  virtual void replace_spells();
  virtual void init_position();
  virtual void init_professions();
  virtual void init_spells();
  virtual void init_items();
  virtual void init_azerite(); /// Initialize azerite-related support structures for the actor
  virtual void init_weapon( weapon_t& );
  virtual void init_base_stats();
  virtual void init_initial_stats();
  virtual void init_defense();
  virtual void create_buffs();
  virtual void create_special_effects();
  virtual void init_special_effects();
  virtual void init_scaling();
  virtual void init_action_list() {}
  virtual void init_gains();
  virtual void init_procs();
  virtual void init_uptimes();
  virtual void init_benefits();
  virtual void init_rng();
  virtual void init_stats();
  virtual void init_distance_targeting();
  virtual void init_absorb_priority();
  virtual void init_assessors();
  virtual void init_background_actions();
  virtual void create_actions();
  virtual void init_actions();
  virtual void init_finished();
  virtual void apply_affecting_auras(action_t&);
  virtual void action_init_finished(action_t&);
  virtual bool verify_use_items() const;
  virtual void reset();
  virtual void combat_begin();
  virtual void combat_end();
  virtual void merge( player_t& other );
  virtual void datacollection_begin();
  virtual void datacollection_end();

  /// Single actor batch mode calls this every time the active (player) actor changes for all targets
  virtual void actor_changed() { }
  virtual void activate();
  virtual void deactivate();
  virtual int level() const;

  virtual double resource_regen_per_second( resource_e ) const;

  double apply_combat_rating_dr( rating_e rating, double value ) const
  {
    switch ( rating )
    {
      case RATING_LEECH:
      case RATING_SPEED:
      case RATING_AVOIDANCE:
        return item_database::curve_point_value( *dbc, DIMINISHING_RETURN_TERTIARY_CR_CURVE, value * 100.0 ) / 100.0;
      case RATING_MASTERY:
        return item_database::curve_point_value( *dbc, DIMINISHING_RETURN_SECONDARY_CR_CURVE, value );
      default:
        // Note, curve uses %-based values, not values divided by 100
        return item_database::curve_point_value( *dbc, DIMINISHING_RETURN_SECONDARY_CR_CURVE, value * 100.0 ) / 100.0;
    }
  }

  virtual double composite_melee_haste() const;
  virtual double composite_melee_speed() const;
  virtual double composite_melee_attack_power() const;
  virtual double composite_melee_attack_power_by_type(attack_power_type type ) const;
  virtual double composite_melee_hit() const;
  virtual double composite_melee_crit_chance() const;
  virtual double composite_melee_crit_chance_multiplier() const
  { return 1.0; }
  virtual double composite_melee_expertise( const weapon_t* w = nullptr ) const;
  virtual double composite_spell_haste() const;
  virtual double composite_spell_speed() const;
  virtual double composite_spell_power( school_e school ) const;
  virtual double composite_spell_crit_chance() const;
  virtual double composite_spell_crit_chance_multiplier() const
  { return 1.0; }
  virtual double composite_spell_hit() const;
  virtual double composite_mastery() const;
  virtual double composite_mastery_value() const;
  virtual double composite_damage_versatility() const;
  virtual double composite_heal_versatility() const;
  virtual double composite_mitigation_versatility() const;
  virtual double composite_leech() const;
  virtual double composite_run_speed() const;
  virtual double composite_avoidance() const;
  virtual double composite_corruption() const;
  virtual double composite_corruption_resistance() const;
  virtual double composite_total_corruption() const;
  virtual double composite_armor() const;
  virtual double composite_bonus_armor() const;
  virtual double composite_base_armor_multiplier() const; // Modify Base Besistance
  virtual double composite_armor_multiplier() const; // Modify Armor%, affects everything
  virtual double composite_miss() const;
  virtual double composite_dodge() const;
  virtual double composite_parry() const;
  virtual double composite_block() const;
  virtual double composite_block_reduction( action_state_t* s ) const;
  virtual double composite_crit_block() const;
  virtual double composite_crit_avoidance() const;
  virtual double composite_attack_power_multiplier() const;
  virtual double composite_spell_power_multiplier() const;
  virtual double matching_gear_multiplier( attribute_e /* attr */ ) const
  { return 0; }
  virtual double composite_player_multiplier   ( school_e ) const;
  virtual double composite_player_dd_multiplier( school_e,  const action_t* /* a */ = nullptr ) const { return 1; }
  virtual double composite_player_td_multiplier( school_e,  const action_t* a = nullptr ) const;
  /// Persistent multipliers that are snapshot at the beginning of the spell application/execution
  virtual double composite_persistent_multiplier( school_e ) const
  { return 1.0; }
  virtual double composite_player_target_multiplier( player_t* target, school_e school ) const;
  virtual double composite_player_heal_multiplier( const action_state_t* s ) const;
  virtual double composite_player_dh_multiplier( school_e ) const { return 1; }
  virtual double composite_player_th_multiplier( school_e ) const;
  virtual double composite_player_absorb_multiplier( const action_state_t* s ) const;
  virtual double composite_player_pet_damage_multiplier( const action_state_t* ) const;
  virtual double composite_player_target_crit_chance( player_t* target ) const;
  virtual double composite_player_critical_damage_multiplier( const action_state_t* s ) const;
  virtual double composite_player_critical_healing_multiplier() const;
  virtual double composite_mitigation_multiplier( school_e ) const;
  virtual double temporary_movement_modifier() const;
  virtual double passive_movement_modifier() const;
  virtual double composite_movement_speed() const;
  virtual double composite_attribute( attribute_e attr ) const;
  virtual double composite_attribute_multiplier( attribute_e attr ) const;
  virtual double composite_rating_multiplier( rating_e /* rating */ ) const;
  virtual double composite_rating( rating_e rating ) const;
  virtual double composite_spell_hit_rating() const
  { return composite_rating( RATING_SPELL_HIT ); }
  virtual double composite_spell_crit_rating() const
  { return composite_rating( RATING_SPELL_CRIT ); }
  virtual double composite_spell_haste_rating() const
  { return composite_rating( RATING_SPELL_HASTE ); }
  virtual double composite_melee_hit_rating() const
  { return composite_rating( RATING_MELEE_HIT ); }
  virtual double composite_melee_crit_rating() const
  { return composite_rating( RATING_MELEE_CRIT ); }
  virtual double composite_melee_haste_rating() const
  { return composite_rating( RATING_MELEE_HASTE ); }
  virtual double composite_ranged_hit_rating() const
  { return composite_rating( RATING_RANGED_HIT ); }
  virtual double composite_ranged_crit_rating() const
  { return composite_rating( RATING_RANGED_CRIT ); }
  virtual double composite_ranged_haste_rating() const
  { return composite_rating( RATING_RANGED_HASTE ); }
  virtual double composite_mastery_rating() const
  { return composite_rating( RATING_MASTERY ); }
  virtual double composite_expertise_rating() const
  { return composite_rating( RATING_EXPERTISE ); }
  virtual double composite_dodge_rating() const
  { return composite_rating( RATING_DODGE ); }
  virtual double composite_parry_rating() const
  { return composite_rating( RATING_PARRY ); }
  virtual double composite_block_rating() const
  { return composite_rating( RATING_BLOCK ); }
  virtual double composite_damage_versatility_rating() const
  { return composite_rating( RATING_DAMAGE_VERSATILITY ); }
  virtual double composite_heal_versatility_rating() const
  { return composite_rating( RATING_HEAL_VERSATILITY ); }
  virtual double composite_mitigation_versatility_rating() const
  { return composite_rating( RATING_MITIGATION_VERSATILITY ); }
  virtual double composite_leech_rating() const
  { return composite_rating( RATING_LEECH ); }
  virtual double composite_speed_rating() const
  { return composite_rating( RATING_SPEED ); }
  virtual double composite_avoidance_rating() const
  { return composite_rating( RATING_AVOIDANCE ); }
  virtual double composite_corruption_rating() const
  { return composite_rating( RATING_CORRUPTION ); }
  virtual double composite_corruption_resistance_rating() const
  { return composite_rating( RATING_CORRUPTION_RESISTANCE ); }

  /// Total activity time for this actor during the iteration
  virtual timespan_t composite_active_time() const
  { return iteration_fight_length; }

  virtual void interrupt();
  virtual void halt();
  virtual void moving();
  virtual void finish_moving();
  virtual void stun();
  virtual void clear_debuffs();
  virtual void trigger_ready();
  virtual void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false );
  virtual void schedule_off_gcd_ready( timespan_t delta_time = timespan_t::from_millis( 100 ) );
  virtual void schedule_cwc_ready( timespan_t delta_time = timespan_t::from_millis( 100 ) );
  virtual void arise();
  virtual void demise();
  virtual timespan_t available() const
  { return timespan_t::from_seconds( 0.1 ); }
  virtual action_t* select_action( const action_priority_list_t&, execute_type type = execute_type::FOREGROUND, const action_t* context = nullptr );
  virtual action_t* execute_action();

  virtual void   regen( timespan_t periodicity = timespan_t::from_seconds( 0.25 ) );
  virtual double resource_gain( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr );
  virtual double resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr );
  virtual void   recalculate_resource_max( resource_e resource_type, gain_t* g = nullptr );
  // Check whether the player has enough of a given resource.
  // The caller needs to ensure current resources are up to date (in particular with dynamic regen).
  virtual bool   resource_available( resource_e resource_type, double cost ) const;
  virtual resource_e primary_resource() const
  { return RESOURCE_NONE; }
  virtual role_e   primary_role() const;
  virtual stat_e convert_hybrid_stat( stat_e s ) const
  { return s; }
  virtual stat_e normalize_by() const;
  virtual double health_percentage() const;
  virtual double max_health() const;
  virtual double current_health() const;
  virtual timespan_t time_to_percent( double percent ) const;
  virtual void cost_reduction_gain( school_e school, double amount, gain_t* g = nullptr, action_t* a = nullptr );
  virtual void cost_reduction_loss( school_e school, double amount, action_t* a = nullptr );
  virtual void collect_resource_timeline_information();

  virtual void assess_damage( school_e, result_amount_type, action_state_t* );
  virtual void target_mitigation( school_e, result_amount_type, action_state_t* );
  virtual void assess_damage_imminent_pre_absorb( school_e, result_amount_type, action_state_t* );
  virtual void assess_damage_imminent( school_e, result_amount_type, action_state_t* );
  virtual void do_damage( action_state_t* );
  virtual void assess_heal( school_e, result_amount_type, action_state_t* );

  virtual bool taunt( player_t* /* source */ ) { return false; }

  virtual void  summon_pet( util::string_view name, timespan_t duration = timespan_t::zero() );
  virtual void dismiss_pet( util::string_view name );

  virtual std::unique_ptr<expr_t> create_expression( util::string_view name );
  virtual std::unique_ptr<expr_t> create_action_expression( action_t&, util::string_view name );
  virtual std::unique_ptr<expr_t> create_resource_expression( util::string_view name );

  virtual void create_options();
  void recreate_talent_str( talent_format format = talent_format::NUMBERS );
  virtual std::string create_profile( save_e = SAVE_ALL );

  virtual void copy_from( player_t* source );

  virtual action_t* create_action( util::string_view name, const std::string& options );
  virtual void      create_pets() { }
  virtual pet_t*    create_pet( util::string_view name,  util::string_view type = "" );

  virtual void armory_extensions( const std::string& /* region */, const std::string& /* server */, const std::string& /* character */,
                                  cache::behavior_e /* behavior */ = cache::players() )
  {}

  virtual void do_dynamic_regen();

  /**
   * Returns owner if available, otherwise the player itself.
   */
  virtual const player_t* get_owner_or_self() const
  { return this; }

  player_t* get_owner_or_self()
  { return const_cast<player_t*>(static_cast<const player_t*>(this) -> get_owner_or_self()); }

  // T18 Hellfire Citadel class trinket detection
  virtual bool has_t18_class_trinket() const;

  // Targetdata stuff
  virtual actor_target_data_t* get_target_data( player_t* /* target */ ) const
  { return nullptr; }

  // Opportunity to perform any stat fixups before analysis
  virtual void pre_analyze_hook() {}

  /* New stuff */
  virtual double composite_player_vulnerability( school_e ) const;

  virtual void activate_action_list( action_priority_list_t* a, execute_type type = execute_type::FOREGROUND );

  virtual void analyze( sim_t& );

  scaling_metric_data_t scaling_for_metric( scale_metric_e metric ) const;

  virtual action_t* create_proc_action( util::string_view /* name */, const special_effect_t& /* effect */ )
  { return nullptr; }
  virtual bool requires_data_collection() const
  { return active_during_iteration; }

  rng::rng_t& rng();
  rng::rng_t& rng() const;
  virtual timespan_t time_to_move() const;
  virtual void trigger_movement( double distance, movement_direction_type);
  virtual void update_movement( timespan_t duration );
  virtual void teleport( double yards, timespan_t duration = timespan_t::zero() );
  virtual movement_direction_type movement_direction() const
  { return current.movement_direction; }

  virtual void reset_auto_attacks( timespan_t delay = timespan_t::zero() );

  virtual void acquire_target( retarget_source /* event */, player_t* /* context */ = nullptr );

  // Various default values for the actor
  virtual std::string default_potion() const
  { return ""; }
  virtual std::string default_flask() const
  { return ""; }
  virtual std::string default_food() const
  { return ""; }
  virtual std::string default_rune() const
  { return ""; }

  /**
   * Default attack power type to use for value computation.
   *
   * Defaults to BfA "new style" base_power + mh_weapon_dps * coefficient, can be overridden in
   * class modules. Used by actions as a "last resort" to determine what attack power type drives
   * the value calculation of the ability.
   */
  virtual attack_power_type default_ap_type() const
  { return attack_power_type::DEFAULT; }

  // JSON Report extension. Overridable in class methods. Root element is an object assigned for
  // each JSON player object under "custom" property.
  virtual void output_json_report( js::JsonOutput& /* root */ ) const
  { }

private:
  std::vector<unsigned> active_dots;
public:
  void add_active_dot( unsigned action_id );
  void remove_active_dot( unsigned action_id );
  unsigned get_active_dots( unsigned action_id ) const;
  virtual void adjust_dynamic_cooldowns();
  virtual void adjust_global_cooldown(gcd_haste_type type );
  virtual void adjust_auto_attack(gcd_haste_type type );

  // 8.2 Vision of Perfection essence
  virtual void vision_of_perfection_proc();

private:
  void do_update_movement( double yards );
  void check_resource_callback_deactivation();
  void reset_resource_callbacks();
  void check_resource_change_for_callback(resource_e resource, double previous_amount, double previous_pct_points);
public:


  // Figure out another actor, by name. Prioritizes pets > harmful targets >
  // other players. Used by "actor.<name>" expression currently.
  virtual player_t* actor_by_name_str( util::string_view ) const;

  // Wicked resource threshold trigger-ready stuff .. work in progress
  event_t* resource_threshold_trigger;
  std::vector<double> resource_thresholds;
  void min_threshold_trigger();

  // Assessors

  // Outgoing damage assessors, pipeline is invoked on all objects passing through
  // action_t::assess_damage.
  assessor::state_assessor_pipeline_t assessor_out_damage;

  /// Start-of-combat effects
  using combat_begin_fn_t = std::function<void(player_t*)>;
  std::vector<combat_begin_fn_t> combat_begin_functions;

  /// Register a buff that triggers at the beginning of combat
  void register_combat_begin( buff_t* b );
  /// Register an action that triggers at the beginning of combat
  void register_combat_begin( action_t* a );
  /// Register a custom function that triggers at the beginning of combat
  void register_combat_begin( const combat_begin_fn_t& fn );
  /// Register a resource gain that triggers at the beginning of combat
  void register_combat_begin( double amount, resource_e resource, gain_t* g = nullptr );

  void update_off_gcd_ready();
  void update_cast_while_casting_ready();

  // Stuff, testing
  std::vector<spawner::base_actor_spawner_t*> spawners;

  spawner::base_actor_spawner_t* find_spawner( util::string_view id ) const;
  int nth_iteration() const;

  friend void format_to( const player_t&, fmt::format_context::iterator );
};
