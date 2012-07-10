// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

//-------------- timespan_t --------------

#ifdef SC_USE_INTEGER_TIME

const timespan_t::time_t timespan_t::MILLIS_PER_SECOND = 1000;
const double timespan_t::SECONDS_PER_MILLI = 1.0 / timespan_t::MILLIS_PER_SECOND;
const timespan_t::time_t timespan_t::MILLIS_PER_MINUTE = 60 * timespan_t::MILLIS_PER_SECOND;
const double timespan_t::MINUTES_PER_MILLI = 1.0 / timespan_t::MILLIS_PER_MINUTE;
const timespan_t timespan_t::zero = timespan_t::from_millis( 0 );

#else // #ifndef SC_USE_INTEGER_TIME

const timespan_t::time_t timespan_t::SECONDS_PER_MILLI = 0.001;
const double timespan_t::MILLIS_PER_SECOND = 1.0 / timespan_t::SECONDS_PER_MILLI;
const timespan_t::time_t timespan_t::SECONDS_PER_MINUTE = 60;
const double timespan_t::MINUTES_PER_SECOND = 1.0 / timespan_t::SECONDS_PER_MINUTE;
const timespan_t timespan_t::zero = timespan_t::from_seconds( 0 );

#endif

//const timespan_t timespan_t::min = timespan_t(std::numeric_limits<timespan_t::time_t>::min());
const timespan_t timespan_t::min = timespan_t::from_seconds( -1.0 );
const timespan_t timespan_t::max = timespan_t( std::numeric_limits<timespan_t::time_t>::max() );
