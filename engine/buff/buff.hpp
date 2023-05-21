// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <memory>

#include "util/timespan.hpp"
#include "sc_enums.hpp"
#include "dbc/data_enums.hh"
#include "player/actor_pair.hpp"
#include "util/sample_data.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"
#include "util/timeline.hpp"
#include "sim/uptime.hpp"
#include "util/format.hpp"

struct buff_t;
class conduit_data_t;
struct stat_buff_t;
struct spelleffect_data_t;
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
  std::string name_str_reporting;
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
  int _initial_stack;
  const spell_data_t* trigger_data;

public:
  double default_value;
  size_t default_value_effect_idx;
  double default_value_effect_multiplier;
  unsigned schools;

  /**
   * Is buff manually activated or not (eg. a proc).
   * non-activated player buffs have a delayed trigger event
   */
  bool activated;
  bool reactable;
  bool reverse, constant, quiet, overridden, can_cancel;
  bool requires_invalidation;
  bool expire_at_max_stack;

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

  buff_constant_behavior constant_behavior;
  buff_refresh_behavior refresh_behavior;
  buff_refresh_duration_callback_t refresh_duration_callback;
  buff_stack_behavior stack_behavior;
  buff_stack_change_callback_t stack_change_callback;
  bool allow_precombat;

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
  bool freeze_stacks; // Do not increment/decrement stack on each tick

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
  simple_sample_data_with_min_max_t uptime_pct;
  simple_sample_data_with_min_max_t start_intervals, trigger_intervals, duration_lengths;
  std::vector<uptime_simple_t> stack_uptime;

  virtual ~buff_t() = default;

  buff_t( actor_pair_t q, util::string_view name );
  buff_t( actor_pair_t q, util::string_view name, const spell_data_t*, const item_t* item = nullptr );
  buff_t( sim_t* sim, util::string_view name );
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

  // Get the base buff duration modified by the duration multiplier, if applicable
  virtual timespan_t buff_duration() const
  {
    return base_buff_duration * buff_duration_multiplier;
  }

  timespan_t remains() const;
  timespan_t tick_time_remains() const;
  timespan_t elapsed( timespan_t t ) const { return t - last_start; }
  timespan_t last_trigger_time() const { return last_trigger; }
  timespan_t last_expire_time() const { return last_expire; }
  bool remains_gt( timespan_t time ) const;
  bool remains_lt( timespan_t time ) const;
  bool has_common_school( school_e ) const;
  bool at_max_stacks( int mod = 0 ) const { return check() + mod >= max_stack(); }
  // For trigger()/execute(), default value of stacks is -1, since we want to allow for explicit calls of stacks=1 to
  // override using buff_t::_initial_stack
  int _resolve_stacks( int stacks );
  bool trigger( action_t*, int stacks = -1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  bool trigger( timespan_t duration );
  bool trigger( int stacks, timespan_t duration );
  virtual bool trigger( int stacks = -1, double value = DEFAULT_VALUE(), double chance = -1.0, timespan_t duration = timespan_t::min() );
  virtual void execute( int stacks = -1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  // For increment()/decrement(), default value of stack is 1, since expectation when calling these with default value
  // is that the stack count will be adjusted by a single stack, regardless of buff_t::_initial_stack
  virtual void increment( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void decrement( int stacks = 1, double value = DEFAULT_VALUE() );
  virtual void extend_duration( player_t* p, timespan_t seconds );
  virtual void extend_duration_or_trigger( timespan_t duration = timespan_t::min(), player_t* p = nullptr );

  virtual void start( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void refresh( int stacks = 0, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() );
  virtual void bump( int stacks = 1, double value = DEFAULT_VALUE() );
  virtual void override_buff ( int stacks = 1, double value = DEFAULT_VALUE() );
  virtual bool may_react( int stacks = 1 );
  virtual int stack_react();
  // NOTE: If you need to override behavior on buff expire, use expire_override. Override "expire"
  // method only if you _REALLY_ know what you are doing.
  virtual void expire( timespan_t delay = timespan_t::zero() );
  // Completely remove the buff, including any delayed applications and expirations.
  void cancel();

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

  timespan_t iteration_uptime() const
  { return iteration_uptime_sum; }

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
  static buff_t* find( sim_t*, util::string_view name );
  static buff_t* find( player_t*, util::string_view name, player_t* source = nullptr );
  static buff_t* find_expressable( util::span<buff_t* const>, util::string_view name, player_t* source = nullptr );

  const char* name() const { return name_str.c_str(); }
  const char* name_reporting() const;
  util::string_view source_name() const;
  int max_stack() const { return _max_stack; }
  int initial_stack() const { return _initial_stack; }
  const spell_data_t* get_trigger_data() const { return trigger_data; }

  rng::rng_t& rng();

  bool change_regen_rate;

  buff_t* set_chance( double chance );
  buff_t* set_duration( timespan_t duration );
  buff_t* modify_duration( timespan_t duration );
  buff_t* set_duration_multiplier( double );
  buff_t* set_max_stack( int max_stack );
  buff_t* modify_max_stack( int max_stack );
  buff_t* set_initial_stack( int initial_stack );
  buff_t* modify_initial_stack( int initial_stack );
  buff_t* set_expire_at_max_stack( bool );
  buff_t* set_cooldown( timespan_t duration );
  buff_t* modify_cooldown( timespan_t duration );
  buff_t* set_period( timespan_t );
  buff_t* modify_period( timespan_t );
  //virtual buff_t* set_chance( double chance );
  buff_t* set_quiet( bool quiet );
  buff_t* add_invalidate( cache_e );
  buff_t* set_schools( unsigned );
  buff_t* set_schools_from_effect( size_t );
  buff_t* add_school( school_e );
  // Treat the buff's value as stat % increase and apply it automatically
  // in the relevant player_t functions.
  buff_t* set_pct_buff_type( stat_pct_buff_type );
  buff_t* set_default_value( double, size_t = 0 );
  virtual buff_t* set_default_value_from_effect( size_t, double = 0.0 );
  virtual buff_t* set_default_value_from_effect_type( effect_subtype_t a_type,
                                                      property_type_t p_type = P_MAX,
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
  buff_t* set_constant_behavior( buff_constant_behavior );
  buff_t* set_refresh_behavior( buff_refresh_behavior );
  buff_t* set_refresh_duration_callback( buff_refresh_duration_callback_t );
  buff_t* set_tick_zero( bool v ) { tick_zero = v; return this; }
  buff_t* set_tick_on_application( bool v ) { tick_on_application = v; return this; }
  buff_t* set_partial_tick( bool v ) { partial_tick = v; return this; }
  buff_t* set_freeze_stacks( bool v ) { freeze_stacks = v; return this; }
  buff_t* set_tick_time_behavior( buff_tick_time_behavior b ) { tick_time_behavior = b; return this; }
  buff_t* set_rppm( rppm_scale_e scale = RPPM_NONE, double freq = -1, double mod = -1);
  buff_t* set_trigger_spell( const spell_data_t* s );
  buff_t* set_stack_change_callback( const buff_stack_change_callback_t& cb );
  buff_t* set_reverse_stack_count( int count );
  buff_t* set_stack_behavior( buff_stack_behavior b );
  buff_t* set_allow_precombat( bool b );
  buff_t* set_name_reporting( std::string_view );

  virtual buff_t* apply_affecting_aura( const spell_data_t* spell );
  virtual buff_t* apply_affecting_effect( const spelleffect_data_t& effect );
  virtual buff_t* apply_affecting_conduit( const conduit_data_t& conduit, int effect_num = 1 );
  virtual buff_t* apply_affecting_conduit_effect( const conduit_data_t& conduit, size_t effect_num );

  friend void sc_format_to( const buff_t&, fmt::format_context::iterator );
private:
  void update_trigger_calculations();
  void adjust_haste();
  void init_haste_type();

protected:
  void update_stack_uptime_array( timespan_t current_time, int old_stacks );
};

struct stat_buff_t : public buff_t
{
  using stat_check_fn = std::function<bool( const stat_buff_t& )>;

  struct buff_stat_t
  {
    stat_e stat;
    double amount;
    double current_value;
    stat_check_fn check_func;

    buff_stat_t( stat_e s, double a,
                 std::function<bool( const stat_buff_t& )> c = std::function<bool( const stat_buff_t& )>() )
      : stat( s ), amount( a ), current_value( 0 ), check_func( std::move( c ) )
    {
    }

    double stack_amount( int stacks ) const
    {
      // Blizzard likes to use effect coefficients that give (almost) exact values at the
      // intended level. Small floating point conversion errors can add up to give the wrong
      // value. We compensate by increasing the absolute value by a tiny bit before truncating.
      double val = std::max( 1.0, std::fabs( amount ) );
      return std::copysign( std::trunc( stacks * val + 1e-3 ), amount );
    }
  };
  std::vector<buff_stat_t> stats;
  gain_t* stat_gain;
  bool manual_stats_added;

  void bump     ( int stacks = 1, double value = -1.0 ) override;
  void decrement( int stacks = 1, double value = -1.0 ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
  double value() override { stack(); return stats[ 0 ].current_value; }

  stat_buff_t* add_stat( stat_e s, double a, const stat_check_fn& c = stat_check_fn() );
  stat_buff_t* set_stat( stat_e s, double a, const stat_check_fn& c = stat_check_fn() );
  stat_buff_t* add_stat_from_effect( size_t i, double a, const stat_check_fn& c = stat_check_fn() );
  stat_buff_t* set_stat_from_effect( size_t i, double a, const stat_check_fn& c = stat_check_fn() );

  stat_buff_t( actor_pair_t q, util::string_view name );
  stat_buff_t( actor_pair_t q, util::string_view name, const spell_data_t*, const item_t* item = nullptr );
};

struct absorb_buff_t : public buff_t
{
  using absorb_eligibility = std::function<bool( const action_state_t* )>;
  school_e absorb_school;
  stats_t* absorb_source;
  gain_t*  absorb_gain;
  bool     high_priority;  // For tank absorbs that should explicitly "go first"
  bool     cumulative;     // when true, each refresh() will add to the existing shield value
  absorb_eligibility eligibility; // A custom function whose result determines if the attack is eligible to be absorbed.

  absorb_buff_t( actor_pair_t q, util::string_view name );
  absorb_buff_t( actor_pair_t q, util::string_view name, const spell_data_t* spell, const item_t* item = nullptr );
protected:

  // Hook for derived classes to recieve notification when some of the absorb is consumed.
  // Called after the adjustment to current_value.
  virtual void absorb_used( double /* amount */ ) {}

public:
  void start( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override;
  void refresh( int stacks = 0, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;

  virtual double consume( double amount );
  absorb_buff_t* set_absorb_gain( gain_t* );
  absorb_buff_t* set_absorb_source( stats_t* );
  absorb_buff_t* set_absorb_school( school_e );
  absorb_buff_t* set_absorb_high_priority( bool );
  absorb_buff_t* set_absorb_eligibility( absorb_eligibility );
  absorb_buff_t* set_cumulative( bool );
};

struct cost_reduction_buff_t : public buff_t
{
  double amount;
  school_e school;

  cost_reduction_buff_t(actor_pair_t q, util::string_view name);
  cost_reduction_buff_t( actor_pair_t q, util::string_view name, const spell_data_t* spell, const item_t* item = nullptr );
  void bump     ( int stacks = 1, double value = -1.0 ) override;
  void decrement( int stacks = 1, double value = -1.0 ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
  cost_reduction_buff_t* set_reduction(school_e school, double amount);
};

struct movement_buff_t : public buff_t
{
  movement_buff_t( player_t* p ) : buff_t( p, "movement" )
  {
    set_max_stack( 1 );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

struct damage_buff_t : public buff_t
{
  struct damage_buff_modifier_t
  {
    const spell_data_t* s_data = nullptr;
    size_t effect_idx = 0;
    double multiplier = 1.0;
  };

  bool is_stacking;
  damage_buff_modifier_t direct_mod;
  damage_buff_modifier_t periodic_mod;
  damage_buff_modifier_t auto_attack_mod;
  damage_buff_modifier_t crit_chance_mod;

  damage_buff_t( actor_pair_t q, util::string_view name );
  damage_buff_t( actor_pair_t q, util::string_view name, const spell_data_t*, bool parse_data = true );
  damage_buff_t( actor_pair_t q, util::string_view name, const spell_data_t*, const conduit_data_t& );
  damage_buff_t( actor_pair_t q, util::string_view name, const spell_data_t*, double );

  damage_buff_t* parse_spell_data( const spell_data_t*, double = 0.0, double = 0.0 );
  damage_buff_t* apply_mod_affecting_effect( damage_buff_modifier_t&, const spelleffect_data_t& );

  buff_t* apply_affecting_effect( const spelleffect_data_t& effect ) override;

  damage_buff_t* apply_affecting_aura( const spell_data_t* spell ) override
  {
    buff_t::apply_affecting_aura( spell );
    return this;
  }

  damage_buff_t* apply_affecting_conduit( const conduit_data_t& conduit, int effect_num = 1 ) override
  {
    buff_t::apply_affecting_conduit( conduit, effect_num );
    return this;
  }

  damage_buff_t* set_is_stacking_mod( bool value )
  {
    is_stacking = value;
    return this;
  };

  damage_buff_t* set_direct_mod( double );
  damage_buff_t* set_direct_mod( const spell_data_t*, size_t, double = 0.0 );
  damage_buff_t* set_periodic_mod( double );
  damage_buff_t* set_periodic_mod( const spell_data_t*, size_t, double = 0.0 );
  damage_buff_t* set_auto_attack_mod( double );
  damage_buff_t* set_auto_attack_mod( const spell_data_t*, size_t, double = 0.0 );
  damage_buff_t* set_crit_chance_mod( double );
  damage_buff_t* set_crit_chance_mod( const spell_data_t*, size_t, double = 0.0 );

  bool is_affecting( const spell_data_t* );
  bool is_affecting_direct( const spell_data_t* );
  bool is_affecting_periodic( const spell_data_t* );
  bool is_affecting_crit_chance( const spell_data_t* );

  // Get current direct damage buff multiplier value + benefit tracking.
  double value_direct()
  {
    buff_t::stack();
    return current_stack ? direct_mod.multiplier : 1.0;
  }

  // Get current direct damage buff multiplier value multiplied by current stacks + benefit tracking.
  double stack_value_direct()
  { return 1.0 + current_stack * ( value_direct() - 1.0 ); }

  // Get current direct damage buff multiplier value + NO benefit tracking.
  double check_value_direct() const
  { return current_stack ? direct_mod.multiplier : 1.0; }

  // Get current direct damage buff multiplier value multiplied by current stacks + NO benefit tracking.
  double check_stack_value_direct() const
  { return 1.0 + current_stack * ( check_value_direct() - 1.0 ); }

  // Get current periodic damage buff multiplier value + benefit tracking.
  double value_periodic()
  {
    buff_t::stack();
    return current_stack ? periodic_mod.multiplier : 1.0;
  }

  // Get current periodic damage buff multiplier value multiplied by current stacks + benefit tracking.
  double stack_value_periodic()
  { return 1.0 + current_stack * ( value_periodic() - 1.0 ); }

  // Get current periodic damage buff multiplier value + NO benefit tracking.
  double check_value_periodic() const
  { return current_stack ? periodic_mod.multiplier : 1.0; }

  // Get current periodic damage buff multiplier value multiplied by current stacks + NO benefit tracking.
  double check_stack_value_periodic() const
  { return 1.0 + current_stack * ( check_value_periodic() - 1.0 ); }

  // Get current AA damage buff multiplier value + benefit tracking.
  double value_auto_attack()
  {
    buff_t::stack();
    return current_stack ? auto_attack_mod.multiplier : 1.0;
  }

  // Get current AA damage buff multiplier value multiplied by current stacks + benefit tracking.
  double stack_value_auto_attack()
  { return 1.0 + current_stack * ( value_auto_attack() - 1.0 ); }

  // Get current AA damage buff multiplier value + NO benefit tracking.
  double check_value_auto_attack() const
  { return current_stack ? auto_attack_mod.multiplier : 1.0; }

  // Get current AA damage buff multiplier value multiplied by current stacks + NO benefit tracking.
  double check_stack_value_auto_attack() const
  { return 1.0 + current_stack * ( check_value_auto_attack() - 1.0 ); }

  // Get current additive crit chance buff value + benefit tracking.
  double value_crit_chance()
  {
    buff_t::stack();
    return current_stack ? crit_chance_mod.multiplier - 1.0 : 0.0;
  }

  // Get current additive crit chance buff value multiplied by current stacks + benefit tracking.
  double stack_value_crit_chance()
  { return current_stack * value_crit_chance(); }

  // Get current additive crit chance buff value + NO benefit tracking.
  double check_value_crit_chance() const
  { return current_stack ? crit_chance_mod.multiplier - 1.0 : 0.0; }

  // Get current additive crit chance buff value multiplied by current stacks + NO benefit tracking.
  double check_stack_value_crit_chance() const
  { return current_stack * check_value_crit_chance(); }

protected:
  damage_buff_t* set_buff_mod( damage_buff_modifier_t&, double );
  damage_buff_t* set_buff_mod( damage_buff_modifier_t&, const spell_data_t*, size_t, double );
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

