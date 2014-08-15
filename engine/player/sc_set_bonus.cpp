// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// set_bonus_t::decode ======================================================

set_e set_bonus_t::decode( const player_t& p,
                         const item_t&   item ) const
{
  if ( ! item.name() ) return SET_NONE;

  if ( p.sim -> challenge_mode ) return SET_NONE;

  set_e set = p.decode_set( item );
  if ( set != SET_NONE ) return set;

  return SET_NONE;
}

// set_bonus_t::init ========================================================

void set_bonus_t::init( const player_t& p )
{
  for ( size_t i = 0; i < p.items.size(); i++ )
  {
    set_e s = decode( p, p.items[ i ] );
    if ( s != SET_NONE )
    {
      count[ s ] += 1;
    }
  }
  initialized = true;
}

expr_t* set_bonus_t::create_expression( const player_t* player,
                                        const std::string& type )
{
  set_e bonus_type = util::parse_set_bonus( type );

  if ( bonus_type == SET_NONE )
    return 0;

  struct set_bonus_expr_t : public expr_t
  {
    const player_t& player;
    set_e set_bonus_type;
    set_bonus_expr_t( const player_t& p, set_e t ) :
      expr_t( util::set_bonus_string( t ) ), player( p ), set_bonus_type( t ) {}
    virtual double evaluate()
    { return player.sets.has_set_bonus( set_bonus_type ); }
  };

  assert( player );
  return new set_bonus_expr_t( *player, bonus_type );
}

inline const spell_data_t* set_bonus_t::create_set_bonus( uint32_t spell_id )
{
  if ( ! p -> dbc.spell( spell_id ) )
  {
    if ( spell_id > 0 )
    {
      p -> sim -> errorf( "Set bonus spell identifier %u for %s not found in spell data.",
                          spell_id, p -> name_str.c_str() );
    }
    return default_value;
  }

  return ( p -> dbc.spell( spell_id ) );
}

// set_bonus_array_t::set_bonus_array_t =====================================

set_bonus_t::set_bonus_t( const player_t* p ) :
     count(), default_value( spell_data_t::nil() ), set_bonuses(),  p( p ),
     initialized( false ), spelldata_registered( false )
{
    count[ SET_T13_2PC_CASTER ] = count[ SET_T13_2PC_MELEE ] = count[ SET_T13_2PC_TANK ] = count[ SET_T13_2PC_HEAL ] = -1;
    count[ SET_T13_4PC_CASTER ] = count[ SET_T13_4PC_MELEE ] = count[ SET_T13_4PC_TANK ] = count[ SET_T13_4PC_HEAL ] = -1;
    count[ SET_T14_2PC_CASTER ] = count[ SET_T14_2PC_MELEE ] = count[ SET_T14_2PC_TANK ] = count[ SET_T14_2PC_HEAL ] = -1;
    count[ SET_T14_4PC_CASTER ] = count[ SET_T14_4PC_MELEE ] = count[ SET_T14_4PC_TANK ] = count[ SET_T14_4PC_HEAL ] = -1;
    count[ SET_T15_2PC_CASTER ] = count[ SET_T15_2PC_MELEE ] = count[ SET_T15_2PC_TANK ] = count[ SET_T15_2PC_HEAL ] = -1;
    count[ SET_T15_4PC_CASTER ] = count[ SET_T15_4PC_MELEE ] = count[ SET_T15_4PC_TANK ] = count[ SET_T15_4PC_HEAL ] = -1;
    count[ SET_T16_2PC_CASTER ] = count[ SET_T16_2PC_MELEE ] = count[ SET_T16_2PC_TANK ] = count[ SET_T16_2PC_HEAL ] = -1;
    count[ SET_T16_4PC_CASTER ] = count[ SET_T16_4PC_MELEE ] = count[ SET_T16_4PC_TANK ] = count[ SET_T16_4PC_HEAL ] = -1;
    count[ SET_T17_2PC_CASTER ] = count[ SET_T17_2PC_MELEE ] = count[ SET_T17_2PC_TANK ] = count[ SET_T17_2PC_HEAL ] = -1;
    count[ SET_T17_4PC_CASTER ] = count[ SET_T17_4PC_MELEE ] = count[ SET_T17_4PC_TANK ] = count[ SET_T17_4PC_HEAL ] = -1;
    count[ SET_PVP_2PC_CASTER ] = count[ SET_PVP_2PC_MELEE ] = count[ SET_PVP_2PC_TANK ] = count[ SET_PVP_2PC_HEAL ] = -1;
    count[ SET_PVP_4PC_CASTER ] = count[ SET_PVP_4PC_MELEE ] = count[ SET_PVP_4PC_TANK ] = count[ SET_PVP_4PC_HEAL ] = -1;
}

