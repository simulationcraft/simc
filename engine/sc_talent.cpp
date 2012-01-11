// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Talent
// ==========================================================================

// talent_t::talent_t =======================================================

talent_t::talent_t( player_t* player, talent_data_t* _td ) :
  spell_id_t( player, _td -> name_cstr() ), t_data( 0 ), t_rank( 0 ), t_overridden( false ),
  t_default_rank( 0 ),
  // Future trimmed down access
  td( _td ), sd( spell_data_t::nil() ), trigger( 0 )
{
  spell_id_t* default_rank = new spell_id_t;
  t_default_rank = default_rank;
  default_rank -> s_enabled = false;
  range::fill( t_rank_spells, default_rank );

  t_data = player -> dbc.talent( td -> id() );
  t_enabled = t_data -> is_enabled();
  const_cast< talent_data_t* >( t_data ) -> set_used( true );
}

talent_t::~talent_t()
{
  for ( size_t i = 0; i < sizeof_array( t_rank_spells ); i++ )
  {
    if ( t_rank_spells[ i ] != this && t_rank_spells[ i ] != t_default_rank )
      delete t_rank_spells[ i ];
  }

  delete t_default_rank;
}

bool talent_t::ok() const
{
  if ( ! s_player || ! t_data )
    return false;

  return ( ( t_rank > 0 ) && spell_id_t::ok() && ( t_enabled ) );
}

std::string talent_t::to_str() const
{
  std::ostringstream s;

  s << spell_id_t::to_str();
  s << " talent_enabled=" << ( t_enabled ? "true" : "false" );
  if ( t_overridden ) s << " (forced)";
  s << " talent_rank=" << t_rank;
  s << " max_rank=" << max_rank();

  return s.str();
}

// talent_t::get_spell_id ===================================================

uint32_t talent_t::spell_id( ) const
{
  assert( s_player -> sim && ( t_rank <= 3 ) );

  if ( ! ok() )
    return 0;

  return t_data -> rank_spell_id( t_rank );
}

bool talent_t::set_rank( uint32_t r, bool overridden )
{
  // Future trimmed down access
  sd = ( ( r >= 3 ) ? td -> spell3 :
         ( r == 2 ) ? td -> spell2 :
         ( r == 1 ) ? td -> spell1 : spell_data_t::nil() );
  trigger = 0;
  if ( ! trigger && sd -> _effect1 -> trigger_spell_id() ) trigger = sd -> _effect1 -> _trigger_spell;
  if ( ! trigger && sd -> _effect2 -> trigger_spell_id() ) trigger = sd -> _effect2 -> _trigger_spell;
  if ( ! trigger && sd -> _effect3 -> trigger_spell_id() ) trigger = sd -> _effect3 -> _trigger_spell;
  // rank = r;

  if ( ! t_data || ! t_enabled )
  {
    if ( s_player -> sim -> debug )
      log_t::output( s_player -> sim, "Talent status: %s", to_str().c_str() );
    return false;
  }

  if ( r > t_data -> max_rank() )
  {
    if ( s_player -> sim -> debug )
      log_t::output( s_player -> sim, "Talent status: %s", to_str().c_str() );
    return false;
  }

  // We cannot allow non-overridden set_rank to take effect, if
  // we have already overridden the talent rank once
  if ( ! t_overridden || overridden )
  {
    t_overridden = overridden;
    t_rank       = r;
    s_id         = rank_spell_id( t_rank );

    if ( t_enabled && t_rank > 0 && ! initialize() )
    {
      if ( s_player -> sim -> debug )
        log_t::output( s_player -> sim, "Talent status: %s", to_str().c_str() );
      return false;
    }
  }

  if ( s_player -> sim -> debug )
    log_t::output( s_player -> sim, "Talent status: %s", to_str().c_str() );

  if ( t_rank > 0 )
  {
    // Setup all of the talent spells
    for ( int i = 0; i < MAX_RANK; i++ )
    {
      if ( t_rank == ( unsigned ) i + 1 )
        t_rank_spells[ i ] = this;
      else if ( t_data -> _rank_id[ i ] && s_player -> dbc.spell( t_data -> _rank_id[ i ] ) )
      {
        char rankbuf[128];
        snprintf( rankbuf, sizeof( rankbuf ), "%s_rank_%d", s_token.c_str(), i + 1 );
        spell_id_t* talent_rank_spell = new spell_id_t( s_player, rankbuf, t_data -> _rank_id[ i ] );
        talent_rank_spell -> s_enabled = s_enabled;
        t_rank_spells[ i ] = talent_rank_spell;
        if ( s_player -> sim -> debug ) log_t::output( s_player -> sim, "Talent Base status: %s", t_rank_spells[ i ] -> to_str().c_str() );
      }
    }
  }
  return true;
}

