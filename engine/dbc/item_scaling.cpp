#include <array>

#include "config.hpp"

#include "item_scaling.hpp"

#include "generated/item_scaling.inc"
#if SC_USE_PTR == 1
#include "generated/item_scaling_ptr.inc"
#endif

arv::array_view<scaling_stat_distribution_t> scaling_stat_distribution_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<scaling_stat_distribution_t>( __ptr_scaling_stat_distribution_data )
                        : arv::array_view<scaling_stat_distribution_t>( __scaling_stat_distribution_data );
#else
  ( void ) ptr;
  const auto& data = __scaling_stat_distribution_data;
#endif

  return data;
}


arv::array_view<curve_point_t> curve_point_t::data( bool ptr )
{
#if SC_USE_PTR == 1
  const auto data = ptr ? arv::array_view<curve_point_t>( __ptr_curve_point_data )
                        : arv::array_view<curve_point_t>( __curve_point_data );
#else
  ( void ) ptr;
  const auto& data = __curve_point_data;
#endif

  return data;
}


arv::array_view<curve_point_t> curve_point_t::find( unsigned id, bool ptr )
{
  const auto __data = data( ptr );

  auto low = std::lower_bound( __data.cbegin(), __data.cend(), id,
                              []( const curve_point_t& entry, const unsigned& id ) {
                                return entry.curve_id < id;
                              } );
  if ( low == __data.cend() )
  {
    return {};
  }

  auto high = std::upper_bound( __data.cbegin(), __data.cend(), id,
                              []( const unsigned& id, const curve_point_t& entry ) {
                                return id < entry.curve_id;
                              } );

  return arv::array_view<curve_point_t>( low, high );
}