void set_bonus_t::register_spelldata( const set_bonus_description_t a_bonus )
{
  // Map two-dimensional array into correct slots in the one-dimensional set_bonuses
  // array, based on set_e enum
  for ( int tier = 0; tier < N_TIER; ++tier )
  {
    for ( int j = 0; j < N_TIER_BONUS; j++ )
    {
      int b = 3 * j / 2 + 1;
      set_bonuses[ 1 + tier * 12 + b ] = create_set_bonus( a_bonus[ tier ][ j ] );
    }
  }
  spelldata_registered = true;
}

/* Check if the player has the set bonus
 */
bool set_bonus_t::has_set_bonus( set_e s ) const
{
  // Disable set bonuses in challenge mode
  if ( p -> sim -> challenge_mode == 1 )
    return false;

  return set_bonus_t::has_set_bonus( p, s );
}

bool set_bonus_t::has_set_bonus( const player_t* p, set_e s )
{
  assert( p -> sets.initialized && "Attempt to check for set bonus before initialization." );

  if ( p -> sets.count[ s ] > 0 )
    return true;

  if ( p -> sets.count[ s ] < 0 )
  {
    assert( s > 0 );
    int tier  = ( s - 1 ) / 12,
        btype = ( ( s - 1 ) % 12 ) / 3,
        bonus = ( s - 1 ) % 3;

    assert( 1 + tier * 12 + btype * 3 < as<int>( p -> sets.count.size() ) );
    if ( p -> sets.count[ 1 + tier * 12 + btype * 3 ] >= bonus * 2 )
      return true;
  }

  return false;
}

/* Return pointer to set_bonus spell_data
 * spell_data_t::nil() if player does not have set_bonus
 */
const spell_data_t* set_bonus_t::set( set_e s ) const
{
  assert( p -> sets.spelldata_registered && "Attempt to use set bonus spelldata before registering spelldata." );

  if ( set_bonuses[ s ] && has_set_bonus( s )  )
    return set_bonuses[ s ];

  return default_value;
}

void set_bonus_t::copy_from( const set_bonus_t& s )
{ count = s.count; }

