// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include <array>

#include "config.hpp"

#include "util/util.hpp"

#include "trait_data.hpp"

#include "generated/trait_data.inc"
#if SC_USE_PTR == 1
#include "generated/trait_data_ptr.inc"
#endif

#include <iostream>

util::span<const trait_data_t> trait_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __trait_data_data, __ptr_trait_data_data, ptr );
}

util::span<const trait_data_t> trait_data_t::data( talent_tree tree, bool ptr )
{
  auto _data = data( ptr );
  auto _tree_index = static_cast<unsigned>( tree );
  auto _tree_range = range::equal_range( _data, _tree_index, {}, &trait_data_t::tree_index );

  return { _tree_range.first, _tree_range.second };
}

util::span<const trait_data_t> trait_data_t::data( unsigned class_id, talent_tree tree, bool ptr )
{
  auto _tree_span = data( tree, ptr );
  auto _class_range = range::equal_range( _tree_span, class_id, {}, &trait_data_t::id_class );

  return { _class_range.first, _class_range.second };
}

const trait_data_t* trait_data_t::find( unsigned trait_node_entry_id, bool ptr )
{
  auto _data = data( ptr );
  auto _it = range::find( _data, trait_node_entry_id, &trait_data_t::id_trait_node_entry );

  if ( _it != _data.end() )
  {
    return _it;
  }

  return &( nil() );
}

const trait_data_t* trait_data_t::find(
    talent_tree       tree,
    util::string_view name,
    unsigned          class_id,
    specialization_e  spec,
    bool              ptr )
{
  auto _data = data( class_id, tree, ptr );
  auto _it = range::find_if( _data, [name, spec]( const trait_data_t& entry ) {
    if ( util::str_compare_ci( name, entry.name ) )
    {
      if ( entry.id_spec[ 0 ] == 0 )
      {
        return true;
      }

      auto _spec_it = range::find( entry.id_spec, static_cast<unsigned>( spec ) );
      if ( _spec_it != entry.id_spec.end() )
      {
        return true;
      }
    }

    return false;
  } );

  if ( _it != _data.end() )
  {
    return _it;
  }

  return &( nil() );
}

const trait_data_t* trait_data_t::find_tokenized(
    talent_tree       tree,
    util::string_view name,
    unsigned          class_id,
    specialization_e  spec,
    bool              ptr )
{
  auto _data = data( class_id, tree, ptr );
  auto _it = range::find_if( _data, [name, spec]( const trait_data_t& entry ) {
    std::string tokenized_name = entry.name;
    util::tokenize( tokenized_name );
    if ( util::str_compare_ci( name, tokenized_name ) )
    {
      if ( entry.id_spec[ 0 ] == 0 )
      {
        return true;
      }

      auto _spec_it = range::find( entry.id_spec, static_cast<unsigned>( spec ) );
      if ( _spec_it != entry.id_spec.end() )
      {
        return true;
      }
    }

    return false;
  } );

  if ( _it != _data.end() )
  {
    return _it;
  }

  return &( nil() );
}

const trait_data_t* trait_data_t::find_by_spell(
    talent_tree      tree,
    unsigned         spell_id,
    unsigned         class_id,
    specialization_e spec,
    bool             ptr )
{
  auto _data = data( class_id, tree, ptr );
  auto _it = range::find_if( _data, [spell_id, spec]( const trait_data_t& entry ) {
    if ( entry.id_spell == spell_id )
    {
      if ( entry.id_spec[ 0 ] == 0 )
      {
        return true;
      }

      auto _spec_it = range::find( entry.id_spec, static_cast<unsigned>( spec ) );
      if ( _spec_it != entry.id_spec.end() )
      {
        return true;
      }
    }

    return false;
  } );

  if ( _it != _data.end() )
  {
    return _it;
  }

  return &( nil() );
}

util::span<const trait_definition_effect_entry_t> trait_definition_effect_entry_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __trait_definition_effect_data, __ptr_trait_definition_effect_data, ptr );
}

util::span<const trait_definition_effect_entry_t> trait_definition_effect_entry_t::find( unsigned id, bool ptr )
{
  auto _data = data( ptr );

  auto it = range::equal_range( _data, id, {},
      &trait_definition_effect_entry_t::id_trait_definition );

  return { it.first, it.second };
}
