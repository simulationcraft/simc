// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <string>

#include "config.hpp"
#include "util/sample_data.hpp"
#include "util/string_view.hpp"

struct sim_t;

struct benefit_t : private noncopyable
{
private:
  int up, down;

public:
  // This is initialized in SIMPLE mode. Only change mode for infrequent procs to keep memory usage reasonable.
  extended_sample_data_t ratio;
  const std::string name_str;

  explicit benefit_t( util::string_view n )
    : up( 0 ), down( 0 ), ratio( "Ratio", true ), name_str( n )
  {
  }

  void update( bool is_up )
  {
    if ( is_up )
      up++;
    else
      down++;
  }

  void analyze()
  {
    ratio.analyze();
  }

  void datacollection_begin()
  {
    up = down = 0;
  }

  void datacollection_end()
  {
    ratio.add( up != 0 ? 100.0 * up / ( down + up ) : 0.0 );
  }

  void merge( const benefit_t& other )
  {
    ratio.merge( other.ratio );
  }

  const std::string& name() const
  {
    return name_str;
  }

  benefit_t* collect_ratio(sim_t& sim, bool collect = true);
};