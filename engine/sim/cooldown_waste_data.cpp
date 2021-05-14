// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "cooldown_waste_data.hpp"

#include "sim/sc_cooldown.hpp"
#include "sim/sc_sim.hpp"

cooldown_waste_data_t::cooldown_waste_data_t( const cooldown_t* cooldown, bool simple )
  : cd( cooldown ),
    buffer(),
    normal( cd->name_str + " cooldown waste", simple ),
    cumulative( cd->name_str + " cumulative cooldown waste", simple )
{
}

void cooldown_waste_data_t::add( timespan_t cd_override, timespan_t time_to_execute )
{
  if ( cd_override == 0_ms || ( cd_override < 0_ms && cd->duration <= 0_ms ) )
    return;

  if ( cd->ongoing() )
  {
    normal.add( 0.0 );
  }
  else
  {
    double wasted = ( cd->sim.current_time() - cd->last_charged ).total_seconds();

    // Waste caused by execute time is unavoidable for single charge spells, don't count it.
    if ( cd->charges == 1 )
      wasted -= time_to_execute.total_seconds();

    normal.add( wasted );
    buffer += wasted;
  }
}

bool cooldown_waste_data_t::active() const
{
  return normal.count() > 0 && cumulative.sum() > 0;
}

void cooldown_waste_data_t::merge( const cooldown_waste_data_t& other )
{
  normal.merge( other.normal );
  cumulative.merge( other.cumulative );
}

void cooldown_waste_data_t::analyze()
{
  normal.analyze();
  cumulative.analyze();
}

void cooldown_waste_data_t::datacollection_begin()
{
  buffer = 0.0;
}

void cooldown_waste_data_t::datacollection_end()
{
  if ( !cd->ongoing() )
    buffer += ( cd->sim.current_time() - cd->last_charged ).total_seconds();

  cumulative.add( buffer );
}