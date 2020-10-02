// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "ground_aoe.hpp"
#include "action/sc_action.hpp"
#include "player/sc_player.hpp"

ground_aoe_params_t::ground_aoe_params_t() :
  target_(nullptr), x_(-1), y_(-1), hasted_(NOTHING), action_(nullptr),
  pulse_time_(timespan_t::from_seconds(1.0)), start_time_(timespan_t::min()),
  duration_(timespan_t::zero()),
  expiration_pulse_(NO_EXPIRATION_PULSE), n_pulses_(0), expiration_cb_(nullptr)
{ }

ground_aoe_params_t& ground_aoe_params_t::target(player_t * p)
{
  target_ = p;
  if (start_time_ == timespan_t::min())
  {
    start_time_ = target_->sim->current_time();
  }

  if (x_ == -1)
  {
    x_ = target_->x_position;
  }

  if (y_ == -1)
  {
    y_ = target_->y_position;
  }

  return *this;
}

ground_aoe_params_t& ground_aoe_params_t::action(action_t* a)
{
  action_ = a;
  if (start_time_ == timespan_t::min())
  {
    start_time_ = action_->sim->current_time();
  }

  return *this;
}

// Internal constructor to schedule next pulses, not to be used outside of the struct (or derived
// structs)

ground_aoe_event_t::ground_aoe_event_t(player_t* p, const ground_aoe_params_t* param, action_state_t* ps, bool immediate_pulse)
  : player_event_t(
    *p, immediate_pulse ? timespan_t::zero() : _pulse_time(param, p)),
  params(param),
  pulse_state(ps), current_pulse(1)
{
  // Ensure we have enough information to start pulsing.
  assert(params->target() != nullptr && "No target defined for ground_aoe_event_t");
  assert(params->action() != nullptr && "No action defined for ground_aoe_event_t");
  assert(params->pulse_time() > timespan_t::zero() &&
    "Pulse time for ground_aoe_event_t must be a positive value");
  assert(params->start_time() >= timespan_t::zero() &&
    "Start time for ground_aoe_event must be defined");
  assert((params->duration() > timespan_t::zero() || params->n_pulses() > 0) &&
    "Duration or number of pulses for ground_aoe_event_t must be defined");

  // Make a state object that persists for this ground aoe event throughout its lifetime
  if (!pulse_state)
  {
    action_t* spell_ = params->action();
    pulse_state = spell_->get_state();
    pulse_state->target = params->target();
    spell_->snapshot_state(pulse_state, spell_->amount_type(pulse_state));
  }

  if (params->state_callback())
  {
    params->state_callback()(ground_aoe_params_t::EVENT_CREATED, this);
  }
}

// Make a copy of the parameters, and use that object until this event expires

ground_aoe_event_t::ground_aoe_event_t(player_t* p, const ground_aoe_params_t& param, bool immediate_pulse) :
  ground_aoe_event_t(p, new ground_aoe_params_t(param), nullptr, immediate_pulse)
{
  if (params->state_callback())
  {
    params->state_callback()(ground_aoe_params_t::EVENT_STARTED, this);
  }
}

// Cleans up memory for any on-going ground aoe events when the iteration ends, or when the ground
// aoe finishes during iteration.

ground_aoe_event_t::~ground_aoe_event_t()
{
  delete params;
  if (pulse_state)
  {
    action_state_t::release(pulse_state);
  }
}

timespan_t ground_aoe_event_t::_time_left(const ground_aoe_params_t* params, const player_t* p)
{
  return params->duration() - (p->sim->current_time() - params->start_time());
}

timespan_t ground_aoe_event_t::_pulse_time(const ground_aoe_params_t* params, const player_t* p, bool clamp)
{
  auto tick = params->pulse_time();
  auto time_left = _time_left(params, p);

  switch (params->hasted())
  {
  case ground_aoe_params_t::SPELL_SPEED:
    tick *= p->cache.spell_speed();
    break;
  case ground_aoe_params_t::SPELL_HASTE:
    tick *= p->cache.spell_haste();
    break;
  case ground_aoe_params_t::ATTACK_SPEED:
    tick *= p->cache.attack_speed();
    break;
  case ground_aoe_params_t::ATTACK_HASTE:
    tick *= p->cache.attack_haste();
    break;
  default:
    break;
  }

  // Clamping can only occur on duration-based ground aoe events.
  if (params->n_pulses() == 0 && clamp && tick > time_left)
  {
    assert(params->expiration_pulse() != ground_aoe_params_t::NO_EXPIRATION_PULSE);
    return time_left;
  }

  return tick;
}

