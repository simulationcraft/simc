// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Talent
// ==========================================================================

// talent_t::talent_t =======================================================

talent_t::talent_t( player_t* p, const char* name, const int32_t specify_tree ) : data( NULL ), rank( 0 )
{
  assert( p && name && p -> sim );
  uint32_t id = find_talent_id( p, name, specify_tree );
  if ( p -> sim -> debug ) log_t::output( p -> sim, "Initializing talent %s", name );

  if ( id != 0 )
  {
    data = p->player_data.m_talents_index[ id ];

    p -> talent_list2.push_back( const_cast<talent_t *>( this ) );

    if ( p -> sim -> debug ) log_t::output( p -> sim, "Talent %s initialized", name );
  }
}

// talent_t::find_talent_id ===================================================

uint32_t talent_t::find_talent_id( const player_t* p, const char* name, const int32_t specify_tree )
{
  uint32_t tab, talent_num, talent_id;

  assert( p && name && name[0] && ( ( specify_tree < 0 ) || ( specify_tree < MAX_TALENT_TABS ) ) );

  uint32_t min_tab = specify_tree >= 0 ? specify_tree : 0;
  uint32_t max_tab = specify_tree >= 0 ? specify_tree + 1 : MAX_TALENT_TABS;

  for ( tab = min_tab; tab < max_tab; tab++ )
  {
    talent_num = 0;
    while ( ( talent_id = p->player_data.talent_player_get_id_by_num( p->type, tab, talent_num ) ) != 0 )
    {
      if ( !_stricmp( p->player_data.talent_name_str( talent_id ), name ) )
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

spell_ids_t::spell_ids_t( const player_t* p, const char* name ) : data( NULL ), enabled( false )
{
  assert( p && name && p -> sim );
  
  if ( p -> sim -> debug ) log_t::output( p -> sim, "Initializing spell %s", name );

  data = find_spell_ids( p, name );
  if ( p -> sim -> debug ) log_t::output( p -> sim, "Spell %s%s initialized", name, data ? "":" NOT" );
}

spell_ids_t::~spell_ids_t()
{
  if ( data ) delete [] data;
}

// spell_ids_t::find_spell_ids ===================================================

spell_data_t** spell_ids_t::find_spell_ids( const player_t* p, const char* name )
{
  uint32_t spell_id;
  spell_data_t** spell_ids = NULL;
  uint32_t num_matches = 0;
  uint32_t match_num = 0;

  assert( p && name && name[0] );

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


