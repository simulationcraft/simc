// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "action/sc_action_state.hpp"
#include "action/sc_action.hpp"
#include "action/dot.hpp"
#include "sc_timespan.hpp"
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

    residual_periodic_state_t(action_t* a, player_t* t);
    std::ostringstream& debug_str(std::ostringstream& s) override;
    void initialize() override;
    void copy_state(const action_state_t* o) override;
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
    residual_periodic_action_t(const std::string& n, T& p, const spell_data_t* s) :
      ab(n, p, s)
    {
      initialize_();
    }

    template <typename T>
    residual_periodic_action_t(const std::string& n, T* p, const spell_data_t* s) :
      ab(n, p, s)
    {
      initialize_();
    }

    void initialize_()
    {
      ab::background = true;

      ab::tick_may_crit = false;
      ab::hasted_ticks = false;
      ab::may_crit = false;
      ab::attack_power_mod.tick = 0;
      ab::spell_power_mod.tick = 0;
      ab::dot_behavior = DOT_REFRESH;
      ab::callbacks = false;
    }

    virtual action_state_t* new_state() override
    {
      return new residual_periodic_state_t(this, ab::target);
    }

    // Residual periodic actions will not be extendeed by the pandemic mechanism,
    // thus the new maximum length of the dot is the ongoing tick plus the
    // duration of the dot.
    virtual timespan_t calculate_dot_refresh_duration(const dot_t* dot, timespan_t triggered_duration) const override
    {
      return dot->time_to_next_tick() + triggered_duration;
    }

    virtual void impact(action_state_t* s) override
    {
      // Residual periodic actions + tick_zero does not work
      assert(!ab::tick_zero);

      dot_t* dot = ab::get_dot(s->target);
      double current_amount = 0, old_amount = 0;
      int ticks_left = 0;
      residual_periodic_state_t* dot_state = debug_cast<residual_periodic_state_t*>(dot->state);

      // If dot is ticking get current residual pool before we overwrite it
      assert( dot_state != nullptr );
      if (dot->is_ticking())
      {
        old_amount = dot_state->tick_amount;
        ticks_left = dot->ticks_left();
        current_amount = old_amount * dot->ticks_left();
      }

      // Add new amount to residual pool
      current_amount += s->result_amount;

      // Trigger the dot, refreshing it's duration or starting it
      ab::trigger_dot(s);

      // If the dot is not ticking, dot_state will be nullptr, so get the
      // residual_periodic_state_t object from the dot again (since it will exist
      // after trigger_dot() is called)
      if (!dot_state)
        dot_state = debug_cast<residual_periodic_state_t*>(dot->state);

      // Compute tick damage after refresh, so we divide by the correct number of
      // ticks
      dot_state->tick_amount = current_amount / dot->ticks_left();

      // Spit out debug for what we did
      if (ab::sim->debug)
        ab::sim->out_debug.printf("%s %s residual_action impact amount=%f old_total=%f old_ticks=%d old_tick=%f current_total=%f current_ticks=%d current_tick=%f",
          ab::player->name(), ab::name(), s->result_amount,
          old_amount * ticks_left, ticks_left, ticks_left > 0 ? old_amount : 0,
          current_amount, dot->ticks_left(), dot_state->tick_amount);
    }

    // The damage of the tick is simply the tick_amount in the state
    virtual double base_ta(const action_state_t* s) const override
    {
      auto dot = ab::find_dot(s->target);
      if (dot)
      {
        auto rd_state = debug_cast<const residual_periodic_state_t*>(dot->state);
        return rd_state->tick_amount;
      }
      return 0.0;
    }

    // Ensure that not travel time exists for the ignite ability. Delay is
    // handled in the trigger via a custom event
    virtual timespan_t travel_time() const override
    {
      return timespan_t::zero();
    }

    // This object is not "executable" normally. Instead, the custom event
    // handles the triggering of ignite
    virtual void execute() override
    {
      assert(0);
    }

    // Ensure that the ignite action snapshots nothing
    virtual void init() override
    {
      ab::init();

      ab::update_flags = ab::snapshot_flags = 0;
    }
  };

  // Trigger a residual periodic action on target t
  void trigger(action_t* residual_action, player_t* t, double amount);

}  // namespace residual_action