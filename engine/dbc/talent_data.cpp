
#include "talent_data.hpp"

#include <array>

#include "generated/sc_talent_data.inc"
#if SC_USE_PTR
#  include "generated/sc_talent_data_ptr.inc"
#endif

#include "dbc/client_data.hpp"
#include "dbc/dbc.hpp"
#include "util/util.hpp"

bool talent_data_t::is_class( player_e c ) const
{
  unsigned mask = util::class_id_mask( c );

  if ( mask == 0 )
    return false;

  return ( ( _m_class & mask ) == mask );
}

const talent_data_t* talent_data_t::find( unsigned id, bool ptr )
{
  const auto __data = data( ptr );
  auto it = range::lower_bound( __data, id, {}, &talent_data_t::id );
  if ( it != __data.end() && it->id() == id )
    return &*it;
  return &talent_data_t::nil();
}

const talent_data_t* talent_data_t::find( unsigned id, util::string_view confirmation, bool ptr )
{
  const talent_data_t* p = find( id, ptr );
  assert( confirmation == p -> name_cstr() );
  ( void )confirmation;
  return p;
}

const talent_data_t* talent_data_t::find( util::string_view name, specialization_e spec, bool ptr )
{
  for ( const talent_data_t& td : data( ptr ) )
  {
    if ( td.specialization() == spec && name == td.name_cstr() )
      return &td;
  }
  return nullptr;
}

const talent_data_t* talent_data_t::find_tokenized( util::string_view name, specialization_e spec, bool ptr )
{
  std::string tokenized_name;
  for ( const talent_data_t& td : data( ptr ) )
  {
    tokenized_name = td.name_cstr();
    util::tokenize( tokenized_name );
    if ( td.specialization() == spec && util::str_compare_ci( name, tokenized_name ) )
      return &td;
  }

  return nullptr;
}

const talent_data_t* talent_data_t::find( player_e c, unsigned int row, unsigned int col, specialization_e spec, bool ptr )
{
  for ( const talent_data_t& td : data( ptr ) )
  {
    if ( td.is_class( c ) && ( row == td.row() ) && ( col == td.col() ) && spec == td.specialization() )
    {
      return &td;
    }
  }

  // Second round to check all talents, either with a none spec, or no spec check at all,
  // depending on what the caller gave in "spec" parameter
  for ( const talent_data_t& td : data( ptr ) )
  {
    if ( spec != SPEC_NONE )
    {
      if ( td.is_class( c ) && ( row == td.row() ) && ( col == td.col() ) && td.specialization() == SPEC_NONE )
        return &td;
    }
    else
    {
      if ( td.is_class( c ) && ( row == td.row() ) && ( col == td.col() ) )
        return &td;
    }
  }

  return nullptr;
}

util::span<const talent_data_t> talent_data_t::data( bool ptr )
{
  return _data( ptr );
}

void talent_data_t::link( bool ptr )
{
  for ( talent_data_t& td : _data( ptr ) )
    td.spell1 = spell_data_t::find( td.spell_id(), ptr );
}

util::span<talent_data_t> talent_data_t::_data( bool ptr )
{
  return SC_DBC_GET_DATA( __talent_data, __ptr_talent_data, ptr );
}

talent_data_nil_t::talent_data_nil_t()
  : talent_data_t()
{
  spell1 = spell_data_t::not_found();
}

/* static */ const talent_data_nil_t talent_data_nil_t::singleton;
