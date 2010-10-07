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

talent_t::talent_t( player_t* player, const char* t_name, const char* name ) : 
  spell_id_t( player, t_name, name, true ), talent_t_data( NULL ), talent_t_rank( 0 ),
  talent_t_enabled( false ), talent_t_forced_override( false ), talent_t_forced_value( false )
{
  assert( pp && name && pp -> sim );

  uint32_t id = find_talent_id( name );
  if ( pp -> sim -> debug ) log_t::output( pp -> sim, "Initializing talent %s", name );

  if ( id != 0 )
  {
    talent_t_data = pp->player_data.m_talents_index[ id ];

    pp -> talent_list2.push_back( const_cast<talent_t *>( this ) );

    pp -> player_data.talent_set_used( id, true );

    if ( pp -> sim -> debug ) log_t::output( pp -> sim, "Talent %s initialized", name );
  }

  talent_init_enabled( false, false );
}

talent_t::talent_t( const talent_t& copy ) :
  spell_id_t( copy ), talent_t_data( copy.talent_t_data ), talent_t_rank( copy.talent_t_rank ),
  talent_t_enabled( copy.talent_t_enabled ), talent_t_forced_override( copy.talent_t_forced_override ), 
  talent_t_forced_value( copy.talent_t_forced_value )
{
// Not sure if I should push back or not yet.
/*
  pp -> talent_list2.push_back( const_cast<talent_t *>( this ) );
*/
}

// talent_t::get_effect_id ===================================================

uint32_t talent_t::get_effect_id( const uint32_t effect_num ) SC_CONST
{
  assert( ( talent_t_rank <= 3 ) && ( effect_num >= 1 ) && ( effect_num <= 3 ) );
  
  if ( !talent_t_enabled || !talent_t_rank || !talent_t_data )
    return 0;

  if ( !spell_id_t_id )
  {
    return 0;
  }

  return pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );
}

bool talent_t::ok() SC_CONST
{
  if ( !pp || !talent_t_data )
    return false;

  return ( ( talent_t_rank > 0 ) && spell_id_t::ok() && ( talent_t_enabled ) );
}

// talent_t::get_spell_id ===================================================

uint32_t talent_t::get_spell_id( ) SC_CONST
{
  assert( pp -> sim && ( talent_t_rank <= 3 ) );
  
  if ( !ok() )
    return 0;

  return pp -> player_data.talent_rank_spell_id( talent_t_data -> id, talent_t_rank );
}

bool talent_t::set_rank( uint32_t value )
{
  if ( !talent_t_data )
    return false;

  if ( value > pp -> player_data.talent_max_rank( talent_t_data -> id ) )
  {
    return false;
  }

  talent_t_rank = value;

  int_init( rank_spell_id( talent_t_rank ) );

  return true;
}

bool talent_t::talent_init_enabled( bool override_enabled, bool override_value )
{
  assert( pp && pp -> sim );

  talent_t_forced_override = override_enabled;
  talent_t_forced_value = override_value;

  if ( talent_t_forced_override )
  {
    talent_t_enabled = talent_t_forced_value;
  }
  else
  {
    if ( !talent_t_data )
    {
      pp -> sim -> errorf( "Error: Unable to init talent \"%s\".\n", token_name.c_str() );
      assert( 0 );
    }
    talent_t_enabled = pp -> player_data.talent_is_enabled( talent_t_data -> id );
  }
  return true;
}

// talent_t::find_talent_id ===================================================

