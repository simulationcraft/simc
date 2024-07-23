#include "action/residual_action.hpp"
#include "action/spell.hpp"
#include "buff/buff.hpp"
#include "dbc/spell_data.hpp"
#include "player/player.hpp"
#include "player/sample_data_helper.hpp"
#include "player/stats.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"
#include "sim/sim.hpp"
#include "util/generic.hpp"
#include "util/io.hpp"
#include "util/timeline.hpp"
#include "util/timespan.hpp"
#include "util/util.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

/*
 * sc_stagger.hpp:stagger_data_t
 *  - provide the following data:
 *     1. std::string_view name; stagger object name (used as a key in stagger
 *        map and for statistics)
 *     2. std::function<bool()> active; enable stagger object condition (talent,
 *        buff state, etc)
 *     3. std::function<bool(school_e, result_amount_type, action_state_t*)> trigger;
 *        validate whether or not stagger_t->trigger(...) should occur
 *     4. std::vector<stagger_level_data_t*> levels; see sc_stagger.hpp:stagger_level_data_t
 *     5. const spell_data_t *self_damage; self damage spell
 *     6. std::vector<std::string_view> mitigation_tokens; list of tokenized
 *        names of all purification spells
 */

/*
 * sc_stagger.hpp:stagger_level_data_t
 *  - provide the following data:
 *     1. const spell_data_t *spell_data; spell data for level debuff. must be non-nil
 *     2. const double min_threshold; minimum value for stagger threshold to be active
 *  - it is expected for there to be 2 additional meta-levels
 *     1. `none` -> no stagger debuff level ( min_threshold = -1.0, spell_data = nil )
 *     2. `maximum` -> maximum stagger level, all stagger overflows ( min_threshold = m, spell_data = nil )
 */

/*
 * FAQ:
 *  - How do I create my own stagger type?
 *     Follow the steps to create a valid `stagger_data_t` object in
 *     `player_t::create_buffs` after base, or any point after during init if you
 *     wish to initalize yourself, otherwise do so before base `player_t::create_buffs`.
 *     Initialize the `stagger_t` object in the `stagger_data_t` object with the
 *     `stagger_data_t::init_stagger` method. Optionally, provide a derived `stagger_impl::stagger_t`
 *     type.
 *  - How do I access methods on my stagger type?
 *     Cast the `stagger_data_t::stagger_t` object to your type. The default interface
 *     should be sufficient in practically all cases.
 *  - I need to modify the level debuffs.
 *  - I need to modify the self damage.
 *  - I need to modify purification behaviour.
 *  - I need to modify tick behaviour.
 *  - I need to modify reporting.
 */

/*
 * Use default action types? If not, need both the buff and spell type for the class.
 * Provide an enum to index on or just drop for unsigned indexing
 * Provide tracking buff spell data
 * Provide tick damage spell data
 * Provide a way to describe guard absorbs
 * Make purification more ergonomic
 * Snapshot_stats helper
 * Collect_resource_timeline helper
 * Pre_analyze_hook helper
 * Combat_begin helper
 * assess_damage_imminent_pre_absorb helper (or turn it into a fractional absorb buff)
 * create_exprn helper
 * merge helper
 * html_customsection helper
 *  - register a vector of custom sections in player_t, which mixins can hook into
 *
 * Override min threshold map
 * Override spell data map (use spell name for level name, autotokenize this)
 * Override purification type map
 * Override base value
 * Override percent
 *
 * TODO: How are `default` stagger types provided (ala undulating maneuvers)
 */

// declaration

struct level_data_t
{
  const spell_data_t *spell_data;
  const double min_threshold;

  std::string_view name() const;
};

struct stagger_data_t
{
  const spell_data_t *self_damage;
  std::vector<level_data_t> levels;
  std::vector<std::string_view> mitigation_tokens;

  std::function<bool()> active;
  std::function<bool( school_e, result_amount_type, action_state_t * )> trigger_filter;
  std::function<double( school_e, result_amount_type, action_state_t * )> percent;

  std::string_view name() const;
};

