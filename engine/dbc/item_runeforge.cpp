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

const runeforge_legendary_entry_t& runeforge_legendary_entry_t::find( util::string_view name, bool ptr )
{
  for ( const auto& entry : data( ptr ) )
  {
    if ( util::str_compare_ci( entry.name, name ) )
    {
      return entry;
    }
  }

  return nil();
}