uint32_t talent_t::find_talent_id( const char* name )
{
  uint32_t i_tab, talent_num, talent_id;

  assert( pp && name && name[0] );

  for ( i_tab = 0; i_tab < MAX_TALENT_TABS; i_tab++ )
  {
    talent_num = 0;
    while ( ( talent_id = pp->player_data.talent_player_get_id_by_num( pp -> type, i_tab, talent_num ) ) != 0 )
    {
      if ( !_stricmp( pp -> player_data.talent_name_str( talent_id ), name ) )
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
  if ( !pp )
    return NULL;

  return ( talent_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( pp -> talent_list2 ), t_name );
}

talent_t* talent_t::find_talent_in_list( const uint32_t id )
{
  if ( !pp )
    return NULL;

  return ( talent_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( pp -> talent_list2 ), id );
}

void talent_t::add_options( player_t* player, std::vector<option_t>& opt_vector )
{
  if ( !player )
    return;

  spell_id_t::add_options( player, ( std::vector<spell_id_t *> * ) &( player -> talent_list2 ), opt_vector, OPT_TALENT_RANK );
}


uint32_t talent_t::max_rank() SC_CONST
{
  if ( !pp || !talent_t_data || !talent_t_data->id )
  {
    return 0;
  }

  return pp -> player_data.talent_max_rank( talent_t_data -> id );
}

uint32_t talent_t::rank_spell_id( const uint32_t r ) SC_CONST
{
  if ( !pp || !talent_t_data || !talent_t_data->id )
  {
    return 0;
  }

  return pp -> player_data.talent_rank_spell_id( talent_t_data -> id, r );
}

uint32_t talent_t::rank() SC_CONST
{
  if ( !ok() )
    return 0;

  return talent_t_rank;
}

// ==========================================================================
// Spell ID
// ==========================================================================

// spell_id_t::spell_id_t =======================================================

spell_id_t::spell_id_t( player_t* player, const char* t_name ) :
    spell_id_t_id( 0 ), spell_id_t_data( NULL ), spell_id_t_enabled( false ), pp( player ),    
    spell_list_type( PLAYER_NONE ), scaling_type( PLAYER_NONE ), spell_id_t_forced_override( false ), spell_id_t_forced_value ( false ),
    spell_id_t_is_mastery( false ), spell_id_t_req_tree( false ), 
    spell_id_t_tab( TALENT_TAB_NONE ), spell_id_t_req_talent( NULL ), spell_id_t_m_is_talent( false ), spell_id_t_tree( -1 )
{
  if ( !t_name )
    token_name = "";
  else
    token_name = t_name;
  // Dummy constructor for old-style
  memset(effects, 0, sizeof(effects));
  single = 0;
}

spell_id_t::spell_id_t( player_t* player, const bool run_init, const char* t_name ) :
    spell_id_t_id( 0 ), spell_id_t_data( NULL ), spell_id_t_enabled( false ), pp( player ),    
    spell_list_type( pp -> type ), scaling_type( pp -> type ), spell_id_t_forced_override( false ), spell_id_t_forced_value ( false ),
    token_name( t_name ), spell_id_t_is_mastery( false ), spell_id_t_req_tree( false ), 
    spell_id_t_tab( TALENT_TAB_NONE ), spell_id_t_req_talent( NULL ), spell_id_t_m_is_talent( false ), spell_id_t_tree( -1 )
{
  memset(effects, 0, sizeof(effects));
  single = 0;
  if ( run_init )
    int_init( t_name );
}


spell_id_t::spell_id_t( player_t* player, const char* t_name, const uint32_t id, const player_type ptype, const player_type stype ) :
    spell_id_t_id( id ), spell_id_t_data( NULL ), spell_id_t_enabled( false ), pp( player ),    
    spell_list_type( ptype ), scaling_type( stype ), spell_id_t_forced_override( false ), spell_id_t_forced_value ( false ),
    token_name( t_name ), spell_id_t_is_mastery( false ), spell_id_t_req_tree( false ), 
    spell_id_t_tab( TALENT_TAB_NONE ), spell_id_t_req_talent( NULL ), spell_id_t_m_is_talent( false ), spell_id_t_tree( -1 )
{
  memset(effects, 0, sizeof(effects));
  single = 0;
  if ( spell_list_type == PLAYER_NONE )
    spell_list_type     = pp -> type;
  if ( scaling_type    == PLAYER_NONE )
    scaling_type        = pp -> type;

  int_init( );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
    spell_id_t_id( id ), spell_id_t_data( NULL ), spell_id_t_enabled( false ), pp( player ),
    spell_list_type( pp -> type ), scaling_type( pp -> type ), spell_id_t_forced_override( false ), spell_id_t_forced_value ( false ),
    token_name( t_name ), spell_id_t_is_mastery( false ), spell_id_t_req_tree( false ),
    spell_id_t_tab( TALENT_TAB_NONE ), spell_id_t_req_talent( talent ),
    spell_id_t_m_is_talent( false ), spell_id_t_tree( -1 )
{
  memset(effects, 0, sizeof(effects));
  single = 0;
  int_init( );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const uint32_t id, const talent_tab_name tree, bool mastery ) :
    spell_id_t_id( id ), spell_id_t_data( NULL ), spell_id_t_enabled( false ), pp( player ),
    spell_list_type( pp -> type ), scaling_type( pp -> type ), spell_id_t_forced_override( false ), spell_id_t_forced_value ( false ),
    token_name( t_name ), spell_id_t_is_mastery( mastery ), spell_id_t_req_tree( true ), 
    spell_id_t_tab( tree ), spell_id_t_req_talent( NULL ),
    spell_id_t_m_is_talent( false ), spell_id_t_tree( -1 )
{
  memset(effects, 0, sizeof(effects));
  single = 0;
  int_init( );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const char* s_name, const bool is_talent, const player_type ptype, const player_type stype ) :
    spell_id_t_id( 0 ), spell_id_t_data( NULL ), spell_id_t_enabled( false ), pp( player ),
    spell_list_type( ptype ), scaling_type( stype ), spell_id_t_forced_override( false ), spell_id_t_forced_value ( false ),
    token_name( t_name ), spell_id_t_is_mastery( false ), spell_id_t_req_tree( false ), 
    spell_id_t_tab( TALENT_TAB_NONE ), spell_id_t_req_talent( NULL ),
    spell_id_t_m_is_talent( is_talent ), spell_id_t_tree( -1 )
{
  memset(effects, 0, sizeof(effects));
  single = 0;
  if ( spell_list_type == PLAYER_NONE )
    spell_list_type     = pp -> type;
  if ( scaling_type    == PLAYER_NONE )
    scaling_type        = pp -> type;

  int_init( s_name );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
    spell_id_t_id( 0 ), spell_id_t_data( NULL ), spell_id_t_enabled( false ), pp( player ), 
    spell_list_type( pp -> type ), scaling_type( pp -> type ), spell_id_t_forced_override( false ), spell_id_t_forced_value ( false ),
    token_name( t_name ), spell_id_t_is_mastery( false ), spell_id_t_req_tree( false ),
    spell_id_t_tab( TALENT_TAB_NONE ), spell_id_t_req_talent( talent ),
    spell_id_t_m_is_talent( false ), spell_id_t_tree( -1 )
{
  memset(effects, 0, sizeof(effects));
  single = 0;
  int_init( s_name );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const char* s_name, const talent_tab_name tree, bool mastery ) :
    spell_id_t_id( 0 ), spell_id_t_data( NULL ), spell_id_t_enabled( false ), pp( player ), 
    spell_list_type( pp -> type ), scaling_type( pp -> type ), spell_id_t_forced_override( false ), spell_id_t_forced_value ( false ),
    token_name( t_name ), spell_id_t_is_mastery( mastery ), spell_id_t_req_tree( true ), 
    spell_id_t_tab( tree ), spell_id_t_req_talent( NULL ),
    spell_id_t_m_is_talent( false ), spell_id_t_tree( -1 )
{
  memset(effects, 0, sizeof(effects));
  single = 0;
  int_init( s_name );
}

spell_id_t::spell_id_t( const spell_id_t& copy, const player_type ptype, const player_type stype ) :
    spell_id_t_id( copy.spell_id_t_id ), spell_id_t_data( copy.spell_id_t_data ), spell_id_t_enabled( copy.spell_id_t_enabled ),
    pp( copy.pp ), spell_list_type( copy.spell_list_type ), scaling_type( copy.scaling_type ), 
    spell_id_t_forced_override( copy.spell_id_t_forced_override ), spell_id_t_forced_value( copy.spell_id_t_forced_value ),
    token_name( copy.token_name ), spell_id_t_is_mastery( copy.spell_id_t_is_mastery ), spell_id_t_req_tree( copy.spell_id_t_req_tree ),
    spell_id_t_tab( copy.spell_id_t_tab ), spell_id_t_req_talent( copy.spell_id_t_req_talent ), spell_id_t_m_is_talent( copy.spell_id_t_m_is_talent ),
    spell_id_t_tree( copy.spell_id_t_tree )
{
  memcpy(effects,copy.effects,sizeof(effects));
  single = copy.single;
  if ( ptype != PLAYER_NONE )
    spell_list_type = ptype;
  if ( stype != PLAYER_NONE )
    scaling_type = stype;
}

bool spell_id_t::int_init( const uint32_t id, int t )
{
  assert( pp && pp -> sim );

  if ( pp -> sim -> debug ) log_t::output( pp -> sim, "Initializing spell %s", token_name.c_str() );

  spell_id_t_id = id;
  spell_id_t_data = NULL;

  if ( t < -1 )
    t = -1;
  if ( t > 3 )
    t = 3;

  spell_id_t_tree = t;

  if ( spell_id_t_id && pp -> player_data.spell_exists( spell_id_t_id ) )
  {
    spell_id_t_data = pp -> player_data.m_spells_index[ spell_id_t_id ];
    if ( pp -> sim -> debug ) log_t::output( pp -> sim, "Spell %s initialized", token_name.c_str() );

    pp -> player_data.spell_set_used( spell_id_t_id, true );
  }
  else if ( !spell_id_t_m_is_talent )
  {
    spell_id_t_id = 0;
    if ( pp -> sim -> debug ) log_t::output( pp -> sim, "Spell %s NOT initialized", token_name.c_str() );
    return false;
  }

  init_enabled( spell_id_t_forced_override, spell_id_t_forced_value );

  uint32_t n_effects = 0;
  
  // Map effects, figure out if this is a single-effect spell
  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ( effects[ i ] = pp -> player_data.m_effects_index[ get_effect_id( i + 1 ) ] ) )
      n_effects++;
  }
  
  if ( n_effects == 1 )
  {
    for ( int i = 0; i < MAX_EFFECTS; i++ )
    {
      if ( ! effects[ i ] )
        continue;
        
      single = effects[ i ];
      break;
    }
  }
  else
    single = 0;

  return true;  
}

bool spell_id_t::int_init( const char* s_name )
{
  assert( pp && pp -> sim );

  if ( pp -> sim -> debug ) log_t::output( pp -> sim, "Initializing spell %s", token_name.c_str() );

  if ( !spell_id_t_id && !spell_id_t_m_is_talent )
  {
    if ( !s_name || !*s_name )
      return false;

    if ( spell_id_t_is_mastery )
    {
      spell_id_t_id = pp -> player_data.find_mastery_spell( spell_list_type, s_name );
    }
    else if ( spell_id_t_req_tree )
    {
      spell_id_t_id = pp -> player_data.find_talent_spec_spell( spell_list_type, spell_id_t_tab, s_name );
    }
    else
    {
      spell_id_t_id = pp -> player_data.find_class_spell( spell_list_type, s_name );
      if ( spell_id_t_id )
      {
        spell_id_t_tree = pp -> player_data.find_class_spell_tree( spell_list_type, s_name );
      }
      if ( !spell_id_t_id )
      {
        spell_id_t_id = pp -> player_data.find_racial_spell( spell_list_type, pp -> race, s_name );
      }
      if ( !spell_id_t_id )
      {
        spell_id_t_id = pp -> player_data.find_glyph_spell( spell_list_type, s_name );
      }
      if ( !spell_id_t_id )
      {
        spell_id_t_id = pp -> player_data.find_set_bonus_spell( spell_list_type, s_name );
      }
    }
  }

  if ( spell_id_t_id && pp -> player_data.spell_exists( spell_id_t_id ) )
  {
    spell_id_t_data = pp -> player_data.m_spells_index[ spell_id_t_id ];
    if ( pp -> sim -> debug ) log_t::output( pp -> sim, "Spell %s initialized", token_name.c_str() );

    pp -> player_data.spell_set_used( spell_id_t_id, true );

    push_back();
  }
  else if ( !spell_id_t_m_is_talent )
  {
    spell_id_t_id = 0;
    if ( pp -> sim -> debug ) log_t::output( pp -> sim, "Spell %s NOT initialized", token_name.c_str() );
    return false;
  }

  init_enabled( spell_id_t_forced_override, spell_id_t_forced_value );

  uint32_t n_effects = 0;
  
  // Map effects, figure out if this is a single-effect spell
  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ( effects[ i ] = pp -> player_data.m_effects_index[ get_effect_id( i + 1 ) ] ) )
      n_effects++;
  }
  
  if ( n_effects == 1 )
  {
    for ( int i = 0; i < MAX_EFFECTS; i++ )
    {
      if ( ! effects[ i ] )
        continue;
        
      single = effects[ i ];
      break;
    }
  }
  else
    single = 0;

  return true;  
}


