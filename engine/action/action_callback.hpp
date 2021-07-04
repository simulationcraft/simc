// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/generic.hpp"
#include <vector>

struct action_t;
struct action_state_t;
struct player_t;

// Additional trigger callbacks, currently used by dbc_proc_callback_t to control
// the triggering conditions of the proc.
enum class callback_trigger_fn_type
{
  /// Trigger uses normal processing
  NONE,
  /// Custom callback enforces an additional trigger check on the proc, but uses normal
  /// proc chance and proc-related semantics.
  CONDITION,
  /// Custom callback trigger completely overrides the trigger attempt logic of the
  /// callback. This includes all aspects of the trigger attempt, including e.g.,
  /// (internal) cooldown checks.
  ///
  /// Proc chance (and probability distribution) is still driven by the characteristics of
  /// proc itself.
  ///
  /// Note, the proc flags still control if the proc is even attempted to trigger.
  TRIGGER
};

struct action_callback_t : private noncopyable
{
  using callback_trigger_fn_t = std::function<bool(const action_callback_t*, action_t*, action_state_t*)>;

  player_t* listener;
  const callback_trigger_fn_t* trigger_fn;
  callback_trigger_fn_type trigger_type;
  bool active;
  bool allow_self_procs;
  bool allow_procs;

  action_callback_t(player_t* l, bool ap = false, bool asp = false);
  virtual ~action_callback_t() {}
  virtual void trigger(action_t*, action_state_t*) = 0;
  virtual void reset() {}
  virtual void initialize() { }
  virtual void activate() { active = true; }
  virtual void deactivate() { active = false; }

  static void trigger(const std::vector<action_callback_t*>& v, action_t* a, action_state_t* state);

  static void reset(const std::vector<action_callback_t*>& v);
};
