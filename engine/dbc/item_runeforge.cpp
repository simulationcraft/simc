#include <array>

#include "config.hpp"

#include "util/util.hpp"

#include "item_runeforge.hpp"

#include "generated/item_runeforge.inc"
#if SC_USE_PTR == 1
#include "generated/item_runeforge_ptr.inc"
#endif

util::span<const runeforge_legendary_entry_t> runeforge_legendary_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __runeforge_legendary_data, __ptr_runeforge_legendary_data, ptr );
}

util::span<const runeforge_legendary_entry_t> runeforge_legendary_entry_t::find( util::string_view name, bool ptr )
{
  auto __data = data( ptr );
  auto __begin = std::find_if( __data.begin(), __data.end(), [name]( const runeforge_legendary_entry_t& e ) {
    return util::str_compare_ci( e.name, name );
  } );

  if ( __begin == __data.end() )
  {
    return {};
  }

  auto __end = std::find_if_not( __begin + 1, __data.end(), [name]( const runeforge_legendary_entry_t& e ) {
    return util::str_compare_ci( e.name, name );
  } );

  return { __begin, __end };
}
