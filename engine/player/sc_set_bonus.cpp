// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

set_role_e set_bonus_t::translate_set_bonus_role_str( const std::string& name )
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

const char* set_bonus_t::translate_set_bonus_role( set_role_e role )
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

set_bonus_t::set_bonus_t( player_t* player ) : actor( player )
{
  // First, pre-allocate vectors based on current boundaries of the set bonus data in DBC
  set_bonus_spec_data.resize( SET_BONUS_MAX );
  set_bonus_spec_count.resize( SET_BONUS_MAX );
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
  for ( size_t bonus_idx = 0; bonus_idx < dbc::n_set_bonus( maybe_ptr( actor -> dbc.ptr ) ); bonus_idx++ )
  {
    const item_set_bonus_t& bonus = dbc::set_bonus( maybe_ptr( actor -> dbc.ptr ) )[ bonus_idx ];
    if ( bonus.class_id != -1 && bonus.class_id != util::class_id( actor -> type ) )
      continue;

    // Translate our DBC set bonus into one of our enums
    size_t bonus_type = bonus.bonus / 2 - 1;

    // T16 and less, old world, "role" specific set bonuses.
    if ( role_set_bonus( bonus.enum_id ) )
    {
      assert( bonus.role != -1 );
      set_bonus_spec_data[ bonus.enum_id ][ bonus.role ][ bonus_type ].bonus = &bonus;
    }
    // T17 onwards and PVP, new world, spec specific set bonuses. Overrides are
    // going to be provided by the new set_bonus= option, so no need to
    // translate anything from the old set bonus options.
    else
    {
      // All specs share the same set bonus
      if ( bonus.spec == -1 )
      {
        for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ bonus.enum_id ].size(); spec_idx++ )
        {
          set_bonus_spec_data[ bonus.enum_id ][ spec_idx ][ bonus_type ].bonus = &bonus;
        }
      }
      else
      {
        specialization_e spec = static_cast< specialization_e >( bonus.spec );
        assert( bonus.spec > 0 );
        assert( ! set_bonus_spec_data[ bonus.enum_id ][ specdata::spec_idx( spec ) ][ bonus_type ].bonus );

        set_bonus_spec_data[ bonus.enum_id ][ specdata::spec_idx( spec ) ][ bonus_type ].bonus = &bonus;
      }
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

    if ( item -> parsed.data.id_set == 0 )
      continue;

    for ( size_t bonus_idx = 0; bonus_idx < dbc::n_set_bonus( maybe_ptr( actor -> dbc.ptr ) ); bonus_idx++ )
    {
      const item_set_bonus_t& bonus = dbc::set_bonus( maybe_ptr( actor -> dbc.ptr ) )[ bonus_idx ];
      if ( bonus.set_id != static_cast<unsigned>( item -> parsed.data.id_set ) )
        continue;

      if ( bonus.class_id != -1 && ! bonus.has_spec( static_cast< int >( actor -> _spec ) ) )
        continue;

      // T17+ and PVP is spec specific, T16 and lower is "role specific"
      if ( role_set_bonus( bonus.enum_id ) )
        set_bonus_spec_count[ bonus.enum_id ][ bonus.role ]++;
      else
        set_bonus_spec_count[ bonus.enum_id ][ specdata::spec_idx( actor -> _spec ) ]++;
      break;
    }
  }
}

std::vector<const item_set_bonus_t*> set_bonus_t::enabled_set_bonus_data() const
{
  std::vector<const item_set_bonus_t*> bonuses;
  // Disable all set bonuses, and set bonus options if challenge mode is set
  if ( actor -> sim -> challenge_mode == 1 )
    return bonuses;

  if ( actor -> sim -> disable_set_bonuses == 1 ) // Or if global disable set bonus override is used.
    return bonuses;

  for ( size_t idx = 0; idx < set_bonus_spec_data.size(); idx++ )
  {
    for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ idx ].size(); spec_idx++ )
    {
      for ( size_t bonus_idx = 0; bonus_idx < set_bonus_spec_data[ idx ][ spec_idx ].size(); bonus_idx++ )
      {
        const set_bonus_data_t& data = set_bonus_spec_data[ idx ][ spec_idx ][ bonus_idx ];
        // Most specs have the fourth specialization empty, or only have
        // limited number of roles, so there's no set bonuses for those entries
        if ( data.bonus == 0 )
          continue;

        if ( data.spell -> id() == 0 )
        {
          continue;
        }

        bonuses.push_back( data.bonus );
      }
    }
  }

  return bonuses;
}