namespace stagger_impl
{
template <class derived_actor_t, class residual_type = residual_action::residual_periodic_action_t<spell_t>>
struct self_damage_t;

struct sample_data_t
{
  sc_timeline_t pool_size;          // raw stagger in pool
  sc_timeline_t pool_size_percent;  // pool as a fraction of current maximum hp
  sc_timeline_t effectiveness;      // stagger effectiveness
  double buffed_base_value;
  double buffed_percent_player_level;
  double buffed_percent_target_level;
  // assert(absorbed == taken + mitigated)
  // assert(sum(mitigated_by_ability) == mitigated)
  sample_data_helper_t *absorbed;
  sample_data_helper_t *taken;
  sample_data_helper_t *mitigated;
  std::unordered_map<std::string_view, sample_data_helper_t *> mitigated_by_ability;

  sample_data_t( player_t *player, const stagger_data_t &data );
};

template <class derived_actor_t, class buff_type = buff_t>
struct debuff_t : buff_type
{
  derived_actor_t *player;

  debuff_t( derived_actor_t *player, const stagger_data_t *parent_data, const level_data_t *data );
};

template <class derived_actor_t>
struct level_t
{
  derived_actor_t *player;
  const stagger_data_t *parent_data;
  const level_data_t *data;
  propagate_const<debuff_t<derived_actor_t> *> debuff;
  sample_data_helper_t *absorbed;
  sample_data_helper_t *taken;
  sample_data_helper_t *mitigated;

  level_t( derived_actor_t *player, const stagger_data_t *parent_data, const level_data_t *data );
  template <class debuff_type>
  void init();
  void trigger();
  bool operator>( const level_t<derived_actor_t> &rhs ) const;
  std::string_view name() const;
  double min_threshold() const;
};

template <class derived_actor_t>
struct stagger_t
{
  derived_actor_t *player;
  stagger_data_t data;

  // TODO: typedef param pack?
  std::function<bool()> active;
  std::function<bool( school_e, result_amount_type, action_state_t * )> trigger_filter;
  std::function<double( school_e, result_amount_type, action_state_t * )> percent;

  std::vector<stagger_impl::level_t<derived_actor_t> *> levels;
  self_damage_t<derived_actor_t> *self_damage;
  level_t<derived_actor_t> *current;
  sample_data_t *sample_data;

  // init
  stagger_t( derived_actor_t *player, const stagger_data_t &data );
  void init();
  void adjust_sample_data( sim_t &sim );
  void merge_sample_data( const stagger_t &other );

  // trigger
  double trigger( school_e school, result_amount_type damage_type, action_state_t *state );

  // purify
  double purify_flat( double amount, std::string_view ability_token, bool report_amount = true );
  double purify_percent( double amount, std::string_view ability_token );
  double purify_all( std::string_view ability_token );
  void delay_tick( timespan_t delay );  // -> return time until next tick?

  // information
  std::string_view name() const;

  double pool_size();
  double pool_size_percent();
  double tick_size();
  double tick_size_percent();
  timespan_t remains();
  bool is_ticking();
  double level_index() const;  // avoid floating point demotion with double

  void set_pool( double amount );
  void damage_changed( bool last_tick = false );
  level_t<derived_actor_t> *find_current_level();
};

template <class derived_actor_t, class residual_type>
struct self_damage_t : residual_type
{
  derived_actor_t *player;
  stagger_impl::stagger_t<derived_actor_t> *stagger_effect;

  self_damage_t( derived_actor_t *player, stagger_impl::stagger_t<derived_actor_t> *stagger_effect );

  proc_types proc_type() const override;
  void impact( action_state_t *state ) override;                                  // add to pool
  void assess_damage( result_amount_type type, action_state_t *state ) override;  // tick from pool
  void last_tick( dot_t *dot ) override;                                          // callback on last tick
};

template <class derived_actor_t>
struct stagger_report_t : player_report_extension_t
{
  derived_actor_t *player;

  stagger_report_t( derived_actor_t *player );
  void html_customsection( report::sc_html_stream &os ) override;
};
}  // namespace stagger_impl

// public interface
template <class base_actor_t, class derived_actor_t /* , class derived_actor_td_t */>
struct stagger_t : base_actor_t
{
  std::unordered_map<std::string_view, stagger_impl::stagger_t<derived_actor_t> *> stagger;

  virtual void create_buffs() override
  {
    base_actor_t::create_buffs();

    for ( auto &[ key, stagger_effect ] : stagger )
      stagger_effect->init();
  }