uint32_t talent_t::max_rank() const
{
  if ( ! s_player || ! t_data || ! t_data->id() )
    return 0;

  return t_data -> max_rank();
}

uint32_t talent_t::rank_spell_id( const uint32_t r ) const
{
  if ( ! s_player || ! t_data || ! t_data->id() )
    return 0;

  return t_data -> rank_spell_id( r );
}

const spell_id_t* talent_t::rank_spell( uint32_t r ) const
{
  assert( r <= max_rank() );

  // With default argument, return base spell always
  if ( ! r )
    return t_rank_spells[ 0 ];

  return t_rank_spells[ r - 1 ];
}

uint32_t talent_t::rank() const
{
  if ( ! ok() )
    return 0;

  return t_rank;
}

// ==========================================================================
// Spell ID
// ==========================================================================

// spell_id_t::spell_id_t ===================================================

spell_id_t::spell_id_t( player_t* player, const char* t_name ) :
  s_type( T_SPELL ), s_id( 0 ), s_data( 0 ), s_enabled( false ), s_player( player ),
  s_overridden( false ), s_token( t_name ? t_name : "" ),
  s_required_talent( 0 ), s_single( 0 ), s_tree( -1 ), s_list( 0 )
{
  armory_t::format( s_token, FORMAT_ASCII_MASK );
  range::fill( s_effects, 0 );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
  s_type( T_SPELL ), s_id( id ), s_data( 0 ), s_enabled( false ), s_player( player ),
  s_overridden( false ), s_token( t_name ), s_required_talent( talent ), s_single( 0 ), s_tree( -1 ), s_list( 0 )
{
  initialize();
  armory_t::format( s_token, FORMAT_ASCII_MASK );
}

spell_id_t::spell_id_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
  s_type( T_SPELL ), s_id( 0 ), s_data( 0 ), s_enabled( false ), s_player( player ),
  s_overridden( false ), s_token( t_name ), s_required_talent( talent ), s_single( 0 ), s_tree( -1 ), s_list( 0 )
{
  initialize( s_name );
  armory_t::format( s_token, FORMAT_ASCII_MASK );
}

spell_id_t::~spell_id_t()
{
  if ( s_list )
    s_list -> erase( s_list_iter );
}

void spell_id_t::queue()
{
  s_list = &s_player -> spell_list;
  s_list_iter = s_list->insert( s_list->end(), this );
}

bool spell_id_t::initialize( const char* s_name )
{
  range::fill( s_effects, 0 );

  assert( s_player && s_player -> sim );

  // For pets, find stuff based on owner class, as that's how our spell lists
  // are structured
  player_type player_class;
  if ( s_player -> is_pet() )
    player_class = s_player -> cast_pet() -> owner -> type;
  else
    player_class = s_player -> type;

  // Search using spell name to find the spell type
  if ( ! s_id )
  {
    if ( ! s_name || ! *s_name )
      return false;

    if ( ( s_id = s_player -> dbc.mastery_ability_id( player_class, s_name ) ) )
      s_type      = T_MASTERY;
    else if ( ! s_id && ( s_id = s_player -> dbc.specialization_ability_id( player_class, s_name ) ) )
      s_type      = T_SPEC;
    else if ( ! s_id && ( s_id = s_player -> dbc.class_ability_id( player_class, s_name ) ) )
      s_type      = T_CLASS;
    else if ( ! s_id && ( s_id = s_player -> dbc.race_ability_id( player_class, s_player -> race, s_name ) ) )
      s_type      = T_RACE;
    else if ( ! s_id && ( s_id = s_player -> dbc.glyph_spell_id( player_class, s_name ) ) )
      s_type      = T_GLYPH;
    else if ( ! s_id && ( s_id = s_player -> dbc.set_bonus_spell_id( player_class, s_name ) ) )
      s_type      = T_ITEM;
  }
  // Search using spell id to find the spell type
  else
  {
    if ( s_player -> dbc.is_mastery_ability( s_id ) )
      s_type      = T_MASTERY;
    else if ( s_player -> dbc.is_specialization_ability( s_id ) )
      s_type      = T_SPEC;
    else if ( s_player -> dbc.is_class_ability( s_id ) )
      s_type      = T_CLASS;
    else if ( s_player -> dbc.is_race_ability( s_id ) )
      s_type      = T_RACE;
    else if ( s_player -> dbc.is_glyph_spell( s_id ) )
      s_type      = T_GLYPH;
    else if ( s_player -> dbc.is_set_bonus_spell( s_id ) )
      s_type      = T_ITEM;
  }

  // At this point, our spell must exist or we are in trouble
  if ( ! s_id || ! s_player -> dbc.spell( s_id ) )
  {
    return false;
  }

  // Do second phase of spell initialization
  s_data = s_player -> dbc.spell( s_id );

  const_cast<spell_data_t*>( s_data ) -> set_used( true );

  // Some spells, namely specialization and class spells
  // can specify a tree for the spell
  switch ( s_type )
  {
  case T_SPEC:
    s_tree = s_player -> dbc.specialization_ability_tree( player_class, s_id );
    break;
  case T_CLASS:
    s_tree = s_player -> dbc.class_ability_tree( player_class, s_id );
    break;
  default:
    s_tree = -1;
    break;
  }

  s_enabled = s_data -> is_enabled() && s_data -> is_level( s_player -> level );

  // Warn if the player is enabling a spell that the player has no level for
  /*
  if ( ! s_player -> dbc.spell_is_level( s_id, s_player -> level ) )
  {
    s_player -> sim -> errorf( "Warning: Player %s level (%d) too low for spell %s, requires level %d",
      s_player -> name_str.c_str(),
      s_player -> level,
      s_data -> name,
      s_data -> spell_level );
  }
  */
  if ( s_type == T_MASTERY )
  {
    if ( s_player -> level < 75 )
      s_enabled = false;
  }

  // Map s_effects, figure out if this is a s_single-effect spell
  uint32_t n_effects = 0;
  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ! s_data -> _effect[ i ] )
      continue;

    if ( ! s_player -> dbc.effect( s_data -> _effect[ i ] ) )
      continue;

    s_effects[ i ] = s_player -> dbc.effect( s_data -> _effect[ i ] );
    n_effects++;
  }

  if ( n_effects == 1 )
  {
    for ( int i = 0; i < MAX_EFFECTS; i++ )
    {
      if ( ! s_effects[ i ] )
        continue;

      s_single = s_effects[ i ];
      break;
    }
  }
  else
    s_single = 0;

  return true;
}

