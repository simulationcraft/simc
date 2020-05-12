// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "item_import.hpp"
#include "item_import_ptr.hpp"

#include "item.hpp"

util::span<const item_data_t> item_data_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  if ( ptr )
  {
    return dbc::items_ptr();
  }
  else
  {
    return dbc::items();
  }
#else
  (void) ptr;
  return dbc::items();
#endif /* SC_USE_PTR */
}