namespace new_set_bonus {

set_role_e translate_set_bonus_role_str( const std::string& name )
{
  if ( util::str_compare_ci( name, "tank" ) )
    return SET_TANK;
  else if ( util::str_compare_ci( name, "healer" ) )
    return SET_HEALER;
  else if ( util::str_compare_ci( name, "melee" ) )
    return SET_MELEE;
  else if ( util::str_compare_ci( name, "caster" ) )
    return SET_CASTER;

  return SET_ROLE_NONE;
}

const char* translate_set_bonus_role( set_role_e role )
{
  switch ( role )
  {
    case SET_TANK: return "tank";
    case SET_HEALER: return "healer";
    case SET_MELEE: return "melee";
    case SET_CASTER: return "caster";
    default: return "unknown";
  }
}

unsigned set_bonus_t::max_tier() const
{ return dbc::set_bonus( maybe_ptr( actor -> dbc.ptr ) )[ dbc::n_set_bonus( maybe_ptr( actor -> dbc.ptr ) ) - 1 ].tier; }

set_bonus_t::set_bonus_t( player_t* player ) : actor( player )
{
  if ( actor -> is_pet() || actor -> is_enemy() )
    return;

  // First, pre-allocate vectors based on current boundaries of the set bonus data in DBC
  set_bonus_spec_data.resize( max_tier() + 1 );
  set_bonus_spec_count.resize( max_tier() + 1 );
  for ( size_t i = 0; i < set_bonus_spec_data.size(); i++ )
  {
    set_bonus_spec_data[ i ].resize( actor -> dbc.specialization_max_per_class() );
    set_bonus_spec_count[ i ].resize( actor -> dbc.specialization_max_per_class() );
    // For now only 2, and 4 set bonuses
    for ( size_t j = 0; j < set_bonus_spec_data[ i ].size(); j++ )
    {
      set_bonus_spec_data[ i ][ j ].resize( N_BONUSES, set_bonus_data_t() );
      set_bonus_spec_count[ i ][ j ] = 0;
    }
  }

  // Initialize the set bonus data structure with correct set bonus data.
  // Spells are not setup yet.
  for ( size_t bonus_idx = 0; bonus_idx < dbc::n_set_bonus( maybe_ptr( actor -> dbc.ptr ) ) - 1; bonus_idx++ )
  {
    const item_set_bonus_t& bonus = dbc::set_bonus( maybe_ptr( actor -> dbc.ptr ) )[ bonus_idx ];
    if ( bonus.class_id != static_cast<unsigned>( util::class_id( actor -> type ) ) )
      continue;

    // Translate our DBC set bonus into one of our enums
    set_e bonus_enum = translate_set_bonus_data( bonus );
    size_t bonus_type = bonus.bonus / 2 - 1;

    // T17 onwards, new world, spec specific set bonuses. Overrides are going
    // to be provided by the new set_bonus= option, so no need to translate
    // anything from the old set bonus options.
    if ( bonus.tier >= TIER_THRESHOLD )
    {
      assert( bonus.spec > 0 );
      specialization_e spec = static_cast< specialization_e >( bonus.spec );
      set_bonus_spec_data[ bonus.tier ][ specdata::spec_idx( spec ) ][ bonus_type ].bonus = &bonus;
    }
    // T16 and less, old world, "role" specific set bonuses.
    else
    {
      set_bonus_spec_data[ bonus.tier ][ bonus.role ][ bonus_type ].bonus = &bonus;
      // If the user had given an override for the set bonus, copy it here
      if ( bonus_enum != SET_NONE && actor -> sets.count[ bonus_enum ] != -1 )
        set_bonus_spec_data[ bonus.tier ][ bonus.role ][ bonus_type ].overridden = actor -> sets.count[ bonus_enum ];
    }
  }
}

// Translate between the DBC role types for specializations, and
// Simulationcraft roles
static int translate_role_type( unsigned role_type )
{
  switch ( role_type )
  {
    case 0: // TANK
      return 2;
    case 1: // HEALER
      return 3;
    case 2: // MELEE
      return 1;
    case 3: // CASTER
      return 0;
    default:
      return -1;
  }
}

set_e translate_set_bonus_data( const item_set_bonus_t& bonus )
{
  unsigned set_bonus_idx = 1;

  int diff = bonus.tier - MIN_TIER;
  if ( diff < 0 )
    return SET_NONE;

  set_bonus_idx += diff * set_bonus_t::tier_divisor;
  set_bonus_idx += translate_role_type( bonus.role ) * set_bonus_t::role_divisor;
  set_bonus_idx += 1 + ( bonus.bonus / 2 - 1 );

  return static_cast<set_e>( set_bonus_idx );
}

// Initialize set bonus counts based on the items of the actor
void set_bonus_t::initialize_items()
{
  for ( size_t i = 0, end = actor -> items.size(); i < end; i++ )
  {
    item_t* item = &( actor -> items[ i ] );
    if ( item -> parsed.data.id == 0 )
      continue;

    for ( size_t bonus_idx = 0; bonus_idx < dbc::n_set_bonus( maybe_ptr( actor -> dbc.ptr ) ) - 1; bonus_idx++ )
    {
      const item_set_bonus_t& bonus = dbc::set_bonus( maybe_ptr( actor -> dbc.ptr ) )[ bonus_idx ];
      if ( bonus.set_id != static_cast<unsigned>( item -> parsed.data.id_set ) )
        continue;

      if ( ! bonus.has_spec( actor -> _spec ) )
        continue;

      // T17+ is spec specific, T16 and lower is "role specific"
      if ( bonus.tier >= TIER_THRESHOLD )
        set_bonus_spec_count[ bonus.tier ][ specdata::spec_idx( actor -> _spec ) ]++;
      else
        set_bonus_spec_count[ bonus.tier ][ bonus.role ]++;
      break;
    }
  }
}

void set_bonus_t::initialize()
{
  initialize_items();

  // Enable set bonuses then. This is a combination of item-based enablation
  // (enough items to enable a set bonus), and override based set bonus
  // enablation. As always, user options override everything else.
  for ( size_t tier_idx = 0; tier_idx < set_bonus_spec_data.size(); tier_idx++ )
  {
    for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ tier_idx ].size(); spec_idx++ )
    {
      for ( size_t bonus_idx = 0; bonus_idx < set_bonus_spec_data[ tier_idx ][ spec_idx ].size(); bonus_idx++ )
      {
        set_bonus_data_t& data = set_bonus_spec_data[ tier_idx ][ spec_idx ][ bonus_idx ];
        // Most specs have the fourth specialization empty, or only have
        // limited number of roles, so there's no set bonuses for those entries
        if ( data.bonus == 0 )
          continue;

        unsigned spec_role_idx = spec_idx;
        // If we're below the threshold tier, the "spec_idx" is actually a role
        // index, and our overrides are based on roles
        if ( tier_idx < TIER_THRESHOLD )
          spec_role_idx = data.bonus -> role;

        // Set bonus is overridden, or we have sufficient number of items to enable the bonus
        if ( data.overridden >= 1 ||
             ( data.overridden == -1 && set_bonus_spec_count[ tier_idx ][ spec_role_idx ] >= data.bonus -> bonus ) )
          data.spell = actor -> find_spell( data.bonus -> spell_id );
      }
    }
  }

  if ( actor -> sim -> debug )
    actor -> sim -> out_debug << to_string();
}

