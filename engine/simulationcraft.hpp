// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SIMULATIONCRAFT_H
#define SIMULATIONCRAFT_H

#define SC_MAJOR_VERSION "830"
#define SC_MINOR_VERSION "02"
#define SC_VERSION ( SC_MAJOR_VERSION "-" SC_MINOR_VERSION )
#define SC_BETA 0
#if SC_BETA
#define SC_BETA_STR "bfa"
#endif

// Platform, compiler and general configuration
#include "config.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <numeric>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <typeinfo>
#include <vector>
#include <bitset>
#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <atomic>
#include <random>
#if defined( SC_OSX )
#include <Availability.h>
#endif

// Needed for usleep in engine/interface/sc_bcp_api.cpp when default apikey builds are done
#if ! defined( SC_WINDOWS )
#include <unistd.h>
#endif

// Forward Declarations =====================================================

struct absorb_buff_t;
struct action_callback_t;
struct action_priority_t;
struct action_priority_list_t;
struct action_state_t;
struct action_t;
struct actor_t;
struct alias_t;
struct attack_t;
struct benefit_t;
struct buff_t;
struct callback_t;
struct cooldown_t;
struct cost_reduction_buff_t;
class dbc_t;
struct dot_t;
struct event_t;
struct expr_t;
struct gain_t;
struct heal_t;
struct item_t;
struct instant_absorb_t;
struct module_t;
struct pet_t;
struct player_t;
struct plot_t;
struct proc_t;
struct reforge_plot_t;
struct scaling_t;
struct sim_t;
struct special_effect_t;
struct spell_data_t;
struct spell_id_t;
struct spelleffect_data_t;
struct spell_t;
struct stats_t;
struct stat_buff_t;
struct stat_pair_t;
struct travel_event_t;
struct xml_node_t;
class xml_writer_t;
struct real_ppm_t;
struct shuffled_rng_t;
struct ground_aoe_event_t;
namespace spawner {
class base_actor_spawner_t;
}
namespace highchart {
  struct chart_t;
}

#include "dbc/data_enums.hh"
#include "dbc/data_definitions.hh"
#include "util/utf8-cpp/utf8.h"

// string formatting library
#include "util/fmt/format.h"
#include "util/fmt/ostream.h"
#include "util/fmt/printf.h"

// GSL-Lite: Guideline Support Library, light version
// Lib to assist with CPP Core Guidelines.
#include "util/gsl-lite/gsl-lite.hpp"

// Time class representing ingame time
#include "sc_timespan.hpp"

// Generic programming tools
#include "util/generic.hpp"

// Sample Data
#include "util/sample_data.hpp"

// Timeline
#include "util/timeline.hpp"

// Random Number Generators
#include "util/rng.hpp"

// mutex, thread
#include "util/concurrency.hpp"

#include "sc_enums.hpp"

// Cache Control ============================================================
#include "util/cache.hpp"

#include "sim/sc_profileset.hpp"

#include "player/artifact_data.hpp"
#include "player/azerite_data.hpp"

// Legion-specific "pantheon trinket" system
#include "sim/x6_pantheon.hpp"

#include "action/sc_action_state.hpp"




// Utilities ================================================================

#include "sc_util.hpp"

#include "util/stopwatch.hpp"
#include "sim/sc_option.hpp"


// Include DBC Module
#include "dbc/dbc.hpp"

// Include IO Module
#include "util/io.hpp"

// Report ===================================================================

#include "report/sc_report.hpp"

#include "sim/artifact_power.hpp"

/* Luxurious sample data container with automatic merge/analyze,
 * intended to be used in class modules for custom reporting.
 * Iteration based sampling
 */
struct luxurious_sample_data_t : public extended_sample_data_t, private noncopyable
{
  luxurious_sample_data_t( player_t& p, std::string n );

  void add( double x )
  { buffer_value += x; }

  void datacollection_begin()
  {
    reset_buffer();
  }
  void datacollection_end()
  {
    write_buffer_as_sample();
  }
  player_t& player;
private:
  double buffer_value;
  void write_buffer_as_sample()
  {
    extended_sample_data_t::add( buffer_value );
    reset_buffer();
  }
  void reset_buffer()
  {
    buffer_value = 0.0;
  }
};

// Raid Event

struct raid_event_t : private noncopyable
{
public:
  sim_t* sim;
  std::string name;
  std::string type;
  int64_t num_starts;
  timespan_t first, last;
  double first_pct, last_pct;
  timespan_t cooldown;
  timespan_t cooldown_stddev;
  timespan_t cooldown_min;
  timespan_t cooldown_max;
  timespan_t duration;
  timespan_t duration_stddev;
  timespan_t duration_min;
  timespan_t duration_max;

  // Player filter options
  double     distance_min; // Minimal player distance
  double     distance_max; // Maximal player distance
  bool players_only; // Don't affect pets
  bool force_stop; // Stop immediately at last/last_pct
  double player_chance; // Chance for individual player to be affected by raid event

  std::string affected_role_str;
  role_e     affected_role;
  std::string player_if_expr_str;

  timespan_t saved_duration;
  std::vector<player_t*> affected_players;
  std::unordered_map<size_t, std::unique_ptr<expr_t>> player_expressions;
  std::vector<std::unique_ptr<option_t>> options;

  raid_event_t( sim_t*, const std::string& );

  virtual ~raid_event_t() {}

  virtual bool filter_player( const player_t* );

  void add_option( std::unique_ptr<option_t> new_option )
  { options.insert( options.begin(), std::move(new_option) ); }
  timespan_t cooldown_time();
  timespan_t duration_time();
  timespan_t next_time() const;
  timespan_t until_next() const;
  timespan_t remains() const;
  bool up() const;
  double distance() { return distance_max; }
  double min_distance() { return distance_min; }
  double max_distance() { return distance_max; }
  void schedule();
  virtual void reset();
  void parse_options( const std::string& options_str );
  static std::unique_ptr<raid_event_t> create( sim_t* sim, const std::string& name, const std::string& options_str );
  static void init( sim_t* );
  static void reset( sim_t* );
  static void combat_begin( sim_t* );
  static void combat_end( sim_t* ) {}
  static double evaluate_raid_event_expression(sim_t* s, std::string& type, std::string& filter,
      bool test_filter = false);

  static bool has_raid_event( sim_t*, const std::string& type );
private:
  virtual void _start() = 0;
  virtual void _finish() = 0;
  void activate();
  void deactivate();
  void combat_begin();
  void start();
  void finish();

  bool is_up;
  enum class activation_status_e
  {
    // three different states so we can detect when raid event is deactivated before it is activated.
    not_yet_activated,
    activated,
    deactivated
  } activation_status;
  event_t* cooldown_event;
  event_t* duration_event;
  event_t* start_event;
  event_t* end_event;
};
std::ostream& operator<<(std::ostream&, const raid_event_t&);

// Gear Stats ===============================================================

#include "player/gear_stats.hpp"

#include "player/sc_actor_pair.hpp"

struct actor_target_data_t : public actor_pair_t, private noncopyable
{
  struct atd_debuff_t
  {
    buff_t* mark_of_doom;
    buff_t* poisoned_dreams;
    buff_t* fel_burn;
    buff_t* flame_wreath;
    buff_t* thunder_ritual;
    buff_t* brutal_haymaker;
    buff_t* taint_of_the_sea;
    buff_t* solar_collapse;
    buff_t* volatile_magic;
    buff_t* maddening_whispers;
    buff_t* shadow_blades;
    // BFA - Azerite
    buff_t* azerite_globules;
    buff_t* dead_ahead;
    buff_t* battlefield_debuff;
    // BFA - Trinkets
    buff_t* wasting_infection;
    buff_t* everchill;
    buff_t* choking_brine;
    buff_t* razor_coral;
    buff_t* conductive_ink;
    buff_t* luminous_algae;
    buff_t* infinite_stars;
    buff_t* psyche_shredder;
    // BFA - Essences
    buff_t* blood_of_the_enemy;
    buff_t* condensed_lifeforce;
    buff_t* focused_resolve;
    buff_t* reaping_flames_tracker;
  } debuff;

  struct atd_dot_t
  {
  } dot;

  actor_target_data_t( player_t* target, player_t* source );
};

#include "util/sc_uptime.hpp"
#include "buff/sc_buff.hpp"

#include "sim/sc_expressions.hpp"

// Spell query expression types =============================================

enum expr_data_e
{
  DATA_SPELL = 0,
  DATA_TALENT,
  DATA_EFFECT,
  DATA_TALENT_SPELL,
  DATA_CLASS_SPELL,
  DATA_RACIAL_SPELL,
  DATA_MASTERY_SPELL,
  DATA_SPECIALIZATION_SPELL,
  DATA_ARTIFACT_SPELL,
  DATA_AZERITE_SPELL,
};

struct spell_data_expr_t
{
  std::string name_str;
  sim_t* sim;
  expr_data_e data_type;
  bool effect_query;

  expression::token_e result_tok;
  double result_num;
  std::vector<uint32_t> result_spell_list;
  std::string result_str;

  spell_data_expr_t( sim_t* sim, const std::string& n,
                     expr_data_e dt = DATA_SPELL, bool eq = false,
                     expression::token_e t = expression::TOK_UNKNOWN )
    : name_str( n ),
      sim( sim ),
      data_type( dt ),
      effect_query( eq ),
      result_tok( t ),
      result_num( 0 ),
      result_spell_list(),
      result_str( "" )
  {
  }
  spell_data_expr_t( sim_t* sim, const std::string& n, double constant_value )
    : name_str( n ),
      sim( sim ),
      data_type( DATA_SPELL ),
      effect_query( false ),
      result_tok( expression::TOK_NUM ),
      result_num( constant_value ),
      result_spell_list(),
      result_str( "" )
  {
  }
  spell_data_expr_t( sim_t* sim, const std::string& n,
                     const std::string& constant_value )
    : name_str( n ),
      sim( sim ),
      data_type( DATA_SPELL ),
      effect_query( false ),
      result_tok( expression::TOK_STR ),
      result_num( 0.0 ),
      result_spell_list(),
      result_str( constant_value )
  {
  }
  spell_data_expr_t( sim_t* sim, const std::string& n,
                     const std::vector<uint32_t>& constant_value )
    : name_str( n ),
      sim( sim ),
      data_type( DATA_SPELL ),
      effect_query( false ),
      result_tok( expression::TOK_SPELL_LIST ),
      result_num( 0.0 ),
      result_spell_list( constant_value ),
      result_str( "" )
  {
  }
  virtual ~spell_data_expr_t()
  {
  }
  virtual int evaluate()
  {
    return result_tok;
  }
  const char* name()
  {
    return name_str.c_str();
  }

