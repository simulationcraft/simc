// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "sc_timespan.hpp"
#include <string>

struct item_t;
struct player_t;
struct spell_data_t;

// "Real" 'Procs per Minute' helper class =====================================
struct real_ppm_t
{
  enum blp : int
  {
    BLP_DISABLED = 0,
    BLP_ENABLED
  };

private:
  player_t*    player;
  std::string  name_str;
  double       freq;
  double       modifier;
  double       rppm;
  timespan_t   last_trigger_attempt;
  timespan_t   last_successful_trigger;
  unsigned     scales_with;
  blp          blp_state;

  real_ppm_t() : player( nullptr ), freq( 0 ), modifier( 0 ), rppm( 0 ), scales_with( 0 ),
    blp_state( BLP_ENABLED )
  { }

  static double max_interval() { return 3.5; }
  static double max_bad_luck_prot() { return 1000.0; }
public:
  static double proc_chance( player_t*         player,
                             double            PPM,
                             const timespan_t& last_trigger,
                             const timespan_t& last_successful_proc,
                             unsigned          scales_with,
                             blp               blp_state );

  real_ppm_t( const std::string& name, player_t* p, double frequency = 0, double mod = 1.0, unsigned s = RPPM_NONE, blp b = BLP_ENABLED ) :
    player( p ),
    name_str( name ),
    freq( frequency ),
    modifier( mod ),
    rppm( freq * mod ),
    last_trigger_attempt( timespan_t::zero() ),
    last_successful_trigger( timespan_t::zero() ),
    scales_with( s ),
    blp_state( b )
  { }

  real_ppm_t( const std::string& name, player_t* p, const spell_data_t* data, const item_t* item = nullptr );

  void set_scaling( unsigned s )
  { scales_with = s; }

  void set_modifier( double mod )
  { modifier = mod; rppm = freq * modifier; }

  const std::string& name() const
  { return name_str; }

  void set_frequency( double frequency )
  { freq = frequency; rppm = freq * modifier; }

  void set_blp_state( blp state )
  { blp_state = state; }

  unsigned get_scaling() const
  {
    return scales_with;
  }

  double get_frequency() const
  { return freq; }

  double get_modifier() const
  { return modifier; }

  double get_rppm() const
  { return rppm; }

  blp get_blp_state() const
  { return blp_state; }

  void set_last_trigger_attempt( const timespan_t& ts )
  { last_trigger_attempt = ts; }

  void set_last_trigger_success( const timespan_t& ts )
  { last_successful_trigger = ts; }

  void reset()
  {
    last_trigger_attempt = timespan_t::zero();
    last_successful_trigger = timespan_t::zero();
  }

  bool trigger();
};