// spell_id_t::get_effect_id ===================================================

uint32_t spell_id_t::get_effect_id( const uint32_t effect_num ) SC_CONST
{
  assert( ( effect_num >= 1 ) && ( effect_num <= 3 ) );
  
  if ( !ok() )
    return 0;

  return pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );
}

bool spell_id_t::init_enabled( bool override_enabled, bool override_value )
{
  assert( pp && pp -> sim );

  spell_id_t_forced_override = override_enabled;
  spell_id_t_forced_value = override_value;

  if ( spell_id_t_forced_override )
  {
    spell_id_t_enabled = spell_id_t_forced_value;
  }
  else if ( spell_id_t_m_is_talent )
  {
    spell_id_t_enabled = pp -> player_data.spell_is_enabled( spell_id_t_id );
  }
  else
  {
    spell_id_t_enabled = pp -> player_data.spell_is_enabled( spell_id_t_id ) & 
                         ( pp -> player_data.spell_is_level( spell_id_t_id, pp -> level ) );
    
    if ( spell_id_t_is_mastery )
    {
      if ( pp -> level < 75 )
        spell_id_t_enabled = false;
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

    if ( p && p -> spell_id_t_id == id )
      return p;

    i++;
  }
  return NULL;
}

bool spell_id_t::ok() SC_CONST
{
  bool res = spell_id_t_enabled;

  if ( ! pp || !spell_id_t_data || !spell_id_t_id )
    return false;

  if ( spell_id_t_req_talent )
    res = res & spell_id_t_req_talent -> ok();

  if ( spell_id_t_req_tree )
  {
    res = res & ( spell_id_t_tab == pp -> pri_tree );
  }

  return res;
}

void spell_id_t::add_options( player_t* p, std::vector<spell_id_t *> *spell_list, std::vector<option_t>& opt_vector, int opt_type )
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
    opt[ 0 ].type = opt_type;
    opt[ 0 ].address = t;

    option_t::copy( opt_vector, opt );

    i++;
  }
}

