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
// Spell ID
// ==========================================================================

// spell_id_t::spell_id_t =======================================================

spell_id_t::spell_id_t( player_t* player, const char* name, const uint32_t id ) : 
    spell_id( id ), data( NULL ), enabled( false ), p( player )
{
  init( player, name, id );
}

bool spell_id_t::init( player_t* player, const char* name, const uint32_t id )
{
  assert( player && name && player -> sim );

  spell_id = id;
  data = NULL;
  enabled = false;
  p = player;

  if ( p -> sim -> debug ) log_t::output( p -> sim, "Initializing spell %s", name );

  if ( spell_id && p -> player_data.spell_exists( spell_id ) )
  {
    data = p -> player_data.m_spells_index[ spell_id ];
    if ( p -> sim -> debug ) log_t::output( p -> sim, "Spell %s initialized", name );
  }
  else
  {
    spell_id = 0;
    if ( p -> sim -> debug ) log_t::output( p -> sim, "Spell %s NOT initialized", name );
    return false;
  }

  return true;
}

// spell_id_t::get_effect_id ===================================================

uint32_t spell_id_t::get_effect_id( const uint32_t effect_num ) SC_CONST
{
  assert( ( effect_num >= 1 ) && ( effect_num <= 3 ) );
  
  if ( !enabled || !data )
    return 0;

  return p -> player_data.spell_effect_id( spell_id, effect_num );
}


// ==========================================================================
// Class Spell ID
// ==========================================================================

// class_spell_id_t::class_spell_id_t =======================================================

class_spell_id_t::class_spell_id_t( player_t* player, const char* name ) : 
    spell_id_t( player, name )
{
  if ( !spell_id )
  {
    spell_id = p -> player_data.find_class_spell( p -> type, name );
  }

  init( p, name, spell_id );
}

// ==========================================================================
// Talent Specialization Spell ID
// ==========================================================================

// talent_spec_spell_id_t::talent_spec_spell_id_t =======================================================

talent_spec_spell_id_t::talent_spec_spell_id_t( player_t* player, const char* name, const talent_tab_name tree_name ) :
    spell_id_t( player, name )
{
  assert( name && ( ( const uint32_t ) tree_name < 3 ) );

  data = NULL;

  spell_id = p -> player_data.find_talent_spec_spell( p -> type, tree_name, name );

  init( p, name, spell_id );
}

// ==========================================================================
// Racial Spell ID class
// ==========================================================================

racial_spell_id_t::racial_spell_id_t( player_t* player, const char* name ) :
    spell_id_t( player, name )
{
  assert( name );

  data = NULL;

  spell_id = p -> player_data.find_racial_spell( p -> type, p -> race, name );

  init( p, name, spell_id );
}

// ==========================================================================
// Mastery Spell ID class
// ==========================================================================

mastery_spell_id_t::mastery_spell_id_t( player_t* player, const char* name ) :
    spell_id_t( player, name )
{
  assert( name );

  data = NULL;

  spell_id = p -> player_data.find_mastery_spell( p -> type, name );
  init( p, name, spell_id );
}

