// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "artifact_power.hpp"
#include "dbc/dbc.hpp"

artifact_power_t::artifact_power_t( unsigned rank, const spell_data_t* spell, const artifact_power_data_t* power,
                                    const artifact_power_rank_t* rank_data )
  : rank_( rank ), spell_( spell ), rank_data_( rank_data ), power_( power )
{
}

artifact_power_t::artifact_power_t()
  : artifact_power_t( 0, spell_data_t::not_found(), artifact_power_data_t::nil(), artifact_power_rank_t::nil() )
{
}

double artifact_power_t::value( std::size_t idx ) const
{
  if ( ( rank() == 1 && rank_data_->value() == 0.0 ) || rank_data_->value() == 0.0 )
  {
    return spell_->effectN( idx ).base_value();
  }
  else
  {
    return rank_data_->value();
  }
}

timespan_t artifact_power_t::time_value( std::size_t idx ) const
{
  if ( rank() == 1 )
  {
    return spell_->effectN( idx ).time_value();
  }
  else
  {
    return timespan_t::from_millis( rank_data_->value() );
  }
}