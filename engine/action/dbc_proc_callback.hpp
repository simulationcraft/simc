// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "action_callback.hpp"

struct action_state_t;
struct buff_t;
struct cooldown_t;
struct item_t;
struct real_ppm_t;
namespace rng {
  struct rng_t;
}
struct special_effect_t;
struct weapon_t;
struct proc_event_t;

/**
 * DBC-driven proc callback. Extensively leans on the concept of "driver"
 * spells that blizzard uses to trigger actual proc spells in most cases. Uses
 * spell data as much as possible (through interaction with special_effect_t).
 * Intentionally vastly simplified compared to our other (older) callback
 * systems. The "complex" logic is offloaded either into special_effect_t
 * (creation of buffs/actions), special effect_t initialization (what kind of
 * special effect to create from DBC data or user given options, or when and
 * why to proc things (new DBC based proc system).
 *
 * The actual triggering logic is also vastly simplified (see execute()), as
 * the majority of procs in WoW are very simple. Custom procs can always be
 * derived off of this struct.
 */
struct dbc_proc_callback_t : public action_callback_t
{
  static const item_t default_item_;

  const item_t& item;
  const special_effect_t& effect;
  cooldown_t* cooldown;

  // Proc trigger types, cached/initialized here from special_effect_t to avoid
  // needless spell data lookups in vast majority of cases
  real_ppm_t* rppm;
  double      proc_chance;
  double      ppm;

  buff_t* proc_buff;
  action_t* proc_action;
  weapon_t* weapon;

  /// Expires proc_buff on max stack, automatically set if proc_buff max_stack > 1
  bool expire_on_max_stack;

  dbc_proc_callback_t(const item_t& i, const special_effect_t& e);

  dbc_proc_callback_t(const item_t* i, const special_effect_t& e);

  dbc_proc_callback_t(player_t* p, const special_effect_t& e);

  virtual void initialize() override;

  void trigger(action_t* a, action_state_t* state) override;

  // Determine target for the callback (action).
  virtual player_t* target(const action_state_t* state) const;

  rng::rng_t& rng() const;

private:
  bool roll(action_t* action);

protected:
  friend struct proc_event_t;
  /**
   * Base rules for proc execution.
   * 1) If we proc a buff, trigger it
   * 2a) If the buff triggered and is at max stack, and we have an action,
   *     execute the action on the target of the action that triggered this
   *     proc.
   * 2b) If we have no buff, trigger the action on the target of the action
   *     that triggered this proc.
   *
   * TODO: Ticking buffs, though that'd be better served by fusing tick_buff_t
   * into buff_t.
   * TODO: Proc delay
   * TODO: Buff cooldown hackery for expressions. Is this really needed or can
   * we do it in a smarter way (better expression support?)
   */
  virtual void execute(action_t* /* a */, action_state_t* state);
};