std::string set_bonus_t::to_string() const
{
  std::string s;

  for ( size_t tier_idx = 0; tier_idx < set_bonus_spec_data.size(); tier_idx++ )
  {
    for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ tier_idx ].size(); spec_idx++ )
    {
      for ( size_t bonus_idx = 0; bonus_idx < set_bonus_spec_data[ tier_idx ][ spec_idx ].size(); bonus_idx++ )
      {
        const set_bonus_data_t& data = set_bonus_spec_data[ tier_idx ][ spec_idx ][ bonus_idx ];
        if ( data.bonus == 0 )
          continue;

        unsigned spec_role_idx = spec_idx;
        if ( tier_idx < TIER_THRESHOLD )
          spec_role_idx = data.bonus -> role;

        if ( data.overridden >= 1 ||
           ( data.overridden == -1 && set_bonus_spec_count[ tier_idx ][ spec_role_idx ] >= data.bonus -> bonus ) )
        {
          if ( ! s.empty() )
            s += ", ";

          std::string spec_role_str;
          if ( tier_idx < TIER_THRESHOLD )
            spec_role_str = translate_set_bonus_role( static_cast<set_role_e>( data.bonus -> role ) );
          else
            spec_role_str = util::specialization_string( actor -> specialization() );

          s += "{ ";
          s += data.bonus -> set_name;
          s += ", tier ";
          s += util::to_string( tier_idx );
          s += ", ";
          s += spec_role_str;
          s += ", ";
          s += util::to_string( data.bonus -> bonus );
          s += " piece bonus";
          if ( data.overridden >= 1 )
            s += " (overridden)";
          s += " }";
        }
      }
    }
  }

  return s;
}