const char* spell_id_t::real_name() SC_CONST
{
  if ( !pp || !spell_id_t_data || !spell_id_t_id )
  {
    return NULL;
  }

  return pp -> player_data.spell_name_str( spell_id_t_id );
}

const std::string spell_id_t::token() SC_CONST
{
  if ( !pp || !spell_id_t_data || !spell_id_t_id )
  {
    return NULL;
  }

  return token_name;
}

double spell_id_t::missile_speed() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  return pp -> player_data.spell_missile_speed( spell_id_t_id );
}

uint32_t spell_id_t::school_mask() SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  return pp -> player_data.spell_school_mask( spell_id_t_id );
}

uint32_t spell_id_t::get_school_mask( const school_type s )
{
  switch ( s )
  {
  case SCHOOL_PHYSICAL      : return 0x01;
  case SCHOOL_HOLY          : return 0x02;
  case SCHOOL_FIRE          : return 0x04;
  case SCHOOL_NATURE        : return 0x08;
  case SCHOOL_FROST         : return 0x10;
  case SCHOOL_SHADOW        : return 0x20;
  case SCHOOL_ARCANE        : return 0x40;
  case SCHOOL_HOLYSTRIKE    : return 0x03;
  case SCHOOL_FLAMESTRIKE   : return 0x05;
  case SCHOOL_HOLYFIRE      : return 0x06;
  case SCHOOL_STORMSTRIKE   : return 0x09;
  case SCHOOL_HOLYSTORM     : return 0x0a;
  case SCHOOL_FIRESTORM     : return 0x0c;
  case SCHOOL_FROSTSTRIKE   : return 0x11;
  case SCHOOL_HOLYFROST     : return 0x12;
  case SCHOOL_FROSTFIRE     : return 0x14;
  case SCHOOL_FROSTSTORM    : return 0x18;
  case SCHOOL_SHADOWSTRIKE  : return 0x21;
  case SCHOOL_SHADOWLIGHT   : return 0x22;
  case SCHOOL_SHADOWFLAME   : return 0x24;
  case SCHOOL_SHADOWSTORM   : return 0x28;
  case SCHOOL_SHADOWFROST   : return 0x30;
  case SCHOOL_SPELLSTRIKE   : return 0x41;
  case SCHOOL_DIVINE        : return 0x42;
  case SCHOOL_SPELLFIRE     : return 0x44;
  case SCHOOL_SPELLSTORM    : return 0x48;
  case SCHOOL_SPELLFROST    : return 0x50;
  case SCHOOL_SPELLSHADOW   : return 0x60;
  case SCHOOL_ELEMENTAL     : return 0x1c;
  case SCHOOL_CHROMATIC     : return 0x7c;
  case SCHOOL_MAGIC         : return 0x7e;
  case SCHOOL_CHAOS         : return 0x7f;
  default:
    return SCHOOL_NONE;
  }
  return 0x00;
}

