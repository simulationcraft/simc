#include <array>

#include "config.hpp"

#include "util/util.hpp"

#include "specialization_spell.hpp"

#include "generated/specialization_spells.inc"
#if SC_USE_PTR == 1
#include "generated/specialization_spells_ptr.inc"
#endif

util::span<const specialization_spell_entry_t> specialization_spell_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __specialization_spell_data, __ptr_specialization_spell_data, ptr );
}

const specialization_spell_entry_t&
specialization_spell_entry_t::find( const std::string& name,
                                    bool               ptr,
                                    specialization_e   spec,
                                    bool               tokenized )
{
  auto __data = data( ptr );

  std::string cmp_str = name;
  if ( tokenized )
  {
    util::tokenize( cmp_str );
  }

  auto it = range::find_if( __data, [tokenized, spec, &cmp_str]( const auto& entry ) {
      if ( spec != SPEC_NONE && spec != entry.specialization_id )
      {
        return false;
      }

      if ( tokenized )
      {
        auto spell_str = util::tokenize_fn( entry.name );
        return util::str_compare_ci( spell_str, cmp_str );
      }
      else
      {
        return util::str_compare_ci( entry.name, cmp_str );
      }
  } );

  if ( it != __data.end() )
  {
    return *it;
  }

  return nil();
}