bool spell_id_t::enable( bool override_value )
{
  assert( s_player && s_player -> sim );

  s_overridden = true;
  s_enabled    = override_value;

  return true;
}

bool spell_id_t::ok() const
{
  bool res = s_enabled;

  if ( ! s_player || ! s_data || ! s_id )
    return false;

  if ( s_required_talent )
    res = res & s_required_talent -> ok();

  if ( s_type == T_SPEC )
    res = res & ( s_tree == s_player -> specialization );

  return res;
}

std::string spell_id_t::to_str() const
{
  std::ostringstream s;

  s << "enabled=" << ( s_enabled ? "true" : "false" );
  s << " (ok=" << ( ok() ? "true" : "false" ) << ")";
  if ( s_overridden ) s << " (forced)";
  s << " token=" << s_token;
  s << " type=" << s_type;
  s << " tree=" << s_tree;
  s << " id=" << s_id;
  s << " player=" << s_player -> name_str;
  if ( s_required_talent )
    s << " req_talent=" << s_required_talent -> s_token;

  return s.str();
}

const char* spell_id_t::real_name() const
{
  if ( ! s_player || ! s_data || ! s_id )
    return 0;

  return s_data -> name_cstr();
}

const std::string spell_id_t::token() const
{
  if ( ! s_player || ! s_data || ! s_id )
    return 0;

  return s_token;
}

double spell_id_t::missile_speed() const
{
  if ( ! ok() )
    return 0.0;

  return s_data -> missile_speed();
}