  virtual std::vector<uint32_t> operator|( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator&( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator-( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> operator<( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator<=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> in( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> not_in( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  static spell_data_expr_t* parse( sim_t* sim, const std::string& expr_str );
  static spell_data_expr_t* create_spell_expression( sim_t* sim, const std::string& name_str );
};


// Iteration data entry for replayability
struct iteration_data_entry_t
{
  double   metric;
  uint64_t seed;
  uint64_t iteration;
  double   iteration_length;
  std::vector <uint64_t> target_health;

  iteration_data_entry_t( double m, double il, uint64_t s, uint64_t h, uint64_t i ) :
    metric( m ), seed( s ), iteration( i ), iteration_length( il )
  { target_health.push_back( h ); }

  iteration_data_entry_t( double m, double il, uint64_t s, uint64_t i ) :
    metric( m ), seed( s ), iteration( i ), iteration_length( il )
  { }

  void add_health( uint64_t h )
  { target_health.push_back( h ); }
};

// Simulation Setup =========================================================

struct player_description_t
{
  // Add just enough to describe a player

  // name, class, talents, gear, professions, actions explicitly stored
  std::string name;
  // etc

  // flesh out API, these functions cannot depend upon sim_t
  // ideally they remain static, but if not then move to sim_control_t
  static void load_bcp    ( player_description_t& /*etc*/ );
  static void load_wowhead( player_description_t& /*etc*/ );
};

struct combat_description_t
{
  std::string name;
  int target_seconds;
  std::string raid_events;
  // etc
};

struct sim_control_t
{
  combat_description_t combat;
  std::vector<player_description_t> players;
  option_db_t options;
};

struct sim_progress_t
{
  int current_iterations;
  int total_iterations;
  double pct() const
  { return std::min( 1.0, current_iterations / static_cast<double>(total_iterations) ); }
};

#include "sim/progress_bar.hpp"

#include "sim/event_manager.hpp"

#include "sim/sc_sim.hpp"

// Module ===================================================================

struct module_t
{
  player_e type;

  module_t( player_e t ) :
    type( t ) {}

  virtual ~module_t() {}
  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const = 0;
  virtual bool valid() const = 0;
  virtual void init( player_t* ) const = 0;
  virtual void static_init() const { }
  virtual void register_hotfixes() const { }
  virtual void combat_begin( sim_t* ) const = 0;
  virtual void combat_end( sim_t* ) const = 0;

  static const module_t* death_knight();
  static const module_t* demon_hunter();
  static const module_t* druid();
  static const module_t* hunter();
  static const module_t* mage();
  static const module_t* monk();
  static const module_t* paladin();
  static const module_t* priest();
  static const module_t* rogue();
  static const module_t* shaman();
  static const module_t* warlock();
  static const module_t* warrior();
  static const module_t* enemy();
  static const module_t* tank_dummy_enemy();
  static const module_t* heal_enemy();

  static const module_t* get( player_e t )
  {
    switch ( t )
    {
      case DEATH_KNIGHT: return death_knight();
      case DEMON_HUNTER: return demon_hunter();
      case DRUID:        return druid();
      case HUNTER:       return hunter();
      case MAGE:         return mage();
      case MONK:         return monk();
      case PALADIN:      return paladin();
      case PRIEST:       return priest();
      case ROGUE:        return rogue();
      case SHAMAN:       return shaman();
      case WARLOCK:      return warlock();
      case WARRIOR:      return warrior();
      case ENEMY:        return enemy();
      case TANK_DUMMY:   return tank_dummy_enemy();
      default: break;
    }
    return nullptr;
  }
  static const module_t* get( const std::string& n )
  {
    return get( util::parse_player_type( n ) );
  }
  static void init()
  {
    for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; i++ )
    {
      const module_t* m = get( i );
      if ( m )
      {
        m -> static_init();
        m -> register_hotfixes();
      }
    }
  }
};

// Scaling ==================================================================

struct scaling_t
{
  mutex_t mutex;
  sim_t* sim;
  sim_t* baseline_sim;
  sim_t* ref_sim;
  sim_t* delta_sim;
  sim_t* ref_sim2;
  sim_t* delta_sim2;
  stat_e scale_stat;
  double scale_value;
  double scale_delta_multiplier;
  int    calculate_scale_factors;
  int    center_scale_delta;
  int    positive_scale_delta;
  int    scale_lag;
  double scale_factor_noise;
  int    normalize_scale_factors;
  int    debug_scale_factors;
  std::string scale_only_str;
  stat_e current_scaling_stat;
  int num_scaling_stats, remaining_scaling_stats;
  std::string scale_over;
  scale_metric_e scaling_metric;
  std::string scale_over_player;

  // Gear delta for determining scale factors
  gear_stats_t stats;

  scaling_t( sim_t* s );

  void init_deltas();
  void analyze();
  void analyze_stats();
  void analyze_ability_stats( stat_e, double, player_t*, player_t*, player_t* );
  void analyze_lag();
  void normalize();
  double progress( std::string& phase, std::string* detailed = nullptr );
  void create_options();
  bool has_scale_factors();
};

// Plot =====================================================================

struct plot_t
{
public:
  sim_t* sim;
  std::string dps_plot_stat_str;
  double dps_plot_step;
  int    dps_plot_points;
  int    dps_plot_iterations;
  double dps_plot_target_error;
  int    dps_plot_debug;
  stat_e current_plot_stat;
  int    num_plot_stats, remaining_plot_stats, remaining_plot_points;
  bool   dps_plot_positive, dps_plot_negative;

  plot_t( sim_t* s );
  void analyze();
  double progress( std::string& phase, std::string* detailed = nullptr );
private:
  void analyze_stats();
  void write_output_file();
  void create_options();
};

// Reforge Plot =============================================================

struct reforge_plot_t
{
  sim_t* sim;
  sim_t* current_reforge_sim;
  std::string reforge_plot_stat_str;
  std::vector<stat_e> reforge_plot_stat_indices;
  int    reforge_plot_step;
  int    reforge_plot_amount;
  int    reforge_plot_iterations;
  double reforge_plot_target_error;
  int    reforge_plot_debug;
  int    current_stat_combo;
  int    num_stat_combos;

  reforge_plot_t( sim_t* s );

  void generate_stat_mods( std::vector<std::vector<int> > &stat_mods,
                           const std::vector<stat_e> &stat_indices,
                           int cur_mod_stat,
                           std::vector<int> cur_stat_mods );
  void analyze();
  void analyze_stats();
  double progress( std::string& phase, std::string* detailed = nullptr );
private:
  void write_output_file();
  void create_options();
};

#include "util/plot_data.hpp"

#include "sim/event.hpp"

#include "player/rating.hpp"

// Weapon ===================================================================
#include "player/weapon.hpp"

// Special Effect ===========================================================

// A special effect callback that will be scoped by the pure virtual valid() method of the class.
// If the implemented valid() returns false, the special effect initializer callbacks will not be
// invoked.
//
// Scoped callbacks are prioritized statically.
//
// Special effects will be selected and initialize automatically for the actor from the highest
// priority class.  Currently PRIORITY_CLASS is used by class-scoped special effects callback
// objects that use the class scope (i.e., player_e) validator, and PRIORITY_SPEC is used by
// specialization-scoped special effect callback objects. The separation of the two allows, for
// example, the creation of a class-scoped special effect, and then an additional
// specialization-scoped special effect for a single specialization for the class.
//
// The PRIORITY_DEFAULT value is used for old-style callbacks (i.e., static functions or
// string-based initialization defined in sc_unique_gear.cpp).
struct scoped_callback_t
{
  enum priority
  {
    PRIORITY_DEFAULT = 0U, // Old-style callbacks, and any kind of "last resort" special effect
    PRIORITY_CLASS,        // Class-filtered callbacks
    PRIORITY_SPEC          // Specialization-filtered callbacks
  };

  priority priority;

  scoped_callback_t() : priority( PRIORITY_DEFAULT )
  { }

  scoped_callback_t( enum priority p ) : priority( p )
  { }

  virtual ~scoped_callback_t()
  { }

  // Validate special effect against conditions. Return value of false indicates that the
  // initializer should not be invoked.
  virtual bool valid( const special_effect_t& ) const = 0;

  // Initialize the special effect.
  virtual void initialize( special_effect_t& ) = 0;
};

struct special_effect_t
{
  const item_t* item;
  player_t* player;

  special_effect_e type;
  special_effect_source_e source;
  std::string name_str, trigger_str, encoding_str;
  unsigned proc_flags_; /* Proc-by */
  unsigned proc_flags2_; /* Proc-on (hit/damage/...) */
  stat_e stat;
  school_e school;
  int max_stacks;
  double stat_amount, discharge_amount, discharge_scaling;
  double proc_chance_;
  double ppm_;
  unsigned rppm_scale_;
  double rppm_modifier_;
  int rppm_blp_;
  timespan_t duration_, cooldown_, tick;
  bool cost_reduction;
  int refresh;
  bool chance_to_discharge;
  unsigned int override_result_es_mask;
  unsigned result_es_mask;
  bool reverse;
  int reverse_stack_reduction;
  int aoe;
  bool proc_delay;
  bool unique;
  bool weapon_proc;
  unsigned spell_id, trigger_spell_id;
  action_t* execute_action; // Allows custom action to be executed on use
  buff_t* custom_buff; // Allows custom action

  bool action_disabled, buff_disabled;

  // Old-style function-based custom second phase initializer (callback)
  std::function<void(special_effect_t&)> custom_init;
  // New-style object-based custom second phase initializer
  std::vector<scoped_callback_t*> custom_init_object;
  // Activation callback, if set, called when an actor becomes active
  std::function<void(void)> activation_cb;
  // Link to an SpellItemEnchantment entry, set for various "enchant"-based special effects
  // (Enchants, Gems, Addons)
  const item_enchantment_data_t* enchant_data;

  special_effect_t( player_t* p ) :
    item( nullptr ), player( p ),
    name_str()
  { reset(); }

  special_effect_t( const item_t* item );

  // Uses a custom initialization callback or object
  bool is_custom() const
  { return custom_init || custom_init_object.size() > 0; }

  // Forcefully disable creation of an (autodetected) buff or action. This is necessary in scenarios
  // where the autodetection decides to create an invalid action or buff due to the spell data.
  void disable_action()
  { action_disabled = true; }
  void disable_buff()
  { buff_disabled = true; }

  void reset();
  std::string to_string() const;
  bool active() { return stat != STAT_NONE || school != SCHOOL_NONE || execute_action; }

  const spell_data_t* driver() const;
  const spell_data_t* trigger() const;
  std::string name() const;
  std::string cooldown_name() const;

  // On-use item cooldown handling accessories
  int cooldown_group() const;
  std::string cooldown_group_name() const;
  timespan_t cooldown_group_duration() const;

  // Buff related functionality
  buff_t* create_buff() const;
  special_effect_buff_e buff_type() const;
  int max_stack() const;

  bool is_stat_buff() const;
  stat_e stat_type() const;
  stat_buff_t* initialize_stat_buff() const;

  bool is_absorb_buff() const;
  absorb_buff_t* initialize_absorb_buff() const;

  // Action related functionality
  action_t* create_action() const;
  special_effect_action_e action_type() const;
  bool is_offensive_spell_action() const;
  spell_t* initialize_offensive_spell_action() const;
  bool is_heal_action() const;
  heal_t* initialize_heal_action() const;
  bool is_attack_action() const;
  attack_t* initialize_attack_action() const;
  bool is_resource_action() const;
  spell_t* initialize_resource_action() const;

  /* Accessors for driver specific features of the proc; some are also used for on-use effects */
  unsigned proc_flags() const;
  unsigned proc_flags2() const;
  double ppm() const;
  double rppm() const;
  unsigned rppm_scale() const;
  double rppm_modifier() const;
  double proc_chance() const;
  timespan_t cooldown() const;

  /* Accessors for buff specific features of the proc. */
  timespan_t duration() const;
  timespan_t tick_time() const;
};

inline std::ostream& operator<<(std::ostream &os, const special_effect_t& x);
// Item =====================================================================

struct stat_pair_t
{
  stat_e stat;
  int value;

  stat_pair_t() : stat( STAT_NONE ), value( 0 )
  { }

  stat_pair_t( stat_e s, int v ) : stat( s ), value( v )
  { }
};

struct item_t
{
  sim_t* sim;
  player_t* player;
  slot_e slot, parent_slot;
  bool unique, unique_addon, is_ptr;

  // Structure contains the "parsed form" of this specific item, be the data
  // from user options, or a data source such as the Blizzard API, or Wowhead
  struct parsed_input_t
  {
    int                                              item_level;
    unsigned                                         enchant_id;
    unsigned                                         addon_id;
    int                                              armor;
    unsigned                                         azerite_level;
    std::array<int, MAX_ITEM_STAT>                   stat_val;
    std::array<int, MAX_GEM_SLOTS>                   gem_id;
    std::array<std::vector<unsigned>, MAX_GEM_SLOTS> gem_bonus_id;
    std::array<unsigned, MAX_GEM_SLOTS>              gem_ilevel;
    std::array<int, MAX_GEM_SLOTS>                   gem_actual_ilevel;
    std::array<int, MAX_GEM_SLOTS>                   gem_color;
    std::vector<int>                                 bonus_id;
    std::vector<stat_pair_t>                         gem_stats, meta_gem_stats, socket_bonus_stats;
    std::string                                      encoded_enchant;
    std::vector<stat_pair_t>                         enchant_stats;
    std::string                                      encoded_addon;
    std::vector<stat_pair_t>                         addon_stats;
    item_data_t                                      data;
    auto_dispose< std::vector<special_effect_t*> >   special_effects;
    std::vector<std::string>                         source_list;
    timespan_t                                       initial_cd;
    unsigned                                         drop_level;
    std::vector<unsigned>                            azerite_ids;

    parsed_input_t() :
      item_level( 0 ), enchant_id( 0 ), addon_id( 0 ),
      armor( 0 ), azerite_level( 0 ), data(), initial_cd( timespan_t::zero() ), drop_level( 0 )
    {
      range::fill( data.stat_type_e, -1 );
      range::fill( data.stat_alloc, 0 );
      range::fill( stat_val, 0 );
      range::fill( gem_id, 0 );
      range::fill( bonus_id, 0 );
      range::fill( gem_color, SOCKET_COLOR_NONE );
      range::fill( gem_ilevel, 0 );
      range::fill( gem_actual_ilevel, 0 );
    }
  } parsed;

  std::shared_ptr<xml_node_t> xml;

  std::string name_str;
  std::string icon_str;

  std::string source_str;
  std::string options_str;

  // Option Data
  std::string option_name_str;
  std::string option_stats_str;
  std::string option_gems_str;
  std::string option_enchant_str;
  std::string option_addon_str;
  std::string option_equip_str;
  std::string option_use_str;
  std::string option_weapon_str;
  std::string option_lfr_str;
  std::string option_warforged_str;
  std::string option_heroic_str;
  std::string option_mythic_str;
  std::string option_armor_type_str;
  std::string option_ilevel_str;
  std::string option_quality_str;
  std::string option_data_source_str;
  std::string option_enchant_id_str;
  std::string option_addon_id_str;
  std::string option_gem_id_str;
  std::string option_gem_bonus_id_str;
  std::string option_gem_ilevel_str;
  std::string option_bonus_id_str;
  std::string option_initial_cd_str;
  std::string option_drop_level_str;
  std::string option_azerite_powers_str;
  std::string option_azerite_level_str;
  double option_initial_cd;

  // Extracted data
  gear_stats_t base_stats, stats;

  item_t() : sim( nullptr ), player( nullptr ), slot( SLOT_INVALID ), parent_slot( SLOT_INVALID ),
    unique( false ), unique_addon( false ), is_ptr( false ),
    parsed(), xml(), option_initial_cd(0)
  { }

  item_t( player_t*, const std::string& options_str );

  bool active() const;
  const char* name() const;
  std::string full_name() const;
  const char* slot_name() const;
  weapon_t* weapon() const;
  void init();
  void parse_options();
  bool initialize_data(); // Initializes item data from a data source
  inventory_type inv_type() const;

  bool is_matching_type() const;
  bool is_valid_type() const;
  bool socket_color_match() const;

  unsigned item_level() const;
  unsigned base_item_level() const;
  stat_e stat( size_t idx ) const;
  int stat_value( size_t idx ) const;
  gear_stats_t total_stats() const;
  bool has_item_stat( stat_e stat ) const;

  std::string encoded_item() const;
  std::string encoded_comment();

  std::string encoded_stats() const;
  std::string encoded_weapon() const;
  std::string encoded_gems() const;
  std::string encoded_enchant() const;
  std::string encoded_addon() const;

  void decode_stats();
  void decode_gems();
  void decode_enchant();
  void decode_addon();
  void decode_weapon();
  void decode_warforged();
  void decode_lfr();
  void decode_heroic();
  void decode_mythic();
  void decode_armor_type();
  void decode_ilevel();
  void decode_quality();
  void decode_data_source();
  void decode_equip_effect();
  void decode_use_effect();


  bool verify_slot();

  void init_special_effects();

  static bool download_item( item_t& );

  static std::vector<stat_pair_t> str_to_stat_pair( const std::string& stat_str );
  static std::string stat_pairs_to_str( const std::vector<stat_pair_t>& stat_pairs );

  std::string to_string() const;
  std::string item_stats_str() const;
  std::string weapon_stats_str() const;
  std::string gem_stats_str() const;
  std::string socket_bonus_stats_str() const;
  std::string enchant_stats_str() const;
  bool has_stats() const;
  bool has_special_effect( special_effect_source_e source = SPECIAL_EFFECT_SOURCE_NONE, special_effect_e type = SPECIAL_EFFECT_NONE ) const;
  bool has_use_special_effect() const
  { return has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ); }
  bool has_scaling_stat_bonus_id() const;

  const special_effect_t* special_effect( special_effect_source_e source = SPECIAL_EFFECT_SOURCE_NONE, special_effect_e type = SPECIAL_EFFECT_NONE ) const;
  const special_effect_t* special_effect_with_name( const std::string& name, special_effect_source_e source = SPECIAL_EFFECT_SOURCE_NONE, special_effect_e type = SPECIAL_EFFECT_NONE ) const;
};
std::ostream& operator<<(std::ostream&, const item_t&);

// Benefit ==================================================================

struct benefit_t : private noncopyable
{
private:
  int up, down;
public:
  // This is initialized in SIMPLE mode. Only change mode for infrequent procs to keep memory usage reasonable.
  extended_sample_data_t ratio;
  const std::string name_str;
  sim_t& sim;

  explicit benefit_t( sim_t& s, const std::string& n ) :
    up( 0 ),
    down( 0 ),
    ratio( "Ratio", true ),
    name_str( n ),
    sim( s )
  {}

  void update( bool is_up )
  { if ( is_up ) up++; else down++; }

  void analyze()
  { ratio.analyze(); }

  void datacollection_begin()
  { up = down = 0; }

  void datacollection_end()
  { ratio.add( up != 0 ? 100.0 * up / ( down + up ) : 0.0 ); }

  void merge( const benefit_t& other )
  { ratio.merge( other.ratio ); }

  const char* name() const
  { return name_str.c_str(); }

  benefit_t* collect_ratio( bool collect = true )
  {
    if ( sim.report_details )
    {
      ratio.change_mode( !collect );
      ratio.reserve( std::min( as<unsigned>( sim.iterations ), 2048u ) );
    }

    return this;
  }

};

// Uptime ===================================================================

struct uptime_t : public uptime_common_t
{
  std::string name_str;
  sim_t& sim;

  extended_sample_data_t uptime_sum;
  extended_sample_data_t uptime_instance;

  uptime_t( sim_t& s, const std::string& n ) : uptime_common_t(),
    name_str( n ),
    sim( s ),
    uptime_sum( "Uptime", true ),
    uptime_instance( "Duration", true )
  {}

  void update( bool is_up, timespan_t current_time ) override
  {
    if ( is_up )
    {
      if ( last_start < timespan_t::zero() )
        last_start = current_time;
    }
    else if ( last_start >= timespan_t::zero() )
    {
      auto delta = current_time - last_start;
      iteration_uptime_sum += delta;
      uptime_instance.add( delta.total_seconds() );
      reset();
    }
  }

  void analyze()
  {
    uptime_sum.analyze();
    uptime_instance.analyze();
  }

  void merge( const uptime_common_t& other ) override
  {
    uptime_sum.merge( dynamic_cast<const uptime_t&>( other ).uptime_sum );
    uptime_instance.merge( dynamic_cast<const uptime_t&>( other ).uptime_instance );
  }

  void datacollection_end( timespan_t t ) override
  { uptime_sum.add( t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0 ); }

  const char* name() const
  { return name_str.c_str(); }

  uptime_t* collect_uptime( bool collect = true )
  {
    if ( sim.report_details )
    {
      uptime_sum.change_mode( !collect );
      uptime_sum.reserve( std::min( as<unsigned>( sim.iterations ), 2048u ) );
    }

    return this;
  }

  uptime_t* collect_duration( bool collect = true )
  {
    if ( sim.report_details )
    {
      uptime_instance.change_mode( !collect );
      uptime_instance.reserve( std::min( as<unsigned>( sim.iterations ), 2048u ) );
    }

    return this;
  }
};

// Proc =====================================================================

struct proc_t : private noncopyable
{
private:
  sim_t& sim;
  size_t iteration_count; // track number of procs during the current iteration
  timespan_t last_proc; // track time of the last proc
public:
  const std::string name_str;
  // These are initialized in SIMPLE mode. Only change mode for infrequent procs to keep memory usage reasonable.
  extended_sample_data_t interval_sum;
  extended_sample_data_t count;

  proc_t( sim_t& s, const std::string& n ) :
    sim( s ),
    iteration_count(),
    last_proc( timespan_t::min() ),
    name_str( n ),
    interval_sum( "Interval", true ),
    count( "Count", true )
  {}

  void occur()
  {
    iteration_count++;
    if ( last_proc >= timespan_t::zero() && last_proc < sim.current_time() )
    {
      interval_sum.add( ( sim.current_time() - last_proc ).total_seconds() );
      reset();
    }
    if ( sim.debug )
      sim.out_debug.printf( "[PROC] %s: iteration_count=%u count.sum=%u last_proc=%f",
                  name(),
                  static_cast<unsigned>( iteration_count ),
                  static_cast<unsigned>( count.sum() ),
                  last_proc.total_seconds() );

    last_proc = sim.current_time();
  }

  void reset()
  { last_proc = timespan_t::min(); }

  void merge( const proc_t& other )
  {
    count.merge( other.count );
    interval_sum.merge( other.interval_sum );
  }

  void analyze()
  {
    count.analyze();
    interval_sum.analyze();
  }

  void datacollection_begin()
  { iteration_count = 0; }

  void datacollection_end()
  { count.add( static_cast<double>( iteration_count ) ); }

  const char* name() const
  { return name_str.c_str(); }

  proc_t* collect_count( bool collect = true )
  {
    if ( sim.report_details )
    {
      count.change_mode( !collect );
      count.reserve( std::min( as<unsigned>( sim.iterations ), 2048u ) );
    }

    return this;
  }

  proc_t* collect_interval( bool collect = true )
  {
    if ( sim.report_details )
    {
      interval_sum.change_mode( !collect );
      interval_sum.reserve( std::min( as<unsigned>( sim.iterations ), 2048u ) );
    }

    return this;
  }
};

// Set Bonus ================================================================

struct set_bonus_t
{
  // Some magic constants
  static const unsigned N_BONUSES = 8;       // Number of set bonuses in tier gear

  struct set_bonus_data_t
  {
    const spell_data_t* spell;
    const item_set_bonus_t* bonus;
    int overridden;
    bool enabled;

    set_bonus_data_t() :
      spell( spell_data_t::not_found() ), bonus( nullptr ), overridden( -1 ), enabled( false )
    { }
  };

  // Data structure definitions
  typedef std::vector<set_bonus_data_t> bonus_t;
  typedef std::vector<bonus_t> bonus_type_t;
  typedef std::vector<bonus_type_t> set_bonus_type_t;

  typedef std::vector<unsigned> bonus_count_t;
  typedef std::vector<bonus_count_t> set_bonus_count_t;

  player_t* actor;

  // Set bonus data structure
  set_bonus_type_t set_bonus_spec_data;
  // Set item counts
  set_bonus_count_t set_bonus_spec_count;

  set_bonus_t( player_t* p );

  // Collect item information about set bonuses, fully DBC driven
  void initialize_items();

  // Initialize set bonuses in earnest
  void initialize();

  std::unique_ptr<expr_t> create_expression( const player_t*, const std::string& type );

  std::vector<const item_set_bonus_t*> enabled_set_bonus_data() const;

  // Fast accessor to a set bonus spell, returns the spell, or spell_data_t::not_found()
  const spell_data_t* set( specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus ) const
  {
    if ( specdata::spec_idx( spec ) < 0 )
    {
      return spell_data_t::nil();
    }
#ifndef NDEBUG
    assert(set_bonus_spec_data.size() > (unsigned)set_bonus );
    assert(set_bonus_spec_data[ set_bonus ].size() > (unsigned)specdata::spec_idx( spec ) );
    assert(set_bonus_spec_data[ set_bonus ][ specdata::spec_idx( spec ) ].size() > (unsigned)bonus );
#endif
    return set_bonus_spec_data[ set_bonus ][ specdata::spec_idx( spec ) ][ bonus ].spell;
  }

  // Fast accessor for checking whether a set bonus is enabled
  bool has_set_bonus( specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus ) const
  {
    if ( specdata::spec_idx( spec ) < 0 )
    {
      return false;
    }

    return set_bonus_spec_data[ set_bonus ][ specdata::spec_idx( spec ) ][ bonus ].enabled;
  }

  bool parse_set_bonus_option( const std::string& opt_str, set_bonus_type_e& set_bonus, set_bonus_e& bonus );
  std::string to_string() const;
  std::string to_profile_string( const std::string& = "\n" ) const;
  std::string generate_set_bonus_options() const;
};

std::ostream& operator<<(std::ostream&, const set_bonus_t&);

#include "sim/real_ppm.hpp"

// "Deck of Cards" randomizer helper class ====================================
// Described at https://www.reddit.com/r/wow/comments/6j2wwk/wow_class_design_ama_june_2017/djb8z68/

struct shuffled_rng_t
{
private:
  player_t*    player;
  std::string  name_str;
  int          success_entries;
  int          total_entries;
  int          success_entries_remaining;
  int          total_entries_remaining;

  shuffled_rng_t() : player(nullptr), success_entries(0), total_entries(0), success_entries_remaining(0), total_entries_remaining(0)
  { }

public:

  shuffled_rng_t(const std::string& name, player_t* p, int success_entries = 0, int total_entries = 0) :
    player(p),
    name_str(name),
    success_entries(success_entries),
    total_entries(total_entries),
    success_entries_remaining(success_entries),
    total_entries_remaining(total_entries)
  { }

  const std::string& name() const
  {
    return name_str;
  }

  int get_success_entries() const
  {
    return success_entries;
  }

  int get_success_entries_remaining() const
  {
    return success_entries_remaining;
  }

  int get_total_entries() const
  {
    return total_entries;
  }

  int get_total_entries_remaining() const
  {
    return total_entries_remaining;
  }

  double get_remaining_success_chance() const
  {
    return (double)success_entries_remaining / (double)total_entries_remaining;
  }

  void reset()
  {
    success_entries_remaining = success_entries;
    total_entries_remaining = total_entries;
  }

  bool trigger();
};

#include "sim/sc_cooldown.hpp"

#include "player/player_stat_cache.hpp"

#include "player/player_processed_report_information.hpp"

#include "player/player_collected_data.hpp"

#include "player/player_talent_points.hpp"

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
  virtual void html_customsection( report::sc_html_stream& )
  {

  }
};
#include "player/assessor.hpp"

// Player ===================================================================

struct action_variable_t
{
  std::string name_;
  double current_value_, default_, constant_value_;
  std::vector<action_t*> variable_actions;

  action_variable_t( const std::string& name, double def = 0 ) :
    name_( name ), current_value_( def ), default_( def ),
    constant_value_( std::numeric_limits<double>::lowest() )
  { }

  double value() const
  { return current_value_; }

  void reset()
  { current_value_ = default_; }

  bool is_constant( double* constant_value ) const
  {
    *constant_value = constant_value_;
    return constant_value_ != std::numeric_limits<double>::lowest();
  }

  // Implemented in sc_player.cpp
  void optimize();
};

struct player_scaling_t
{
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling;
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling_normalized;
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling_error;
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling_delta_dps;
  std::array<gear_stats_t, SCALE_METRIC_MAX> scaling_compare_error;
  std::array<double, SCALE_METRIC_MAX> scaling_lag, scaling_lag_error;
  std::array<bool, STAT_MAX> scales_with;
  std::array<double, STAT_MAX> over_cap;
  std::array<std::vector<stat_e>, SCALE_METRIC_MAX> scaling_stats; // sorting vector

  player_scaling_t()
  {
    range::fill( scaling_lag, 0 );
    range::fill( scaling_lag_error, 0 );
    range::fill( scales_with, false );
    range::fill( over_cap, 0 );
  }

  void enable( stat_e stat )
  { scales_with[ stat ] = true; }

  void disable( stat_e stat )
  { scales_with[ stat ] = false; }

  void set( stat_e stat, bool state )
  { scales_with[ stat ] = state; }
};

#include "player/player_resources.hpp"

#include "player/sc_player.hpp"

#include "player/target_specific.hpp"

struct player_event_t : public event_t
{
  player_t* _player;
  player_event_t( player_t& p, timespan_t delta_time ) :
    event_t( p, delta_time ),
    _player( &p ){}
  player_t* p()
  { return player(); }
  const player_t* p() const
  { return player(); }
  player_t* player()
  { return _player; }
  const player_t* player() const
  { return _player; }
  virtual const char* name() const override
  { return "event_t"; }
};

/* Event which will demise the player
 * - Reason for it are that we need to finish the current action ( eg. a dot tick ) without
 * killing off target dependent things ( eg. dot state ).
 */
struct player_demise_event_t : public player_event_t
{
  player_demise_event_t( player_t& p, timespan_t delta_time = timespan_t::zero() /* Instantly kill the player */ ) :
     player_event_t( p, delta_time )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New Player-Demise Event: %s", p.name() );
  }
  virtual const char* name() const override
  { return "Player-Demise"; }
  virtual void execute() override
  {
    p() -> demise();
  }
};

// Pet ======================================================================

struct pet_t : public player_t
{
  typedef player_t base_t;

  std::string full_name_str;
  player_t* const owner;
  double stamina_per_owner;
  double intellect_per_owner;
  bool summoned;
  bool dynamic;
  pet_e pet_type;
  event_t* expiration;
  timespan_t duration;
  bool affects_wod_legendary_ring;

  struct owner_coefficients_t
  {
    double armor = 1.0;
    double health = 1.0;
    double ap_from_ap = 0.0;
    double ap_from_sp = 0.0;
    double sp_from_ap = 0.0;
    double sp_from_sp = 0.0;
  } owner_coeff;

public:
  pet_t( sim_t* sim, player_t* owner, const std::string& name, bool guardian = false, bool dynamic = false );
  pet_t( sim_t* sim, player_t* owner, const std::string& name, pet_e pt, bool guardian = false, bool dynamic = false );

  void create_options() override;
  void create_buffs() override;
  void init() override;
  void init_base_stats() override;
  void init_target() override;
  void init_finished() override;
  void reset() override;
  void assess_damage( school_e, result_amount_type, action_state_t* s ) override;

  virtual void summon( timespan_t duration = timespan_t::zero() );
  virtual void dismiss( bool expired = false );
  // Adjust pet remaining duration. New duration of <= 0 dismisses pet. No-op on
  // persistent pets.
  virtual void adjust_duration( const timespan_t& adjustment );

  const char* name() const override { return full_name_str.c_str(); }
  const player_t* get_owner_or_self() const override
  { return owner; }

  const spell_data_t* find_pet_spell( const std::string& name );

  double composite_attribute( attribute_e attr ) const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_target_multiplier( player_t*, school_e ) const override;

  // new pet scaling by Ghostcrawler, see http://us.battle.net/wow/en/forum/topic/5889309137?page=49#977
  // http://us.battle.net/wow/en/forum/topic/5889309137?page=58#1143

  double hit_exp() const;

  double composite_movement_speed() const override
  { return owner -> composite_movement_speed(); }

  double composite_melee_expertise( const weapon_t* ) const override
  { return hit_exp(); }
  double composite_melee_hit() const override
  { return hit_exp(); }
  double composite_spell_hit() const override
  { return hit_exp() * 2.0; }

  double pet_crit() const;

  double composite_melee_crit_chance() const override
  { return pet_crit(); }
  double composite_spell_crit_chance() const override
  { return pet_crit(); }

  double composite_melee_speed() const override
  { return owner -> cache.attack_speed(); }

  double composite_melee_haste() const override
  { return owner -> cache.attack_haste(); }

  double composite_spell_haste() const override
  { return owner -> cache.spell_haste(); }

  double composite_spell_speed() const override
  { return owner -> cache.spell_speed(); }

  double composite_bonus_armor() const override
  { return owner -> cache.bonus_armor(); }

  double composite_damage_versatility() const override
  { return owner -> cache.damage_versatility(); }

  double composite_heal_versatility() const override
  { return owner -> cache.heal_versatility(); }

  double composite_mitigation_versatility() const override
  { return owner -> cache.mitigation_versatility(); }

  double composite_melee_attack_power() const override;

  double composite_spell_power( school_e school ) const override;

  double composite_player_critical_damage_multiplier( const action_state_t* s ) const override;

  // Assuming diminishing returns are transfered to the pet as well
  double composite_dodge() const override
  { return owner -> cache.dodge(); }

  double composite_parry() const override
  { return owner -> cache.parry(); }

  // Influenced by coefficients [ 0, 1 ]
  double composite_armor() const override
  { return owner -> cache.armor() * owner_coeff.armor; }

  void init_resources( bool force ) override;
  bool requires_data_collection() const override
  { return active_during_iteration || ( dynamic && sim -> report_pets_separately == 1 ); }

  timespan_t composite_active_time() const override;

  void acquire_target( retarget_source /* event */, player_t* /* context */ = nullptr ) override;
};

#include "sim/gain.hpp"

#include "player/stats.hpp"

#include "action/sc_action.hpp"
// Attack ===================================================================

struct attack_t : public action_t
{
  double base_attack_expertise;

  attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Attack Overrides
  virtual timespan_t execute_time() const override;
  virtual void execute() override;
  virtual result_e calculate_result( action_state_t* ) const override;
  virtual void   init() override;

  virtual result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override;
  virtual result_amount_type report_amount_type( const action_state_t* /* state */ ) const override;

  virtual double  miss_chance( double hit, player_t* t ) const override;
  virtual double  dodge_chance( double /* expertise */, player_t* t ) const override;
  virtual double  block_chance( action_state_t* s ) const override;
  virtual double  crit_block_chance( action_state_t* s ) const override;

  virtual double action_multiplier() const override
  {
    double mul = action_t::action_multiplier();

    if ( ! special )
    {
      mul *= player -> auto_attack_multiplier;
    }

    return mul;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double mul = action_t::composite_target_multiplier( target );

    mul *= composite_target_damage_vulnerability( target );

    return mul;
  }

  virtual double composite_hit() const override
  { return action_t::composite_hit() + player -> cache.attack_hit(); }

  virtual double composite_crit_chance() const override
  { return action_t::composite_crit_chance() + player -> cache.attack_crit_chance(); }

  virtual double composite_crit_chance_multiplier() const override
  { return action_t::composite_crit_chance_multiplier() * player -> composite_melee_crit_chance_multiplier(); }

  virtual double composite_haste() const override
  { return action_t::composite_haste() * player -> cache.attack_haste(); }

  virtual double composite_expertise() const
  { return base_attack_expertise + player -> cache.attack_expertise(); }

  virtual double composite_versatility( const action_state_t* state ) const override
  { return action_t::composite_versatility( state ) + player -> cache.damage_versatility(); }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = action_t::recharge_multiplier( cd );

    if ( cd.hasted )
    {
      m *= player->cache.attack_haste();
    }

    return m;
  }

  virtual void reschedule_auto_attack( double old_swing_haste );

  virtual void reset() override
  {
    attack_table.reset();
    action_t::reset();
  }

private:
  /// attack table generator with caching
  struct attack_table_t{
    std::array<double, RESULT_MAX> chances;
    std::array<result_e, RESULT_MAX> results;
    int num_results;
    double attack_table_sum; // Used to check whether we can use cached values or not.

    attack_table_t()
    {reset(); }

    void reset()
    { attack_table_sum = std::numeric_limits<double>::min(); }

    void build_table( double miss_chance, double dodge_chance,
                      double parry_chance, double glance_chance,
                      double crit_chance, sim_t* );
  };
  mutable attack_table_t attack_table;


};

// Melee Attack ===================================================================

struct melee_attack_t : public attack_t
{
  melee_attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Melee Attack Overrides
  virtual void init() override;
  virtual double parry_chance( double /* expertise */, player_t* t ) const override;
  virtual double glance_chance( int delta_level ) const override;

