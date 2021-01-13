// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "action/sc_action.hpp"
#include "util/timespan.hpp"
#include <vector>
#include <string>

struct action_t;
struct action_state_t;
struct player_t;

// Sequence =================================================================

struct sequence_t : public action_t
{
  bool waiting;
  int sequence_wait_on_ready;
  std::vector<action_t*> sub_actions;
  int current_action;
  bool restarted;
  timespan_t last_restart;
  bool has_option;

  sequence_t(player_t*, util::string_view sub_action_str);

  virtual void schedule_execute(action_state_t* execute_state = nullptr) override;
  virtual void reset() override;
  virtual bool ready() override;
  void restart();
  bool can_restart();
  void init_finished() override;
};

struct strict_sequence_t : public action_t
{
  size_t current_action;
  std::vector<action_t*> sub_actions;
  std::string seq_name_str;

  // Allow strict sequence sub-actions to be skipped if they are not ready. Default = false.
  bool allow_skip;

  strict_sequence_t( player_t*, util::string_view options );

  bool ready() override;
  void reset() override;
  void cancel() override;
  void interrupt_action() override;
  void schedule_execute(action_state_t* execute_state = nullptr) override;
  void init_finished() override;
};