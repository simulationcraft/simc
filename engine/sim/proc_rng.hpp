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
#include <utility>

struct action_state_t;
struct item_t;
struct player_t;
struct sim_t;
struct spell_data_t;
namespace rng {
  struct rng_t;
}

enum class reset_type_e : int
{
  COMBAT = 0,
  ITERATION = 1
};

struct proc_rng_t
{
protected:
  std::string name_str;
  player_t* player;
  const rng_type_e rng_type_;

public:
  static constexpr rng_type_e rng_type = RNG_NONE;

  proc_rng_t( rng_type_e type_ );
  proc_rng_t( rng_type_e type_, std::string_view n, player_t* p );
  virtual ~proc_rng_t() = default;

  virtual int trigger( action_state_t* = nullptr ) = 0;
  virtual void reset( reset_type_e reset_type ) = 0;

  std::string_view name() const
  { return name_str; }

  rng_type_e type() const
  { return rng_type_; }
};

struct simple_proc_t final : public proc_rng_t
{
private:
  double chance;

public:
  static constexpr rng_type_e rng_type = RNG_SIMPLE;

  simple_proc_t( std::string_view n, player_t* p, double c = 0.0 );

  void reset( reset_type_e /* reset_type */ ) override {}
  int trigger( action_state_t* = nullptr ) override;
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
  static constexpr rng_type_e rng_type = RNG_RPPM;

  real_ppm_t( std::string_view n, player_t* p, double f = 0, double mod = 1.0, unsigned s = RPPM_NONE,
              blp b = BLP_ENABLED );

  real_ppm_t( std::string_view n, player_t* p, const spell_data_t* data, const item_t* item = nullptr );

  double proc_chance();

  void reset( reset_type_e reset_type ) override;
  int trigger( action_state_t* = nullptr) override;

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

// Extended "Deck of Cards" to support multiple success/failure types
// Described at https://www.reddit.com/r/wow/comments/6j2wwk/wow_class_design_ama_june_2017/djb8z68/
enum shuffled_rng_e : int
{
  FAIL = 0,
  SUCCESS = 1
};

struct shuffled_rng_t : public proc_rng_t
{
  using initializer = std::initializer_list<std::pair<int, int>>;
private:
  void init( initializer data );
protected:
  std::vector<int>::iterator position;
  std::vector<int> entries;

public:
  static constexpr rng_type_e rng_type = RNG_SHUFFLE;

  shuffled_rng_t( std::string_view n, player_t* p, initializer data );
  shuffled_rng_t( std::string_view n, player_t* p, int success_entries = 0, int total_entries = 0 );
  void reset( reset_type_e reset_type ) override;
  int trigger( action_state_t* = nullptr ) override;

  int count_remains( int key );
  int entry_remains();
};

namespace prd {
// Computes the expected number of attempts before getting a proc given the
// PRD constant C and a cap after which the proc is guaranteed. Returns the
// expectation and its derivative with respect to C.
constexpr std::pair<double, double> expected_attempts( double C, int K )
{
  double chain = 1.0;   // Chance of getting a chain of unsuccessful procs
  double d_chain = 0.0; // and its derivative

  double expected = 0.0;
  double d_expected = 0.0;

  for ( int i = 1; i <= K; i++ )
  {
    expected += chain;
    d_expected += d_chain;

    double p = i * C;
    if ( p >= 1.0 )
      break;

    d_chain = d_chain * ( 1 - p ) - chain * i; // Chain rule
    chain *= 1 - p;
  }

  return { expected, d_expected };
}

// Finds the PRD constant C given the average proc rate p and a cap K
// after which a proc is guaranteed. K <= 0 is treated as no cap.
constexpr double find_constant( double p, int K = 0 )
{
  // This isn't strictly speaking correct for really small p values
  // (< 0.05%) but such values aren't realistic in the sim setting.
  if ( K <= 0 )
    K = 100000;

  if ( p <= 1.0 / K )
    return 0.0;
  if ( p >= 1.0 )
    return 1.0;

  double tgt_expected = 1.0 / p;
  // Initial guess that works well for normal p and K values.
  double guess = std::min( 1.25 * p * p, 0.99 );

  constexpr int max_iterations = 20;
  constexpr double precision = 1e-12;

  for ( int i = 0; i < max_iterations; i++ )
  {
    auto [ expected, d_expected ] = expected_attempts( guess, K );
    double error = expected - tgt_expected;
    // TODO: Change to std::fabs when we switch to C++ version where it's constexpr
    if ( error < precision && -error < precision )
      break;

    // Newton's method update
    guess -= error / d_expected;
  }

  return guess;
}

}  // namespace prd

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
using accumulated_rng_fn = std::function<double( double, unsigned, action_state_t* )>;

struct accumulated_rng_t : public proc_rng_t
{
private:
  accumulated_rng_fn accumulator_fn;
  double proc_chance;
  unsigned initial_count;
  unsigned trigger_count;

public:
  static constexpr rng_type_e rng_type = RNG_ACCUMULATE;

  accumulated_rng_t( std::string_view n, player_t* p, double c, accumulated_rng_fn fn = nullptr,
                     unsigned initial_count = 0 );

  void reset( reset_type_e reset_type ) override;
  int trigger( action_state_t* = nullptr ) override;
};

// Threshold RNG rng helper class ==========================
//
// This accumulates an incremental value, returning a successful trigger when the accumulated value goes over 1.
//
// roll_over is an optional parameter, default false. if set the accumulated amount is decremented by 1, else reset to 0
// for the next trigger.
//
// random_initial_state is an optional parameter, default true, that indicates the RNG will start somewhere between [0,1)
// upon reset();
//
// accumulator_fn is an optional functor that takes the increment_max value as a parameter and returns the amount accumulated
// by that call.
using threshold_rng_fn = std::function<double( double, action_state_t* )>;

struct threshold_rng_t : public proc_rng_t
{
private:
  threshold_rng_fn accumulator_fn;
  double increment_max;
  double accumulated_chance;
  bool random_initial_state;
  bool roll_over;

public:
  static constexpr rng_type_e rng_type = RNG_THRESHOLD;

  threshold_rng_t( std::string_view n, player_t* p, double increment_max, threshold_rng_fn fn = nullptr,
                   bool random_initial_state = true, bool roll_over = false );

  void reset( reset_type_e reset_type ) override;
  int trigger( action_state_t* = nullptr ) override;

  double get_accumulated_chance();
  double get_increment_max();
};
