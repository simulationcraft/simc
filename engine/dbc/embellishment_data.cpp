// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "embellishment_data.hpp"

#include "config.hpp"
#include "util/util.hpp"

#include <array>

#include "generated/embellishment_data.inc"
#if SC_USE_PTR == 1
#include "generated/embellishment_data_ptr.inc"
#endif

util::span<const embellishment_data_t> embellishment_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __embellishment_data, __ptr_embellishment_data, ptr );
}

const embellishment_data_t& embellishment_data_t::find( std::string_view name, bool ptr, bool tokenized )
{
  const auto __data = data( ptr );
  auto it = range::find_if( __data, [ &name, &tokenized ]( const embellishment_data_t& e ) {
    if ( tokenized )
    {
      auto emb_name = util::tokenize_fn( e.name );
      return util::str_compare_ci( emb_name, name );
    }
    else
    {
      return util::str_compare_ci( e.name, name );
    }
  } );

  if ( it != __data.end() )
    return *it;
  else
    return nil();
}
