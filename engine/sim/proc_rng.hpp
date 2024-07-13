// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "sc_enums.hpp"
#include "util/timespan.hpp"

#include <functional>
#include <string>

struct item_t;
struct player_t;
struct sim_t;
struct spell_data_t;
namespace rng {
  struct rng_t;
}

struct proc_rng_t
{
protected:
  std::string name_str;
  player_t* player;
  rng_type_e rng_type;

public:
  proc_rng_t();
  proc_rng_t( std::string_view n, player_t* p, rng_type_e type );
  virtual ~proc_rng_t() = default;

  virtual bool trigger() = 0;
  virtual void reset() = 0;

  std::string_view name() const
  { return name_str; }

  rng_type_e type() const
  { return rng_type; }
};

struct simple_proc_t final : public proc_rng_t
{
private:
  double chance;

public:
  simple_proc_t( std::string_view n, player_t* p, double c = 0.0 );

  void reset() override {}
  bool trigger() override;
};

// "Real" 'Procs per Minute' helper class =====================================
struct real_ppm_t final : public proc_rng_t
{
  enum blp : int
  {
    BLP_DISABLED = 0,
    BLP_ENABLED
  };

private:
  double freq;
  double modifier;
  double rppm;
  timespan_t last_trigger_attempt;
  timespan_t accumulated_blp;
  unsigned scales_with;
  blp blp_state;

  static constexpr timespan_t max_interval = 3.5_s;
  static constexpr timespan_t max_bad_luck_prot = 1000_s;

public:
  real_ppm_t();

  real_ppm_t( std::string_view n, player_t* p, double f = 0, double mod = 1.0, unsigned s = RPPM_NONE,
              blp b = BLP_ENABLED );

  real_ppm_t( std::string_view n, player_t* p, const spell_data_t* data, const item_t* item = nullptr );

  double proc_chance();

  void reset() override;
  bool trigger() override;

  void set_scaling( unsigned s )
  { scales_with = s; }

  void set_modifier( double mod )
  { modifier = mod; rppm = freq * modifier; }

  void set_frequency( double frequency )
  { freq = frequency; rppm = freq * modifier; }

  void set_blp_state( blp state )
  { blp_state = state; }

  unsigned get_scaling() const
  { return scales_with; }

  double get_frequency() const
  { return freq; }

  double get_modifier() const
  { return modifier; }

  double get_rppm() const
  { return rppm; }

  blp get_blp_state() const
  { return blp_state; }

  timespan_t get_last_trigger_attempt()
  { return last_trigger_attempt; }

  void set_last_trigger_attempt( timespan_t ts )
  { last_trigger_attempt = ts; }

  void set_accumulated_blp( timespan_t ts )
  { accumulated_blp = ts; }
};

// "Deck of Cards" randomizer helper class ====================================
// Described at https://www.reddit.com/r/wow/comments/6j2wwk/wow_class_design_ama_june_2017/djb8z68/
struct shuffled_rng_t final : public proc_rng_t
{
private:
  int success_entries;
  int total_entries;
  int success_entries_remaining;
  int total_entries_remaining;

public:
  shuffled_rng_t( std::string_view n, player_t* p, int success_entries = 0, int total_entries = 0 );

  void reset() override;
  bool trigger() override;

  int get_success_entries() const
  { return success_entries; }

  int get_success_entries_remaining() const
  { return success_entries_remaining; }

  int get_total_entries() const
  { return total_entries; }

  int get_total_entries_remaining() const
  { return total_entries_remaining; }

  double get_remaining_success_chance() const
  { return static_cast<double>( success_entries_remaining ) / static_cast<double>( total_entries_remaining ); }
};

// Accumulated back luck protection rng helper class ==========================
//
// This class of rng will increase the chance of success with each failed trigger. By default, the chance of success is
// given by:
//
//   % success = proc_chance * trigger_count
//
// where trigger_count is the number of trigger attempts since the last success, including the current attempt.
// Thefirst trigger attempt after a successful proc will have a trigger count of 1.
//
// accumulator_fn is an optional functor that takes the proc chance and current trigger count and returns the chance of
// success.
//
// initial_count is an optional parameter that sets the initial trigger count. If initial_count is set, the first
// trigger after a successful proc will have a trigger count of initial_count + 1.
struct accumulated_rng_t final : public proc_rng_t
{
private:
  std::function<double( double, unsigned )> accumulator_fn;
  double proc_chance;
  unsigned initial_count;
  unsigned trigger_count;

public:
  accumulated_rng_t( std::string_view n, player_t* p, double c, std::function<double( double, unsigned )> fn = nullptr,
                     unsigned initial_count = 0 );

  void reset() override;
  bool trigger() override;
};
