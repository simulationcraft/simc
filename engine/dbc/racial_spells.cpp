#include <array>

#include "config.hpp"
#include "util/util.hpp"

#include "racial_spells.hpp"

#include "generated/racial_spells.inc"
#if SC_USE_PTR == 1
#include "generated/racial_spells_ptr.inc"
#endif

util::span<const racial_spell_entry_t> racial_spell_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __racial_spell_data, __ptr_racial_spell_data, ptr );
}

const racial_spell_entry_t&
racial_spell_entry_t::find( util::string_view name, bool ptr, race_e r, player_e c )
{
  const racial_spell_entry_t* n = nullptr;
  auto class_mask = util::class_id_mask( c );
  auto race_mask = util::race_mask( r );

  if ( race_mask == 0 )
  {
    return nil();
  }

  for ( const auto& entry : data( ptr ) )
  {
    if ( entry.mask_race > race_mask )
    {
      break;
    }

    if ( !( entry.mask_race & race_mask ) )
    {
      continue;
    }

    if ( !util::str_compare_ci( entry.name, name ) )
    {
      continue;
    }

    if ( entry.mask_class & class_mask )
    {
      return entry;
    }
    else
    {
      n = &entry;
    }
  }

  return n ? *n : nil();
}

