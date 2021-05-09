// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "player_event.hpp"
#include "util/timespan.hpp"
#include <functional>

struct action_t;
struct action_state_t;
struct ground_aoe_event_t;
struct player_t;

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

  ground_aoe_params_t();

  player_t* target() const { return target_; }
  double x() const { return x_; }
  double y() const { return y_; }
  hasted_with hasted() const { return hasted_; }
  action_t* action() const { return action_; }
  timespan_t pulse_time() const { return pulse_time_; }
  timespan_t start_time() const { return start_time_; }
  timespan_t duration() const { return duration_; }
  expiration_pulse_type expiration_pulse() const { return expiration_pulse_; }
  unsigned n_pulses() const { return n_pulses_; }
  const param_cb_t& expiration_callback() const { return expiration_cb_; }
  const state_cb_t& state_callback() const { return state_cb_; }

  ground_aoe_params_t& target(player_t* p);

  ground_aoe_params_t& action(action_t* a);

  ground_aoe_params_t& x(double x)
  {
    x_ = x; return *this;
  }

  ground_aoe_params_t& y(double y)
  {
    y_ = y; return *this;
  }

  ground_aoe_params_t& hasted(hasted_with state)
  {
    hasted_ = state; return *this;
  }

  ground_aoe_params_t& pulse_time(timespan_t t)
  {
    pulse_time_ = t; return *this;
  }

  ground_aoe_params_t& start_time(timespan_t t)
  {
    start_time_ = t; return *this;
  }

  ground_aoe_params_t& duration(timespan_t t)
  {
    duration_ = t; return *this;
  }

  ground_aoe_params_t& expiration_pulse(expiration_pulse_type state)
  {
    expiration_pulse_ = state; return *this;
  }

  ground_aoe_params_t& n_pulses(unsigned n)
  {
    n_pulses_ = n; return *this;
  }

  ground_aoe_params_t& expiration_callback(const param_cb_t& cb)
  {
    expiration_cb_ = cb; return *this;
  }

  ground_aoe_params_t& state_callback(const state_cb_t& cb)
  {
    state_cb_ = cb; return *this;
  }
};

// Delayed expiration callback for groud_aoe_event_t
struct expiration_callback_event_t : public event_t
{
  ground_aoe_params_t::param_cb_t callback;

  expiration_callback_event_t(sim_t& sim, const ground_aoe_params_t* p, timespan_t delay) :
    event_t(sim, delay), callback(p -> expiration_callback())
  { }

  void execute() override
  {
    callback();
  }
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
  friend Event* make_event(sim_t& sim, Args&&... args);
  // Internal constructor to schedule next pulses, not to be used outside of the struct (or derived
  // structs)
  ground_aoe_event_t(player_t* p, const ground_aoe_params_t* param,
    action_state_t* ps, bool immediate_pulse = false);
public:
  // Make a copy of the parameters, and use that object until this event expires
  ground_aoe_event_t(player_t* p, const ground_aoe_params_t& param, bool immediate_pulse = false);

  // Cleans up memory for any on-going ground aoe events when the iteration ends, or when the ground
  // aoe finishes during iteration.
  ~ground_aoe_event_t();

  void set_current_pulse(unsigned v)
  {
    current_pulse = v;
  }

  static timespan_t _time_left(const ground_aoe_params_t* params, const player_t* p);

  static timespan_t _pulse_time(const ground_aoe_params_t* params, const player_t* p, bool clamp = true);

  timespan_t remaining_time() const
  {
    return _time_left(params, player());
  }

  bool may_pulse() const;

  virtual timespan_t pulse_time(bool clamp = true) const
  {
    return _pulse_time(params, player(), clamp);
  }

  virtual void schedule_event();

  const char* name() const override
  {
    return "ground_aoe_event";
  }

  void execute() override;

  // Figure out how to handle expiration callback if it's defined
  void handle_expiration();
};