void set_bonus_t::initialize()
{
  // Disable all set bonuses, and set bonus options if challenge mode is set
  if ( actor -> sim -> challenge_mode == 1 )
    return;

  if ( actor -> sim -> disable_set_bonuses == 1 ) // Or if global disable set bonus override is used.
    return;

  initialize_items();

  // Enable set bonuses then. This is a combination of item-based enablation
  // (enough items to enable a set bonus), and override based set bonus
  // enablation. As always, user options override everything else.
  for ( size_t idx = 0; idx < set_bonus_spec_data.size(); idx++ )
  {
    for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ idx ].size(); spec_idx++ )
    {
      for ( size_t bonus_idx = 0; bonus_idx < set_bonus_spec_data[ idx ][ spec_idx ].size(); bonus_idx++ )
      {
        set_bonus_data_t& data = set_bonus_spec_data[ idx ][ spec_idx ][ bonus_idx ];
        // Most specs have the fourth specialization empty, or only have
        // limited number of roles, so there's no set bonuses for those entries
        if ( data.bonus == 0 )
          continue;

        unsigned spec_role_idx = static_cast<int>( spec_idx );
        // If we're below the threshold tier, the "spec_idx" is actually a role
        // index, and our overrides are based on roles
        if ( role_set_bonus( idx ) )
          spec_role_idx = data.bonus -> role;

        // Set bonus is overridden, or we have sufficient number of items to enable the bonus
        if ( data.overridden >= 1 ||
          (data.overridden == -1 && set_bonus_spec_count[idx][spec_role_idx] >= data.bonus -> bonus) )
        {
          if ( data.bonus -> bonus == 2 )
          {
            if ( !actor -> sim -> disable_2_set_bonus )
              data.spell = actor -> find_spell( data.bonus -> spell_id );
          }
          else if ( data.bonus -> bonus == 4 )
          {
            if ( !actor -> sim -> disable_4_set_bonus )
              data.spell = actor -> find_spell( data.bonus -> spell_id );
          }
          else
          {
            data.spell = actor -> find_spell( data.bonus -> spell_id );
          }
        }
      }
    }
  }

  if ( actor -> sim -> debug )
    actor -> sim -> out_debug << to_string();
}

