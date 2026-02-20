// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include <array>

#include "config.hpp"

#include "dbc/dbc.hpp"
#include "util/util.hpp"

#include "trait_data.hpp"

#include "generated/trait_data.inc"
#if SC_USE_PTR == 1
#include "generated/trait_data_ptr.inc"
#endif

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

util::span<const trait_data_t> trait_data_t::data( unsigned node_id, unsigned class_id, talent_tree tree, bool ptr )
{
  auto _class_span = data( class_id, tree, ptr );
  auto _node_range = range::equal_range( _class_span, node_id, {}, &trait_data_t::id_node );

  return { _node_range.first, _node_range.second };
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

const trait_data_t* trait_data_t::find( talent_tree tree, std::string_view name, unsigned class_id,
                                        specialization_e spec, bool ptr, bool tokenize, unsigned index, unsigned sub_tree_id )
{
  std::vector<const trait_data_t*> _traits;

  auto _data = data( class_id, tree, ptr );

  for ( const auto& entry : _data )
  {
    if ( sub_tree_id != 0 && entry.id_sub_tree != 0 && entry.id_sub_tree != sub_tree_id )
    {
      continue;
    }

    if ( util::str_compare_ci( name, tokenize ? util::tokenize_fn( entry.name ) : entry.name ) )
    {
      // hero talents seem to ignore id_spec requirement
      if ( tree == talent_tree::HERO || entry.id_spec[ 0 ] == 0 ||
           range::contains( entry.id_spec, static_cast<unsigned>( spec ) ) )
      {
        _traits.push_back( &entry );
      }
    }
  }

  if ( _traits.size() == 1 )
  {
    return _traits.front();
  }
  else if ( !_traits.empty() )
  {
    // There are situations where you can have multiple traits with the same name:
    //  1) different specs have different entry_id for the same node with the same name
    //  2) tiered nodes (apex) with multiple traits of the same name
    //  3) 'dirty' talent code where a choice node is split up and a duplicate entry is left behind

    // If index is passed, return the n-1'th matching node as the generated arrays are already sorted by selection index.
    // Note there is no check to see if traits are actually apex traits or on the same node - this is done on purpose to
    // allow usage of index argument for other name clash resolution uses
    if ( index && index <= _traits.size() )
    {
      return _traits[ index - 1 ];
    }
    else
    {
      // if one and only one trait has no spec requirement, assume it's the correct one
      if ( auto it = range::partition( _traits, []( const trait_data_t* t ) {
        return range::all_of( t->id_spec, []( unsigned s ) { return s == 0; } );
      } ); it == _traits.begin() + 1 )
      {
        return _traits.front();
      }

      // if one and only one trait matches the spec, assume it's the correct one
      if ( auto it = range::partition( _traits, [ spec ]( const trait_data_t* t ) {
        return range::contains( t->id_spec, spec );
      } ); it == _traits.begin() + 1 )
      {
        return _traits.front();
      }

      for ( auto trait : _traits )
      {
        // find all entries on the same node
        auto _entries = trait_data_t::data( trait->id_node, class_id, tree, ptr );

        // if this is the only entry on the node, assume it's the correct one
        if ( _entries.size() == 1 )
          return trait;

        // if this is the first entry on the node, assume it's the correct one
        if ( trait->id_trait_node_entry == _entries.front().id_trait_node_entry )
          return trait;
      }
    }
  }

  return &( nil() );
}

std::vector<const trait_data_t*> trait_data_t::find_by_spell( talent_tree tree, unsigned spell_id, unsigned class_id,
                                                              specialization_e spec, bool ptr )
{
  const auto _data = data( ptr );
  const auto _index = SC_DBC_GET_DATA( __trait_spell_id_index, __ptr_trait_spell_id_index, ptr );
  auto span = range::equal_range( _index, spell_id, {},
      [ _data ]( uint16_t index ) { return _data[ index ].id_spell; } );

  if ( span.first == _index.end() )
  {
    return {};
  }

  std::vector<const trait_data_t*> generic_entries, spec_entries;

  for ( auto i = span.first; i < span.second; ++i )
  {
    const auto& entry = _data[ *i ];
    if ( tree != talent_tree::INVALID &&
         entry.tree_index != static_cast<unsigned>( tree ) )
    {
      continue;
    }

    if ( class_id != 0 && entry.id_class != class_id )
    {
      continue;
    }

    // If no spec filter, store everything as "generic entry"
    if ( entry.id_spec[ 0U ] == 0U || spec == SPEC_NONE )
    {
      generic_entries.push_back( &( entry ) );
    }

    if ( spec != SPEC_NONE )
    {
      auto it = range::find( entry.id_spec, static_cast<unsigned>( spec ) );
      if ( it != entry.id_spec.end() )
      {
        spec_entries.push_back( &( entry ) );
      }
    }
  }

  if ( spec != SPEC_NONE && !spec_entries.empty() )
  {
    return spec_entries;
  }
  else
  {
    return generic_entries;
  }
}

const trait_data_t* trait_data_t::find_by_trait_definition( unsigned trait_definition_id, bool ptr )
{
  auto _data = data( ptr );
  auto _it = range::find( _data, trait_definition_id, &trait_data_t::id_trait_definition );

  if ( _it != _data.end() )
  {
    return _it;
  }

  return &( nil() );
}

const std::string_view trait_data_t::get_hero_tree_name( unsigned id_sub_tree, bool ptr )
{
  auto _data = SC_DBC_GET_DATA( __trait_sub_tree_data, __ptr_trait_sub_tree_data, ptr );
  auto _it = range::find_if( _data, [ id_sub_tree ]( const std::tuple<unsigned, std::string, unsigned>& entry ){
    return id_sub_tree == std::get<0>( entry );
  });

  if ( _it != _data.end() )
  {
    return std::get<1>(*_it);
  }

  return {};
}

unsigned trait_data_t::get_hero_tree_id( std::string_view name, bool ptr )
{
  auto _data = SC_DBC_GET_DATA( __trait_sub_tree_data, __ptr_trait_sub_tree_data, ptr );
  auto _it = range::find_if( _data, [ name ]( const std::tuple<unsigned, std::string, unsigned>& entry ) {
    return util::str_compare_ci( name, util::tokenize_fn( std::get<1>( entry ) ) );
  } );

  if ( _it != _data.end() )
  {
    return std::get<0>(*_it);
  }

  return 0;
}

bool trait_data_t::is_hero_trait_available( const trait_data_t* trait, player_e type, specialization_e spec, bool ptr )
{
  if ( static_cast<talent_tree>( trait->tree_index ) != talent_tree::HERO || !trait->id_sub_tree )
  {
    return false;
  }

  // as hero traits can be missing proper id_spec, check the id_spec of the sub tree's selection talent
  for ( const auto& entry : data( util::class_id( type ), talent_tree::SELECTION, ptr ) )
  {
    if ( entry.id_sub_tree == trait->id_sub_tree && range::contains( entry.id_spec, static_cast<unsigned>( spec ) ) )
    {
      return true;
    }
  }

  return false;
}

// TODO: perhaps this should be locally cached post processing for 'missing' id_spec_starter. currently only called
// during player initialization & html report generation, so not a runtime issue.
bool trait_data_t::is_granted( const trait_data_t* trait, player_e type, specialization_e spec, bool ptr )
{
  // check if the trait is the initial starting node on the spec/hero tree (1,1)
  // we can parse this from DBC via traitcond for the nodegroup but seems unnecessary for now
  if ( static_cast<talent_tree>( trait->tree_index ) == talent_tree::HERO && trait->col == 1 && trait->row == 1 )
  {
    return is_hero_trait_available( trait, type, spec, ptr );
  }
  // check if trait is a free class trait for the spec
  else if ( trait->id_spec_starter[ 0 ] && range::contains( trait->id_spec_starter, spec ) )
  {
    return true;
  }

  return false;
}

std::vector<unsigned> trait_data_t::get_valid_hero_tree_ids( specialization_e spec, bool ptr )
{
  auto class_id = util::class_id( dbc::get_class_from_spec( spec ) );
  auto _data = data( class_id, talent_tree::SELECTION, ptr );

  std::vector<unsigned> id_list;

  for ( const auto& entry : _data )
  {
    for ( const auto& spec_entry : entry.id_spec )
    {
      if ( spec_entry == static_cast<unsigned>( spec ) )
      {
        id_list.push_back( entry.id_sub_tree );
      }
    }
  }

  auto it = range::unique( id_list );
  id_list.erase( it, id_list.end() );

  return id_list;
}

bool trait_data_t::is_hero_tree_valid( hero_tree_e hero, specialization_e spec, bool ptr )
{
  return range::contains( get_valid_hero_tree_ids( spec, ptr ), static_cast<unsigned>( hero ) );
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