  virtual proc_types proc_type() const override;
};

// Ranged Attack ===================================================================

struct ranged_attack_t : public attack_t
{
  ranged_attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Ranged Attack Overrides
  virtual double composite_target_multiplier( player_t* ) const override;
  virtual void schedule_execute( action_state_t* execute_state = nullptr ) override;

  virtual proc_types proc_type() const override;
};

// Spell Base ====================================================================

struct spell_base_t : public action_t
{
  // special item flags
  bool procs_courageous_primal_diamond;

  spell_base_t( action_e at, const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Spell Base Overrides
  virtual timespan_t execute_time() const override;
  virtual result_e   calculate_result( action_state_t* ) const override;
  virtual void   execute() override;
  virtual void   schedule_execute( action_state_t* execute_state = nullptr ) override;

  virtual double composite_crit_chance() const override
  { return action_t::composite_crit_chance() + player -> cache.spell_crit_chance(); }

  virtual double composite_haste() const override
  { return action_t::composite_haste() * player -> cache.spell_speed(); }

  virtual double composite_crit_chance_multiplier() const override
  { return action_t::composite_crit_chance_multiplier() * player -> composite_spell_crit_chance_multiplier(); }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = action_t::recharge_multiplier( cd );

    if ( cd.hasted )
    {
      m *= player->cache.spell_haste();
    }

    return m;
  }