bool spell_id_t::is_school( const school_type s, const school_type s2 )
{
  return ( get_school_mask( s ) & get_school_mask( s2 ) ) != 0;
}

school_type spell_id_t::get_school_type( const uint32_t mask )
{
  switch ( mask )
  {
  case 0x01: return SCHOOL_PHYSICAL;
  case 0x02: return SCHOOL_HOLY;
  case 0x04: return SCHOOL_FIRE;
  case 0x08: return SCHOOL_NATURE;
  case 0x10: return SCHOOL_FROST;
  case 0x20: return SCHOOL_SHADOW;
  case 0x40: return SCHOOL_ARCANE;
  case 0x03: return SCHOOL_HOLYSTRIKE;
  case 0x05: return SCHOOL_FLAMESTRIKE;
  case 0x06: return SCHOOL_HOLYFIRE;
  case 0x09: return SCHOOL_STORMSTRIKE;
  case 0x0a: return SCHOOL_HOLYSTORM;
  case 0x0c: return SCHOOL_FIRESTORM;
  case 0x11: return SCHOOL_FROSTSTRIKE;
  case 0x12: return SCHOOL_HOLYFROST;
  case 0x14: return SCHOOL_FROSTFIRE;
  case 0x18: return SCHOOL_FROSTSTORM;
  case 0x21: return SCHOOL_SHADOWSTRIKE;
  case 0x22: return SCHOOL_SHADOWLIGHT;
  case 0x24: return SCHOOL_SHADOWFLAME;
  case 0x28: return SCHOOL_SHADOWSTORM;
  case 0x30: return SCHOOL_SHADOWFROST;
  case 0x41: return SCHOOL_SPELLSTRIKE;
  case 0x42: return SCHOOL_DIVINE;
  case 0x44: return SCHOOL_SPELLFIRE;
  case 0x48: return SCHOOL_SPELLSTORM;
  case 0x50: return SCHOOL_SPELLFROST;
  case 0x60: return SCHOOL_SPELLSHADOW;
  case 0x1c: return SCHOOL_ELEMENTAL;
  case 0x7c: return SCHOOL_CHROMATIC;
  case 0x7e: return SCHOOL_MAGIC;
  case 0x7f: return SCHOOL_CHAOS;
  default:
    return SCHOOL_NONE;
  }
}

