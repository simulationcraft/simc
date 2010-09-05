// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

static talent_tab_name tree_to_tab_name( const player_type c, const talent_tree_type tree )
{
  switch ( tree )
  {
  case TREE_BLOOD: return DEATH_KNIGHT_BLOOD;
  case TREE_FROST: return c == MAGE ? MAGE_FROST : DEATH_KNIGHT_FROST;
  case TREE_UNHOLY: return DEATH_KNIGHT_UNHOLY;
  case TREE_BALANCE: return DRUID_BALANCE;
  case TREE_FERAL: return DRUID_FERAL;
  case TREE_RESTORATION: return c == DRUID ? DRUID_RESTORATION : SHAMAN_RESTORATION;
  case TREE_BEAST_MASTERY: return HUNTER_BEAST_MASTERY;
  case TREE_MARKSMANSHIP: return HUNTER_MARKSMANSHIP;
  case TREE_SURVIVAL: return HUNTER_SURVIVAL;
  case TREE_ARCANE: return MAGE_ARCANE;
  case TREE_FIRE: return MAGE_FIRE;
  case TREE_HOLY: return c == PALADIN ? PALADIN_HOLY : PRIEST_HOLY;
  case TREE_PROTECTION: return c == PALADIN? PALADIN_PROTECTION : WARRIOR_PROTECTION;
  case TREE_RETRIBUTION: return PALADIN_RETRIBUTION;
  case TREE_DISCIPLINE: return PRIEST_DISCIPLINE;
  case TREE_SHADOW: return PRIEST_SHADOW;
  case TREE_ASSASSINATION: return ROGUE_ASSASSINATION;
  case TREE_COMBAT: return ROGUE_COMBAT;
  case TREE_SUBTLETY: return ROGUE_SUBTLETY;
  case TREE_ELEMENTAL: return SHAMAN_ELEMENTAL;
  case TREE_ENHANCEMENT: return SHAMAN_ENHANCEMENT;
  case TREE_AFFLICTION: return WARLOCK_AFFLICTION;
  case TREE_DEMONOLOGY: return WARLOCK_DEMONOLOGY;
  case TREE_DESTRUCTION: return WARLOCK_DESTRUCTION;
  case TREE_ARMS: return WARRIOR_ARMS;
  case TREE_FURY: return WARRIOR_FURY;
  default: return WARRIOR_ARMS;
  }
}

// ==========================================================================
// Talent
// ==========================================================================

// talent_t::talent_t =======================================================

talent_t::talent_t( player_t* player, const char* t_name, const char* name, const int32_t specify_tree ) : 
  data( NULL ), rank( 0 ), p( player ), enabled( true ), forced_override( false ), forced_value( false ),
  token_name( t_name )
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

  init_enabled( false, false );
}

// talent_t::get_effect_id ===================================================

uint32_t talent_t::get_effect_id( const uint32_t effect_num ) SC_CONST
{
  assert( ( rank <= 3 ) && ( effect_num >= 1 ) && ( effect_num <= 3 ) );
  
  if ( !enabled || !rank || !data )
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
  
  if ( !enabled || !rank || !data )
    return 0;

  return p -> player_data.talent_rank_spell_id( data -> id, rank );
}

bool talent_t::set_rank( uint32_t value )
{
  if ( !data )
    return false;

  if ( value > p -> player_data.talent_max_rank( data->id ) )
  {
    return false;
  }

  rank = value;

  return true;
}

bool talent_t::init_enabled( bool override_enabled, bool override_value )
{
  assert( p && p -> sim );

  forced_override = override_enabled;
  forced_value = override_value;

  if ( forced_override )
  {
    enabled = forced_value;
  }
  else
  {
    enabled = ( p -> player_data.talent_is_enabled( data -> id ) );
  }
  return true;
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

spell_id_t::spell_id_t( player_t* player, const char* t_name, const char* name, const uint32_t id ) : 
    spell_id( id ), data( NULL ), enabled( false ), p( player ), forced_override( false ), forced_value( false ),
    token_name( t_name )
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

  init_enabled( false, false );

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

bool spell_id_t::init_enabled( bool override_enabled, bool override_value )
{
  assert( p && p -> sim );

  forced_override = override_enabled;
  forced_value = override_value;

  if ( forced_override )
  {
    enabled = forced_value;
  }
  else
  {
    enabled = ( p -> player_data.spell_is_enabled( spell_id ) );
  }
  return true;
}


// ==========================================================================
// Class Spell ID
// ==========================================================================

// class_spell_id_t::class_spell_id_t =======================================================

class_spell_id_t::class_spell_id_t( player_t* player, const char* t_name, const char* name ) : 
    spell_id_t( player, t_name, name )
{
  if ( !spell_id )
  {
    spell_id = p -> player_data.find_class_spell( p -> type, name );
  }

  init( p, name, spell_id );
}

bool class_spell_id_t::init_enabled( bool override_enabled, bool override_value )
{
  spell_id_t::init_enabled( override_enabled, override_value );

  if ( !forced_override )
  {
    enabled = enabled & p -> player_data.spell_is_level( spell_id, p->level );
  }
  return true;
}

// ==========================================================================
// Talent Specialization Spell ID
// ==========================================================================

// talent_spec_spell_id_t::talent_spec_spell_id_t =======================================================

talent_spec_spell_id_t::talent_spec_spell_id_t( player_t* player, const char* t_name, const char* name, const talent_tab_name tree_name ) :
    spell_id_t( player, t_name, name ), tab( tree_name )
{
  assert( name && ( ( const uint32_t ) tree_name < 3 ) );

  data = NULL;

  spell_id = p -> player_data.find_talent_spec_spell( p -> type, tree_name, name );

  init( p, name, spell_id );
}

bool talent_spec_spell_id_t::init_enabled( bool override_enabled, bool override_value )
{
  spell_id_t::init_enabled( override_enabled, override_value );

  if ( !forced_override )
  {
    enabled = enabled & ( tab == tree_to_tab_name( p -> type, p -> primary_tree() ) );
  }
  return true;
}

// ==========================================================================
// Racial Spell ID class
// ==========================================================================

racial_spell_id_t::racial_spell_id_t( player_t* player, const char* t_name, const char* name ) :
    spell_id_t( player, t_name, name )
{
  assert( name );

  data = NULL;

  spell_id = p -> player_data.find_racial_spell( p -> type, p -> race, name );

  init( p, name, spell_id );
}

bool racial_spell_id_t::init_enabled( bool override_enabled, bool override_value )
{
  spell_id_t::init_enabled( override_enabled, override_value );

  if ( !forced_override )
  {
    enabled = enabled & ( p -> player_data.spell_is_race( spell_id, p -> race ) );
  }
  return true;
}


// ==========================================================================
// Mastery Spell ID class
// ==========================================================================

mastery_spell_id_t::mastery_spell_id_t( player_t* player, const char* t_name, const char* name, const talent_tab_name tree_name ) :
    spell_id_t( player, t_name, name ), tab( tree_name )
{
  assert( name );

  data = NULL;

  spell_id = p -> player_data.find_mastery_spell( p -> type, name );
  init( p, name, spell_id );
}

bool mastery_spell_id_t::init_enabled( bool override_enabled, bool override_value )
{
  spell_id_t::init_enabled( override_enabled, override_value );

  if ( !forced_override )
  {
    enabled = enabled & ( p -> level >= 75 ) && ( tab == tree_to_tab_name( p -> type, p -> primary_tree() ) );
  }
  return true;
}