  virtual void combat_begin() override
  {
    base_actor_t::combat_begin();

    for ( auto &[ key, stagger_effect ] : stagger )
      if ( stagger_effect->active() )
        stagger_effect->current->trigger();
  }

  virtual void pre_analyze_hook() override
  {
    base_actor_t::pre_analyze_hook();

    for ( auto &[ key, stagger_effect ] : stagger )
      stagger_effect->adjust_sample_data( *this->sim );
  }

  virtual void assess_damage_imminent_pre_absorb( school_e school, result_amount_type damage_type,
                                                  action_state_t *state ) override
  {
    base_actor_t::assess_damage_imminent_pre_absorb( school, damage_type, state );

    for ( auto &[ key, stagger_effect ] : stagger )
      stagger_effect->trigger( school, damage_type, state );
  }

  virtual void merge( player_t &other ) override
  {
    base_actor_t::merge( other );

    derived_actor_t &actor = static_cast<derived_actor_t &>( other );
    for ( auto &[ key, stagger_effect ] : stagger )
      stagger_effect->merge_sample_data( *actor.stagger[ key ] );
  }

  virtual std::unique_ptr<expr_t> create_expression( std::string_view name_str ) override
  {
    auto splits = util::string_split<std::string_view>( name_str, "." );

    // TODO: CXX20, captures can be reduced to [ stagger_effect ] (etc)
    for ( auto &[ key, stagger_effect ] : stagger )
    {
      if ( splits.size() != 2 || splits[ 0 ] != stagger_effect->name() )
        continue;
      for ( auto &level : stagger_effect->levels )
        if ( splits[ 1 ] == level->name() )
          return make_fn_expr( name_str, [ &stagger_effect = stagger_effect, &level = level ] {
            return stagger_effect->current->data == level->data;
          } );
      if ( splits[ 1 ] == "amount" )
        return make_fn_expr( name_str, [ &stagger_effect = stagger_effect ] { return stagger_effect->tick_size(); } );
      if ( splits[ 1 ] == "pct" )
        return make_fn_expr( name_str,
                             [ &stagger_effect = stagger_effect ] { return stagger_effect->tick_size_percent(); } );
      if ( splits[ 1 ] == "amount_remains" )
        return make_fn_expr( name_str, [ &stagger_effect = stagger_effect ] { return stagger_effect->pool_size(); } );
      if ( splits[ 1 ] == "amounttotalpct" )
        return make_fn_expr( name_str,
                             [ &stagger_effect = stagger_effect ] { return stagger_effect->pool_size_percent(); } );
      if ( splits[ 1 ] == "remains" )
        return make_fn_expr( name_str, [ &stagger_effect = stagger_effect ] { return stagger_effect->remains(); } );
      if ( splits[ 1 ] == "ticking" )
        return make_fn_expr( name_str, [ &stagger_effect = stagger_effect ] { return stagger_effect->is_ticking(); } );
    }
    return base_actor_t::create_expression( name_str );
  }

  // These are hypothetically placed in a sensible order of most->least likely to override
  template <class debuff_type      = stagger_impl::debuff_t<derived_actor_t>,
            class self_damage_type = stagger_impl::self_damage_t<derived_actor_t>,
            class level_type       = stagger_impl::level_t<derived_actor_t>,
            class stagger_type     = stagger_impl::stagger_t<derived_actor_t>,
            class sample_data_type = stagger_impl::sample_data_t>
  void create_stagger( const stagger_data_t &data )
  {
    if ( !data.active() )
      return;
    derived_actor_t *derived_actor        = debug_cast<derived_actor_t *>( this );
    const auto &[ stagger_pair, success ] = stagger.insert( { data.name(), new stagger_type( derived_actor, data ) } );
    assert( success && "Stagger effect failed to be inserted into stagger map." );
    auto &[ name, stagger_effect ] = *stagger_pair;
    for ( const auto &level_data : stagger_effect->data.levels )
      stagger_effect->levels.push_back( new level_type( derived_actor, &stagger_effect->data, &level_data ) );
    stagger_effect->self_damage = new self_damage_type( derived_actor, stagger_effect );
    stagger_effect->sample_data = new sample_data_type( derived_actor, stagger_effect->data );

    stagger_effect->init();
    for ( level_type *level : stagger_effect->levels )
      level->template init<debuff_type>();
  }

