#include <array>

#include "config.hpp"

#include "mastery_spells.hpp"

#include "generated/mastery_spells.inc"
#if SC_USE_PTR == 1
#include "generated/mastery_spells_ptr.inc"
#endif

util::span<const mastery_spell_entry_t> mastery_spell_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __mastery_spell_data, __ptr_mastery_spell_data, ptr );
}