  virtual proc_types proc_type() const override;
};

// Harmful Spell ====================================================================

struct spell_t : public spell_base_t
{
public:
  spell_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Harmful Spell Overrides
  virtual result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override;
  virtual result_amount_type report_amount_type( const action_state_t* /* state */ ) const override;
  virtual double miss_chance( double hit, player_t* t ) const override;
  virtual void   init() override;
  virtual double composite_hit() const override
  { return action_t::composite_hit() + player -> cache.spell_hit(); }
  virtual double composite_versatility( const action_state_t* state ) const override
  { return spell_base_t::composite_versatility( state ) + player -> cache.damage_versatility(); }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double mul = action_t::composite_target_multiplier( target );

    mul *= composite_target_damage_vulnerability( target );

    return mul;
  }

};

// Heal =====================================================================

struct heal_t : public spell_base_t
{
public:
  typedef spell_base_t base_t;
  bool group_only;
  double base_pct_heal;
  double tick_pct_heal;
  gain_t* heal_gain;

  heal_t( const std::string& name, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  virtual double composite_pct_heal( const action_state_t* ) const;
  virtual void assess_damage( result_amount_type, action_state_t* ) override;
  virtual result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override;
  virtual result_amount_type report_amount_type( const action_state_t* /* state */ ) const override;
  virtual size_t available_targets( std::vector< player_t* >& ) const override;
  void activate() override;
  virtual double calculate_direct_amount( action_state_t* state ) const override;
  virtual double calculate_tick_amount( action_state_t* state, double dmg_multiplier ) const override;
  player_t* find_greatest_difference_player();
  player_t* find_lowest_player();
  std::vector < player_t* > find_lowest_players( int num_players ) const;
  player_t* smart_target() const; // Find random injured healing target, preferring non-pets // Might need to move up hierarchy if there are smart absorbs
  virtual int num_targets() const override;
  void   parse_effect_data( const spelleffect_data_t& ) override;

  virtual double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = action_multiplier() * action_da_multiplier() *
           player -> cache.player_heal_multiplier( s ) *
           player -> composite_player_dh_multiplier( get_school() );

    return m;
  }

  virtual double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = action_multiplier() * action_ta_multiplier() *
           player -> cache.player_heal_multiplier( s ) *
           player -> composite_player_th_multiplier( get_school() );

    return m;
  }

  virtual double composite_player_critical_multiplier( const action_state_t* /* s */ ) const override
  { return player -> composite_player_critical_healing_multiplier(); }

  virtual double composite_versatility( const action_state_t* state ) const override
  { return spell_base_t::composite_versatility( state ) + player -> cache.heal_versatility(); }

  virtual std::unique_ptr<expr_t> create_expression( const std::string& name ) override;
};

// Absorb ===================================================================

struct absorb_t : public spell_base_t
{
  target_specific_t<absorb_buff_t> target_specific;

  absorb_t( const std::string& name, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Allows customization of the absorb_buff_t that we are creating.
  virtual absorb_buff_t* create_buff( const action_state_t* s )
  {
    buff_t* b = buff_t::find( s -> target, name_str );
    if ( b )
      return debug_cast<absorb_buff_t*>( b );

    std::string stats_obj_name = name_str;
    if ( s -> target != player )
      stats_obj_name += "_" + player -> name_str;
    stats_t* stats_obj = player -> get_stats( stats_obj_name, this );
    if ( stats != stats_obj )
    {
      // Add absorb target stats as a child to the main stats object for reporting
      stats -> add_child( stats_obj );
    }
    auto buff = make_buff<absorb_buff_t>( s -> target, name_str, &data() );
    buff->set_absorb_source( stats_obj );

    return buff;
  }

  virtual void assess_damage( result_amount_type, action_state_t* ) override;
  virtual result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override
  { return result_amount_type::ABSORB; }
  virtual void impact( action_state_t* ) override;
  virtual void activate() override;
  virtual size_t available_targets( std::vector< player_t* >& ) const override;
  virtual int num_targets() const override;

  virtual double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = action_multiplier() * action_da_multiplier() *
           player -> composite_player_absorb_multiplier( s );

    return m;
  }
  virtual double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = action_multiplier() * action_ta_multiplier() *
           player -> composite_player_absorb_multiplier( s );

    return m;
  }
  virtual double composite_versatility( const action_state_t* state ) const override
  { return spell_base_t::composite_versatility( state ) + player -> cache.heal_versatility(); }
};

// Sequence =================================================================

struct sequence_t : public action_t
{
  bool waiting;
  int sequence_wait_on_ready;
  std::vector<action_t*> sub_actions;
  int current_action;
  bool restarted;
  timespan_t last_restart;
  bool has_option;

  sequence_t( player_t*, const std::string& sub_action_str );

  virtual void schedule_execute( action_state_t* execute_state = nullptr ) override;
  virtual void reset() override;
  virtual bool ready() override;
  void restart() { current_action = 0; restarted = true; last_restart = sim -> current_time(); }
  bool can_restart()
  { return ! restarted && last_restart < sim -> current_time(); }
};

struct strict_sequence_t : public action_t
{
  size_t current_action;
  std::vector<action_t*> sub_actions;
  std::string seq_name_str;

  // Allow strict sequence sub-actions to be skipped if they are not ready. Default = false.
  bool allow_skip;

  strict_sequence_t( player_t*, const std::string& opts );

  bool ready() override;
  void reset() override;
  void cancel() override;
  void interrupt_action() override;
  void schedule_execute( action_state_t* execute_state = nullptr ) override;
};

// Primary proc type of the result (direct (aoe) damage/heal, periodic
// damage/heal)
inline proc_types action_state_t::proc_type() const
{
  if ( result_type == result_amount_type::DMG_DIRECT || result_type == result_amount_type::HEAL_DIRECT )
    return action -> proc_type();
  else if ( result_type == result_amount_type::DMG_OVER_TIME )
    return PROC1_PERIODIC;
  else if ( result_type == result_amount_type::HEAL_OVER_TIME )
    return PROC1_PERIODIC_HEAL;

  return PROC1_INVALID;
}

// Secondary proc type of the "finished casting" (i.e., execute()). Only
// triggers the "landing", dodge, parry, and miss procs
inline proc_types2 action_state_t::execute_proc_type2() const
{
  if ( result == RESULT_DODGE )
    return PROC2_DODGE;
  else if ( result == RESULT_PARRY )
    return PROC2_PARRY;
  else if ( result == RESULT_MISS )
    return PROC2_MISS;
  // Bunch up all non-damaging harmful attacks that land into "hit"
  else if ( action -> harmful )
    return PROC2_LANDED;

  return PROC2_INVALID;
}

inline proc_types2 action_state_t::cast_proc_type2() const
{
  // Only foreground actions may trigger the "on cast" procs
  if ( action -> background )
  {
    return PROC2_INVALID;
  }

  if ( action -> attack_direct_power_coefficient( this ) ||
       action -> attack_tick_power_coefficient( this ) ||
       action -> spell_direct_power_coefficient( this ) ||
       action -> spell_tick_power_coefficient( this ) ||
       action -> base_ta( this ) || action -> base_da_min( this ) ||
       action -> bonus_ta( this ) || action -> bonus_da( this ) )
  {
    // This is somewhat naive way to differentiate, better way would be to classify based on the
    // actual proc types, but it will serve our purposes for now.
    return action -> harmful ? PROC2_CAST_DAMAGE : PROC2_CAST_HEAL;
  }

  // Generic fallback "on any cast"
  return PROC2_CAST;
}

// DoT ======================================================================

// DoT Tick Event ===========================================================

struct dot_tick_event_t : public event_t
{
public:
  dot_tick_event_t( dot_t* d, timespan_t time_to_tick );

private:
  virtual void execute() override;
  virtual const char* name() const override
  { return "Dot Tick"; }
  dot_t* dot;
};

// DoT End Event ===========================================================

struct dot_end_event_t : public event_t
{
public:
  dot_end_event_t( dot_t* d, timespan_t time_to_end );

private:
  virtual void execute() override;
  virtual const char* name() const override
  { return "DoT End"; }
  dot_t* dot;
};

struct dot_t : private noncopyable
{
private:
  sim_t& sim;
  bool ticking;
  timespan_t current_duration;
  timespan_t last_start;
  timespan_t prev_tick_time; // Used for rescheduling when refreshing right before last partial tick
  timespan_t extended_time; // Added time per extend_duration for the current dot application
  timespan_t reduced_time; // Removed time per reduce_duration for the current dot application
  int stack;
public:
  event_t* tick_event;
  event_t* end_event;
  double last_tick_factor;

  player_t* const target;
  player_t* const source;
  action_t* current_action;
  action_state_t* state;
  int num_ticks, current_tick;
  int max_stack;
  timespan_t miss_time;
  timespan_t time_to_tick;
  std::string name_str;

  dot_t( const std::string& n, player_t* target, player_t* source );

  void   extend_duration( timespan_t extra_seconds, timespan_t max_total_time = timespan_t::min(), uint32_t state_flags = -1 );
  void   extend_duration( timespan_t extra_seconds, uint32_t state_flags )
  { extend_duration( extra_seconds, timespan_t::min(), state_flags ); }
  void   reduce_duration( timespan_t remove_seconds, uint32_t state_flags = -1 );
  void   refresh_duration( uint32_t state_flags = -1 );
  void   reset();
  void   cancel();
  void   trigger( timespan_t duration );
  void   decrement( int stacks );
  void   increment(int stacks);
  void   copy( player_t* destination, dot_copy_e = DOT_COPY_START ) const;
  void   copy( dot_t* dest_dot ) const;
  // Scale on-going dot remaining time by a coefficient during a tick. Note that this should be
  // accompanied with the correct (time related) scaling information in the action's supporting
  // methods (action_t::tick_time, action_t::composite_dot_ruration), otherwise bad things will
  // happen.
  void   adjust( double coefficient );
  // Alternative to adjust() based on the rogue ability Exsanguinate and how it works with hasted bleeds.
  // It rounds the number of ticks to the nearest integer and reschedules the remaining ticks
  // as full ticks from the end. If one tick would theoretically occur before Exsanguinate, it will
  // happen immediately instead.
  // Note that this should be accompanied with the correct (time related) scaling information in the
  // action's supporting methods (action_t::tick_time, action_t::composite_dot_ruration), otherwise
  // bad things will happen.
  void   adjust_full_ticks( double coefficient );
  static std::unique_ptr<expr_t> create_expression( dot_t* dot, action_t* action, action_t* source_action,
      const std::string& name_str, bool dynamic );

  timespan_t remains() const;
  timespan_t time_to_next_tick() const;
  timespan_t duration() const
  { return ! is_ticking() ? timespan_t::zero() : current_duration; }
  int    ticks_left() const;
  const char* name() const
  { return name_str.c_str(); }
  bool is_ticking() const
  { return ticking; }
  timespan_t get_extended_time() const
  { return extended_time; }
  double get_last_tick_factor() const
  { return last_tick_factor; }
  int current_stack() const
  { return ticking ? stack : 0; }

  void tick();
  void last_tick();
  bool channel_interrupt();

private:
  void tick_zero();
  void schedule_tick();
  void start( timespan_t duration );
  void refresh( timespan_t duration );
  void check_tick_zero( bool start );
  bool is_higher_priority_action_available() const;
  void recalculate_num_ticks();

  friend struct dot_tick_event_t;
  friend struct dot_end_event_t;
};

inline double action_t::last_tick_factor( const dot_t* /* d */, const timespan_t& time_to_tick, const timespan_t& duration ) const
{ return std::min( 1.0, duration / time_to_tick ); }

inline dot_tick_event_t::dot_tick_event_t( dot_t* d, timespan_t time_to_tick ) :
    event_t( *d -> source, time_to_tick ),
  dot( d )
{
  if ( sim().debug )
    sim().out_debug.printf( "New DoT Tick Event: %s %s %d-of-%d %.4f",
                d -> source -> name(), dot -> name(), dot -> current_tick + 1, dot -> num_ticks, time_to_tick.total_seconds() );
}


inline void dot_tick_event_t::execute()
{
  dot -> tick_event = nullptr;
  dot -> current_tick++;

  if ( dot -> current_action -> channeled &&
       dot -> current_action -> action_skill < 1.0 &&
       dot -> remains() >= dot -> current_action -> tick_time( dot -> state ) )
  {
    if ( rng().roll( std::max( 0.0, dot -> current_action -> action_skill - dot -> current_action -> player -> current.skill_debuff ) ) )
    {
      dot -> tick();
    }
  }
  else // No skill-check required
  {
    dot -> tick();
  }

  // Some dots actually cancel themselves mid-tick. If this happens, we presume
  // that the cancel has been "proper", and just stop event execution here, as
  // the dot no longer exists.
  if ( ! dot -> is_ticking() )
    return;

  if ( ! dot -> current_action -> consume_cost_per_tick( *dot ) )
  {
    return;
  }

  if ( dot -> channel_interrupt() )
  {
    return;
  }

  // continue ticking
  dot->schedule_tick();
}

inline dot_end_event_t::dot_end_event_t( dot_t* d, timespan_t time_to_end ) :
    event_t( *d -> source, time_to_end ),
    dot( d )
{
  if ( sim().debug )
    sim().out_debug.printf( "New DoT End Event: %s %s %.3f",
                d -> source -> name(), dot -> name(), time_to_end.total_seconds() );
}

inline void dot_end_event_t::execute()
{
  dot -> end_event = nullptr;
  if ( dot -> current_tick < dot -> num_ticks )
  {
    dot -> current_tick++;
    dot -> tick();
  }
  // If for some reason the last tick has already ticked, ensure that the next tick has not
  // consumed any time yet, i.e., the last tick has occurred on the same timestamp as this end
  // event. This situation may occur in conjunction with extensive dot extension, where the last
  // rescheduling of the dot-end-event occurs between the second to last and last ticks. That will
  // in turn flip the order of the dot-tick-event and dot-end-event.
  else
  {
    assert( ! dot -> tick_event || ( dot -> tick_event && dot -> time_to_tick == dot -> tick_event -> remains() ) );
  }

  // Aand sanity check that the dot has consumed all ticks just in case./
  assert( dot -> current_tick == dot -> num_ticks );
  dot -> last_tick();
}

// Action Callback ==========================================================

struct action_callback_t : private noncopyable
{
  player_t* listener;
  bool active;
  bool allow_self_procs;
  bool allow_procs;

  action_callback_t( player_t* l, bool ap = false, bool asp = false ) :
    listener( l ), active( true ), allow_self_procs( asp ), allow_procs( ap )
  {
    assert( l );
    if ( std::find( l -> callbacks.all_callbacks.begin(), l -> callbacks.all_callbacks.end(), this )
        == l -> callbacks.all_callbacks.end() )
      l -> callbacks.all_callbacks.push_back( this );
  }
  virtual ~action_callback_t() {}
  virtual void trigger( action_t*, void* call_data ) = 0;
  virtual void reset() {}
  virtual void initialize() { }
  virtual void activate() { active = true; }
  virtual void deactivate() { active = false; }

  static void trigger( const std::vector<action_callback_t*>& v, action_t* a, void* call_data = nullptr )
  {
    if ( a && ! a -> player -> in_combat ) return;

    std::size_t size = v.size();
    for ( std::size_t i = 0; i < size; i++ )
    {
      action_callback_t* cb = v[ i ];
      if ( cb -> active )
      {
        if ( ! cb -> allow_procs && a && a -> proc ) return;
        cb -> trigger( a, call_data );
      }
    }
  }

  static void reset( const std::vector<action_callback_t*>& v )
  {
    std::size_t size = v.size();
    for ( std::size_t i = 0; i < size; i++ )
    {
      v[ i ] -> reset();
    }
  }
};

// Generic proc callback ======================================================

/**
 * DBC-driven proc callback. Extensively leans on the concept of "driver"
 * spells that blizzard uses to trigger actual proc spells in most cases. Uses
 * spell data as much as possible (through interaction with special_effect_t).
 * Intentionally vastly simplified compared to our other (older) callback
 * systems. The "complex" logic is offloaded either into special_effect_t
 * (creation of buffs/actions), special effect_t initialization (what kind of
 * special effect to create from DBC data or user given options, or when and
 * why to proc things (new DBC based proc system).
 *
 * The actual triggering logic is also vastly simplified (see execute()), as
 * the majority of procs in WoW are very simple. Custom procs can always be
 * derived off of this struct.
 */
struct dbc_proc_callback_t : public action_callback_t
{
  struct proc_event_t : public event_t
  {
    dbc_proc_callback_t* cb;
    action_t*            source_action;
    action_state_t*      source_state;

    proc_event_t( dbc_proc_callback_t* c, action_t* a, action_state_t* s ) :
      event_t( *a->sim ), cb( c ), source_action( a ),
      // Note, state has to be cloned as it's about to get recycled back into the action state cache
      source_state( s->action->get_state( s ) )
    {
      schedule( timespan_t::zero() );
    }

