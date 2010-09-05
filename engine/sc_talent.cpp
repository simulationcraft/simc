// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

#if 0
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
#endif

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

    p -> player_data.talent_set_used( id, true );

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

talent_t* talent_t::find_talent_in_list( const char* t_name )
{
  if ( !p )
    return NULL;

  uint32_t i = 0;
  while ( i < p -> talent_list2.size() )
  {
    talent_t *t = p -> talent_list2[ 0 ];

    if ( t && !_stricmp( t -> token_name.c_str(), t_name ) )
      return t;

    i++;
  }
  return NULL;
}

talent_t* talent_t::find_talent_in_list( const uint32_t id )
{
  if ( !p )
    return NULL;

  uint32_t i = 0;
  while ( i < p -> talent_list2.size() )
  {
    talent_t *t = p -> talent_list2[ 0 ];

    if ( t && t -> data && t -> data -> id == id )
      return t;

    i++;
  }
  return NULL;
}

void talent_t::add_options( player_t* p, std::vector<option_t>& opt_vector )
{
  if ( !p )
    return;

  option_t opt[ 2 ];
  opt[ 1 ].name = NULL;
  opt[ 1 ].type = OPT_UNKNOWN;
  opt[ 1 ].address = NULL;
  uint32_t i = 0;
  while ( i < p -> talent_list2.size() )
  {
    talent_t *t = p -> talent_list2[ i ];

    opt[ 0 ].name = t -> token_name.c_str();
    opt[ 0 ].type = OPT_TALENT_RANK;
    opt[ 0 ].address = t;

    option_t::copy( opt_vector, opt );

    i++;
  }
}

bool talent_t::ok()
{
  if ( ! p )
    return false;

  return ( ( rank > 0 ) && ( enabled ) );
}

uint32_t talent_t::max_rank() SC_CONST
{
  if ( !p || !data || !data->id )
  {
    return 0;
  }

  return p -> player_data.talent_max_rank( data -> id );
}

uint32_t talent_t::rank_spell_id( const uint32_t r ) SC_CONST
{
  if ( !p || !data || !data->id )
  {
    return 0;
  }

  return p -> player_data.talent_rank_spell_id( data -> id, r );
}


// ==========================================================================
// Spell ID
// ==========================================================================

// spell_id_t::spell_id_t =======================================================