school_type spell_id_t::get_school_type() SC_CONST
{
  if ( !ok() )
  {
    return SCHOOL_NONE;
  }

  return get_school_type( school_mask() );
}

resource_type spell_id_t::power_type() SC_CONST
{
  if ( !ok() )
  {
    return RESOURCE_NONE;
  }

  return pp -> player_data.spell_power_type( spell_id_t_id );
}

double spell_id_t::min_range() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  return pp -> player_data.spell_min_range( spell_id_t_id );
}

double spell_id_t::max_range() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  return pp -> player_data.spell_max_range( spell_id_t_id );
}

bool spell_id_t::in_range() SC_CONST
{
  if ( !ok() )
  {
    return false;
  }

  return pp -> player_data.spell_in_range( spell_id_t_id, pp -> distance );
}

double spell_id_t::cooldown() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  double d = pp -> player_data.spell_cooldown( spell_id_t_id );

  if ( d > ( pp -> sim -> wheel_seconds - 2.0 ) )
    d = pp -> sim -> wheel_seconds - 2.0;

  return d;
}

double spell_id_t::gcd() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  return pp -> player_data.spell_gcd( spell_id_t_id );
}

uint32_t spell_id_t::category() SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  return pp -> player_data.spell_category( spell_id_t_id );
}