std::string set_bonus_t::to_profile_string( const std::string& newline ) const
{
  std::string s;

  for ( size_t tier_idx = 0; tier_idx < set_bonus_spec_data.size(); tier_idx++ )
  {
    for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ tier_idx ].size(); spec_idx++ )
    {
      for ( size_t bonus_idx = 0; bonus_idx < set_bonus_spec_data[ tier_idx ][ spec_idx ].size(); bonus_idx++ )
      {
        const set_bonus_data_t& data = set_bonus_spec_data[ tier_idx ][ spec_idx ][ bonus_idx ];
        if ( data.bonus == 0 )
          continue;

        unsigned spec_role_idx = spec_idx;
        if ( tier_idx < TIER_THRESHOLD )
          spec_role_idx = data.bonus -> role;

        if ( data.overridden >= 1 ||
           ( data.overridden == -1 && set_bonus_spec_count[ tier_idx ][ spec_role_idx ] >= data.bonus -> bonus ) )
        {
          std::string spec_role_str;
          if ( tier_idx < TIER_THRESHOLD )
            spec_role_str = translate_set_bonus_role( static_cast<set_role_e>( data.bonus -> role ) );

          s += "# set_bonus=tier" + util::to_string( tier_idx );
          s += "_" + util::to_string( data.bonus -> bonus ) + "pc";
          if ( ! spec_role_str.empty() )
            s += "_" + spec_role_str;
          s += "=1";
          s += newline;
        }
      }
    }
  }

  return s;
}

expr_t* set_bonus_t::create_expression( const player_t*, const std::string& type )
{
  std::vector<std::string> split = util::string_split( type, "_" );
  unsigned tier = 0, bonus = 0;
  set_role_e role = SET_ROLE_NONE;

  size_t bonus_idx = 0;
  size_t role_idx = 1;

  if ( split[ 0 ].find( "tier" ) != std::string::npos )
    tier = util::to_unsigned( split[ 0 ].substr( 4 ) );

  if ( split.size() == 3 )
  {
    bonus_idx++;
    role_idx++;
  }

  if ( split[ bonus_idx ].find( "pc" ) != std::string::npos )
    bonus = util::to_unsigned( split[ bonus_idx ].substr( 0, split[ bonus_idx ].size() - 2 ) );

  if ( role_idx > 0 )
    role = translate_set_bonus_role_str( split[ role_idx ] );

  if ( tier < 1 || tier > max_tier() )
    return 0;

  if ( bonus != 2 && bonus != 4 )
    return 0;

  if ( tier < TIER_THRESHOLD && role == SET_ROLE_NONE )
    return 0;

  if ( tier >= TIER_THRESHOLD && role != SET_ROLE_NONE )
    return 0;

  bonus = bonus / 2 - 1;


  bool state = false;
  if ( tier < TIER_THRESHOLD )
    state = set_bonus_spec_data[ tier ][ role ][ bonus ].spell -> id() > 0;
  else
    state = set_bonus_spec_data[ tier ][ specdata::spec_idx( actor -> specialization() ) ][ bonus ].spell -> id() > 0;

  struct set_bonus_expr_t : public expr_t
  {
    double state_;

    set_bonus_expr_t( bool state ) :
      expr_t( "set_bonus_expr" ), state_( state ) { }
    double evaluate() { return state_; }
  };

  return new set_bonus_expr_t( state );
}

}
