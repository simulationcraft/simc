// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/timespan.hpp"
#include "sc_enums.hpp"
#include "util/string_view.hpp"
#include "util/format.hpp"

#include <string>
#include <memory>

struct action_t;
struct event_t;
struct expr_t;
struct player_t;
struct sim_t;

// Cooldown =================================================================

struct cooldown_t
{
  sim_t& sim;
  player_t* player;
  std::string name_str;
  timespan_t duration;
  timespan_t ready;
  timespan_t reset_react;
  // Only adjust during initialization, otherwise use adjust/set_max_charges function
  int charges;
  event_t* recharge_event;
  event_t* ready_trigger_event;
  timespan_t last_start, last_charged;
  bool hasted; // Hasted cooldowns will reschedule based on haste state changing (through buffs). TODO: Separate hastes?
  action_t* action; // Dynamic cooldowns will need to know what action triggered the cd

  // Associated execution types amongst all the actions shared by this cooldown. Bitmasks based on
  // the execute_type enum class
  unsigned execute_types_mask;

  // State of the current cooldown progression. Only updated for ongoing cooldowns.
  int current_charge;
  double recharge_multiplier;
  timespan_t base_duration;

  cooldown_t( util::string_view name, player_t& );
  cooldown_t( util::string_view name, sim_t& );

  // Adjust the CD. If "requires_reaction" is true (or not provided), then the CD change is something
  // the user would react to rather than plan ahead for.
  void adjust( timespan_t, bool requires_reaction = true );
  void adjust_recharge_multiplier(); // Reacquire cooldown recharge multiplier from the action to adjust the cooldown time
  void adjust_base_duration(); // Reacquire base cooldown duration from the action to adjust the cooldown time
  // Instantly recharge a cooldown. For multicharge cooldowns, charges_ specifies how many charges to reset.
  // If less than zero, all charges are reset.
  void reset( bool require_reaction, int charges_ = 1 );
  void start( action_t* action, timespan_t override = timespan_t::min(), timespan_t delay = timespan_t::zero() );
  void start( timespan_t override = timespan_t::min(), timespan_t delay = timespan_t::zero() );

  void reset_init();

  timespan_t remains() const;

  timespan_t current_charge_remains() const;

  // Return true if the cooldown is ready (has at least one charge).
  bool up() const;

  // Return true if the cooldown is not ready (has zero charges).
  bool down() const;

  // Return true if the cooldown is in progress.
  bool ongoing() const
  { return down() || recharge_event; }

  // Return true if the action bound to this cooldown is ready. Cooldowns are ready either when
  // their cooldown has elapsed, or a short while before its cooldown is finished. The latter case
  // is in use when the cooldown is associated with a foreground action (i.e., a player button
  // press) to model in-game behavior of queueing soon-to-be-ready abilities. Inlined below.
  bool is_ready() const;

  // Return the queueing delay for cooldowns that are queueable
  timespan_t queue_delay() const;

  // Point in time when the cooldown is queueable
  timespan_t queueable() const;

  double charges_fractional() const;

  // Trigger update of specialized execute thresholds for this cooldown
  void update_ready_thresholds();

  const std::string& name() const
  { return name_str; }

  std::unique_ptr<expr_t> create_expression( util::string_view name_str );

  void add_execute_type( execute_type e )
  { execute_types_mask |= ( 1 << static_cast<unsigned>( e ) ); }

  static timespan_t ready_init()
  { return timespan_t::from_seconds( -60 * 60 ); }

  static timespan_t cooldown_duration( const cooldown_t* cd );

  /**
   * Adjust maximum charges for a cooldown
   * Takes the cooldown and new maximum charge count
   * Function depends on the internal working of cooldown_t::reset
   */
  void set_max_charges( int new_max_charges );
  void adjust_max_charges( int charge_change );

  friend void format_to( const cooldown_t&, fmt::format_context::iterator );

private:
  void adjust_remaining_duration( double delta ); // Modify the remaining duration of an ongoing cooldown.
};