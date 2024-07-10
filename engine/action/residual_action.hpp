// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "action/action.hpp"
#include "action/action_state.hpp"
#include "action/dot.hpp"
#include "util/timespan.hpp"

#include <iosfwd>

struct player_t;
struct spell_data_t;

// Namespace for a ignite like action. Not mandatory, but encouraged to use it.
namespace residual_action
{
// Custom state for ignite-like actions. tick_amount contains the current
// ticking value of the ignite periodic effect, and is adjusted every time
// residual_periodic_action_t (the ignite spell) impacts on the target.
struct residual_periodic_state_t : public action_state_t
{
  double tick_amount;

  residual_periodic_state_t( action_t* a, player_t* t );
  std::ostringstream& debug_str( std::ostringstream& s ) override;
  void initialize() override;
  void copy_state( const action_state_t* o ) override;
};

template <class Base>
struct residual_periodic_action_t : public Base
{
private:
  using ab = Base;  // typedef for the templated action type, spell_t, or heal_t
public:
  using base_t = residual_periodic_action_t;
  using residual_action_t = residual_periodic_action_t<Base>;

  template <class... Args>
  residual_periodic_action_t( Args &&...args ) : ab( std::forward<Args>( args )... )
  {
    initialize_();
  }

  void initialize_()
  {
    ab::background = true;
    ab::may_crit = false;

    ab::attack_power_mod.tick = 0;
    ab::spell_power_mod.tick = 0;
    // Current assumption is that residual based rolling periodics behave the same way as coeff based rolling periodics.
    // If this is disproven in the future, the overrides below may need to be reactivated.
    // ab::tick_may_crit = false;
    // ab::hasted_ticks = false;
    // ab::rolling_periodic = false;
    // ab::dot_behavior = dot_behavior_e::DOT_REFRESH_DURATION;
    // ab::snapshot_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA | STATE_MUL_PERSISTENT | STATE_VERSATILITY;
  }

  action_state_t* new_state() override
  {
    return new residual_periodic_state_t( this, ab::target );
  }

  void impact( action_state_t* s ) override
  {
    // Residual periodic actions + tick_zero does not work
    assert( !ab::tick_zero );

    dot_t* dot = this->get_dot( s->target );
    double current_amount = 0.0;
    double old_amount = 0.0;
    double ticks_left = 0.0;
    residual_periodic_state_t* dot_state = debug_cast<residual_periodic_state_t*>( dot->state );

    // If dot is ticking get current residual pool before we overwrite it
    if ( dot->is_ticking() )
    {
      old_amount = dot_state->tick_amount;
      ticks_left = dot->ticks_left_fractional();
      current_amount = old_amount * ticks_left;
    }

    // Add new amount to residual pool
    current_amount += s->result_amount;

    // Trigger the dot, refreshing it's duration or starting it
    this->trigger_dot( s );

    // If the dot is not ticking, dot_state will be nullptr, so get the
    // residual_periodic_state_t object from the dot again (since it will exist
    // after trigger_dot() is called)
    if ( !dot_state )
      dot_state = debug_cast<residual_periodic_state_t*>( dot->state );

    // Compute tick damage after refresh, so we divide by the correct number of
    // ticks
    dot_state->tick_amount = current_amount / dot->ticks_left_fractional();

    // Spit out debug for what we did
    ab::sim->print_debug(
        "{} {} residual_action impact amount={} old_total={} old_ticks={} old_tick={} current_total={} "
        "current_ticks={} current_tick={}",
        *ab::player, *this, s->result_amount, old_amount * ticks_left, ticks_left, ticks_left > 0 ? old_amount : 0,
        current_amount, dot->ticks_left_fractional(), dot_state->tick_amount );
  }

  // The damage of the tick is simply the tick_amount in the state
  double base_ta( const action_state_t* s ) const override
  {
    auto dot = this->find_dot( s->target );
    if ( dot )
    {
      auto rd_state = debug_cast<const residual_periodic_state_t*>( dot->state );
      return rd_state->tick_amount;
    }
    return 0.0;
  }

  // Rolling over the damage is handled by separately via the state, so we set this to 1.0 for now.
  // TODO: Consolidate this with coeff based rolling periodics for uniform handling of all rolling periodics
  double composite_rolling_ta_multiplier( const action_state_t* ) const override
  {
    return 1.0;
  }

  // Ensure that not travel time exists for the ignite ability. Delay is
  // handled in the trigger via a custom event
  timespan_t travel_time() const override
  {
    return 0_ms;
  }

  // This object is not "executable" normally. Instead, the custom event
  // handles the triggering of ignite
  void execute() override
  {
    assert( 0 );
  }

  // Ensure that the ignite action snapshots nothing
  void init() override
  {
    ab::init();

    // The rolling periodic flag (334) is not sufficient to ignore player & target multipliers. They separately
    // require the Ignore Damage Take Modifiers (136) and the Ignore Caster Damage Modifiers (221) flags. As these
    // flags are parsed and the snapshot_flags/update_flags set in action_t:init(), we don't further override them
    // here.
    // ab::update_flags = ab::snapshot_flags = 0;

    // Residuals snapshot versatility and player damage multipliers. Spell multipliers likely snapshot as well,
    // but there currently isn't a residual that could be used to test this.
    ab::update_flags &= ~( STATE_MUL_SPELL_TA | STATE_VERSATILITY | STATE_MUL_PLAYER_DAM );
  }
};

// Trigger a residual periodic action on target t
void trigger( action_t* residual_action, player_t* t, double amount );

}  // namespace residual_action