uint32_t spell_id_t::school_mask() const
{
  if ( ! ok() )
    return 0;

  return s_data -> school_mask();
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

school_type spell_id_t::get_school_type() const
{
  if ( ! ok() )
    return SCHOOL_NONE;

  return get_school_type( school_mask() );
}

resource_type spell_id_t::power_type() const
{
  if ( ! ok() )
    return RESOURCE_NONE;

  return s_data -> power_type();
}

double spell_id_t::min_range() const
{
  if ( ! ok() )
    return 0.0;

  return s_data -> min_range();
}

double spell_id_t::max_range() const
{
  if ( ! ok() )
    return 0.0;

  return s_data -> max_range();
}

double spell_id_t::extra_coeff() const
{
  if ( ! ok() )
    return 0.0;

  return s_data -> extra_coeff();
}

bool spell_id_t::in_range() const
{
  if ( ! ok() )
    return false;

  return s_data -> in_range( s_player -> distance );
}

timespan_t spell_id_t::cooldown() const
{
  if ( ! ok() )
    return timespan_t::zero;

  double d = s_data -> cooldown().total_seconds();

  if ( d > ( s_player -> sim -> wheel_seconds - 2.0 ) )
    d = s_player -> sim -> wheel_seconds - 2.0;

  return timespan_t::from_seconds(d);
}

timespan_t spell_id_t::gcd() const
{
  if ( ! ok() )
    return timespan_t::zero;

  return s_data -> gcd();
}

uint32_t spell_id_t::category() const
{
  if ( ! ok() )
    return 0;

  return s_data -> category();
}

timespan_t spell_id_t::duration() const
{
  if ( ! ok() )
    return timespan_t::zero;

  timespan_t d = s_data -> duration();
  timespan_t player_wheel_seconds = timespan_t::from_seconds(s_player -> sim -> wheel_seconds - 2.0);
  if ( d > player_wheel_seconds )
    d = player_wheel_seconds;

  return d;
}

double spell_id_t::cost() const
{
  if ( ! ok() )
    return 0.0;

  return s_data -> cost();
}

uint32_t spell_id_t::rune_cost() const
{
  if ( ! ok() )
    return 0;

  return s_data -> rune_cost();
}

double spell_id_t::runic_power_gain() const
{
  if ( ! ok() )
    return 0.0;

  return s_data -> runic_power_gain();
}

uint32_t spell_id_t::max_stacks() const
{
  if ( ! ok() )
    return 0;

  return s_data -> max_stacks();
}

uint32_t spell_id_t::initial_stacks() const
{
  if ( ! ok() )
    return 0;

  return s_data -> initial_stacks();
}

double spell_id_t::proc_chance() const
{
  if ( ! ok() )
    return 0.0;

  return s_data -> proc_chance();
}

timespan_t spell_id_t::cast_time() const
{
  if ( ! ok() )
    return timespan_t::zero;

  return s_data -> cast_time( s_player -> level );
}

uint32_t spell_id_t::effect_id( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  return s_data -> effect_id( effect_num );
}

bool spell_id_t::flags( spell_attribute_t f ) const
{
  if ( ! ok() )
    return false;

  return s_data -> flags( f );
}

const char* spell_id_t::desc() const
{
  if ( ! ok() )
    return 0;

  return s_data -> desc();
}

const char* spell_id_t::tooltip() const
{
  if ( ! ok() )
    return 0;

  return s_data -> tooltip();
}

int32_t spell_id_t::effect_type( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> type();
}

int32_t spell_id_t::effect_subtype( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> subtype();
}

int32_t spell_id_t::effect_base_value( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> base_value();
}

int32_t spell_id_t::effect_misc_value1( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> misc_value1();
}

int32_t spell_id_t::effect_misc_value2( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> misc_value2();
}

uint32_t spell_id_t::effect_trigger_spell( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> trigger_spell_id();
}

double spell_id_t::effect_chain_multiplier( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> chain_multiplier();
}

double spell_id_t::effect_average( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect_average( effect_id, s_player -> level );
}

double spell_id_t::effect_delta( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect_delta( effect_id, s_player -> level );
}

double spell_id_t::effect_bonus( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect_bonus( effect_id, s_player -> level );
}

double spell_id_t::effect_min( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect_min( effect_id, s_player -> level );
}

double spell_id_t::effect_max( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect_max( effect_id, s_player -> level );
}

double spell_id_t::effect_coeff( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> coeff();
}

timespan_t spell_id_t::effect_period( uint32_t effect_num ) const
{
  if ( ! ok() )
    return timespan_t::zero;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> period();
}

double spell_id_t::effect_radius( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> radius();
}

double spell_id_t::effect_radius_max( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> radius_max();
}

double spell_id_t::effect_pp_combo_points( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> pp_combo_points();
}

double spell_id_t::effect_real_ppl( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0.0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> real_ppl();
}

int spell_id_t::effect_die_sides( uint32_t effect_num ) const
{
  if ( ! ok() )
    return 0;

  uint32_t effect_id = s_data -> effect_id( effect_num );

  return s_player -> dbc.effect( effect_id ) -> die_sides();
}

double spell_id_t::base_value( effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) const
{
  if ( ! ok() )
    return 0.0;

  if ( s_single &&
       ( type == E_MAX || s_single -> type() == type ) &&
       ( sub_type == A_MAX || s_single -> subtype() == sub_type ) &&
       ( misc_value == DEFAULT_MISC_VALUE || s_single -> misc_value1() == misc_value ) &&
       ( misc_value2 == DEFAULT_MISC_VALUE || s_single -> misc_value2() == misc_value2 ) )
  {
    if ( ( type == E_MAX || s_single -> type() == type ) &&
         ( sub_type == A_MAX || s_single -> subtype() == sub_type ) &&
         ( misc_value == DEFAULT_MISC_VALUE || s_single -> misc_value1() == misc_value ) &&
         ( misc_value2 == DEFAULT_MISC_VALUE || s_single -> misc_value2() == misc_value2 ) )
      return dbc_t::fmt_value( s_single -> base_value(), s_single -> type(), s_single -> subtype() );
    else
      return 0.0;
  }

  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ! s_effects[ i ] )
      continue;

    if ( ( type == E_MAX || s_effects[ i ] -> type() == type ) &&
         ( sub_type == A_MAX || s_effects[ i ] -> subtype() == sub_type ) &&
         ( misc_value == DEFAULT_MISC_VALUE || s_effects[ i ] -> misc_value1() == misc_value ) &&
         ( misc_value2 == DEFAULT_MISC_VALUE || s_effects[ i ] -> misc_value2() == misc_value2 ) )
      return dbc_t::fmt_value( s_effects[ i ] -> base_value(), type, sub_type );
  }

  return 0.0;
}

