// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SIMULATIONCRAFT_H
#define SIMULATIONCRAFT_H


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
struct scale_factor_control_t;
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
class shuffled_rng_t;
struct ground_aoe_event_t;
namespace spawner {
class base_actor_spawner_t;
}
namespace highchart {
  struct chart_t;
}

#include "dbc/data_enums.hh"
#include "dbc/data_definitions.hh"
#include "utf8-cpp/utf8.h"

// string formatting library
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "fmt/printf.h"

// GSL-Lite: Guideline Support Library, light version
// Lib to assist with CPP Core Guidelines.
#include "gsl-lite/gsl-lite.hpp"

// Time class representing ingame time
#include "sc_timespan.hpp"

// Generic programming tools
#include "util/generic.hpp"

#include "util/sample_data.hpp"

#include "util/timeline.hpp"

#include "util/rng.hpp"

#include "util/concurrency.hpp"

#include "sc_enums.hpp"

#include "util/cache.hpp"

#include "sim/sc_profileset.hpp"

#include "player/artifact_data.hpp"
#include "player/azerite_data.hpp"

// Legion-specific "pantheon trinket" system
#include "sim/x6_pantheon.hpp"

#include "action/sc_action_state.hpp"

#include "interfaces/sc_http.hpp"

#include "util/util.hpp"

#include "util/stopwatch.hpp"
#include "sim/sc_option.hpp"
#include "player/consumable.hpp"

// Include DBC Module
#include "dbc/dbc.hpp"

// Include IO Module
#include "util/io.hpp"

// Report ===================================================================

#include "report/sc_report.hpp"

#include "sim/artifact_power.hpp"
#include "player/sample_data_helper.hpp"

#include "sim/raid_event.hpp"

#include "player/gear_stats.hpp"

#include "player/sc_actor_pair.hpp"
#include "player/actor_target_data.hpp"

#include "buff/sc_buff.hpp"

#include "sim/sc_expressions.hpp"

#include "dbc/spell_query/spell_data_expr.hpp"

#include "sim/iteration_data_entry.hpp"

#include "sim/sim_control.hpp"
#include "sim/progress_bar.hpp"

#include "sim/event_manager.hpp"

#include "sim/sc_sim.hpp"

#include "class_modules/class_module.hpp"

#include "sim/scale_factor_control.hpp"
#include "sim/plot.hpp"
#include "sim/reforge_plot.hpp"

#include "util/plot_data.hpp"

#include "sim/event.hpp"

#include "player/rating.hpp"

#include "player/weapon.hpp"

#include "util/scoped_callback.hpp"

#include "item/special_effect.hpp"

#include "item/item.hpp"
#include "sim/benefit.hpp"
#include "sim/uptime.hpp"
#include "sim/proc.hpp"
#include "player/set_bonus.hpp"
#include "sim/real_ppm.hpp"

#include "sim/shuffled_rng.hpp"

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

#include "player/player_scaling.hpp"

#include "player/player_resources.hpp"

#include "player/sc_player.hpp"

#include "player/target_specific.hpp"

#include "player/player_event.hpp"

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

#include "player/pet.hpp"

#include "sim/gain.hpp"

#include "player/stats.hpp"

#include "action/sc_action.hpp"
#include "action/attack.hpp"
#include "action/spell_base.hpp"
#include "action/spell.hpp"
#include "action/heal.hpp"
#include "action/absorb.hpp"

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

#include "action/dot.hpp"

#include "action/action_callback.hpp"
#include "action/dbc_proc_callback.hpp"

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

#include "item/enchants.hpp"

#include "player/unique_gear.hpp"

#include "interfaces/wowhead.hpp"
#include "interfaces/bcp_api.hpp"

#include "util/xml.hpp"

// Handy Actions ============================================================

struct wait_action_base_t : public action_t
{
  wait_action_base_t( player_t* player, const std::string& name ):
    action_t( ACTION_OTHER, name, player, spell_data_t::nil() )
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

#include "player/spawner_base.hpp"

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
