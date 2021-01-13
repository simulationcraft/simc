#include <array>

#include "config.hpp"
#include "util/util.hpp"

#include "covenant_data.hpp"

#include "generated/covenant_data.inc"
#if SC_USE_PTR == 1
#include "generated/covenant_data_ptr.inc"
#endif

namespace
{
template <typename T>
const T& find_entry( util::span<const T> entries, util::string_view str, bool tokenized )
{
  std::string search_str = tokenized ? std::string( str ) : util::tokenize_fn( str );

  for ( const auto& entry : entries )
  {
    if ( tokenized )
    {
      if ( util::str_compare_ci( util::tokenize_fn( entry.name ), str ) )
      {
        return entry;
      }
    }
    else
    {
      if ( util::str_compare_ci( entry.name, str ) )
      {
        return entry;
      }
    }
  }

  return T::nil();
}
} // Namespace anonymous ends

util::span<const conduit_entry_t> conduit_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __conduit_data, __ptr_conduit_data, ptr );
}

util::span<const conduit_rank_entry_t> conduit_rank_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __conduit_rank_data, __ptr_conduit_rank_data, ptr );
}

util::span<const soulbind_ability_entry_t> soulbind_ability_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __soulbind_ability_data, __ptr_soulbind_ability_data, ptr );
}

util::span<const covenant_ability_entry_t> covenant_ability_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __covenant_ability_data, __ptr_covenant_ability_data, ptr );
}

const conduit_entry_t& conduit_entry_t::find( util::string_view name,
                                              bool              ptr,
                                              bool              tokenized )
{
  return find_entry( data( ptr ), name , tokenized );
}

const conduit_entry_t& conduit_entry_t::find_by_spellid( unsigned spell_id, bool ptr )
{
  const auto index = SC_DBC_GET_DATA( __conduit_spell_id_index, __ptr_conduit_spell_id_index, ptr );
  return dbc::find_indexed( spell_id, data( ptr ), index, &conduit_entry_t::spell_id );
}

const conduit_rank_entry_t& conduit_rank_entry_t::find( unsigned id,
                                                        unsigned rank,
                                                        bool     ptr )
{
  auto __data = find( id, ptr );
  if ( __data.empty() )
  {
    return nil();
  }

  auto it = range::lower_bound( __data, rank, {}, &conduit_rank_entry_t::rank );

  if ( it == __data.end() || it->rank != rank )
  {
    return nil();
  }

  return *it;
}

const soulbind_ability_entry_t& soulbind_ability_entry_t::find( util::string_view name,
                                                                bool              ptr,
                                                                bool              tokenized )
{
  return find_entry( data( ptr ), name , tokenized );
}

const soulbind_ability_entry_t& soulbind_ability_entry_t::find_by_soulbind_id(
    unsigned soulbind_id, bool ptr )
{
  auto __data = soulbind_ability_entry_t::data( ptr );

  auto it = range::find_if( __data, [soulbind_id]( const soulbind_ability_entry_t& e ) {
    return e.soulbind_id == soulbind_id;
  } );

  if ( it != __data.end() )
  {
    return *it;
  }

  return nil();
}

const covenant_ability_entry_t& covenant_ability_entry_t::find( util::string_view name,
                                                                bool              ptr )
{
  auto __data = data( ptr );
  auto it = range::lower_bound( __data, name, {}, &covenant_ability_entry_t::name );
  if ( it == __data.end() || !util::str_compare_ci( it->name, name ) )
  {
    return nil();
  }

  return *it;
}

const covenant_ability_entry_t& covenant_ability_entry_t::find( unsigned spell_id,
                                                                bool     ptr )
{
  auto __data = data( ptr );
  auto it = range::find_if( __data, [spell_id]( const auto& entry ) {
    return entry.spell_id == spell_id;
  } );

  if ( it == __data.end() )
  {
    return nil();
  }

  return *it;
}