std::string set_bonus_t::to_string() const
{
  std::string s;

  for ( size_t idx = 0; idx < set_bonus_spec_data.size(); idx++ )
  {
    for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ idx ].size(); spec_idx++ )
    {
      for ( size_t bonus_idx = 0; bonus_idx < set_bonus_spec_data[ idx ][ spec_idx ].size(); bonus_idx++ )
      {
        const set_bonus_data_t& data = set_bonus_spec_data[ idx ][ spec_idx ][ bonus_idx ];
        if ( data.bonus == 0 )
          continue;

        unsigned spec_role_idx = static_cast<int>( spec_idx );
        if ( role_set_bonus( idx ) )
          spec_role_idx = data.bonus -> role;

        if ( data.overridden >= 1 ||
           ( data.overridden == -1 && set_bonus_spec_count[ idx ][ spec_role_idx ] >= data.bonus -> bonus ) )
        {
          if ( ! s.empty() )
            s += ", ";

          std::string spec_role_str;
          if ( role_set_bonus( idx ) )
            spec_role_str = translate_set_bonus_role( static_cast<set_role_e>( data.bonus -> role ) );
          else
            spec_role_str = util::specialization_string( actor -> specialization() );

          s += "{ ";
          s += data.bonus -> set_name;
          s += ", ";
          s += data.bonus -> set_opt_name;
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

  for ( size_t idx = 0; idx < set_bonus_spec_data.size(); idx++ )
  {
    for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ idx ].size(); spec_idx++ )
    {
      for ( size_t bonus_idx = 0; bonus_idx < set_bonus_spec_data[ idx ][ spec_idx ].size(); bonus_idx++ )
      {
        const set_bonus_data_t& data = set_bonus_spec_data[ idx ][ spec_idx ][ bonus_idx ];
        if ( data.bonus == 0 )
          continue;

        unsigned spec_role_idx = static_cast<int>( spec_idx );
        if ( role_set_bonus( idx ) )
          spec_role_idx = data.bonus -> role;

        if ( data.overridden >= 1 ||
           ( data.overridden == -1 && set_bonus_spec_count[ idx ][ spec_role_idx ] >= data.bonus -> bonus ) )
        {
          std::string spec_role_str;
          if ( role_set_bonus( idx ) )
            spec_role_str = translate_set_bonus_role( static_cast<set_role_e>( data.bonus -> role ) );

          s += "# set_bonus=";
          s += data.bonus -> set_opt_name;
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

expr_t* set_bonus_t::create_expression( const player_t* p, const std::string& type )
{
  set_bonus_type_e set_bonus = SET_BONUS_NONE;
  set_role_e role = SET_ROLE_NONE;
  set_bonus_e bonus = B_NONE;

  if ( ! parse_set_bonus_option( type, set_bonus, role, bonus ) )
  {
    return 0;
  }

  double state;

  if ( role_set_bonus( set_bonus ) )
    state = set_bonus_spec_data[ set_bonus ][ role ][ bonus ].spell -> id() > 0;
  else
    state = set_bonus_spec_data[ set_bonus ][ specdata::spec_idx( actor -> specialization() ) ][ bonus ].spell -> id() > 0;

  struct set_bonus_expr_t : public expr_t
  {
    double state_;

    set_bonus_expr_t( bool state ) :
      expr_t( "set_bonus_expr" ), state_( state ) { }
    double evaluate() { return state_; }
  };

  if ( p -> sim -> optimize_expressions )
    return expr_t::create_constant( type, state );
  else
    return new set_bonus_expr_t( state != 0 );
}

bool set_bonus_t::parse_set_bonus_option( const std::string& opt_str,
                                          set_bonus_type_e& set_bonus,
                                          set_role_e& role,
                                          set_bonus_e& bonus )
{
  set_bonus = SET_BONUS_NONE;
  role = SET_ROLE_NONE;
  bonus = B_NONE;

  std::vector<std::string> split = util::string_split( opt_str, "_" );
  if ( split.size() < 2 || split.size() > 3 )
  {
    return false;
  }

  size_t bonus_offset = 1;
  int role_offset = -1;
  if ( split.size() == 3 )
  {
    role_offset = 2;
  }

  if ( role_offset > 0 )
  {
    role = translate_set_bonus_role_str( split[ role_offset ] );
  }

  if ( util::str_compare_ci( split[ bonus_offset ], "2pc" ) )
    bonus = B2;
  else if ( util::str_compare_ci( split[ bonus_offset ], "4pc" ) )
    bonus = B4;

  for ( size_t bonus_idx = 0; bonus_idx < dbc::n_set_bonus( maybe_ptr( actor -> dbc.ptr ) ); bonus_idx++ )
  {
    const item_set_bonus_t& bonus = dbc::set_bonus( maybe_ptr( actor -> dbc.ptr ) )[ bonus_idx ];
    if ( bonus.class_id != -1 && bonus.class_id != util::class_id( actor -> type ) )
      continue;

    if ( set_bonus == SET_BONUS_NONE && util::str_compare_ci( split[ 0 ], bonus.set_opt_name ) )
    {
      set_bonus = static_cast< set_bonus_type_e >( bonus.enum_id );
      break;
    }
  }

  if ( ! role_set_bonus( set_bonus ) )
    return set_bonus != SET_BONUS_NONE && bonus != B_NONE;
  else
    return set_bonus != SET_BONUS_NONE && role != SET_ROLE_NONE && bonus != B_NONE;
}

std::string set_bonus_t::generate_set_bonus_options() const
{
  std::vector<std::string> opts;

  for ( size_t bonus_idx = 0; bonus_idx < dbc::n_set_bonus( maybe_ptr( actor -> dbc.ptr ) ); bonus_idx++ )
  {
    const item_set_bonus_t& bonus = dbc::set_bonus( maybe_ptr( actor -> dbc.ptr ) )[ bonus_idx ];
    std::string opt = bonus.set_opt_name;
    opt += "_" + util::to_string( bonus.bonus ) + "pc";
    if ( role_set_bonus( bonus.enum_id ) )
    {
      opt += "_";
      opt += translate_set_bonus_role( static_cast<set_role_e>( bonus.role ) );
    }

    if ( std::find( opts.begin(), opts.end(), opt ) == opts.end() )
    {
      opts.push_back(opt);
    }
  }

  std::stringstream ss;
  for ( size_t i = 0; i < opts.size(); i++ )
  {
    ss << opts[ i ];
    if ( i < opts.size() - 1 )
    {
      ss << ", ";
    }
  }
  return ss.str();
}

