#include <array>

#include "config.hpp"

#include "util/util.hpp"

#include "active_spells.hpp"

#include "generated/active_spells.inc"
#if SC_USE_PTR == 1
#include "generated/active_spells_ptr.inc"
#endif

namespace
{
const active_class_spell_t& __find_class( util::string_view name,
                                          bool              ptr,
                                          bool              tokenized,
                                          player_e          class_,
                                          specialization_e  spec )
{
  std::string name_str = tokenized ? util::tokenize_fn( name ) : std::string( name );
  unsigned class_id = util::class_id( class_ );
  unsigned spec_id = static_cast<unsigned>( spec );

  const auto __data = active_class_spell_t::data( ptr );

  auto it = range::find_if( __data,
  [&name_str, tokenized, class_id, spec_id]( const active_class_spell_t& e ) {
    if ( class_id != 0 && e.class_id != class_id )
    {
      return false;
    }

    if ( spec_id != 0 && e.spec_id != spec_id )
    {
      return false;
    }

    auto str = tokenized ? util::tokenize_fn( e.name ) : e.name;
    return util::str_compare_ci( name_str, str );
  } );

  if ( it == __data.cend() )
  {
    return active_class_spell_t::nil();
  }

  return *it;
}

const active_pet_spell_t& __find_pet( util::string_view name,
                                      bool              ptr,
                                      bool              tokenized,
                                      player_e          class_)
{
  std::string name_str = tokenized ? util::tokenize_fn( name ) : std::string( name );
  unsigned class_id = util::class_id( class_ );

  const auto __data = active_pet_spell_t::data( ptr );

  auto it = range::find_if( __data,
  [&name_str, tokenized, class_id]( const active_pet_spell_t& e ) {
    if ( class_id != 0 && e.owner_class_id != class_id )
    {
      return false;
    }

    auto str = tokenized ? util::tokenize_fn( e.name ) : e.name;
    return util::str_compare_ci( name_str, str );
  } );

  if ( it == __data.cend() )
  {
    return active_pet_spell_t::nil();
  }

  return *it;
}
} // Namespace anonymous ends

util::span<const active_class_spell_t> active_class_spell_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __active_spells_data, __ptr_active_spells_data, ptr );
}

const active_class_spell_t&
active_class_spell_t::find( util::string_view name, bool ptr, bool tokenized )
{
  return __find_class( name, ptr, tokenized, PLAYER_NONE, SPEC_NONE );
}

const active_class_spell_t&
active_class_spell_t::find( util::string_view name,
                            player_e          class_,
                            bool              ptr,
                            bool              tokenized )
{
  return __find_class( name, ptr, tokenized, class_, SPEC_NONE );
}

const active_class_spell_t&
active_class_spell_t::find( util::string_view name,
                            specialization_e  spec,
                            bool              ptr,
                            bool              tokenized )
{
  return __find_class( name, ptr, tokenized, PLAYER_NONE, spec );
}

util::span<const active_pet_spell_t> active_pet_spell_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __active_pet_spells_data, __ptr_active_pet_spells_data, ptr );
}

const active_pet_spell_t&
active_pet_spell_t::find( util::string_view name, bool ptr, bool tokenized )
{
  return __find_pet( name, ptr, tokenized, PLAYER_NONE );
}

const active_pet_spell_t&
active_pet_spell_t::find( util::string_view name,
                          player_e          class_,
                          bool              ptr,
                          bool              tokenized )
{
  return __find_pet( name, ptr, tokenized, class_ );
}