spell_id_t::spell_id_t( player_t* player, const char* t_name, const uint32_t id ) :
    spell_id( id ), data( NULL ), enabled( false ), p( player ), forced_override( false ), forced_value ( false ),
    token_name( t_name ), is_mastery( false ), req_tree( false ), tab( TALENT_TAB_NONE ), req_talent( NULL )
{
  init( );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
    spell_id( id ), data( NULL ), enabled( false ), p( player ), forced_override( false ), forced_value ( false ),
    token_name( t_name ), is_mastery( false ), req_tree( false ), tab( TALENT_TAB_NONE ), req_talent( talent )
{
  init( );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const uint32_t id, const talent_tab_name tree, bool mastery ) :
    spell_id( id ), data( NULL ), enabled( false ), p( player ), forced_override( false ), forced_value ( false ),
    token_name( t_name ), is_mastery( mastery ), req_tree( true ), tab( tree ), req_talent( NULL )
{
  init( );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const char* s_name ) :
    spell_id( 0 ), data( NULL ), enabled( false ), p( player ), forced_override( false ), forced_value ( false ),
    token_name( t_name ), is_mastery( false ), req_tree( false ), tab( TALENT_TAB_NONE ), req_talent( NULL )
{
  init( s_name );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
    spell_id( 0 ), data( NULL ), enabled( false ), p( player ), forced_override( false ), forced_value ( false ),
    token_name( t_name ), is_mastery( false ), req_tree( false ), tab( TALENT_TAB_NONE ), req_talent( talent )
{
  init( s_name );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const char* s_name, const talent_tab_name tree, bool mastery ) :
    spell_id( 0 ), data( NULL ), enabled( false ), p( player ), forced_override( false ), forced_value ( false ),
    token_name( t_name ), is_mastery( mastery ), req_tree( true ), tab( tree ), req_talent( NULL )
{
  init( s_name );
}

bool spell_id_t::init(  const char* s_name )
{
  assert( p && p -> sim );

  if ( p -> sim -> debug ) log_t::output( p -> sim, "Initializing spell %s", token_name.c_str() );

  if ( !spell_id )
  {
    if ( !s_name || !*s_name )
      return false;

    if ( is_mastery )
    {
      spell_id = p -> player_data.find_mastery_spell( p ->type, s_name );
    }
    else if ( req_tree )
    {
      spell_id = p -> player_data.find_talent_spec_spell( p -> type, tab, s_name );
    }
    else
    {
      spell_id = p -> player_data.find_class_spell( p -> type, s_name );
      if ( !spell_id )
      {
        spell_id = p -> player_data.find_racial_spell( p -> type, p -> race, s_name );
      }
    }
  }

  if ( spell_id && p -> player_data.spell_exists( spell_id ) )
  {
    data = p -> player_data.m_spells_index[ spell_id ];
    if ( p -> sim -> debug ) log_t::output( p -> sim, "Spell %s initialized", token_name.c_str() );

    p -> player_data.spell_set_used( spell_id, true );

    push_back();
  }
  else
  {
    spell_id = 0;
    if ( p -> sim -> debug ) log_t::output( p -> sim, "Spell %s NOT initialized", token_name.c_str() );
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
    enabled = p -> player_data.spell_is_enabled( spell_id ) & ( p -> player_data.spell_is_level( spell_id, p -> level ) );
    
    if ( is_mastery )
    {
      if ( p -> level < 75 )
        enabled = false;
    }
    else if ( !p -> player_data.spell_is_level( spell_id, p -> level ) )
    {
      enabled = false;
    }
  }
  return true;
}

spell_id_t* spell_id_t::find_spell_in_list( std::vector<spell_id_t *> *spell_list, const char* t_name )
{
  if ( !spell_list )
    return NULL;

  uint32_t i = 0;
  while ( i < spell_list->size() )
  {
    spell_id_t *p = (*spell_list)[ 0 ];

    if ( p && !_stricmp( p->token_name.c_str(), t_name ) )
      return p;

    i++;
  }
  return NULL;
}

spell_id_t* spell_id_t::find_spell_in_list( std::vector<spell_id_t *> *spell_list, const uint32_t id )
{
  if ( !spell_list )
    return NULL;

  uint32_t i = 0;
  while ( i < spell_list->size() )
  {
    spell_id_t *p = (*spell_list)[ 0 ];

    if ( p && p -> spell_id == id )
      return p;

    i++;
  }
  return NULL;
}

bool spell_id_t::ok()
{
  bool res = enabled;

  if ( ! p )
    return false;

  if ( req_talent )
    res = res & req_talent -> ok();

  return res;
}

void spell_id_t::add_options( player_t* p, std::vector<spell_id_t *> *spell_list, std::vector<option_t>& opt_vector )
{
  if ( !p || !spell_list )
    return;

  option_t opt[ 2 ];
  opt[ 1 ].name = NULL;
  opt[ 1 ].type = OPT_UNKNOWN;
  opt[ 1 ].address = NULL;
  uint32_t i = 0;

  while ( i < spell_list->size() )
  {
    spell_id_t *t = (*spell_list)[ i ];

    opt[ 0 ].name = t -> token_name.c_str();
    opt[ 0 ].type = OPT_SPELL_ENABLED;
    opt[ 0 ].address = t;

    option_t::copy( opt_vector, opt );

    i++;
  }
}

const char* spell_id_t::real_name() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return NULL;
  }

  return p -> player_data.spell_name_str( spell_id );
}

const std::string spell_id_t::token() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return NULL;
  }

  return token_name;
}

double spell_id_t::missile_speed() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_missile_speed( spell_id );
}

uint32_t spell_id_t::school_mask() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  return p -> player_data.spell_school_mask( spell_id );
}

resource_type spell_id_t::power_type() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return RESOURCE_NONE;
  }

  return p -> player_data.spell_power_type( spell_id );
}

double spell_id_t::min_range() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_min_range( spell_id );
}

double spell_id_t::max_range() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_max_range( spell_id );
}

bool spell_id_t::in_range() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return false;
  }

  return p -> player_data.spell_in_range( spell_id, p -> distance );
}

double spell_id_t::cooldown() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_cooldown( spell_id );
}

double spell_id_t::gcd() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_gcd( spell_id );
}

uint32_t spell_id_t::category() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  return p -> player_data.spell_category( spell_id );
}

double spell_id_t::duraton() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_duration( spell_id );
}

double spell_id_t::cost() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_cost( spell_id );
}

uint32_t spell_id_t::rune_cost() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  return p -> player_data.spell_rune_cost( spell_id );
}

double spell_id_t::runic_power_gain() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_runic_power_gain( spell_id );
}

uint32_t spell_id_t::max_stacks() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  return p -> player_data.spell_max_stacks( spell_id );
}

uint32_t spell_id_t::initial_stacks() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  return p -> player_data.spell_initial_stacks( spell_id );
}

double spell_id_t::proc_chance() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_proc_chance( spell_id );
}

double spell_id_t::cast_time() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  return p -> player_data.spell_cast_time( spell_id, p -> level );
}

uint32_t spell_id_t::effect_id( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  return p -> player_data.spell_effect_id( spell_id, effect_num );
}