    ~proc_event_t()
    { action_state_t::release( source_state ); }

    const char* name() const override
    { return "dbc_proc_event"; }

    void execute() override
    { cb->execute( source_action, source_state ); }
  };

  static const item_t default_item_;

  const item_t& item;
  const special_effect_t& effect;
  cooldown_t* cooldown;

  // Proc trigger types, cached/initialized here from special_effect_t to avoid
  // needless spell data lookups in vast majority of cases
  real_ppm_t* rppm;
  double      proc_chance;
  double      ppm;

  buff_t* proc_buff;
  action_t* proc_action;
  weapon_t* weapon;

  dbc_proc_callback_t( const item_t& i, const special_effect_t& e ) :
    action_callback_t( i.player ), item( i ), effect( e ), cooldown( nullptr ),
    rppm( nullptr ), proc_chance( 0 ), ppm( 0 ),
    proc_buff( nullptr ), proc_action( nullptr ), weapon( nullptr )
  {
    assert( e.proc_flags() != 0 );
  }

  dbc_proc_callback_t( const item_t* i, const special_effect_t& e ) :
    action_callback_t( i -> player ), item( *i ), effect( e ), cooldown( nullptr ),
    rppm( nullptr ), proc_chance( 0 ), ppm( 0 ),
    proc_buff( nullptr ), proc_action( nullptr ), weapon( nullptr )
  {
    assert( e.proc_flags() != 0 );
  }

  dbc_proc_callback_t( player_t* p, const special_effect_t& e ) :
    action_callback_t( p ), item( default_item_ ), effect( e ), cooldown( nullptr ),
    rppm( nullptr ), proc_chance( 0 ), ppm( 0 ),
    proc_buff( nullptr ), proc_action( nullptr ), weapon( nullptr )
  {
    assert( e.proc_flags() != 0 );
  }

  virtual void initialize() override;

  void trigger( action_t* a, void* call_data ) override
  {
    if ( cooldown && cooldown -> down() ) return;

    // Weapon-based proc triggering differs from "old" callbacks. When used
    // (weapon_proc == true), dbc_proc_callback_t _REQUIRES_ that the action
    // has the correct weapon specified. Old style procs allowed actions
    // without any weapon to pass through.
    if ( weapon && ( ! a -> weapon || ( a -> weapon && a -> weapon != weapon ) ) )
      return;

    auto state = static_cast<action_state_t*>( call_data );

    // Don't allow procs to proc itself
    if ( proc_action && state->action && state->action->internal_id == proc_action->internal_id )
    {
      return;
    }

    if ( proc_action && proc_action->harmful )
    {
      // Don't allow players to harm other players, and enemies harm other enemies
      if (state->action && state->action->player->is_enemy() == state->target->is_enemy())
      {
        return;
      }
    } 

    bool triggered = roll( a );
    if ( listener -> sim -> debug )
      listener -> sim -> out_debug.printf( "%s attempts to proc %s on %s: %d",
                                 listener -> name(),
                                 effect.to_string().c_str(),
                                 a -> name(), triggered );
    if ( triggered )
    {
      // Detach proc execution from proc triggering
      make_event<proc_event_t>( *listener->sim, this, a, state );

      if ( cooldown )
        cooldown -> start();
    }
  }

  // Determine target for the callback (action).
  virtual player_t* target( const action_state_t* state ) const
  {
    // Incoming event to the callback actor, or outgoing
    bool incoming = state -> target == listener;

    // Outgoing callbacks always target the target of the state object
    if ( ! incoming )
    {
      return state -> target;
    }

    // Incoming callbacks target either the callback actor, or the source of the incoming state.
    // Which is selected depends on the type of the callback proc action.
    //
    // Technically, this information is exposed in the client data, but simc needs a proper
    // targeting system first before we start using it.
    switch ( proc_action -> type )
    {
      // Heals are always targeted to the callback actor on incoming events
      case ACTION_ABSORB:
      case ACTION_HEAL:
        return listener;
      // The rest are targeted to the source of the callback event
      default:
        return state -> action -> player;
    }
  }

  rng::rng_t& rng() const
  { return listener -> rng(); }

private:
  bool roll( action_t* action )
  {
    if ( rppm )
      return rppm -> trigger();
    else if ( ppm > 0 )
      return rng().roll( action -> ppm_proc_chance( ppm ) );
    else if ( proc_chance > 0 )
      return rng().roll( proc_chance );

    assert( false );
    return false;
  }

protected:
  /**
   * Base rules for proc execution.
   * 1) If we proc a buff, trigger it
   * 2a) If the buff triggered and is at max stack, and we have an action,
   *     execute the action on the target of the action that triggered this
   *     proc.
   * 2b) If we have no buff, trigger the action on the target of the action
   *     that triggered this proc.
   *
   * TODO: Ticking buffs, though that'd be better served by fusing tick_buff_t
   * into buff_t.
   * TODO: Proc delay
   * TODO: Buff cooldown hackery for expressions. Is this really needed or can
   * we do it in a smarter way (better expression support?)
   */
  virtual void execute( action_t* /* a */, action_state_t* state )
  {
    if ( state->target->is_sleeping() )
    {
      return;
    }

    bool triggered = proc_buff == nullptr;
    if ( proc_buff )
      triggered = proc_buff -> trigger();

    if ( triggered && proc_action &&
         ( ! proc_buff || proc_buff -> check() == proc_buff -> max_stack() ) )
    {
      proc_action -> target = target( state );
      proc_action -> schedule_execute();

      // Decide whether to expire the buff even with 1 max stack
      if ( proc_buff && proc_buff -> max_stack() > 1 )
      {
        proc_buff -> expire();
      }
    }
  }
};

// Action Priority List =====================================================

struct action_priority_t
{
  std::string action_;
  std::string comment_;

  action_priority_t( const std::string& a, const std::string& c ) :
    action_( a ), comment_( c )
  { }

  action_priority_t* comment( const std::string& c )
  { comment_ = c; return this; }
};

struct action_priority_list_t
{
  // Internal ID of the action list, used in conjunction with the "new"
  // call_action_list action, that allows for potential infinite loops in the
  // APL.
  unsigned internal_id;
  uint64_t internal_id_mask;
  std::string name_str;
  std::string action_list_comment_str;
  std::string action_list_str;
  std::vector<action_priority_t> action_list;
  player_t* player;
  bool used;
  std::vector<action_t*> foreground_action_list;
  std::vector<action_t*> off_gcd_actions;
  std::vector<action_t*> cast_while_casting_actions;
  int random; // Used to determine how faceroll something actually is. :D
  action_priority_list_t( std::string name, player_t* p, const std::string& list_comment = std::string() ) :
    internal_id( 0 ), internal_id_mask( 0 ), name_str( name ), action_list_comment_str( list_comment ), player( p ), used( false ),
    foreground_action_list(), off_gcd_actions(), cast_while_casting_actions(), random( 0 )
  { }

  action_priority_t* add_action( const std::string& action_priority_str, const std::string& comment = std::string() );
  action_priority_t* add_action( const player_t* p, const spell_data_t* s, const std::string& action_name,
                                 const std::string& action_options = std::string(), const std::string& comment = std::string() );
  action_priority_t* add_action( const player_t* p, const std::string& name, const std::string& action_options = std::string(),
                                 const std::string& comment = std::string() );
  action_priority_t* add_talent( const player_t* p, const std::string& name, const std::string& action_options = std::string(),
                                 const std::string& comment = std::string() );
};

struct travel_event_t : public event_t
{
  action_t* action;
  action_state_t* state;
  travel_event_t( action_t* a, action_state_t* state, timespan_t time_to_travel );
  virtual ~travel_event_t() { if ( state && canceled ) action_state_t::release( state ); }
  virtual void execute() override;
  virtual const char* name() const override
  { return "Stateless Action Travel"; }
};

#include "dbc/item_database.hpp"

// Procs ====================================================================

namespace special_effect
{
  void parse_special_effect_encoding( special_effect_t& effect, const std::string& str );
  bool usable_proc( const special_effect_t& effect );
}

// Enchants =================================================================

namespace enchant
{
  struct enchant_db_item_t
  {
    const char* enchant_name;
    unsigned    enchant_id;
  };

  unsigned find_enchant_id( const std::string& name );
  std::string find_enchant_name( unsigned enchant_id );
  std::string encoded_enchant_name( const dbc_t&, const item_enchantment_data_t& );

  const item_enchantment_data_t& find_item_enchant( const item_t& item, const std::string& name );
  const item_enchantment_data_t& find_meta_gem( const dbc_t& dbc, const std::string& encoding );
  meta_gem_e meta_gem_type( const dbc_t& dbc, const item_enchantment_data_t& );
  bool passive_enchant( item_t& item, unsigned spell_id );

  void initialize_item_enchant( item_t& item, std::vector< stat_pair_t >& stats, special_effect_source_e source, const item_enchantment_data_t& enchant );
  item_socket_color initialize_gem( item_t& item, size_t gem_idx );
  item_socket_color initialize_relic( item_t& item, size_t relic_idx, const gem_property_data_t& gem_property );
}

// Unique Gear ==============================================================

namespace unique_gear
{
  typedef std::function<void(special_effect_t&)> custom_cb_t;
  typedef void(*custom_fp_t)(special_effect_t&);

  struct wrapper_callback_t : public scoped_callback_t
  {
    custom_cb_t cb;

    wrapper_callback_t( const custom_cb_t& cb_ ) :
      scoped_callback_t(), cb( cb_ )
    { }

    bool valid( const special_effect_t& ) const override
    { return true; }

    void initialize( special_effect_t& effect ) override
    { cb( effect ); }
  };

  // A scoped special effect callback that validates against a player class or specialization.
  struct class_scoped_callback_t : public scoped_callback_t
  {
    std::vector<player_e> class_;
    std::vector<specialization_e> spec_;

    class_scoped_callback_t( player_e c ) :
      scoped_callback_t( PRIORITY_CLASS ), class_( { c } )
    { }

    class_scoped_callback_t( const std::vector<player_e>& c ) :
      scoped_callback_t( PRIORITY_CLASS ), class_( c )
    { }

    class_scoped_callback_t( specialization_e s ) :
      scoped_callback_t( PRIORITY_SPEC ), spec_( { s } )
    { }

    class_scoped_callback_t( const std::vector<specialization_e>& s ) :
      scoped_callback_t( PRIORITY_SPEC ), spec_( s )
    { }

    bool valid( const special_effect_t& effect ) const override
    {
      assert( effect.player );

      if ( class_.size() > 0 && range::find( class_, effect.player -> type ) == class_.end() )
      {
        return false;
      }

      if ( spec_.size() > 0 && range::find( spec_, effect.player -> specialization() ) == spec_.end() )
      {
        return false;
      }

      return true;
    }
  };

  // A templated, class-scoped special effect initializer that automatically generates a single buff
  // on the actor.
  //
  // First template argument is the actor class (e.g., shaman_t), required parameter
  // Second template argument is the buff type (e.g., buff_t, stat_buff_t, ...)
  template <typename T, typename T_BUFF = buff_t>
  struct class_buff_cb_t : public class_scoped_callback_t
  {
    private:
    // Note, the dummy buff is assigned to from multiple threads, but since it is not meant to be
    // accessed, it does not really matter what buff ends up there.
    T_BUFF* __dummy;

    public:
    typedef class_buff_cb_t<T, T_BUFF> super;

    // The buff name. Optional if create_buff and create_fallback are both overridden. If fallback
    // behavior is required and the method is not overridden, must be provided.
    const std::string buff_name;

    class_buff_cb_t( specialization_e spec, const std::string& name = std::string() ) :
      class_scoped_callback_t( spec ), __dummy( nullptr ), buff_name( name )
    { }

    class_buff_cb_t( player_e class_, const std::string& name = std::string() ) :
      class_scoped_callback_t( class_ ), __dummy( nullptr ), buff_name( name )
    { }

    T* actor( const special_effect_t& e ) const
    { return debug_cast<T*>( e.player ); }

    // Generic initializer for the class-scoped special effect buff creator. Override to customize
    // buff creation if a simple binary fallback yes/no switch is not enough.
    void initialize( special_effect_t& effect ) override
    {
      if ( ! is_fallback( effect ) )
      {
        create_buff( effect );
      }
      else
      {
        create_fallback( effect );
      }
    }

    // Determine (from the given special_effect_t) whether to create the real buff, or the fallback
    // buff. All fallback special effects will have their source set to
    // SPECIAL_EFFECT_SOURCE_FALLBACK.
    virtual bool is_fallback( const special_effect_t& e ) const
    { return e.source == SPECIAL_EFFECT_SOURCE_FALLBACK; }

    // Create a correct type buff creator. Derived classes can override this method to fully
    // customize the buff creation.
    virtual T_BUFF* creator( const special_effect_t& e ) const
    { return new T_BUFF( e.player, buff_name ); }

    // An accessor method to return the assignment pointer for the buff (in the actor). Primary use
    // is to automatically assign fallback buffs to correct member variables. If the special effect
    // requires a fallback buff, and create_fallback is not fully overridden, must be implemented.
    virtual T_BUFF*& buff_ptr( const special_effect_t& )
    { return __dummy; }

    // Create the real buff, default implementation calls creator() to create a correct buff
    // creator, that will then instantiate the buff to the assigned member variable pointer returned
    // by buff_ptr.
    virtual void create_buff( const special_effect_t& e )
    { buff_ptr( e ) = creator( e ); }

    // Create a generic fallback buff. If the special effect requires a fallback buff, the developer
    // must also provide the following attributes, if the method is not overridden.
    // 1) A non-empty buff_name in the constructor
    // 2) Overridden buff_ptr method that returns a reference to a pointer where the buff should be
    //    assigned to.
    virtual void create_fallback( const special_effect_t& e )
    {
      assert( ! buff_name.empty() );
      // Assert here, but note that release builds will not crash, rather the buff is assigned
      // to a dummy pointer inside this initializer object
      assert( &buff_ptr( e ) != &__dummy );

      // Proc chance is hardcoded to zero, essentially disabling the buff described by creator()
      // call.
      buff_ptr( e ) = creator( e );
      buff_ptr( e )->set_chance( 0 );
      if ( e.player -> sim -> debug )
      {
        e.player -> sim -> out_debug.printf( "Player %s created fallback buff for %s",
            e.player -> name(), buff_name.c_str() );
      }
    }
  };

  // Manipulate actor somehow (using an overridden manipulate method)
  template<typename T_ACTOR>
  struct scoped_actor_callback_t : public class_scoped_callback_t
  {
    typedef scoped_actor_callback_t<T_ACTOR> super;

    scoped_actor_callback_t( player_e c ) : class_scoped_callback_t( c )
    { }

    scoped_actor_callback_t( specialization_e s ) : class_scoped_callback_t( s )
    { }

    void initialize( special_effect_t& e ) override
    { manipulate( debug_cast<T_ACTOR*>( e.player ), e ); }

    // Overridable method to manipulate the actor. Must be implemented.
    virtual void manipulate( T_ACTOR* actor, const special_effect_t& e ) = 0;
  };

  // Manipulate the action somehow (using an overridden manipulate method)
  template<typename T_ACTION = action_t>
  struct scoped_action_callback_t : public class_scoped_callback_t
  {
    typedef scoped_action_callback_t<T_ACTION> super;

    const std::string name;
    const int spell_id;

    scoped_action_callback_t( player_e c, const std::string& n ) :
      class_scoped_callback_t( c ), name( n ), spell_id( -1 )
    { }

    scoped_action_callback_t( specialization_e s, const std::string& n ) :
      class_scoped_callback_t( s ), name( n ), spell_id( -1 )
    { }

    scoped_action_callback_t( player_e c, unsigned sid ) :
      class_scoped_callback_t( c ), spell_id( sid )
    { }

    scoped_action_callback_t( specialization_e s, unsigned sid ) :
      class_scoped_callback_t( s ), spell_id( sid )
    { }

    // Initialize the callback by manipulating the action(s)
    void initialize( special_effect_t& e ) override
    {
      // "Snapshot" size's value so we don't attempt to manipulate any actions created during the loop.
      size_t size = e.player -> action_list.size();

      for ( size_t i = 0; i < size; i++ )
      {
        action_t* a = e.player -> action_list[ i ];

        if ( ( ! name.empty() && util::str_compare_ci( name, a -> name_str ) ) ||
             ( spell_id > 0 && spell_id == as<int>( a -> id ) ) )
        {
          if ( a -> sim -> debug )
          {
            e.player -> sim -> out_debug.printf( "Player %s manipulating action %s",
                e.player -> name(), a -> name() );
          }
          manipulate( debug_cast<T_ACTION*>( a ), e );
        }
      }
    }