double spell_id_t::duration() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }
  
  double d = pp -> player_data.spell_duration( spell_id_t_id );

  if ( d > ( pp -> sim -> wheel_seconds - 2.0 ) )
    d = pp -> sim -> wheel_seconds - 2.0;

  return d;
}

double spell_id_t::cost() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  return pp -> player_data.spell_cost( spell_id_t_id );
}

uint32_t spell_id_t::rune_cost() SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  return pp -> player_data.spell_rune_cost( spell_id_t_id );
}

double spell_id_t::runic_power_gain() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  return pp -> player_data.spell_runic_power_gain( spell_id_t_id );
}

uint32_t spell_id_t::max_stacks() SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  return pp -> player_data.spell_max_stacks( spell_id_t_id );
}

uint32_t spell_id_t::initial_stacks() SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  return pp -> player_data.spell_initial_stacks( spell_id_t_id );
}

double spell_id_t::proc_chance() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  return pp -> player_data.spell_proc_chance( spell_id_t_id );
}

double spell_id_t::cast_time() SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  return pp -> player_data.spell_cast_time( spell_id_t_id, pp -> level );
}

uint32_t spell_id_t::effect_id( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  return pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );
}

bool spell_id_t::flags( const spell_attribute_t f ) SC_CONST
{
  if ( !ok() )
  {
    return false;
  }

  return pp -> player_data.spell_flags( spell_id_t_id, f );
}

const char* spell_id_t::desc() SC_CONST
{
  if ( !ok() )
  {
    return NULL;
  }

  return pp -> player_data.spell_desc( spell_id_t_id );
}

const char* spell_id_t::tooltip() SC_CONST
{
  if ( !ok() )
  {
    return NULL;
  }

  return pp -> player_data.spell_tooltip( spell_id_t_id );
}

int32_t spell_id_t::effect_type( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_type( effect_id );
}

int32_t spell_id_t::effect_subtype( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_subtype( effect_id );
}

int32_t spell_id_t::effect_base_value( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_base_value( effect_id );
}

int32_t spell_id_t::effect_misc_value1( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_misc_value1( effect_id );
}

int32_t spell_id_t::effect_misc_value2( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_misc_value2( effect_id );
}

uint32_t spell_id_t::effect_trigger_spell( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_trigger_spell_id( effect_id );
}

double spell_id_t::effect_average( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_average( effect_id, scaling_type, pp -> level );
}

double spell_id_t::effect_delta( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_delta( effect_id, scaling_type, pp -> level );
}

double spell_id_t::effect_unk( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_unk( effect_id, scaling_type, pp -> level );
}

double spell_id_t::effect_min( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_min( effect_id, scaling_type, pp -> level );
}

double spell_id_t::effect_max( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_max( effect_id, scaling_type, pp -> level );
}

double spell_id_t::effect_coeff( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_coeff( effect_id );
}

double spell_id_t::effect_period( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_period( effect_id );
}

double spell_id_t::effect_radius( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_radius( effect_id );
}

double spell_id_t::effect_radius_max( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_radius_max( effect_id );
}

double spell_id_t::effect_pp_combo_points( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_pp_combo_points( effect_id );
}

double spell_id_t::effect_real_ppl( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0.0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_real_ppl( effect_id );
}

int spell_id_t::effect_die_sides( const uint32_t effect_num ) SC_CONST
{
  if ( !ok() )
  {
    return 0;
  }

  uint32_t effect_id = pp -> player_data.spell_effect_id( spell_id_t_id, effect_num );

  return pp -> player_data.effect_die_sides( effect_id );
}

double spell_id_t::base_value( effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) SC_CONST
{
  if ( single )
    return sc_data_access_t::fmt_value( single -> base_value, (effect_type_t) single -> type, (effect_subtype_t) single -> subtype );

  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ! effects[ i ] )
      continue;

    if ( ( type == E_MAX || effects[ i ] -> type == type ) && 
         ( sub_type == A_MAX || effects[ i ] -> subtype == sub_type ) && 
         ( misc_value == DEFAULT_MISC_VALUE || effects[ i ] -> misc_value == misc_value ) &&
         ( misc_value2 == DEFAULT_MISC_VALUE || effects[ i ] -> misc_value_2 == misc_value2 ) )
      return sc_data_access_t::fmt_value( effects[ i ] -> base_value, type, sub_type );
  }
  
  return 0.0;
}

