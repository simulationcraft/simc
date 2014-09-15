// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

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

  if ( actor -> is_pet() || actor -> is_enemy() )
    return;

  // Initialize the set bonus data structure with correct set bonus data.
  // Spells are not setup yet.
  for ( size_t bonus_idx = 0; bonus_idx < dbc::n_set_bonus( maybe_ptr( actor -> dbc.ptr ) ) - 1; bonus_idx++ )
  {
    const item_set_bonus_t& bonus = dbc::set_bonus( maybe_ptr( actor -> dbc.ptr ) )[ bonus_idx ];
    if ( bonus.class_id != static_cast<unsigned>( util::class_id( actor -> type ) ) )
      continue;

    // Translate our DBC set bonus into one of our enums
    size_t bonus_type = bonus.bonus / 2 - 1;

    // T16 and less, old world, "role" specific set bonuses.
    if ( old_tier( bonus.tier ) )
      set_bonus_spec_data[ bonus.tier ][ bonus.role ][ bonus_type ].bonus = &bonus;
    // T17 onwards and PVP, new world, spec specific set bonuses. Overrides are
    // going to be provided by the new set_bonus= option, so no need to
    // translate anything from the old set bonus options.
    else
    {
      assert( bonus.spec > 0 );
      specialization_e spec = static_cast< specialization_e >( bonus.spec );
      set_bonus_spec_data[ bonus.tier ][ specdata::spec_idx( spec ) ][ bonus_type ].bonus = &bonus;
    }
  }
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

      // T17+ and PVP is spec specific, T16 and lower is "role specific"
      if ( old_tier( bonus.tier ) )
        set_bonus_spec_count[ bonus.tier ][ bonus.role ]++;
      else
        set_bonus_spec_count[ bonus.tier ][ specdata::spec_idx( actor -> _spec ) ]++;
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

        unsigned spec_role_idx = static_cast<int>( spec_idx );
        // If we're below the threshold tier, the "spec_idx" is actually a role
        // index, and our overrides are based on roles
        if ( old_tier( tier_idx ) )
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

        unsigned spec_role_idx = static_cast<int>( spec_idx );
        if ( old_tier( tier_idx ) )
          spec_role_idx = data.bonus -> role;

        if ( data.overridden >= 1 ||
           ( data.overridden == -1 && set_bonus_spec_count[ tier_idx ][ spec_role_idx ] >= data.bonus -> bonus ) )
        {
          if ( ! s.empty() )
            s += ", ";

          std::string spec_role_str;
          if ( old_tier( tier_idx ) )
            spec_role_str = translate_set_bonus_role( static_cast<set_role_e>( data.bonus -> role ) );
          else
            spec_role_str = util::specialization_string( actor -> specialization() );

          s += "{ ";
          s += data.bonus -> set_name;
          s += ", ";
          s += tier_type_str( static_cast<tier_e>( tier_idx ) );
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

        unsigned spec_role_idx = static_cast<int>( spec_idx );
        if ( old_tier( tier_idx ) )
          spec_role_idx = data.bonus -> role;

        if ( data.overridden >= 1 ||
           ( data.overridden == -1 && set_bonus_spec_count[ tier_idx ][ spec_role_idx ] >= data.bonus -> bonus ) )
        {
          std::string spec_role_str;
          if ( old_tier( tier_idx ) )
            spec_role_str = translate_set_bonus_role( static_cast<set_role_e>( data.bonus -> role ) );

          s += "# set_bonus=" + tier_type_str( static_cast<tier_e>( tier_idx ) );
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
  int tier = -1, bonus = -1;
  set_role_e role = SET_ROLE_NONE;

  size_t bonus_idx = 1;
  size_t role_idx = 0;

  if ( split[ 0 ].find( "tier" ) != std::string::npos )
    tier = util::to_unsigned( split[ 0 ].substr( 4 ) );

  if ( split[ 0 ].find( "pvp" ) != std::string::npos )
    tier = 0;

  if ( tier == -1 || static_cast<unsigned>( tier ) > max_tier() )
    return 0;

  if ( split.size() == 3 )
    role_idx = 2;

  if ( split[ bonus_idx ].find( "pc" ) != std::string::npos )
    bonus = util::to_unsigned( split[ bonus_idx ].substr( 0, split[ bonus_idx ].size() - 2 ) );

  if ( role_idx > 0 )
    role = translate_set_bonus_role_str( split[ role_idx ] );

  if ( bonus != 2 && bonus != 4 )
    return 0;

  if ( old_tier( tier ) && role == SET_ROLE_NONE )
    return 0;

  if ( ! old_tier( tier ) && role != SET_ROLE_NONE )
    return 0;

  bonus = bonus / 2 - 1;

  bool state = false;
  if ( old_tier( static_cast<size_t>( tier ) ) )
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

std::string set_bonus_t::tier_type_str( tier_e tier )
{
  if ( tier == PVP )
    return "pvp";
  else
    return "tier" + util::to_string( tier );
}