bool ground_aoe_event_t::may_pulse() const
{
  if (params->n_pulses() > 0)
  {
    return current_pulse < params->n_pulses();
  }
  else
  {
    auto time_left = _time_left(params, player());

    if (params->expiration_pulse() == ground_aoe_params_t::NO_EXPIRATION_PULSE)
    {
      return time_left >= pulse_time(false);
    }
    else
    {
      return time_left > timespan_t::zero();
    }
  }
}

void ground_aoe_event_t::schedule_event()
{
  auto event = make_event<ground_aoe_event_t>(sim(), _player, params, pulse_state);
  // If the ground-aoe event is a pulse-based one, increase the current pulse of the newly created
  // event.
  if (params->n_pulses() > 0)
  {
    event->set_current_pulse(current_pulse + 1);
  }
}

void ground_aoe_event_t::execute()
{
  action_t* spell_ = params->action();

  if (sim().debug)
  {
    sim().out_debug.printf("%s %s pulse start_time=%.3f remaining_time=%.3f tick_time=%.3f",
      player()->name(), spell_->name(), params->start_time().total_seconds(),
      params->n_pulses() > 0
      ? (params->n_pulses() - current_pulse) * pulse_time().total_seconds()
      : (params->duration() - (sim().current_time() - params->start_time())).total_seconds(),
      pulse_time(false).total_seconds());
  }

  // Manually snapshot the state so we can adjust the x and y coordinates of the snapshotted
  // object. This is relevant if sim -> distance_targeting_enabled is set, since then we need to
  // use the ground object's x, y coordinates, instead of the source actor's.
  pulse_state->target = params->target();
  pulse_state->original_x = params->x();
  pulse_state->original_y = params->y();
  spell_->update_state(pulse_state, spell_->amount_type(pulse_state));

  // Update state multipliers if expiration_pulse() is PARTIAL_PULSE, and the object is pulsing
  // for the last (partial) time. Note that pulse-based ground aoe events do not have a concept of
  // partial ticks.
  if (params->n_pulses() == 0 &&
    params->expiration_pulse() == ground_aoe_params_t::PARTIAL_EXPIRATION_PULSE)
  {
    // Don't clamp the pulse time here, since we need to figure out the fractional multiplier for
    // the last pulse.
    auto time = pulse_time(false);
    auto time_left = _time_left(params, player());
    if (time > time_left)
    {
      double multiplier = time_left / time;
      pulse_state->da_multiplier *= multiplier;
      pulse_state->ta_multiplier *= multiplier;
    }
  }

  spell_->schedule_execute(spell_->get_state(pulse_state));

  // This event is about to be destroyed, notify callback of the event if needed
  if (params->state_callback())
  {
    params->state_callback()(ground_aoe_params_t::EVENT_DESTRUCTED, this);
  }

  // Schedule next tick, if it can fit into the duration
  if (may_pulse())
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
    if (params->state_callback())
    {
      params->state_callback()(ground_aoe_params_t::EVENT_STOPPED, this);
    }
  }
}

// Figure out how to handle expiration callback if it's defined

void ground_aoe_event_t::handle_expiration()
{
  if (!params->expiration_callback())
  {
    return;
  }

  auto time_left = _time_left(params, player());

  // Trigger immediately, since no time left. Can happen for example when ground aoe events are
  // not hasted, or when pulse-based behavior is used (instead of duration-based behavior)
  if (time_left <= timespan_t::zero())
  {
    params->expiration_callback()();
  }
  // Defer until the end of the ground aoe event, even if there are no ticks left
  else
  {
    make_event<expiration_callback_event_t>(sim(), sim(), params, time_left);
  }
}