double spell_id_t::mod_additive( property_type_t p_type ) const
{
  // Move this somewhere sane, here for now
  static const double property_flat_divisor[] =
  {
    1.0,    // P_GENERIC
    1000.0, // P_DURATION
    1.0,    // P_THREAT
    1.0,    // P_EFFECT_1
    1.0,    // P_STACK
    1.0,    // P_RANGE
    1.0,    // P_RADIUS
    100.0,  // P_CRIT
    1.0,    // P_UNKNOWN_1
    1.0,    // P_PUSHBACK
    1000.0, // P_CAST_TIME
    1000.0, // P_COOLDOWN
    1.0,    // P_EFFECT_2
    1.0,    // Unused
    1.0,    // P_RESOURCE_COST
    1.0,    // P_CRIT_DAMAGE
    1.0,    // P_PENETRATION
    1.0,    // P_TARGET
    100.0,  // P_PROC_CHANCE
    1000.0, // P_TICK_TIME
    1.0,    // P_TARGET_BONUS
    1000.0, // P_GCD
    1.0,    // P_TICK_DAMAGE
    1.0,    // P_EFFECT_3
    100.0,  // P_SPELL_POWER
    1.0,    // Unused
    1.0,    // P_PROC_FREQUENCY
    1.0,    // P_DAMAGE_TAKEN
    100.0,  // P_DISPEL_CHANCE
  };

  if ( ! ok() )
    return 0.0;

  if ( s_single )
  {
    if ( ( p_type == P_MAX ) || ( s_single -> subtype() == A_ADD_FLAT_MODIFIER ) || ( s_single -> subtype() == A_ADD_PCT_MODIFIER ) )
    {
      if ( s_single -> subtype() == ( int ) A_ADD_PCT_MODIFIER )
        return s_single -> base_value() / 100.0;
      // Divide by property_flat_divisor for every A_ADD_FLAT_MODIFIER
      else
        return s_single -> base_value() / property_flat_divisor[ s_single -> misc_value1() ];
    }
    else
      return 0.0;
  }

  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ! s_effects[ i ] )
      continue;

    if ( s_effects[ i ] -> subtype() != A_ADD_FLAT_MODIFIER && s_effects[ i ] -> subtype() != A_ADD_PCT_MODIFIER )
      continue;

    if ( p_type == P_MAX || s_effects[ i ] -> misc_value1() == p_type )
    {
      // Divide by 100 for every A_ADD_PCT_MODIFIER
      if ( s_effects[ i ] -> subtype() == ( int ) A_ADD_PCT_MODIFIER )
        return s_effects[ i ] -> base_value() / 100.0;
      // Divide by property_flat_divisor for every A_ADD_FLAT_MODIFIER
      else
        return s_effects[ i ] -> base_value() / property_flat_divisor[ s_effects[ i ] -> misc_value1() ];
    }
  }

  return 0.0;
}


