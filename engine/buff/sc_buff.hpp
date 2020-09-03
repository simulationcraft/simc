// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "util/timespan.hpp"
#include "sc_enums.hpp"
#include "dbc/dbc.hpp"
#include "player/sc_actor_pair.hpp"
#include "player/covenant.hpp"
#include "util/sample_data.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"
#include "util/timeline.hpp"
#include "sim/uptime.hpp"
#include "util/format.hpp"

struct buff_t;
struct stat_buff_t;
struct absorb_buff_t;
struct cost_reduction_buff_t;
struct actor_pair_t;
struct sim_t;
struct action_t;
struct item_t;
struct gain_t;
struct action_state_t;
struct stats_t;
struct event_t;
struct cooldown_t;
struct real_ppm_t;
struct expr_t;
struct spell_data_t;
namespace rng{
struct rng_t;
}


using buff_tick_callback_t = std::function<void(buff_t*, int, timespan_t)>;
using buff_tick_time_callback_t = std::function<timespan_t(const buff_t*, unsigned)>;
using buff_refresh_duration_callback_t = std::function<timespan_t(const buff_t*, timespan_t)>;
using buff_stack_change_callback_t = std::function<void(buff_t*, int, int)>;

// Buffs ====================================================================

struct buff_t : private noncopyable
{
public:
  sim_t* const sim;
  player_t* const player;
  const item_t* const item;
  const std::string name_str;
  const spell_data_t* s_data;
  const spell_data_t* s_data_reporting;
  player_t* const source;
  std::vector<event_t*> expiration;
  event_t* delay;
  event_t* expiration_delay;
  cooldown_t* cooldown;
  sc_timeline_t uptime_array;
  real_ppm_t* rppm;

  // static values
private: // private because changing max_stacks requires resizing some stack-dependant vectors
  int _max_stack;
  const spell_data_t* trigger_data;

public:
  double default_value;
  size_t default_value_effect_idx;
  double default_value_effect_multiplier;

  /**
   * Is buff manually activated or not (eg. a proc).
   * non-activated player buffs have a delayed trigger event
   */
  bool activated;
  bool reactable;
  bool reverse, constant, quiet, overridden, can_cancel;
  bool requires_invalidation;

  int reverse_stack_reduction; /// Number of stacks reduced when reverse = true

  // dynamic values
  double current_value;
  int current_stack;
  timespan_t base_buff_duration;
  double buff_duration_multiplier;
  double default_chance;
  double manual_chance; // user-specified "overridden" proc-chance
  std::vector<timespan_t> stack_react_time;
  std::vector<event_t*> stack_react_ready_triggers;

  buff_refresh_behavior refresh_behavior;
  buff_refresh_duration_callback_t refresh_duration_callback;
  buff_stack_behavior stack_behavior;
  buff_stack_change_callback_t stack_change_callback;

  // Ticking buff values
  unsigned current_tick;
  timespan_t buff_period;
  buff_tick_time_behavior tick_time_behavior;
  buff_tick_behavior tick_behavior;
  event_t* tick_event;
  buff_tick_callback_t tick_callback;
  buff_tick_time_callback_t tick_time_callback;
  bool tick_zero;
  bool tick_on_application; // Immediately tick when the buff first goes up, but not on refreshes
  bool partial_tick; // Allow non-full duration ticks at the end of the buff period

  // tmp data collection
protected:
  timespan_t last_start;
  timespan_t last_trigger;
  timespan_t last_expire;
  timespan_t last_stack_change;
  timespan_t iteration_uptime_sum;
  timespan_t last_benefite_update;
  unsigned int up_count, down_count, start_count, refresh_count, expire_count;
  unsigned int overflow_count, overflow_total;
  int trigger_attempts, trigger_successes;
  int simulation_max_stack;
  std::vector<cache_e> invalidate_list;
  gcd_haste_type gcd_type;