    // Overridable method to manipulate the action. Must be implemented.
    virtual void manipulate( T_ACTION* action, const special_effect_t& e ) = 0;
  };

  // Manipulate the buff somehow (using an overridden manipulate method)
  template<typename T_BUFF = buff_t>
  struct scoped_buff_callback_t : public class_scoped_callback_t
  {
    typedef scoped_buff_callback_t<T_BUFF> super;

    const std::string name;
    const int spell_id;

    scoped_buff_callback_t( player_e c, const std::string& n ) :
      class_scoped_callback_t( c ), name( n ), spell_id( -1 )
    { }

    scoped_buff_callback_t( specialization_e s, const std::string& n ) :
      class_scoped_callback_t( s ), name( n ), spell_id( -1 )
    { }

    scoped_buff_callback_t( player_e c, unsigned sid ) :
      class_scoped_callback_t( c ), spell_id( sid )
    { }

    scoped_buff_callback_t( specialization_e s, unsigned sid ) :
      class_scoped_callback_t( s ), spell_id( sid )
    { }

    // Initialize the callback by manipulating the action(s)
    void initialize( special_effect_t& e ) override
    {
      // "Snapshot" size's value so we don't attempt to manipulate any buffs created during the loop.
      size_t size = e.player -> buff_list.size();

      for ( size_t i = 0; i < size; i++ )
      {
        buff_t* b = e.player -> buff_list[ i ];

        if ( ( ! name.empty() && util::str_compare_ci( name, b -> name_str ) ) ||
              ( spell_id > 0 && spell_id == as<int>( b -> data().id() ) ) )
        {
          if ( b -> sim -> debug )
          {
            e.player -> sim -> out_debug.printf( "Player %s manipulating buff %s",
                e.player -> name(), b -> name() );
          }
          manipulate( debug_cast<T_BUFF*>( b ), e );
        }
      }
    }

    // Overridable method to manipulate the action. Must be implemented.
    virtual void manipulate( T_BUFF* buff, const special_effect_t& e ) = 0;
  };

  struct special_effect_db_item_t
  {
    unsigned    spell_id;
    std::string encoded_options;
    scoped_callback_t* cb_obj;
    bool fallback;

    special_effect_db_item_t() :
      spell_id( 0 ), encoded_options(), cb_obj( nullptr ), fallback( false )
    { }
  };

typedef std::vector<const special_effect_db_item_t*> special_effect_set_t;

void register_hotfixes();
void register_hotfixes_legion();
void register_hotfixes_bfa();
void register_special_effects();
void register_special_effects_legion(); // Legion special effects
void register_special_effects_bfa(); // Battle for Azeroth special effects
void sort_special_effects();
void unregister_special_effects();

void add_effect( const special_effect_db_item_t& );
special_effect_set_t find_special_effect_db_item( unsigned spell_id );

// Old-style special effect registering functions
void register_special_effect( unsigned spell_id, const char* encoded_str );
void register_special_effect( unsigned spell_id, const custom_cb_t& init_cb );
void register_special_effect( unsigned spell_id, const custom_fp_t& init_cb );

// New-style special effect registering function
template <typename T>
void register_special_effect( unsigned spell_id, const T& cb, bool fallback = false )
{
  static_assert( std::is_base_of<scoped_callback_t, T>::value,
      "register_special_effect callback object must be derived from scoped_callback_t" );

  special_effect_db_item_t dbitem;
  dbitem.spell_id = spell_id;
  dbitem.cb_obj = new T( cb );
  dbitem.fallback = fallback;

  add_effect( dbitem );
}

void register_target_data_initializers( sim_t* );
void register_target_data_initializers_legion( sim_t* ); // Legion targetdata initializers
void register_target_data_initializers_bfa( sim_t* ); // Battle for Azeroth targetdata initializers

void init( player_t* );

special_effect_t* find_special_effect( player_t* actor, unsigned spell_id, special_effect_e = SPECIAL_EFFECT_NONE );

// First-phase special effect initializers
void initialize_special_effect( special_effect_t& effect, unsigned spell_id );
void initialize_special_effect_fallbacks( player_t* actor );
// Second-phase special effect initializer
void initialize_special_effect_2( special_effect_t* effect );

// Initialize special effects related to various race spells
void initialize_racial_effects( player_t* );

const item_data_t* find_consumable( const dbc_t& dbc, const std::string& name, item_subclass_consumable type );
const item_data_t* find_item_by_spell( const dbc_t& dbc, unsigned spell_id );

std::unique_ptr<expr_t> create_expression( player_t& player, const std::string& name_str );

// Kludge to automatically apply all player-derived, label based modifiers to unique effects. Will
// be replaced in the future by something else.
void apply_label_modifiers( action_t* a );

// Base template for various "proc actions".
template <typename T_ACTION>
struct proc_action_t : public T_ACTION
{
  using super = T_ACTION;
  using base_action_t = proc_action_t<T_ACTION>;
  const special_effect_t* effect;

  void __initialize()
  {
    this->background = true;
    this->hasted_ticks = false;

    this->callbacks = !this->data().flags( spell_attribute::SX_DISABLE_PLAYER_PROCS );
    this->may_crit = !this->data().flags( spell_attribute::SX_CANNOT_CRIT );
    this->tick_may_crit = this->data().flags( spell_attribute::SX_TICK_MAY_CRIT );
    this->may_miss = !this->data().flags( spell_attribute::SX_ALWAYS_HIT );
    this->may_dodge = this->may_parry =
      this->may_block = !this->data().flags( spell_attribute::SX_NO_D_P_B );

    if ( this -> radius > 0 )
      this -> aoe = -1;

    bool has_dot = false;
    // Reparse effect data for any item-dependent variables.
    for ( size_t i = 1; i <= this -> data().effect_count(); i++ )
    {
      this -> parse_effect_data( this -> data().effectN( i ) );
      if ( this -> data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
      {
        has_dot = true;
      }
    }

    // Auto-infer dot max stack
    if ( has_dot && this -> data().max_stacks() > 1 )
    {
      this -> dot_max_stack = this -> data().max_stacks();
    }

    this->hasted_ticks = this -> data().flags( spell_attribute::SX_DOT_HASTED );
    this->tick_on_application = this->data().flags( spell_attribute::SX_TICK_ON_APPLICATION );

    unique_gear::apply_label_modifiers( this );
  }

  proc_action_t( const special_effect_t& e ) :
    super( e.name(), e.player, e.trigger() ),
    effect(&e)
  {
    this -> item = e.item;
    this -> cooldown = e.player -> get_cooldown( e.cooldown_name() );

    __initialize();
  }

  void init() override
  {
    super::init();
    if ( effect )
    {

      override_data( *effect );
    }
  }

  proc_action_t( const std::string& token, player_t* p, const spell_data_t* s, const item_t* i = nullptr ) :
    super( token, p, s ),
    effect(nullptr)
  {
    this -> item = i;

    __initialize();
  }

  virtual void override_data( const special_effect_t& e );
};

// Base proc spells used by the generic special effect initialization
struct proc_spell_t : public proc_action_t<spell_t>
{
  using super = proc_action_t<spell_t>;

  proc_spell_t( const special_effect_t& e ) :
    super( e )
  { }

  proc_spell_t( const std::string& token, player_t* p, const spell_data_t* s, const item_t* i = nullptr ) :
    super( token, p, s, i )
  { }
};

struct proc_heal_t : public proc_action_t<heal_t>
{
  using super = proc_action_t<heal_t>;

  proc_heal_t( const std::string& token, player_t* p, const spell_data_t* s, const item_t* i = nullptr ) :
    super( token, p, s, i )
  { }

  proc_heal_t( const special_effect_t& e ) :
    super( e )
  { }
};

struct proc_attack_t : public proc_action_t<attack_t>
{
  using super = proc_action_t<attack_t>;

  proc_attack_t( const special_effect_t& e ) :
    super( e )
  {
  }

  proc_attack_t( const std::string& token, player_t* p, const spell_data_t* s, const item_t* i = nullptr ) :
    super( token, p, s, i )
  { }

  void override_data( const special_effect_t& e ) override;
};

struct proc_resource_t : public proc_action_t<spell_t>
{
  using super = proc_action_t<spell_t>;

  gain_t* gain;
  double gain_da, gain_ta;
  resource_e gain_resource;

  // Note, not called by proc_action_t
  void __initialize()
  {
    may_miss = may_dodge = may_parry = may_block = harmful = false;
    target = player;

    for ( size_t i = 1; i <= data().effect_count(); i++ )
    {
      const spelleffect_data_t& effect = data().effectN( i );
      if ( effect.type() == E_ENERGIZE )
      {
        gain_da = effect.average( item );
        gain_resource = effect.resource_gain_type();
      }
      else if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_ENERGIZE )
      {
        gain_ta = effect.average( item );
        gain_resource = effect.resource_gain_type();
      }
    }

    gain = player -> get_gain( name() );
  }

  proc_resource_t( const special_effect_t& e ) :
    super( e ), gain_da( 0 ), gain_ta( 0 ), gain_resource( RESOURCE_NONE )
  {
    __initialize();
  }

  proc_resource_t( const std::string& token, player_t* p, const spell_data_t* s, const item_t* item_ = nullptr ) :
    super( token, p, s, item_ ), gain_da( 0 ), gain_ta( 0 ), gain_resource( RESOURCE_NONE )
  {
    __initialize();
  }

  result_e calculate_result( action_state_t* /* state */ ) const override
  { return RESULT_HIT; }

  void init() override
  {
    super::init();

    snapshot_flags = update_flags = 0;
  }

  void execute() override
  {
    super::execute();

    player -> resource_gain( gain_resource, gain_da, gain );
  }

  void tick( dot_t* d ) override
  {
    super::tick( d );

    player -> resource_gain( gain_resource, gain_ta, gain );
  }
};

template <typename CLASS, typename ...ARGS>
action_t* create_proc_action( const std::string& name, const special_effect_t& effect, ARGS&&... args )
{
  auto player = effect.player;
  auto a = player -> find_action( name );

  if ( a == nullptr )
  {
    a = player -> create_proc_action( name, effect );
  }

  if ( a == nullptr )
  {
    a = new CLASS( effect, args... );
  }

  return a;
}

template <typename BUFF, typename ...ARGS>
BUFF* create_buff( player_t* p, const std::string& name, ARGS&&... args )
{
  auto b = buff_t::find( p, name );
  if ( b != nullptr )
  {
    return debug_cast<BUFF*>( b );
  }

  return make_buff<BUFF>( p, name, args... );
}
} // namespace unique_gear ends

// Consumable ===============================================================

namespace consumable
{
action_t* create_action( player_t*, const std::string& name, const std::string& options );
}

// Wowhead  =================================================================

namespace wowhead
{
// 2016-07-20: Wowhead's XML output for item stats produces weird results on certain items that are
// no longer available in game. Skip very high values to let the sim run, but not use completely
// silly values.
enum
{
  WOWHEAD_STAT_MAX = 10000
};

enum wowhead_e
{
  LIVE,
  PTR,
  BETA
};

bool download_item( item_t&, wowhead_e source = LIVE, cache::behavior_e b = cache::items() );
bool download_item_data( item_t&            item,
                         cache::behavior_e  caching,
                         wowhead_e          source );

std::string domain_str( wowhead_e domain );
}


// Blizzard Community Platform API ==========================================

namespace bcp_api
{
bool download_guild( sim_t* sim,
                     const std::string& region,
                     const std::string& server,
                     const std::string& name,
                     const std::vector<int>& ranks,
                     int player_e = PLAYER_NONE,
                     int max_rank = 0,
                     cache::behavior_e b = cache::players() );

player_t* download_player( sim_t*,
                           const std::string& region,
                           const std::string& server,
                           const std::string& name,
                           const std::string& talents = std::string( "active" ),
                           cache::behavior_e b = cache::players(),
                           bool allow_failures = false );

player_t* from_local_json( sim_t*,
                           const std::string&,
                           const std::string&,
                           const std::string&
                         );

bool download_item( item_t&, cache::behavior_e b = cache::items() );
void token_load();
void token_save();

slot_e translate_api_slot( const std::string& slot_str );
}

// HTTP Download  ===========================================================

namespace http
{
struct proxy_t
{
  std::string type;
  std::string host;
  int port;
};
void set_proxy( const std::string& type, const std::string& host, const unsigned port );

void cache_load( const std::string& file_name );
void cache_save( const std::string& file_name );
bool clear_cache( sim_t*, const std::string& name, const std::string& value );

int get( std::string& result, const std::string& url,
          cache::behavior_e caching, const std::string& confirmation = "",
          const std::vector<std::string>& headers = {} );
}

// XML ======================================================================
#include "util/xml.hpp"

// Handy Actions ============================================================

struct wait_action_base_t : public action_t
{
  wait_action_base_t( player_t* player, const std::string& name ):
    action_t( ACTION_OTHER, name, player )
  {
    trigger_gcd = timespan_t::zero();
    interrupt_auto_attack = false;
  }

  virtual void execute() override
  { player -> iteration_waiting_time += time_to_execute; }
};

// Wait For Cooldown Action =================================================

struct wait_for_cooldown_t : public wait_action_base_t
{
  cooldown_t* wait_cd;
  action_t* a;
  wait_for_cooldown_t( player_t* player, const std::string& cd_name );
  virtual bool usable_moving() const override { return a -> usable_moving(); }
  virtual timespan_t execute_time() const override;
};

// Namespace for a ignite like action. Not mandatory, but encouraged to use it.
namespace residual_action
{
// Custom state for ignite-like actions. tick_amount contains the current
// ticking value of the ignite periodic effect, and is adjusted every time
// residual_periodic_action_t (the ignite spell) impacts on the target.
struct residual_periodic_state_t : public action_state_t
{
  double tick_amount;

  residual_periodic_state_t( action_t* a, player_t* t ) :
    action_state_t( a, t ),
    tick_amount( 0 )
  { }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  { action_state_t::debug_str( s ) << " tick_amount=" << tick_amount; return s; }

  void initialize() override
  { action_state_t::initialize(); tick_amount = 0; }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    const residual_periodic_state_t* dps_t = debug_cast<const residual_periodic_state_t*>( o );
    tick_amount = dps_t -> tick_amount;
  }
};

template <class Base>
struct residual_periodic_action_t : public Base
{
private:
  typedef Base ab; // typedef for the templated action type, spell_t, or heal_t
public:
  typedef residual_periodic_action_t base_t;
  typedef residual_periodic_action_t<Base> residual_action_t;

  template <typename T>
  residual_periodic_action_t( const std::string& n, T& p, const spell_data_t* s ) :
    ab( n, p, s )
  {
    initialize_();
  }

  template <typename T>
  residual_periodic_action_t( const std::string& n, T* p, const spell_data_t* s ) :
    ab( n, p, s )
  {
    initialize_();
  }

  void initialize_()
  {
    ab::background = true;

    ab::tick_may_crit = false;
    ab::hasted_ticks  = false;
    ab::may_crit = false;
    ab::attack_power_mod.tick = 0;
    ab::spell_power_mod.tick = 0;
    ab::dot_behavior  = DOT_REFRESH;
    ab::callbacks = false;
  }

  virtual action_state_t* new_state() override
  { return new residual_periodic_state_t( this, ab::target ); }

  // Residual periodic actions will not be extendeed by the pandemic mechanism,
  // thus the new maximum length of the dot is the ongoing tick plus the
  // duration of the dot.
  virtual timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
  { return dot -> time_to_next_tick() + triggered_duration; }

  virtual void impact( action_state_t* s ) override
  {
    // Residual periodic actions + tick_zero does not work
    assert( ! ab::tick_zero );

    dot_t* dot = ab::get_dot( s -> target );
    double current_amount = 0, old_amount = 0;
    int ticks_left = 0;
    residual_periodic_state_t* dot_state = debug_cast<residual_periodic_state_t*>( dot -> state );

    // If dot is ticking get current residual pool before we overwrite it
    if ( dot -> is_ticking() )
    {
      old_amount = dot_state -> tick_amount;
      ticks_left = dot -> ticks_left();
      current_amount = old_amount * dot -> ticks_left();
    }

    // Add new amount to residual pool
    current_amount += s -> result_amount;

    // Trigger the dot, refreshing it's duration or starting it
    ab::trigger_dot( s );

    // If the dot is not ticking, dot_state will be nullptr, so get the
    // residual_periodic_state_t object from the dot again (since it will exist
    // after trigger_dot() is called)
    if ( ! dot_state )
      dot_state = debug_cast<residual_periodic_state_t*>( dot -> state );

    // Compute tick damage after refresh, so we divide by the correct number of
    // ticks
    dot_state -> tick_amount = current_amount / dot -> ticks_left();

    // Spit out debug for what we did
    if ( ab::sim -> debug )
      ab::sim -> out_debug.printf( "%s %s residual_action impact amount=%f old_total=%f old_ticks=%d old_tick=%f current_total=%f current_ticks=%d current_tick=%f",
                  ab::player -> name(), ab::name(), s -> result_amount,
                  old_amount * ticks_left, ticks_left, ticks_left > 0 ? old_amount : 0,
                  current_amount, dot -> ticks_left(), dot_state -> tick_amount );
  }

