// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "report/report_helper.hpp"
#include "sc_enums.hpp"
#include "util/format.hpp"
#include "util/generic.hpp"
#include "util/io.hpp"
#include "util/rng.hpp"
#include "util/string_view.hpp"
#include "util/timespan.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct event_t;
struct expr_t;
struct option_t;
struct player_t;
struct sim_t;

struct raid_event_t : private noncopyable
{
public:
  sim_t* sim;
  std::string name;
  std::string type;
  int64_t num_starts;
  timespan_t first, last;
  double first_pct, last_pct;
  rng::truncated_gauss_t cooldown;
  rng::truncated_gauss_t duration;
  int pull;
  std::string pull_target_str;

  // Player filter options
  double distance_min;   // Minimal player distance
  double distance_max;   // Maximal player distance
  bool players_only;     // Don't affect pets
  bool force_stop;       // Stop immediately at last/last_pct
  double player_chance;  // Chance for individual player to be affected by raid event

  std::string affected_role_str;
  role_e affected_role;
  std::string player_if_expr_str;

  timespan_t saved_duration;
  timespan_t saved_cooldown;
  std::vector<player_t*> affected_players;
  std::unordered_map<size_t, std::unique_ptr<expr_t>> player_expressions;
  std::vector<std::unique_ptr<option_t>> options;

  raid_event_t( sim_t*, util::string_view type );

  virtual ~raid_event_t() = default;

  virtual bool filter_player( const player_t* );

  void add_option( std::unique_ptr<option_t> new_option )
  {
    options.insert( options.begin(), std::move( new_option ) );
  }
  const char* log_name() const
  {
    return name.empty() ? type.c_str() : name.c_str();
  }
  timespan_t cooldown_time();
  virtual timespan_t duration_time();
  timespan_t next_time() const;
  timespan_t until_next() const;
  virtual timespan_t remains() const;
  bool up() const;
  void schedule();
  void deactivate( util::string_view reason );
  virtual void reset();
  virtual void combat_begin();
  void parse_options( util::string_view options_str );
  static std::unique_ptr<raid_event_t> create( sim_t* sim, util::string_view name, util::string_view options_str );
  static void init( sim_t* );
  static void reset( sim_t* );
  static void combat_begin( sim_t* );
  static void combat_end( sim_t* )
  {
  }
  static void merge( sim_t* sim, sim_t* other );
  static void analyze( sim_t* sim );
  static void report( sim_t* sim, report::sc_html_stream& os );
  static double evaluate_raid_event_expression( sim_t* s, util::string_view type, util::string_view filter,
                                                bool test_filter, bool* is_constant );

private:
  virtual void _start()  = 0;
  virtual void _finish() = 0;
  void activate( util::string_view reason );
  void start();
  void finish();

  bool is_up;
  enum class activation_status_e
  {
    // three different states so we can detect when raid event is deactivated before it is activated.
    not_yet_activated,
    activated,
    deactivated
  } activation_status;
  event_t* cooldown_event;
  event_t* duration_event;
  event_t* start_event;
  event_t* end_event;

  friend void sc_format_to( const raid_event_t&, fmt::format_context::iterator );
};
