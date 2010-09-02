// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Talent
// ==========================================================================

// talent_t::talent_t =======================================================

talent_t::talent_t( player_t* player, const char* name, const int32_t specify_tree ) : data( NULL ), rank( 0 ), p( player )
{
  assert( p && name && p -> sim );
  uint32_t id = find_talent_id( name, specify_tree );
  if ( p -> sim -> debug ) log_t::output( p -> sim, "Initializing talent %s", name );

  if ( id != 0 )
  {
    data = p->player_data.m_talents_index[ id ];

    p -> talent_list2.push_back( const_cast<talent_t *>( this ) );

    if ( p -> sim -> debug ) log_t::output( p -> sim, "Talent %s initialized", name );
  }
}

// talent_t::get_effect_id ===================================================

uint32_t talent_t::get_effect_id( const uint32_t effect_num ) SC_CONST
{
  assert( ( rank <= 3 ) && ( effect_num >= 1 ) && ( effect_num <= 3 ) );
  
  if ( !rank || !data )
    return 0;

  uint32_t spell_id = p -> player_data.talent_rank_spell_id( data -> id, rank );

  if ( !spell_id )
    return 0;

  return p -> player_data.spell_effect_id( spell_id, effect_num );
}

// talent_t::get_spell_id ===================================================

uint32_t talent_t::get_spell_id( ) SC_CONST
{
  assert( p -> sim && ( rank <= 3 ) );
  
  if ( !rank || !data )
    return 0;

  return p -> player_data.talent_rank_spell_id( data -> id, rank );
}


// talent_t::find_talent_id ===================================================

uint32_t talent_t::find_talent_id( const char* name, const int32_t specify_tree )
{
  uint32_t tab, talent_num, talent_id;

  assert( name && name[0] && ( ( specify_tree < 0 ) || ( specify_tree < MAX_TALENT_TABS ) ) );

  uint32_t min_tab = specify_tree >= 0 ? specify_tree : 0;
  uint32_t max_tab = specify_tree >= 0 ? specify_tree + 1 : MAX_TALENT_TABS;

  for ( tab = min_tab; tab < max_tab; tab++ )
  {
    talent_num = 0;
    while ( ( talent_id = p->player_data.talent_player_get_id_by_num( p->type, tab, talent_num ) ) != 0 )
    {
      if ( !_stricmp( p -> player_data.talent_name_str( talent_id ), name ) )
      {
        return talent_id;
      }
      talent_num++;
    }
  }
  return 0; 
}

// ==========================================================================
// Spell and Talent Specialization
// ==========================================================================

// spell_ids_t::spell_ids_t =======================================================

spell_ids_t::spell_ids_t( player_t* player, const char* name ) : data( NULL ), enabled( false ), p( player )
{
  assert( name && p -> sim );
  
  if ( p -> sim -> debug ) log_t::output( p -> sim, "Initializing spell %s", name );

  data = find_spell_ids( name );
  if ( p -> sim -> debug ) log_t::output( p -> sim, "Spell %s%s initialized", name, data ? "":" NOT" );
}

spell_ids_t::~spell_ids_t()
{
  if ( data ) delete [] data;
}

// spell_ids_t::get_effect_id ===================================================

uint32_t spell_ids_t::get_effect_id( const uint32_t effect_num, const uint32_t spell_num ) SC_CONST
{
  assert( ( effect_num >= 1 ) && ( effect_num <= 3 ) );
  
  if ( !enabled || !data || !spell_num )
    return 0;

  for ( uint32_t i = 0; i < spell_num; i++ )
  {
    if ( !data[ i ] )
      return 0;
  }

  return p -> player_data.spell_effect_id( data[ spell_num - 1 ] -> id, effect_num );
}

// spell_ids_t::find_spell_ids ===================================================

spell_data_t** spell_ids_t::find_spell_ids( const char* name )
{
  uint32_t spell_id;
  spell_data_t** spell_ids = NULL;
  uint32_t num_matches = 0;
  uint32_t match_num = 0;

  assert( name && name[0] );

  for ( spell_id = 0; spell_id < p -> player_data.m_spells_index_size; spell_id++ )
  {
    if ( p -> player_data.spell_exists( spell_id ) && !_stricmp( p -> player_data.spell_name_str( spell_id ), name ) )
    {
      num_matches++;
    }
  }

  if ( !num_matches )
    return NULL;

  spell_ids = new spell_data_t*[ num_matches + 1 ];

  memset( spell_ids, 0, ( num_matches + 1 ) * sizeof( spell_data_t * ) );

  for ( spell_id = 0; spell_id < p -> player_data.m_spells_index_size; spell_id++ )
  {
    if ( p -> player_data.spell_exists( spell_id ) && !_stricmp( p -> player_data.spell_name_str( spell_id ), name ) )
    {
      spell_ids[ match_num ] = p -> player_data.m_spells_index[ spell_id ];
      match_num++;
    }
  }
  spell_ids[ match_num ] = NULL;

  return spell_ids; 
}