  stagger_impl::stagger_t<derived_actor_t> *find_stagger( std::string_view name )
  {
    auto &effect_pair = stagger.find( name );
    assert( effect_pair != stagger.end() );
    auto &[ key, effect ] = *effect_pair;
    return effect;
  }

  const stagger_impl::stagger_t<derived_actor_t> *find_stagger( std::string_view name ) const
  {
    const auto &effect_pair = stagger.find( name );
    assert( effect_pair != stagger.end() );
    const auto &[ key, effect ] = *effect_pair;
    return effect;
  }

  template <typename... Args>
  stagger_t( Args &&...args ) : base_actor_t( std::forward<Args>( args )... )
  {
    static_assert(
        std::is_base_of_v<player_t, base_actor_t>,
        "stagger_t<base_actor_t, ..> base_actor is not derived from player_t (should be module actor base type)" );
    static_assert(
        std::is_base_of_v<player_t, derived_actor_t>,
        "stagger_t<.., derived_actor_t> derived_actor is not derived from player_t (should be actor derived type)" );

    // TODO: Move this so it can be opted out of without grossness
    this->mixin_reports.push_back(
        std::make_unique<stagger_impl::stagger_report_t<stagger_t<base_actor_t, derived_actor_t>>>( this ) );
  }
};

// implementation
namespace stagger_impl
{
// debuff_t impl
template <class derived_actor_t, class buff_type>
debuff_t<derived_actor_t, buff_type>::debuff_t( derived_actor_t *player, const stagger_data_t *parent_data,
                                                const level_data_t *data )
  : buff_type( player, util::tokenize_fn( fmt::format( "{}_{}", data->name(), parent_data->name() ) ),
               data->spell_data ),
    player( player )
{
  static_assert( std::is_base_of_v<buff_t, buff_type>, "debuff_t<..., buff_type> must be derived from buff_t." );
  // debuff duration logic is handled by self_damage_t
  this->base_buff_duration = 0_s;
  this->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
}

// level_t impl
template <class derived_actor_t>
level_t<derived_actor_t>::level_t( derived_actor_t *player, const stagger_data_t *parent_data,
                                   const level_data_t *data )
  : player( player ), parent_data( parent_data ), data( data )
{
  absorbed  = player->get_sample_data( fmt::format( "{} added to pool while at {}.", parent_data->name(), name() ) );
  taken     = player->get_sample_data( fmt::format( "{} damage taken from {}.", parent_data->name(), name() ) );
  mitigated = player->get_sample_data( fmt::format( "{} damage mitigated while at {}.", parent_data->name(), name() ) );
}

template <class derived_actor_t>
template <class debuff_type>
void level_t<derived_actor_t>::init()
{
  debuff = make_buff<debuff_type>( player, parent_data, data );
}

template <class derived_actor_t>
void level_t<derived_actor_t>::trigger()
{
  debuff->trigger();
}

template <class derived_actor_t>
bool level_t<derived_actor_t>::operator>( const level_t<derived_actor_t> &rhs ) const
{
  return data->min_threshold > rhs.data->min_threshold;
}

template <class derived_actor_t>
std::string_view level_t<derived_actor_t>::name() const
{
  return data->name();
}

template <class derived_actor_t>
double level_t<derived_actor_t>::min_threshold() const
{
  return data->min_threshold;
}

// stagger_t impl
template <class derived_actor_t>
stagger_t<derived_actor_t>::stagger_t( derived_actor_t *player, const stagger_data_t &data )
  : player( player ), data( std::move( data ) )
{
  // "none" level, note that we std::move'd unqualified object `data`
  this->data.levels.push_back( { spell_data_t::nil(), -1.0 } );
  active         = std::move( this->data.active );
  trigger_filter = std::move( this->data.trigger_filter );
  percent        = std::move( this->data.percent );
}

template <class derived_actor_t>
bool level_cmp( stagger_impl::level_t<derived_actor_t> *lhs, stagger_impl::level_t<derived_actor_t> *rhs )
{
  return lhs->data->min_threshold > rhs->data->min_threshold;
}

template <class derived_actor_t>
void stagger_t<derived_actor_t>::init()
{
  std::sort( levels.begin(), levels.end(), level_cmp<derived_actor_t> );
  current = levels.back();
}

template <class derived_actor_t>
void stagger_t<derived_actor_t>::adjust_sample_data( sim_t &sim )
{
  sample_data->pool_size.adjust( sim );
  sample_data->pool_size_percent.adjust( sim );
  sample_data->effectiveness.adjust( sim );
}

template <class derived_actor_t>
void stagger_t<derived_actor_t>::merge_sample_data( const stagger_t &other )
{
  sample_data->pool_size.merge( other.sample_data->pool_size );
  sample_data->pool_size_percent.merge( other.sample_data->pool_size );
  sample_data->effectiveness.merge( other.sample_data->pool_size );
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::trigger( school_e school, result_amount_type damage_type, action_state_t *state )
{
  /*
   * TODO: Implement Stagger Guard absorbs
   *  - Must support one stagger type or a set of stagger types
   *  - Ideally uses the standard buff interface
   */
  if ( state->result_amount <= 0.0 )
    return 0.0;

  if ( !active() )
    return 0.0;

  if ( !trigger_filter( school, damage_type, state ) )
    return 0.0;

  /*
   * Example:
   *  - result_amount   = 100
   *  - stagger_percent = 0.7
   *  - absorb          = 20
   *  - pool_size       = 990
   *  - cap             = 1000
   *  70 = 100 * 0.7                      -> amount
   *  20 = min( 70, 20 )                  -> absorbed
   *  50 = 70 - 20                        -> amount
   *  10 = 70 - max( 70 + 950 - 1000, 0 ) -> amount
   *  70 = 100 - 10 - 20                  -> result_amount final
   *  100 damage gets split into:
   *   - 10 staggered
   *   - 20 absorbed
   *   - 30 to face (due to %)
   *   - 40 to face (due to cap)
   */

  double amount   = state->result_amount * percent( school, damage_type, state );
  double base     = amount;
  double base_hit = state->result_amount;

  double absorb   = 0.0;
  double absorbed = std::min( amount, absorb );
  amount -= absorbed;

  double pre_cap = amount;

  double cap = levels.front()->min_threshold() * player->resources.max[ RESOURCE_HEALTH ];
  amount -= std::max( amount + pool_size() - cap, 0.0 );

  state->result_amount -= amount + absorbed;
  state->result_mitigated -= amount + absorbed;

  if ( absorbed > 0.0 )
    player->sim->print_debug( "{} absorbed {} out of {} damage about to be added to {} pool", player->name(), absorbed,
                              base, name() );

  if ( amount <= 0.0 )
    return 0.0;

  // TODO: Fix all prints
  player->sim->print_debug(
      "{} added {} to {} pool (percent={} base_hit={} final_hit={} staggered={} absorbed={} overcapped={})",
      player->name(), amount, name(), percent( school, damage_type, state ), base_hit, state->result_amount, amount,
      absorbed, std::max( pre_cap + pool_size() - cap, 0.0 ) );

  residual_action::trigger( self_damage, player, amount );

  return amount;
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::purify_flat( double amount, std::string_view ability_token, bool report_amount )
{
  if ( !is_ticking() )
    return 0.0;

  double pool    = pool_size();
  double cleared = std::clamp( amount, 0.0, pool );
  double remains = pool - cleared;

  sample_data->mitigated->add( cleared );
  current->mitigated->add( cleared );
  assert( sample_data->mitigated_by_ability.find( ability_token ) != sample_data->mitigated_by_ability.end() );
  sample_data->mitigated_by_ability[ ability_token ]->add( cleared );

  set_pool( remains );
  if ( report_amount )
    player->sim->print_debug( "{} reduced {} pool from {} to {} ({})", player->name(), name(), pool, remains, cleared );

  return cleared;
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::purify_percent( double amount, std::string_view ability_token )
{
  double pool    = pool_size();
  double cleared = purify_flat( amount * pool, ability_token, false );

  if ( cleared )
    player->sim->print_debug( "{} reduced {} pool by {}% from {} to {} ({})", player->name(), name(), amount * 100.0,
                              pool, ( 1.0 - amount ) * pool, cleared );

  return cleared;
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::purify_all( std::string_view ability_token )
{
  return purify_flat( pool_size(), ability_token, true );
}

template <class derived_actor_t>
void stagger_t<derived_actor_t>::delay_tick( timespan_t delay )
{
  dot_t *dot = self_damage->get_dot();
  if ( !dot || !dot->is_ticking() || !dot->tick_event )
    return;

  player->sim->print_debug( "{} delayed tick scheduled of {} for {} to {}", player->name(), name(),
                            player->sim->current_time() + dot->tick_event->remains(),
                            player->sim->current_time() + dot->tick_event->remains() + delay );
  dot->tick_event->reschedule( dot->tick_event->remains() + delay );
  if ( dot->end_event )
    dot->end_event->reschedule( dot->end_event->remains() + delay );
}

template <class derived_actor_t>
std::string_view stagger_t<derived_actor_t>::name() const
{
  return data.self_damage->name_cstr();
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::pool_size()
{
  dot_t *dot = self_damage->get_dot();
  if ( !dot || !dot->state || !dot->is_ticking() )
    return 0.0;

  return self_damage->base_ta( dot->state ) * dot->ticks_left();
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::pool_size_percent()
{
  return pool_size() / player->resources.max[ RESOURCE_HEALTH ];
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::tick_size()
{
  dot_t *dot = self_damage->get_dot();
  if ( !dot || !dot->state || !dot->is_ticking() )
    return 0.0;

  return self_damage->base_ta( dot->state );
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::tick_size_percent()
{
  return tick_size() / player->resources.max[ RESOURCE_HEALTH ];
}

template <class derived_actor_t>
timespan_t stagger_t<derived_actor_t>::remains()
{
  dot_t *dot = self_damage->get_dot();
  if ( !dot || !dot->is_ticking() )
    return 0_s;

  return dot->remains();
}

template <class derived_actor_t>
bool stagger_t<derived_actor_t>::is_ticking()
{
  dot_t *dot = self_damage->get_dot();
  if ( !dot )
    return false;

  return dot->is_ticking();
}

template <class derived_actor_t>
double stagger_t<derived_actor_t>::level_index() const
{
  return as<double>( levels.size() -
                     std::distance( levels.begin(), std::find( levels.begin(), levels.end(), current ) ) - 1 );
}

template <class derived_actor_t>
void stagger_t<derived_actor_t>::set_pool( double amount )
{
  if ( !is_ticking() )
    return;

  dot_t *dot        = self_damage->get_dot();
  auto state        = debug_cast<residual_action::residual_periodic_state_t *>( dot->state );
  double ticks_left = dot->ticks_left();

  state->tick_amount = amount / ticks_left;
  player->sim->print_debug( "{} set {} pool amount={} ticks_left={}, tick_size={}", player->name(), name(), amount,
                            ticks_left, amount / ticks_left );

  damage_changed();
}

template <class derived_actor_t>
void stagger_t<derived_actor_t>::damage_changed( bool last_tick )
{
  if ( last_tick )
  {
    current->debuff->expire();
    return;
  }

  level_t<derived_actor_t> *level = find_current_level();
  // player->sim->print_debug( "{} level changing current={} ({}) new={} ({}) pool_size_percent={}",
  //                           player->name(),
  //                           current->name,
  //                           static_cast<int>( current->level ),
  //                           stagger_levels[ level ]->name,
  //                           static_cast<int>( stagger_levels[ level ]->level ),
  //                           pool_size_percent() );
  // This comparison is a little sketchy, but if we assume no one fiddles with
  // our pointers, we're good to go.
  if ( level == current )
    return;

  current->debuff->expire();
  current = level;
  current->debuff->trigger();
}

template <class derived_actor_t>
level_t<derived_actor_t> *stagger_t<derived_actor_t>::find_current_level()
{
  double current_percent = pool_size_percent();
  for ( level_t<derived_actor_t> *level : levels )
    if ( current_percent > level->min_threshold() )
      return level;
  return levels.back();
}

template <class derived_actor_t, class residual_type>
self_damage_t<derived_actor_t, residual_type>::self_damage_t( derived_actor_t *player,
                                                              stagger_impl::stagger_t<derived_actor_t> *stagger_effect )
  : residual_type( stagger_effect->name(), player, stagger_effect->data.self_damage ),
    player( player ),
    stagger_effect( stagger_effect )
{
  // to verify if base_type is of type residual_periodic_action_t<...> we'd need some extra machinery, so we won't check
  // :) static_assert( std::is_base_of_v<residual_action::residual_periodic_action_t)
  residual_type::target      = player;
  residual_type::stats->type = stats_e::STATS_NEUTRAL;
}

template <class derived_actor_t, class residual_type>
proc_types self_damage_t<derived_actor_t, residual_type>::proc_type() const
{
  return PROC1_ANY_DAMAGE_TAKEN;
}

template <class derived_actor_t, class residual_type>
void self_damage_t<derived_actor_t, residual_type>::impact( action_state_t *state )
{
  residual_type::impact( state );
  this->player->sim->print_debug(
      "{} current {} pool statistics pool_size={} pool_size_percent={} tick_size={} tick_size_percent={} cap={}",
      this->player->name(), this->stagger_effect->data.name(), this->stagger_effect->pool_size(),
      this->stagger_effect->pool_size_percent(), this->stagger_effect->tick_size(),
      this->stagger_effect->tick_size_percent(), this->player->resources.max[ RESOURCE_HEALTH ] );
  this->stagger_effect->damage_changed();
  this->stagger_effect->sample_data->absorbed->add( state->result_amount );
  this->stagger_effect->current->absorbed->add( state->result_amount );
  this->stagger_effect->sample_data->pool_size.add( this->sim->current_time(), this->stagger_effect->pool_size() );
  this->stagger_effect->sample_data->pool_size_percent.add( this->sim->current_time(),
                                                            this->stagger_effect->pool_size_percent() );
}

template <class derived_actor_t, class residual_type>
void self_damage_t<derived_actor_t, residual_type>::assess_damage( result_amount_type type, action_state_t *state )
{
  residual_type::assess_damage( type, state );
  this->player->sim->print_debug(
      "{} current {} pool statistics pool_size={} pool_size_percent={} tick_size={} tick_size_percent={} cap={}",
      this->player->name(), this->stagger_effect->data.name(), this->stagger_effect->pool_size(),
      this->stagger_effect->pool_size_percent(), this->stagger_effect->tick_size(),
      this->stagger_effect->tick_size_percent(), this->player->resources.max[ RESOURCE_HEALTH ] );
  this->stagger_effect->damage_changed();
  this->stagger_effect->sample_data->taken->add( state->result_amount );
  this->stagger_effect->current->taken->add( state->result_amount );
  this->stagger_effect->sample_data->pool_size.add( this->sim->current_time(), this->stagger_effect->pool_size() );
  this->stagger_effect->sample_data->pool_size_percent.add( this->sim->current_time(),
                                                            this->stagger_effect->pool_size_percent() );
}

template <class derived_actor_t, class residual_type>
void self_damage_t<derived_actor_t, residual_type>::last_tick( dot_t *dot )
{
  residual_type::last_tick( dot );
  this->stagger_effect->damage_changed( true );
}

template <class derived_actor_t>
stagger_report_t<derived_actor_t>::stagger_report_t( derived_actor_t *player ) : player( player )
{
}

template <class derived_actor_t>
void stagger_report_t<derived_actor_t>::html_customsection( report::sc_html_stream &os )
{
  if ( player->stagger.empty() )
    return;
  os << "\t\t\t\t<div class=\"player-section stagger_sections\">\n";
  for ( auto &[ key, stagger_effect ] : player->stagger )
  {
    os << fmt::format( "\t\t\t\t\t<div class=\"player-section {}\">\n", key )
       << fmt::format( "\t\t\t\t\t\t<h3 class=\"toggle\">{}</h3>\n", key )
       << "\t\t\t\t\t\t<div class=\"toggle-content hide\">\n";
    os << "\t\t\t\t\t\t<p>Note that these charts are extremely sensitive to bucket"
       << "sizes. It is not uncommon to exceed stagger cap, etc. If you wish to "
       << "see exact values, use debug output.</p>\n";

    auto chart_print = [ &os, &sim = *player->sim, &player = *player ]( std::string title, std::string id,
                                                                        sc_timeline_t &timeline ) {
      highchart::time_series_t chart_( highchart::build_id( player, id ), sim );
      chart::generate_actor_timeline( chart_, player, title, color::resource_color( RESOURCE_HEALTH ), timeline );
      chart_.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart_.set( "chart.width", "575" );
      os << chart_.to_target_div();
      sim.add_chart_data( chart_ );
    };

    chart_print( fmt::format( "{} Pool", key ), fmt::format( "{}_pool", key ), stagger_effect->sample_data->pool_size );
    chart_print( fmt::format( "{} Pool / Maximum Health", key ), fmt::format( "{}_pool_percent", key ),
                 stagger_effect->sample_data->pool_size_percent );
    chart_print( fmt::format( "{} Effectiveness", key ), fmt::format( "{}_effectiveness", key ),
                 stagger_effect->sample_data->effectiveness );

    double absorbed_mean = stagger_effect->sample_data->absorbed->mean();
    // TODO: what was this before it was 100.0?
    // os << fmt::format( "\t\t\t\t\t\t<p>Total {} damage added: {} / {:.2f}%</p>\n", key,  100.0 );

    os << "\t\t\t\t\t\t<table class=\"sc\"\n"
       << "\t\t\t\t\t\t\t<tbody>\n"
       << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Purification Stats</th>\n"
       << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Purified</th>\n"
       << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Purified %</th>\n"
       << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Purification Count</th>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    auto purify_print = [ absorbed_mean, &os ]( std::string_view name, sample_data_helper_t *effect ) {
      double mean = effect->mean();
      // int count   = effect->count();
      double percent = mean / absorbed_mean * 100.0;
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">" << name << "</td>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << mean << "</td>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << percent
         << "%</td>\n"
         // << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << count << "</td>\n"
         << "\t\t\t\t\t\t\t\t</tr>\n";
    };

    for ( auto &[ purify_key, purify_effect ] : stagger_effect->sample_data->mitigated_by_ability )
      purify_print( purify_key, purify_effect );

    os << "\t\t\t\t\t\t\t</tbody>\n"
       << "\t\t\t\t\t\t</table>\n";

    os << "\t\t\t\t\t\t<table class=\"sc\">\n"
       << "\t\t\t\t\t\t\t<tbody>\n"
       << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Stagger Stats</th>\n"
       << "\t\t\t\t\t\t\t\t\t<th>Absorbed</th>\n"
       << "\t\t\t\t\t\t\t\t\t<th>Taken</th>\n"
       << "\t\t\t\t\t\t\t\t\t<th>Mitigated</th>\n"
       << "\t\t\t\t\t\t\t\t\t<th>Mitigated %</th>\n"
       // << "\t\t\t\t\t\t\t\t\t<th>Ticks</th>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    auto stagger_print = [ absorbed_mean, &os ]( const auto *level ) {
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">" << level->name() << "</td>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << level->absorbed->mean() << "</td>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << level->taken->mean() << "</td>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << level->mitigated->mean() << "</td>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( absorbed_mean != 0.0 ? level->mitigated->mean() / absorbed_mean * 100.0 : 0 )
         // << ( level->absorbed->mean() != 0.0 ? level->mitigated->mean() / level->absorbed->mean() * 100.0 : 0 )
         << "%</td>\n"
         // << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << level->absorbed->count() << "</td>\n"
         << "\t\t\t\t\t\t\t\t</tr>\n";
    };

    for ( const auto *level : stagger_effect->levels )
    {
      stagger_print( level );
    }

    double taken_mean     = stagger_effect->sample_data->taken->mean();
    double mitigated_mean = stagger_effect->sample_data->mitigated->mean();
    // int absorbed_count = stagger_effect->sample_data->absorbed->count();

    os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">Total</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << absorbed_mean << "</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << taken_mean << "</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << mitigated_mean << "</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
       << ( absorbed_mean != 0.0 ? mitigated_mean / absorbed_mean * 100.0 : 0 )
       << "%</td>\n"
       // << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << absorbed_count << "</td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    os << "\t\t\t\t\t\t\t</tbody>\n"
       << "\t\t\t\t\t\t</table>\n";
  }
  os << "\t\t\t\t\t</div>\n";

  os << "\t\t\t\t\t\t</div>\n"
     << "\t\t\t\t\t</div>\n";
}
}  // namespace stagger_impl
