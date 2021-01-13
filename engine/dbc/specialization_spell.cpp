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
specialization_spell_entry_t::find( util::string_view name,
                                    bool              ptr,
                                    specialization_e  spec,
                                    util::string_view desc,
                                    bool              tokenized )
{
  auto __data = data( ptr );

  std::string cmp_str;
  std::string desc_cmp_str;
  if ( tokenized )
  {
    cmp_str = util::tokenize_fn( name );
    name = cmp_str;
    desc_cmp_str = util::tokenize_fn( desc );
    desc = desc_cmp_str;
  }

  auto it = range::find_if( __data, [tokenized, spec, name, desc]( const auto& entry ) {
      if ( spec != SPEC_NONE && spec != entry.specialization_id )
      {
        return false;
      }

      if ( tokenized )
      {
        if ( !desc.empty() )
        {
          if ( !entry.desc ||
               !util::str_compare_ci( util::tokenize_fn( entry.desc ), desc ) )
          {
            return false;
          }
        }
        else
        {
          if ( entry.desc )
          {
            return false;
          }
        }

        auto spell_str = util::tokenize_fn( entry.name );
        return util::str_compare_ci( spell_str, name );
      }
      else
      {
        if ( !desc.empty() && !util::str_compare_ci( entry.desc, desc ) )
        {
          return false;
        }

        if ( desc.empty() && entry.desc )
        {
          return false;
        }

        return util::str_compare_ci( entry.name, name );
      }
  } );

  if ( it != __data.end() )
  {
    return *it;
  }

  return nil();
}

