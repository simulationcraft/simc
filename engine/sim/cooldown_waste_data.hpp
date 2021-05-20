// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "util/generic.hpp"
#include "util/sample_data.hpp"
#include "util/timespan.hpp"

struct cooldown_t;

struct cooldown_waste_data_t : private noncopyable
{
  const cooldown_t* cd;
  double buffer;
  extended_sample_data_t normal;
  extended_sample_data_t cumulative;

  cooldown_waste_data_t( const cooldown_t* cooldown, bool simple = true );

  void add( timespan_t cd_override = timespan_t::min(), timespan_t time_to_execute = 0_ms );
  bool active() const;
  void merge( const cooldown_waste_data_t& other );
  void analyze();
  void datacollection_begin();
  void datacollection_end();
};