// ==========================================================================
// Active Spell ID
// ==========================================================================

active_spell_t::active_spell_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
  spell_id_t( player, t_name, id, talent )
{
  queue();

  if ( s_player -> sim -> debug )
    log_t::output( s_player -> sim, "Active Spell status: %s", to_str().c_str() );
}

active_spell_t::active_spell_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
  spell_id_t( player, t_name, s_name, talent )
{
  queue();

  if ( s_player -> sim -> debug )
    log_t::output( s_player -> sim, "Active Spell status: %s", to_str().c_str() );
}

// ==========================================================================
// Passive Spell ID
// ==========================================================================

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent ) :
  spell_id_t( player, t_name, id, talent )
{
  queue();

  if ( s_player -> sim -> debug )
    log_t::output( s_player -> sim, "Passive Spell status: %s", to_str().c_str() );
}

passive_spell_t::passive_spell_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent ) :
  spell_id_t( player, t_name, s_name, talent )
{
  queue();

  if ( s_player -> sim -> debug )
    log_t::output( s_player -> sim, "Passive Spell status: %s", to_str().c_str() );
}

// Glyph basic object

glyph_t::glyph_t( player_t* player, spell_data_t* _sd ) :
  spell_id_t( player, _sd -> name_cstr() ),
  // Future trimmed down access
  sd( _sd ), sd_enabled( spell_data_t::nil() )
{
  initialize( sd -> name_cstr() );
  if ( s_token.substr( 0, 9 ) == "glyph_of_" ) s_token.erase( 0, 9 );
  if ( s_token.substr( 0, 7 ) == "glyph__"   ) s_token.erase( 0, 7 );
  s_enabled = false;
}

bool glyph_t::enable( bool override_value )
{
  sd_enabled = override_value ? sd : spell_data_t::nil();

  spell_id_t::enable( override_value );

  if ( s_player -> sim -> debug )
    log_t::output( s_player -> sim, "Glyph Spell status: %s", to_str().c_str() );

  return s_enabled;
}

mastery_t::mastery_t( player_t* player, const char* t_name, const uint32_t id, int tree ) :
  spell_id_t( player, t_name, id ), m_tree( tree )
{
  queue();

  if ( s_player -> sim -> debug )
    log_t::output( s_player -> sim, "Mastery status: %s", to_str().c_str() );
}

mastery_t::mastery_t( player_t* player, const char* t_name, const char* s_name, int tree ) :
  spell_id_t( player, t_name, s_name ), m_tree( tree )
{
  queue();

  if ( s_player -> sim -> debug )
    log_t::output( s_player -> sim, "Mastery status: %s", to_str().c_str() );
}

bool mastery_t::ok() const
{
  return spell_id_t::ok() && ( s_player -> primary_tree() == m_tree );
}

double mastery_t::base_value( effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) const
{
  return spell_id_t::base_value( type, sub_type, misc_value, misc_value2 ) / 10000.0;
}

std::string mastery_t::to_str() const
{
  std::ostringstream s;

  s << spell_id_t::to_str() << " mastery_tree=" << m_tree;

  return s.str();
}