  // The damage of the tick is simply the tick_amount in the state
  virtual double base_ta( const action_state_t* s ) const override
  {
    auto dot = ab::find_dot( s -> target );
    if ( dot )
    {
      auto rd_state = debug_cast<const residual_periodic_state_t*>( dot -> state );
      return rd_state -> tick_amount;
    }
    return 0.0;
  }

  // Ensure that not travel time exists for the ignite ability. Delay is
  // handled in the trigger via a custom event
  virtual timespan_t travel_time() const override
  { return timespan_t::zero(); }

  // This object is not "executable" normally. Instead, the custom event
  // handles the triggering of ignite
  virtual void execute() override
  { assert( 0 ); }

  // Ensure that the ignite action snapshots nothing
  virtual void init() override
  {
    ab::init();

    ab::update_flags = ab::snapshot_flags = 0;
  }
};

// Trigger a residual periodic action on target t
void trigger( action_t* residual_action, player_t* t, double amount );

}  // namespace residual_action

#include "player/expansion_effects.hpp"

// Inlines ==================================================================

inline bool player_t::is_my_pet( const player_t* t ) const
{ return t -> is_pet() && t -> cast_pet() -> owner == this; }


/**
 * Perform dynamic resource regeneration
 */
inline void player_t::do_dynamic_regen()
{
  if ( sim -> current_time() == last_regen )
    return;

  regen( sim -> current_time() - last_regen );
  last_regen = sim -> current_time();

  if ( dynamic_regen_pets )
  {
    for (auto & elem : active_pets)
    {
      if ( elem -> resource_regeneration == regen_type::DYNAMIC)
        elem -> do_dynamic_regen();
    }
  }
}

inline target_wrapper_expr_t::target_wrapper_expr_t( action_t& a, const std::string& name_str, const std::string& expr_str ) :
  expr_t( name_str ), action( a ), suffix_expr_str( expr_str )
{
  std::generate_n(std::back_inserter(proxy_expr), action.sim->actor_list.size(), []{ return std::unique_ptr<expr_t>(); });
}

inline double target_wrapper_expr_t::evaluate()
{
  assert( target() );

  size_t actor_index = target() -> actor_index;

  if ( proxy_expr[ actor_index ] == nullptr )
  {
    proxy_expr[ actor_index ] = target() -> create_expression( suffix_expr_str );
  }

  return proxy_expr[ actor_index ] -> eval();
}

inline player_t* target_wrapper_expr_t::target() const
{ return action.target; }

inline actor_target_data_t::actor_target_data_t( player_t* target, player_t* source ) :
  actor_pair_t( target, source ), debuff(), dot()
{
  for (auto & elem : source -> sim -> target_data_initializer)
  {
    elem( this );
  }
}

// Real PPM inlines

inline real_ppm_t::real_ppm_t( const std::string& name, player_t* p, const spell_data_t* data, const item_t* item ) :
  player( p ),
  name_str( name ),
  freq( data -> real_ppm() ),
  modifier( p -> dbc.real_ppm_modifier( data -> id(), player, item ? item -> item_level() : 0 ) ),
  rppm( freq * modifier ),
  last_trigger_attempt( timespan_t::zero() ),
  last_successful_trigger( timespan_t::zero() ),
  scales_with( p -> dbc.real_ppm_scale( data -> id() ) ),
  blp_state( BLP_ENABLED )
{ }

inline double real_ppm_t::proc_chance( player_t*         player,
                                       double            PPM,
                                       const timespan_t& last_trigger,
                                       const timespan_t& last_successful_proc,
                                       unsigned          scales_with,
                                       blp               blp_state )
{
  auto sim = player->sim;
  double coeff = 1.0;
  double seconds = std::min( ( sim->current_time() - last_trigger ).total_seconds(), max_interval() );

  if ( scales_with & RPPM_HASTE )
    coeff *= player->cache.rppm_haste_coeff();

  // This might technically be two separate crit values, but this should be sufficient for our
  // cases. In any case, the client data does not offer information which crit it is (attack or
  // spell).
  if ( scales_with & RPPM_CRIT )
    coeff *= player->cache.rppm_crit_coeff();

  if ( scales_with & RPPM_ATTACK_SPEED )
    coeff *= 1.0 / player -> cache.attack_speed();

  double real_ppm = PPM * coeff;
  double old_rppm_chance = real_ppm * ( seconds / 60.0 );
  double rppm_chance = 0;

  if ( blp_state == BLP_ENABLED )
  {
    // RPPM Extension added on 12. March 2013: http://us.battle.net/wow/en/blog/8953693?page=44
    // Formula see http://us.battle.net/wow/en/forum/topic/8197741003#1
    double last_success = std::min( ( sim->current_time() - last_successful_proc ).total_seconds(), max_bad_luck_prot() );
    double expected_average_proc_interval = 60.0 / real_ppm;

    rppm_chance = std::max( 1.0, 1 + ( ( last_success / expected_average_proc_interval - 1.5 ) * 3.0 ) )  * old_rppm_chance;
  }
  else
  {
    rppm_chance = old_rppm_chance;
  }

  if ( sim->debug )
  {
    sim->out_debug.print( "base={:.3f} coeff={:.3f} last_trig={:.3f} last_proc={:.3f}"
                          " scales={} blp={} chance={:.5f}%",
        PPM, coeff, last_trigger.total_seconds(), last_successful_proc.total_seconds(),
        scales_with, blp_state == BLP_ENABLED ? "enabled" : "disabled", rppm_chance * 100.0 );
  }

  return rppm_chance;
}

inline bool real_ppm_t::trigger()
{
  if ( freq <= 0 )
  {
    return false;
  }

  if ( last_trigger_attempt == player->sim->current_time() )
    return false;

  double chance = proc_chance( player, rppm, last_trigger_attempt, last_successful_trigger,
      scales_with, blp_state );
  bool success = player->rng().roll( chance );

  last_trigger_attempt = player->sim->current_time();

  if ( success )
    last_successful_trigger = player->sim->current_time();
  return success;
}

// Shuffle Proc inlines

inline bool shuffled_rng_t::trigger()
{
  if (total_entries <= 0 || success_entries <= 0)
  {
    return false;
  }

  if (total_entries_remaining <= 0)
  {
    reset(); // Re-Shuffle the "Deck"
  }

  bool result = false;
  if (success_entries_remaining > 0)
  {
    result = player->rng().roll(get_remaining_success_chance());
    if (result)
    {
      success_entries_remaining--;
    }
  }

  total_entries_remaining--;

  if (total_entries_remaining <= 0)
  {
    reset(); // Re-Shuffle the "Deck"
  }

  return result;
}

// Instant absorbs
struct instant_absorb_t
{
private:
  /* const spell_data_t* spell; */
  std::function<double( const action_state_t* )> absorb_handler;
  stats_t* absorb_stats;
  gain_t*  absorb_gain;
  player_t* player;

public:
  std::string name;

  instant_absorb_t( player_t* p, const spell_data_t* s, const std::string n,
                    std::function<double( const action_state_t* )> handler ) :
    /* spell( s ), */ absorb_handler( handler ), player( p ), name( n )
  {
    util::tokenize( name );

    absorb_stats = p -> get_stats( name );
    absorb_gain  = p -> get_gain( name );
    absorb_stats -> type = STATS_ABSORB;
    absorb_stats -> school = s -> get_school_type();
  }

  double consume( const action_state_t* s )
  {
    double amount = std::min( s -> result_amount, absorb_handler( s ) );

    if ( amount > 0 )
    {
      absorb_stats -> add_result( amount, 0, result_amount_type::ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, player );
      absorb_stats -> add_execute( timespan_t::zero(), player );
      absorb_gain -> add( RESOURCE_HEALTH, amount, 0 );

      if ( player -> sim -> debug )
        player -> sim -> out_debug.printf( "%s %s absorbs %.2f", player -> name(), name.c_str(), amount );
    }

    return amount;
  }
};

inline timespan_t cooldown_t::queueable() const
{
  return ready - player->cooldown_tolerance();
}

inline bool cooldown_t::is_ready() const
{
  if ( up() )
  {
    return true;
  }

  // Cooldowns that are not bound to specific actions should not be considered as queueable in the
  // simulator, ever, so just return up() here. This limits the actual queueable cooldowns to
  // basically only abilities, where the user must press a button to initiate the execution. Note
  // that off gcd abilities that bypass schedule_execute (i.e., action_t::use_off_gcd is set to
  // true) will for now not use the queueing system.
  if ( ! action || ! player )
  {
    return up();
  }

  // Cooldown is not up, and is action-bound (essentially foreground action), check if it's within
  // the player's (or default) cooldown tolerance for queueing.
  return queueable() <= sim.current_time();
}

template <class T>
sim_ostream_t& sim_ostream_t::operator<< (T const& rhs)
{
  fmt::fprintf(*_raw.get_stream(), "%.3f ", sim.current_time().total_seconds());
  _raw << rhs << "\n";

  return *this;
}


struct ground_aoe_params_t
{
  enum hasted_with
  {
    NOTHING = 0,
    SPELL_HASTE,
    SPELL_SPEED,
    ATTACK_HASTE,
    ATTACK_SPEED
  };

  enum expiration_pulse_type
  {
    NO_EXPIRATION_PULSE = 0,
    FULL_EXPIRATION_PULSE,
    PARTIAL_EXPIRATION_PULSE
  };

  enum state_type
  {
    EVENT_STARTED = 0,  // Ground aoe event started
    EVENT_CREATED,      // A new ground_aoe_event_t object created
    EVENT_DESTRUCTED,   // A ground_aoe_Event_t object destructed
    EVENT_STOPPED       // Ground aoe event stopped
  };

  using param_cb_t = std::function<void(void)>;
  using state_cb_t = std::function<void(state_type, ground_aoe_event_t*)>;

  player_t* target_;
  double x_, y_;
  hasted_with hasted_;
  action_t* action_;
  timespan_t pulse_time_, start_time_, duration_;
  expiration_pulse_type expiration_pulse_;
  unsigned n_pulses_;
  param_cb_t expiration_cb_;
  state_cb_t state_cb_;

  ground_aoe_params_t() :
    target_( nullptr ), x_( -1 ), y_( -1 ), hasted_( NOTHING ), action_( nullptr ),
    pulse_time_( timespan_t::from_seconds( 1.0 ) ), start_time_( timespan_t::min() ),
    duration_( timespan_t::zero() ),
    expiration_pulse_( NO_EXPIRATION_PULSE ), n_pulses_( 0 ), expiration_cb_( nullptr )
  { }

  player_t* target() const { return target_; }
  double x() const { return x_; }
  double y() const { return y_; }
  hasted_with hasted() const { return hasted_; }
  action_t* action() const { return action_; }
  const timespan_t& pulse_time() const { return pulse_time_; }
  const timespan_t& start_time() const { return start_time_; }
  const timespan_t& duration() const { return duration_; }
  expiration_pulse_type expiration_pulse() const { return expiration_pulse_; }
  unsigned n_pulses() const { return n_pulses_; }
  const param_cb_t& expiration_callback() const { return expiration_cb_; }
  const state_cb_t& state_callback() const { return state_cb_; }

  ground_aoe_params_t& target( player_t* p )
  {
    target_ = p;
    if ( start_time_ == timespan_t::min() )
    {
      start_time_ = target_ -> sim -> current_time();
    }

    if ( x_ == -1 )
    {
      x_ = target_ -> x_position;
    }

    if ( y_ == -1 )
    {
      y_ = target_ -> y_position;
    }

    return *this;
  }

  ground_aoe_params_t& action( action_t* a )
  {
    action_ = a;
    if ( start_time_ == timespan_t::min() )
    {
      start_time_ = action_ -> sim -> current_time();
    }

    return *this;
  }

  ground_aoe_params_t& x( double x )
  { x_ = x; return *this; }

  ground_aoe_params_t& y( double y )
  { y_ = y; return *this; }

  ground_aoe_params_t& hasted( hasted_with state )
  { hasted_ = state; return *this; }

  ground_aoe_params_t& pulse_time( const timespan_t& t )
  { pulse_time_ = t; return *this; }

  ground_aoe_params_t& start_time( const timespan_t& t )
  { start_time_ = t; return *this; }

  ground_aoe_params_t& duration( const timespan_t& t )
  { duration_ = t; return *this; }

  ground_aoe_params_t& expiration_pulse( expiration_pulse_type state )
  { expiration_pulse_ = state; return *this; }

  ground_aoe_params_t& n_pulses( unsigned n )
  { n_pulses_ = n; return *this; }

  ground_aoe_params_t& expiration_callback( const param_cb_t& cb )
  { expiration_cb_ = cb; return *this; }

  ground_aoe_params_t& state_callback( const state_cb_t& cb )
  { state_cb_ = cb; return *this; }
};

// Delayed expiration callback for groud_aoe_event_t
struct expiration_callback_event_t : public event_t
{
  ground_aoe_params_t::param_cb_t callback;

  expiration_callback_event_t( sim_t& sim, const ground_aoe_params_t* p, const timespan_t& delay ) :
    event_t( sim, delay ), callback( p -> expiration_callback() )
  { }

  void execute() override
  { callback(); }
};

// Fake "ground aoe object" for things. Pulses until duration runs out, does not perform partial
// ticks (fits as many ticks as possible into the duration). Intended to be able to spawn multiple
// ground aoe objects, each will have separate state. Currently does not snapshot stats upon object
// creation. Parametrization through object above (ground_aoe_params_t).
struct ground_aoe_event_t : public player_event_t
{
  // Pointer needed here, as simc event system cannot fit all params into event_t
  const ground_aoe_params_t* params;
  action_state_t* pulse_state;
  unsigned current_pulse;

protected:
  template <typename Event, typename... Args>
  friend Event* make_event( sim_t& sim, Args&&... args );
  // Internal constructor to schedule next pulses, not to be used outside of the struct (or derived
  // structs)
  ground_aoe_event_t( player_t* p, const ground_aoe_params_t* param,
                      action_state_t* ps, bool immediate_pulse = false )
    : player_event_t(
          *p, immediate_pulse ? timespan_t::zero() : _pulse_time( param, p ) ),
      params( param ),
      pulse_state( ps ), current_pulse( 1 )
  {
    // Ensure we have enough information to start pulsing.
    assert( params -> target() != nullptr && "No target defined for ground_aoe_event_t" );
    assert( params -> action() != nullptr && "No action defined for ground_aoe_event_t" );
    assert( params -> pulse_time() > timespan_t::zero() &&
            "Pulse time for ground_aoe_event_t must be a positive value" );
    assert( params -> start_time() >= timespan_t::zero() &&
            "Start time for ground_aoe_event must be defined" );
    assert( ( params -> duration() > timespan_t::zero() || params -> n_pulses() > 0 ) &&
            "Duration or number of pulses for ground_aoe_event_t must be defined" );

    // Make a state object that persists for this ground aoe event throughout its lifetime
    if ( ! pulse_state )
    {
      pulse_state = params -> action() -> get_state();
      action_t* spell_ = params -> action();
      spell_ -> snapshot_state( pulse_state, spell_ -> amount_type( pulse_state ) );
    }

    if ( params -> state_callback() )
    {
      params -> state_callback()( ground_aoe_params_t::EVENT_CREATED, this );
    }
  }
public:
  // Make a copy of the parameters, and use that object until this event expires
  ground_aoe_event_t( player_t* p, const ground_aoe_params_t& param, bool immediate_pulse = false ) :
    ground_aoe_event_t( p, new ground_aoe_params_t( param ), nullptr, immediate_pulse )
  {
    if ( params -> state_callback() )
    {
      params -> state_callback()( ground_aoe_params_t::EVENT_STARTED, this );
    }
  }

  // Cleans up memory for any on-going ground aoe events when the iteration ends, or when the ground
  // aoe finishes during iteration.
  ~ground_aoe_event_t()
  {
    delete params;
    if ( pulse_state )
    {
      action_state_t::release( pulse_state );
    }
  }