  // report data
public:
  simple_sample_data_t benefit_pct, trigger_pct;
  simple_sample_data_t avg_start, avg_refresh, avg_expire;
  simple_sample_data_t avg_overflow_count, avg_overflow_total;
  simple_sample_data_t uptime_pct;
  simple_sample_data_with_min_max_t start_intervals, trigger_intervals, duration_lengths;
  std::vector<uptime_simple_t> stack_uptime;

  virtual ~buff_t() {}

  buff_t(actor_pair_t q, util::string_view name);
  buff_t( actor_pair_t q, util::string_view name, const spell_data_t*, const item_t* item = nullptr );
  buff_t(sim_t* sim, util::string_view name);
  buff_t( sim_t* sim, util::string_view name, const spell_data_t*, const item_t* item = nullptr );
protected:
  buff_t( sim_t* sim, player_t* target, player_t* source, util::string_view name, const spell_data_t*, const item_t* item );
public:
  const spell_data_t& data() const { return *s_data; }
  const spell_data_t& data_reporting() const;

  /**
   * Get current number of stacks, no benefit tracking.
   * Use check() inside of ready() and cost() methods to prevent skewing of "benefit" calculations.
   * Use up() where the presence of the buff affects the action mechanics.
   */
  int check() const
  {
    return current_stack;
  }

  /**
   * Get current number of stacks + benefit tracking.
   * Use check() inside of ready() and cost() methods to prevent skewing of "benefit" calculations.
   * This will only modify the benefit counters once per sim time unit.
   * Use up()/down where the presence of the buff affects the action mechanics.
   */
  int stack();

  /**
   * Check if buff is up
   * Use check() inside of ready() and cost() methods to prevent skewing of "benefit" calculations.
   * Use up()/down where the presence of the buff affects the action mechanics.
   */
  bool up()
  {
    return stack() > 0;
  }

  /**
   * Get current buff value + benefit tracking.
   */
  virtual double value()
  {
    stack();
    return current_value;
  }

  /**
   * Get current buff value  multiplied by current stacks + benefit tracking.
   */
  double stack_value()
  {
    return current_stack * value();
  }

  /**
   * Get current buff value + NO benefit tracking.
   */
  double check_value() const
  {
    return current_value;
  }

  /**
   * Get current buff value  multiplied by current stacks + NO benefit tracking.
   */
  double check_stack_value() const
  {
    return current_stack * check_value();
  }

  /**
   * Return true if tick should not call bump() or decrement()
   */
  virtual bool freeze_stacks()
  {
    return false;
  }

  // Get the base buff duration modified by the duration multiplier, if applicable
  virtual timespan_t buff_duration() const
  {
    return base_buff_duration * buff_duration_multiplier;
  }

