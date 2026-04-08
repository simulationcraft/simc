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
struct spell_data_t;
enum proc_trigger_type_e : unsigned short;

struct proc_data_t
{
  const spell_data_t* spell;
  bool suppress_caster_procs;
  bool enable_proc_from_suppressed;
  bool can_proc_from_suppressed;
  bool suppress_target_procs;
  bool can_proc_from_suppressed_target;
  bool allow_class_ability_procs;
  bool can_only_proc_from_class_abilities;
  bool can_proc_from_procs;

  proc_data_t();
  proc_data_t( const spell_data_t* );

  void _init();

  const spell_data_t* operator->() const
  { return spell; }

  operator const spell_data_t*() const
  { return spell; }

  static bool check_proc_trigger( const proc_data_t& source, const proc_data_t& target, proc_trigger_type_e type );

  static const proc_data_t& nil();
};

inline const proc_data_t proc_data_t_nil_v = proc_data_t();
inline const proc_data_t& proc_data_t::nil() { return proc_data_t_nil_v; }

struct action_callback_t : private noncopyable
{
  player_t* listener;
  bool active;
  bool allow_self_procs;
  bool allow_pet_procs;

  action_callback_t( player_t* l );
  virtual ~action_callback_t() = default;
  virtual void trigger( const proc_data_t& data, player_t* target, action_state_t* state, proc_trigger_type_e type ) = 0;
  virtual void reset() {}
  virtual void initialize() {}
  virtual void activate() { active = true; }
  virtual void deactivate() { active = false; }

  static void trigger( const std::vector<action_callback_t*>& v, const proc_data_t& data, player_t* player,
                       player_t* target, action_state_t* state, proc_trigger_type_e type );

  static void reset( const std::vector<action_callback_t*>& v );
};