  void set_current_pulse( unsigned v )
  { current_pulse = v; }

  static timespan_t _time_left( const ground_aoe_params_t* params, const player_t* p )
  { return params -> duration() - ( p -> sim -> current_time() - params -> start_time() ); }

  static timespan_t _pulse_time( const ground_aoe_params_t* params, const player_t* p, bool clamp = true )
  {
    auto tick = params -> pulse_time();
    auto time_left = _time_left( params, p );

    switch ( params -> hasted() )
    {
      case ground_aoe_params_t::SPELL_SPEED:
        tick *= p -> cache.spell_speed();
        break;
      case ground_aoe_params_t::SPELL_HASTE:
        tick *= p -> cache.spell_haste();
        break;
      case ground_aoe_params_t::ATTACK_SPEED:
        tick *= p -> cache.attack_speed();
        break;
      case ground_aoe_params_t::ATTACK_HASTE:
        tick *= p -> cache.attack_haste();
        break;
      default:
        break;
    }

    // Clamping can only occur on duration-based ground aoe events.
    if ( params -> n_pulses() == 0 && clamp && tick > time_left )
    {
      assert( params -> expiration_pulse() != ground_aoe_params_t::NO_EXPIRATION_PULSE );
      return time_left;
    }

    return tick;
  }

  timespan_t remaining_time() const
  { return _time_left( params, player() ); }

  bool may_pulse() const
  {
    if ( params -> n_pulses() > 0 )
    {
      return current_pulse < params -> n_pulses();
    }
    else
    {
      auto time_left = _time_left( params, player() );

      if ( params -> expiration_pulse() == ground_aoe_params_t::NO_EXPIRATION_PULSE )
      {
        return time_left >= pulse_time( false );
      }
      else
      {
        return time_left > timespan_t::zero();
      }
    }
  }

  virtual timespan_t pulse_time( bool clamp = true ) const
  { return _pulse_time( params, player(), clamp ); }

  virtual void schedule_event()
  {
    auto event = make_event<ground_aoe_event_t>( sim(), _player, params, pulse_state );
    // If the ground-aoe event is a pulse-based one, increase the current pulse of the newly created
    // event.
    if ( params -> n_pulses() > 0 )
    {
      event -> set_current_pulse( current_pulse + 1 );
    }
  }

  const char* name() const override
  { return "ground_aoe_event"; }

  void execute() override
  {
    action_t* spell_ = params -> action();

    if ( sim().debug )
    {
      sim().out_debug.printf( "%s %s pulse start_time=%.3f remaining_time=%.3f tick_time=%.3f",
          player() -> name(), spell_ -> name(), params -> start_time().total_seconds(),
          params -> n_pulses() > 0
            ? ( params -> n_pulses() - current_pulse ) * pulse_time().total_seconds()
            : ( params -> duration() - ( sim().current_time() - params -> start_time() ) ).total_seconds(),
          pulse_time( false ).total_seconds() );
    }

    // Manually snapshot the state so we can adjust the x and y coordinates of the snapshotted
    // object. This is relevant if sim -> distance_targeting_enabled is set, since then we need to
    // use the ground object's x, y coordinates, instead of the source actor's.
    spell_ -> update_state( pulse_state, spell_ -> amount_type( pulse_state ) );
    pulse_state -> target = params -> target();
    pulse_state -> original_x = params -> x();
    pulse_state -> original_y = params -> y();

    // Update state multipliers if expiration_pulse() is PARTIAL_PULSE, and the object is pulsing
    // for the last (partial) time. Note that pulse-based ground aoe events do not have a concept of
    // partial ticks.
    if ( params -> n_pulses() == 0 &&
         params -> expiration_pulse() == ground_aoe_params_t::PARTIAL_EXPIRATION_PULSE )
    {
      // Don't clamp the pulse time here, since we need to figure out the fractional multiplier for
      // the last pulse.
      auto time = pulse_time( false );
      auto time_left = _time_left( params, player() );
      if ( time > time_left )
      {
        double multiplier = time_left / time;
        pulse_state -> da_multiplier *= multiplier;
        pulse_state -> ta_multiplier *= multiplier;
      }
    }

    spell_ -> schedule_execute( spell_ -> get_state( pulse_state ) );

    // This event is about to be destroyed, notify callback of the event if needed
    if ( params -> state_callback() )
    {
      params -> state_callback()( ground_aoe_params_t::EVENT_DESTRUCTED, this );
    }

    // Schedule next tick, if it can fit into the duration
    if ( may_pulse() )
    {
      schedule_event();
      // Ugly hack-ish, but we want to re-use the parmas and pulse state objects while this ground
      // aoe is pulsing, so nullptr the params from this (soon to be recycled) event.
      params = nullptr;
      pulse_state = nullptr;
    }
    else
    {
      handle_expiration();

      // This event is about to be destroyed, notify callback of the event if needed
      if ( params -> state_callback() )
      {
        params -> state_callback()( ground_aoe_params_t::EVENT_STOPPED, this );
      }
    }
  }

  // Figure out how to handle expiration callback if it's defined
  void handle_expiration()
  {
    if ( ! params -> expiration_callback() )
    {
      return;
    }

    auto time_left = _time_left( params, player() );

    // Trigger immediately, since no time left. Can happen for example when ground aoe events are
    // not hasted, or when pulse-based behavior is used (instead of duration-based behavior)
    if ( time_left <= timespan_t::zero() )
    {
      params -> expiration_callback()();
    }
    // Defer until the end of the ground aoe event, even if there are no ticks left
    else
    {
      make_event<expiration_callback_event_t>( sim(), sim(), params, time_left );
    }
  }
};

// Player Callbacks

// effect_callbacks_t::register_callback =====================================

template <typename T>
inline void add_callback( std::vector<T*>& callbacks, T* cb )
{
  if ( range::find( callbacks, cb ) == callbacks.end() )
    callbacks.push_back( cb );
}

template <typename T_CB>
void effect_callbacks_t<T_CB>::add_proc_callback( proc_types type,
                                                  unsigned flags,
                                                  T_CB* cb )
{
  std::stringstream s;
  if ( sim -> debug )
    s << "Registering procs: ";

  // Setup the proc-on-X types for the proc
  for ( proc_types2 pt = PROC2_TYPE_MIN; pt < PROC2_TYPE_MAX; pt++ )
  {
    if ( ! ( flags & ( 1 << pt ) ) )
      continue;

    // Healing and ticking spells all require "an amount" on landing, so
    // automatically convert a "proc on spell landed" type to "proc on
    // hit/crit".
    if ( pt == PROC2_LANDED &&
         ( type == PROC1_PERIODIC || type == PROC1_PERIODIC_TAKEN ||
           type == PROC1_PERIODIC_HEAL || type == PROC1_PERIODIC_HEAL_TAKEN ||
           type == PROC1_NONE_HEAL || type == PROC1_MAGIC_HEAL ) )
    {
      add_callback( procs[ type ][ PROC2_HIT  ], cb );
      if ( cb -> listener -> sim -> debug )
        s << util::proc_type_string( type ) << util::proc_type2_string( PROC2_HIT ) << " ";

      add_callback( procs[ type ][ PROC2_CRIT ], cb );
      if ( cb -> listener -> sim -> debug )
        s << util::proc_type_string( type ) << util::proc_type2_string( PROC2_CRIT ) << " ";
    }
    // Do normal registration based on the existence of the flag
    else
    {
      add_callback( procs[ type ][ pt ], cb );
      if ( cb -> listener -> sim -> debug )
        s << util::proc_type_string( type ) << util::proc_type2_string( pt ) << " ";
    }
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s", s.str().c_str() );
}

template <typename T_CB>
void effect_callbacks_t<T_CB>::register_callback( unsigned proc_flags,
                                                  unsigned proc_flags2,
                                                  T_CB* cb )
{
  // We cannot default the "what kind of abilities proc this callback" flags,
  // they need to be non-zero
  assert( proc_flags != 0 && cb != 0 );

  if ( sim -> debug )
    sim -> out_debug.printf( "Registering callback proc_flags=%#.8x proc_flags2=%#.8x",
        proc_flags, proc_flags2 );

  // Default method for proccing is "on spell landing", if no "what type of
  // result procs this callback" is given
  if ( proc_flags2 == 0 )
    proc_flags2 = PF2_LANDED;

  for ( proc_types t = PROC1_TYPE_MIN; t < PROC1_TYPE_BLIZZARD_MAX; t++ )
  {
    // If there's no proc-by-X, we don't need to add anything
    if ( ! ( proc_flags & ( 1 << t ) ) )
      continue;

    // Skip periodic procs, they are handled below as a special case
    if ( t == PROC1_PERIODIC || t == PROC1_PERIODIC_TAKEN )
      continue;

    add_proc_callback( t, proc_flags2, cb );
  }

  // Periodic X done
  if ( proc_flags & PF_PERIODIC )
  {
    // 1) Periodic damage only. This is the default behavior of our system when
    // only PROC1_PERIODIC is defined on a trinket.
    if ( ! ( proc_flags & PF_ALL_HEAL ) &&                                               // No healing ability type flags
         ! ( proc_flags2 & PF2_PERIODIC_HEAL ) )                                         // .. nor periodic healing result type flag
    {
      add_proc_callback( PROC1_PERIODIC, proc_flags2, cb );
    }
    // 2) Periodic heals only. Either inferred by a "proc by direct heals" flag,
    //    or by "proc on periodic heal ticks" flag, but require that there's
    //    no direct / ticked spell damage in flags.
    else if ( ( ( proc_flags & PF_ALL_HEAL ) || ( proc_flags2 & PF2_PERIODIC_HEAL ) ) && // Healing ability
              ! ( proc_flags & PF_ALL_DAMAGE ) &&                                        // .. with no damaging ability type flags
              ! ( proc_flags2 & PF2_PERIODIC_DAMAGE ) )                                  // .. nor periodic damage result type flag
    {
      add_proc_callback( PROC1_PERIODIC_HEAL, proc_flags2, cb );
    }
    // Both
    else
    {
      add_proc_callback( PROC1_PERIODIC, proc_flags2, cb );
      add_proc_callback( PROC1_PERIODIC_HEAL, proc_flags2, cb );
    }
  }

  // Periodic X taken
  if ( proc_flags & PF_PERIODIC_TAKEN )
  {
    // 1) Periodic damage only. This is the default behavior of our system when
    // only PROC1_PERIODIC is defined on a trinket.
    if ( ! ( proc_flags & PF_ALL_HEAL_TAKEN ) &&                                         // No healing ability type flags
         ! ( proc_flags2 & PF2_PERIODIC_HEAL ) )                                         // .. nor periodic healing result type flag
    {
      add_proc_callback( PROC1_PERIODIC_TAKEN, proc_flags2, cb );
    }
    // 2) Periodic heals only. Either inferred by a "proc by direct heals" flag,
    //    or by "proc on periodic heal ticks" flag, but require that there's
    //    no direct / ticked spell damage in flags.
    else if ( ( ( proc_flags & PF_ALL_HEAL_TAKEN ) || ( proc_flags2 & PF2_PERIODIC_HEAL ) ) && // Healing ability
              ! ( proc_flags & PF_DAMAGE_TAKEN ) &&                                        // .. with no damaging ability type flags
              ! ( proc_flags & PF_ANY_DAMAGE_TAKEN ) &&                                    // .. nor Blizzard's own "any damage taken" flag
              ! ( proc_flags2 & PF2_PERIODIC_DAMAGE ) )                                    // .. nor periodic damage result type flag
    {
      add_proc_callback( PROC1_PERIODIC_HEAL_TAKEN, proc_flags2, cb );
    }
    // Both
    else
    {
      add_proc_callback( PROC1_PERIODIC_TAKEN, proc_flags2, cb );
      add_proc_callback( PROC1_PERIODIC_HEAL_TAKEN, proc_flags2, cb );
    }
  }
}

template <typename T_CB>
void effect_callbacks_t<T_CB>::reset()
{
  T_CB::reset( all_callbacks );
}

/**
 * Targetdata initializer for items. When targetdata is constructed (due to a call to
 * player_t::get_target_data failing to find an object for the given target), all targetdata
 * initializers in the sim are invoked. Most class specific targetdata is handled by the derived
 * class-specific targetdata, however there are a couple of trinkets that require "generic"
 * targetdata support. Item targetdata initializers will create the relevant debuffs/buffs needed.
 * Note that the buff/debuff needs to be created always, since expressions for buffs/debuffs presume
 * the buff exists or the whole sim fails to init.
 *
 * See unique_gear::register_target_data_initializers() for currently supported target-based debuffs
 * in generic items.
 */
struct item_targetdata_initializer_t
{
  unsigned item_id;
  std::vector< slot_e > slots_;

  item_targetdata_initializer_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_id( iid ), slots_( s )
  { }

  virtual ~item_targetdata_initializer_t() {}

  // Returns the special effect based on item id and slots to source from. Overridable if more
  // esoteric functionality is needed
  virtual const special_effect_t* find_effect( player_t* player ) const
  {
    // No need to check items on pets/enemies
    if ( player -> is_pet() || player -> is_enemy() || player -> type == HEALING_ENEMY )
    {
      return 0;
    }

    for ( size_t i = 0; i < slots_.size(); ++i )
    {
      if ( player -> items[slots_[ i ] ].parsed.data.id == item_id )
      {
        return player -> items[slots_[ i ] ].parsed.special_effects[ 0 ];
      }
    }

    return 0;
  }

  // Override to initialize the targetdata object.
  virtual void operator()( actor_target_data_t* ) const = 0;
};

namespace report
{
class buff_decorator_t : public spell_decorator_t<buff_t>
{
  using super = spell_decorator_t<buff_t>;

protected:
  std::vector<std::string> parms() const override;

public:
  buff_decorator_t( const buff_t* obj ) :
    super( obj )
  { }

  buff_decorator_t( const buff_t& obj ) :
    buff_decorator_t( &obj )
  { }

  // Buffs have pet names included in them
  std::string url_name_prefix() const override;
};

class action_decorator_t : public spell_decorator_t<action_t>
{
  using super = spell_decorator_t<action_t>;

protected:
  std::vector<std::string> parms() const override;

public:
  action_decorator_t( const action_t* obj ) :
    super( obj )
  { }

  action_decorator_t( const action_t& obj ) :
    action_decorator_t( &obj )
  { }
};

}

namespace spawner
{
void merge( sim_t& parent_sim, sim_t& other_sim );
void create_persistent_actors( player_t& player );

// Minimal base class to store in owner actors automatically, all functionality should be
// implemented in a templated class (pet_spawner_t for example). Methods that need to be invoked
// from the core simulator should be declared here as pure virtual (must be implemented in the
// derived class).
class base_actor_spawner_t
{
protected:
  std::string m_name;
  player_t*   m_owner;

public:
  base_actor_spawner_t( const std::string& id, player_t* o ) :
    m_name( id ), m_owner( o )
  {
    register_object();
  }

  virtual ~base_actor_spawner_t()
  { }

  const std::string& name() const
  { return m_name; }

  virtual void create_persistent_actors() = 0;

  // Data merging
  virtual void merge( base_actor_spawner_t* other ) = 0;

  // Expressions
  virtual std::unique_ptr<expr_t> create_expression( const arv::array_view<std::string>& expr ) = 0;

  // Uptime
  virtual timespan_t iteration_uptime() const = 0;

  // State reset
  virtual void reset() = 0;

  // Data collection
  virtual void datacollection_end() = 0;

private:
  // Register this pet spawner object to owner
  void register_object()
  {
    auto it = range::find_if( m_owner -> spawners,
        [ this ]( const base_actor_spawner_t* obj ) {
          return util::str_compare_ci( obj -> name(), name() );
        } );

    if ( it == m_owner -> spawners.end() )
    {
      m_owner -> spawners.push_back( this );
    }
    else
    {
      m_owner -> sim -> errorf( "%s attempting to create duplicate pet spawner object %s",
        m_owner -> name(), name().c_str() );
    }
  }
};
} // Namespace spawner ends

/**
 * Snapshot players stats during pre-combat to get raid-buffed stats values.
 *
 * This allows us to report "raid-buffed" player stats by collecting the values through this action,
 * which is executed by the player action system.
 */
struct snapshot_stats_t : public action_t
{
  bool completed;
  spell_t* proxy_spell;
  attack_t* proxy_attack;
  role_e role;

  snapshot_stats_t( player_t* player, const std::string& options_str );

  void init_finished() override;
  void execute() override;
  void reset() override;
  bool ready() override;
};
#endif // SIMULATIONCRAFT_H