  timespan_t remains() const;
  timespan_t elapsed( timespan_t t ) const { return t - last_start; }
  timespan_t last_trigger_time() const { return last_trigger; }
  timespan_t last_expire_time() const { return last_expire; }
  bool   remains_gt( timespan_t time ) const;
  bool   remains_lt( timespan_t time ) const;
  bool   trigger  ( action_t*, int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  bool   trigger  ( timespan_t duration );
  bool   trigger  ( int stacks, timespan_t duration );
  virtual bool   trigger  ( int stacks = 1, double value = DEFAULT_VALUE(), double chance = -1.0, timespan_t duration = timespan_t::min() );
  virtual void   execute ( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void   increment( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void   decrement( int stacks = 1, double value = DEFAULT_VALUE() );
  virtual void   extend_duration( player_t* p, timespan_t seconds );

  virtual void start    ( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void refresh  ( int stacks = 0, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void bump     ( int stacks = 1, double value = DEFAULT_VALUE() );
  virtual void override_buff ( int stacks = 1, double value = DEFAULT_VALUE() );
  virtual bool may_react( int stacks = 1 );
  virtual int stack_react();
  // NOTE: If you need to override behavior on buff expire, use expire_override. Override "expire"
  // method only if you _REALLY_ know what you are doing.
  virtual void expire( timespan_t delay = timespan_t::zero() );

  // Called only if previously active buff expires
  virtual void expire_override( int /* expiration_stacks */, timespan_t /* remaining_duration */ ) {}
  virtual void predict();
  virtual void reset();
  virtual void aura_gain();
  virtual void aura_loss();
  virtual void merge( const buff_t& other_buff );
  virtual void analyze();
  virtual void datacollection_begin();
  virtual void datacollection_end();

  virtual timespan_t refresh_duration( timespan_t new_duration ) const;
  virtual timespan_t tick_time() const;

#if defined(SC_USE_STAT_CACHE)
  virtual void invalidate_cache();
#else
  void invalidate_cache() {}
#endif

  virtual int total_stack();

  static std::unique_ptr<expr_t> create_expression( util::string_view buff_name,
                                    util::string_view type,
                                    action_t& action );
  static std::unique_ptr<expr_t> create_expression( util::string_view buff_name,
                                    util::string_view type,
                                    buff_t& static_buff );
  std::string to_str() const;

  static double DEFAULT_VALUE() { return -std::numeric_limits< double >::min(); }
  static buff_t* find( util::span<buff_t* const>, util::string_view name, player_t* source = nullptr );
  static buff_t* find(    sim_t*, util::string_view name );
  static buff_t* find( player_t*, util::string_view name, player_t* source = nullptr );
  static buff_t* find_expressable( util::span<buff_t* const>, util::string_view name, player_t* source = nullptr );

  const char* name() const { return name_str.c_str(); }
  util::string_view source_name() const;
  int max_stack() const { return _max_stack; }
  const spell_data_t* get_trigger_data() const
  { return trigger_data; }

  rng::rng_t& rng();

  bool change_regen_rate;

  buff_t* set_chance( double chance );
  buff_t* set_duration( timespan_t duration );
  buff_t* modify_duration( timespan_t duration );
  buff_t* set_duration_multiplier( double );
  buff_t* set_max_stack( int max_stack );
  buff_t* modify_max_stack( int max_stack );
  buff_t* set_cooldown( timespan_t duration );
  buff_t* modify_cooldown( timespan_t duration );
  buff_t* set_period( timespan_t );
  //virtual buff_t* set_chance( double chance );
  buff_t* set_quiet( bool quiet );
  buff_t* add_invalidate( cache_e );
  buff_t* set_default_value( double, size_t = 0 );
  buff_t* set_default_value_from_effect( size_t, double = 0.01 );
  buff_t* set_default_value_from_effect_type( effect_subtype_t a_type,
                                              property_type_t p_type = P_GENERIC,
                                              double multiplier      = 0.0,
                                              effect_type_t e_type   = E_APPLY_AURA );
  buff_t* modify_default_value( double );
  buff_t* set_reverse( bool );
  buff_t* set_activated( bool );
  buff_t* set_can_cancel( bool cc );
  buff_t* set_tick_behavior( buff_tick_behavior );
  buff_t* set_tick_callback( buff_tick_callback_t );
  buff_t* set_tick_time_callback( buff_tick_time_callback_t );
  buff_t* set_affects_regen( bool state );
  buff_t* set_refresh_behavior( buff_refresh_behavior );
  buff_t* set_refresh_duration_callback( buff_refresh_duration_callback_t );
  buff_t* set_tick_zero( bool v )
  { tick_zero = v; return this; }
  buff_t* set_tick_on_application( bool v )
  { tick_on_application = v; return this; }
  buff_t* set_partial_tick( bool v )
  { partial_tick = v; return this; }
  buff_t* set_tick_time_behavior( buff_tick_time_behavior b )
  { tick_time_behavior = b; return this; }
  buff_t* set_rppm( rppm_scale_e scale = RPPM_NONE, double freq = -1, double mod = -1);
  buff_t* set_trigger_spell( const spell_data_t* s );
  buff_t* set_stack_change_callback( const buff_stack_change_callback_t& cb );
  buff_t* set_reverse_stack_count( int value );
  buff_t* set_stack_behavior( buff_stack_behavior b );

  buff_t* apply_affecting_aura( const spell_data_t* spell );
  buff_t* apply_affecting_effect( const spelleffect_data_t& effect );
  buff_t* apply_affecting_conduit( const conduit_data_t& conduit, int effect_num = 1 );
  buff_t* apply_affecting_conduit_effect( const conduit_data_t& conduit, size_t effect_num );

  friend void format_to( const buff_t&, fmt::format_context::iterator );
private:
  void update_trigger_calculations();
  void adjust_haste();
  void init_haste_type();

protected:
  void update_stack_uptime_array( timespan_t current_time, int old_stacks );
};

struct stat_buff_t : public buff_t
{
  struct buff_stat_t
  {
    stat_e stat;
    double amount;
    double current_value;
    std::function<bool(const stat_buff_t&)> check_func;

    buff_stat_t( stat_e s, double a, std::function<bool(const stat_buff_t&)> c = std::function<bool(const stat_buff_t&)>() ) :
      stat( s ), amount( a ), current_value( 0 ), check_func( c ) {}
  };
  std::vector<buff_stat_t> stats;
  gain_t* stat_gain;
  bool manual_stats_added;

  virtual void bump     ( int stacks = 1, double value = -1.0 ) override;
  virtual void decrement( int stacks = 1, double value = -1.0 ) override;
  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
  virtual double value() override { stack(); return stats[ 0 ].current_value; }

  stat_buff_t* add_stat( stat_e s, double a, std::function<bool(const stat_buff_t&)> c = std::function<bool(const stat_buff_t&)>() );

  stat_buff_t(actor_pair_t q, util::string_view name);
  stat_buff_t( actor_pair_t q, util::string_view name, const spell_data_t*, const item_t* item = nullptr );
};

struct absorb_buff_t : public buff_t
{
  using absorb_eligibility = std::function<bool(const action_state_t*)>;
  school_e absorb_school;
  stats_t* absorb_source;
  gain_t*  absorb_gain;
  bool     high_priority; // For tank absorbs that should explicitly "go first"
  absorb_eligibility eligibility; // A custom function whose result determines if the attack is eligible to be absorbed.

  absorb_buff_t(actor_pair_t q, util::string_view name);
  absorb_buff_t( actor_pair_t q, util::string_view name, const spell_data_t* spell, const item_t* item = nullptr );
protected:

  // Hook for derived classes to recieve notification when some of the absorb is consumed.
  // Called after the adjustment to current_value.
  virtual void absorb_used( double /* amount */ ) {}

public:
  virtual void start( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override;
  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;

  virtual double consume( double amount );
  absorb_buff_t* set_absorb_gain( gain_t* );
  absorb_buff_t* set_absorb_source( stats_t* );
  absorb_buff_t* set_absorb_school( school_e );
  absorb_buff_t* set_absorb_high_priority( bool );
  absorb_buff_t* set_absorb_eligibility( absorb_eligibility );
};

struct cost_reduction_buff_t : public buff_t
{
  double amount;
  school_e school;

  cost_reduction_buff_t(actor_pair_t q, util::string_view name);
  cost_reduction_buff_t( actor_pair_t q, util::string_view name, const spell_data_t* spell, const item_t* item = nullptr );
  virtual void bump     ( int stacks = 1, double value = -1.0 ) override;
  virtual void decrement( int stacks = 1, double value = -1.0 ) override;
  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
  cost_reduction_buff_t* set_reduction(school_e school, double amount);
};

/**
 * @brief Creates a buff
 *
 * Small helper function to write
 * buff_t* b = make_buff<buff_t>(player, name, spell); instead of
 * buff_t* b = (new buff_t(player, name, spell));
 */
template <typename Buff = buff_t, typename... Args>
inline Buff* make_buff( Args&&... args )
{
  static_assert( std::is_base_of<buff_t, Buff>::value,
                 "Buff must be derived from buff_t" );
  return new Buff( std::forward<Args>(args)... );
}

struct movement_buff_t : public buff_t
{
  movement_buff_t( player_t* p ) : buff_t( p, "movement" )
  {
    set_max_stack( 1 );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