// ==========================================================================
// Active Spell ID
// ==========================================================================

active_spell_t::active_spell_t( player_t* player, const char* t_name ) :
  spell_id_t( player, t_name )
{
  
}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const uint32_t id, const player_type ptype, const player_type stype ) :
  spell_id_t( player, t_name, id, ptype, stype )
{

}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
  spell_id_t( player, t_name, id, talent )
{
  push_back();
}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const uint32_t id, const talent_tab_name tree, bool mastery ) :
  spell_id_t( player, t_name, id, tree, mastery )
{
  push_back();
}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const char* s_name, const player_type ptype, const player_type stype ) :
  spell_id_t( player, t_name, s_name, false, ptype, stype )
{

}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
  spell_id_t( player, t_name, s_name, talent )
{
  push_back();
}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const char* s_name, const talent_tab_name tree, bool mastery ) :
  spell_id_t( player, t_name, s_name, tree, mastery )
{
  push_back();
}

active_spell_t::active_spell_t( const active_spell_t& copy, const player_type ptype, const player_type stype ) :
  spell_id_t( copy, ptype, stype )
{
  
}

active_spell_t* active_spell_t::find_spell_in_list( const char* t_name )
{
  if ( !pp )
    return NULL;

  return ( active_spell_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( pp -> active_spell_list ), t_name );
}

active_spell_t* active_spell_t::find_spell_in_list( const uint32_t id )
{
  if ( !pp )
    return NULL;

  return ( active_spell_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( pp -> active_spell_list ), id );
}

void active_spell_t::add_options( player_t* player, std::vector<option_t>& opt_vector )
{
  if ( !player )
    return;

  spell_id_t::add_options( player, ( std::vector<spell_id_t *> * ) &( player -> active_spell_list ), opt_vector );
}

void active_spell_t::push_back()
{
  if ( pp && spell_id_t_id )
    pp -> active_spell_list.push_back( this );
}

// ==========================================================================
// Passive Spell ID
// ==========================================================================

passive_spell_t::passive_spell_t( player_t* player, const char* t_name ) :
  spell_id_t( player, t_name )
{
  
}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const uint32_t id, const player_type ptype, const player_type stype ) :
  spell_id_t( player, t_name, id, ptype, stype )
{
  push_back();
}

passive_spell_t::passive_spell_t( player_t* player, const bool run_init, const char* t_name ) :
  spell_id_t( player, run_init, t_name )
{
  push_back();
}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
  spell_id_t( player, t_name, id, talent )
{
  push_back();
}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const uint32_t id, const talent_tab_name tree, bool mastery ) :
  spell_id_t( player, t_name, id, tree, mastery )
{
  push_back();
}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const char* s_name, const player_type ptype, const player_type stype ) :
  spell_id_t( player, t_name, s_name, false, ptype, stype )
{
  push_back();
}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
  spell_id_t( player, t_name, s_name, talent )
{
  push_back();
}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const char* s_name, const talent_tab_name tree, bool mastery ) :
  spell_id_t( player, t_name, s_name, tree, mastery )
{
  push_back();
}

passive_spell_t::passive_spell_t( const passive_spell_t& copy, const player_type ptype, const player_type stype ) :
  spell_id_t( copy, ptype, stype )
{

}

passive_spell_t* passive_spell_t::find_spell_in_list( const char* t_name )
{
  if ( !pp )
    return NULL;

  return ( passive_spell_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( pp -> passive_spell_list ), t_name );
}

passive_spell_t* passive_spell_t::find_spell_in_list( const uint32_t id )
{
  if ( !pp )
    return NULL;

  return ( passive_spell_t* ) spell_id_t::find_spell_in_list( ( std::vector<spell_id_t *> * ) &( pp -> passive_spell_list ), id );
}

void passive_spell_t::add_options( player_t* player, std::vector<option_t>& opt_vector )
{
  if ( !player )
    return;

  spell_id_t::add_options( player, ( std::vector<spell_id_t *> * ) &( player -> passive_spell_list ), opt_vector );
}

void passive_spell_t::push_back()
{
  if ( pp && spell_id_t_id )
    pp -> passive_spell_list.push_back( this );
}

