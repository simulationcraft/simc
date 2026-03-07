#include <array>

#include "config.hpp"

#include "util/util.hpp"

#include "rank_spells.hpp"

#include "generated/rank_spells.inc"
#if SC_USE_PTR == 1
#include "generated/rank_spells_ptr.inc"
#endif

util::span<const rank_class_spell_t> rank_class_spell_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __rank_spells_data, __ptr_rank_spells_data, ptr );
}

const rank_class_spell_t& rank_class_spell_t::find( util::string_view name,
                                                    util::string_view rank,
                                                    unsigned          class_id,
                                                    unsigned          spec_id,
                                                    bool              ptr )
{
  auto __data = data( ptr );
  auto __range = range::equal_range( __data, class_id, {}, &rank_class_spell_t::class_id );

  if ( __range.first == __data.end() )
  {
    return nil();
  }

  // Just do a linear search for the rank spells, there are at most a few for each spell
  auto it = std::find_if( __range.first, __range.second, [ name, rank, spec_id ]( const rank_class_spell_t& e ) {
    if ( e.spec_id && e.spec_id != spec_id )
    {
      return false;
    }

    return util::str_compare_ci( name, e.name ) && util::str_compare_ci( rank, e.rank );
  } );

  if ( it == __range.second )
  {
    return nil();
  }

  return *it;
}