bool spell_id_t::flags( const spell_attribute_t f ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return false;
  }

  return p -> player_data.spell_flags( spell_id, f );
}

const char* spell_id_t::desc() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return NULL;
  }

  return p -> player_data.spell_desc( spell_id );
}

const char* spell_id_t::tooltip() SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return NULL;
  }

  return p -> player_data.spell_tooltip( spell_id );
}

uint32_t spell_id_t::effect_type( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_type( effect_id );
}

uint32_t spell_id_t::effect_subtype( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_subtype( effect_id );
}

int32_t spell_id_t::effect_base_value( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_base_value( effect_id );
}

int32_t spell_id_t::effect_misc_value1( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_misc_value1( effect_id );
}

int32_t spell_id_t::effect_misc_value2( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_misc_value2( effect_id );
}

uint32_t spell_id_t::effect_trigger_spell( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_trigger_spell_id( effect_id );
}

double spell_id_t::effect_average( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_average( effect_id, p -> type, p -> level );
}

double spell_id_t::effect_delta( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_delta( effect_id, p -> type, p -> level );
}

double spell_id_t::effect_unk( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_unk( effect_id, p -> type, p -> level );
}

double spell_id_t::effect_min( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_min( effect_id, p -> type, p -> level );
}

double spell_id_t::effect_max( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_max( effect_id, p -> type, p -> level );
}

double spell_id_t::effect_coeff( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_coeff( effect_id );
}

double spell_id_t::effect_period( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_period( effect_id );
}

double spell_id_t::effect_radius( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_radius( effect_id );
}

double spell_id_t::effect_radius_max( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_radius_max( effect_id );
}

double spell_id_t::effect_pp_combo_points( const uint32_t effect_num ) SC_CONST
{
  if ( !p || !data || !spell_id )
  {
    return 0.0;
  }

  uint32_t effect_id = p -> player_data.spell_effect_id( spell_id, effect_num );

  return p -> player_data.effect_pp_combo_points( effect_id );
}

// ==========================================================================
// Active Spell ID
// ==========================================================================


active_spell_t::active_spell_t( player_t* player, const char* t_name, const uint32_t id ) :
  spell_id_t( player, t_name, id )
{

}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
  spell_id_t( player, t_name, id, talent )
{

}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const uint32_t id, const talent_tab_name tree, bool mastery ) :
  spell_id_t( player, t_name, id, tree, mastery )
{

}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const char* s_name ) :
  spell_id_t( player, t_name, s_name )
{

}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
  spell_id_t( player, t_name, s_name, talent )
{

}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const char* s_name, const talent_tab_name tree, bool mastery ) :
  spell_id_t( player, t_name, s_name, tree, mastery )
{

}

active_spell_t* active_spell_t::find_spell_in_list( const char* t_name )
{
  if ( !p )
    return NULL;

  return ( active_spell_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( p -> active_spell_list ), t_name );
}

active_spell_t* active_spell_t::find_spell_in_list( const uint32_t id )
{
  if ( !p )
    return NULL;

  return ( active_spell_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( p -> active_spell_list ), id );
}

void active_spell_t::add_options( player_t* player, std::vector<option_t>& opt_vector )
{
  if ( !player )
    return;

  spell_id_t::add_options( player, ( std::vector<spell_id_t *> * ) &( player -> active_spell_list ), opt_vector );
}

void active_spell_t::push_back()
{
  p -> active_spell_list.push_back( this );
}

// ==========================================================================
// Passive Spell ID
// ==========================================================================


passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const uint32_t id ) :
  spell_id_t( player, t_name, id )
{

}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
  spell_id_t( player, t_name, id, talent )
{

}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const uint32_t id, const talent_tab_name tree, bool mastery ) :
  spell_id_t( player, t_name, id, tree, mastery )
{

}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const char* s_name ) :
  spell_id_t( player, t_name, s_name )
{

}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
  spell_id_t( player, t_name, s_name, talent )
{

}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const char* s_name, const talent_tab_name tree, bool mastery ) :
  spell_id_t( player, t_name, s_name, tree, mastery )
{

}

passive_spell_t* passive_spell_t::find_spell_in_list( const char* t_name )
{
  if ( !p )
    return NULL;

  return ( passive_spell_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( p -> passive_spell_list ), t_name );
}

passive_spell_t* passive_spell_t::find_spell_in_list( const uint32_t id )
{
  if ( !p )
    return NULL;

  return ( passive_spell_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( p -> passive_spell_list ), id );
}

void passive_spell_t::add_options( player_t* player, std::vector<option_t>& opt_vector )
{
  if ( !player )
    return;

  spell_id_t::add_options( player, ( std::vector<spell_id_t *> * ) &( player -> passive_spell_list ), opt_vector );
}

void passive_spell_t::push_back()
{
  p -> passive_spell_list.push_back( this );
}

