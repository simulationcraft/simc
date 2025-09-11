// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "set_bonus.hpp"

#include "dbc/dbc.hpp"
#include "dbc/item_set_bonus.hpp"
#include "item/item.hpp"
#include "player/player.hpp"
#include "sim/expressions.hpp"
#include "sim/sim.hpp"
#include "sc_enums.hpp"

set_bonus_t::set_bonus_t( player_t* player )
  : actor( player ), set_bonus_spec_data( SET_BONUS_MAX ), set_bonus_spec_count( SET_BONUS_MAX )
{
  for ( size_t i = 0; i < set_bonus_spec_data.size(); i++ )
  {
    unsigned index_count = actor->dbc->specialization_max_per_class() + actor->dbc->hero_trees_max_per_class();
    set_bonus_spec_data[ i ].resize( index_count );
    set_bonus_spec_count[ i ].resize( index_count );
    // For now only 2, and 4 set bonuses
    for ( size_t j = 0; j < set_bonus_spec_data[ i ].size(); j++ )
    {
      set_bonus_spec_data[ i ][ j ].resize( N_BONUSES, set_bonus_data_t( spell_data_t::not_found() ) );
      set_bonus_spec_count[ i ][ j ] = 0;
    }
  }

  if ( actor->is_pet() || actor->is_enemy() )
    return;

  auto set_bonuses = item_set_bonus_t::data( actor->dbc->ptr );

  // Initialize the set bonus data structure with correct set bonus data.
  // Spells are not setup yet.
  for ( const auto& bonus : set_bonuses )
  {
    if ( bonus.class_id != -1 && bonus.class_id != util::class_id( actor->type ) )
      continue;

    // Ensure Blizzard does not go crazy and add set bonuses past 8 item requirement
    assert( bonus.bonus <= B_MAX );

    // T17 onwards and PVP, new world, spec specific set bonuses. Overrides are
    // going to be provided by the new set_bonus= option, so no need to
    // translate anything from the old set bonus options.
    //
    // All specs and trait sub trees share the same set bonus
    if ( bonus.spec == -1 && bonus.trait_sub_tree == -1 )
    {
      for ( size_t spec_idx = 0; spec_idx < set_bonus_spec_data[ bonus.enum_id ].size(); spec_idx++ )
      {
        // Don't allow generic set bonus to override spec specific one, if it exists. The exporter
        // should export data so that this will never happen, but be extra careful.
        if ( set_bonus_spec_data[ bonus.enum_id ][ spec_idx ][ bonus.bonus - 1 ].bonus )
        {
          continue;
        }

        set_bonus_spec_data[ bonus.enum_id ][ spec_idx ][ bonus.bonus - 1 ].bonus = &bonus;
      }
    }
    // Note, spec and hero tree specific set bonuses are allowed to override "generic" bonuses (bonus.spec == -1 &&
    // bonus.trait_sub_tree == -1)
    else
    {
      assert( bonus.spec > 0 || bonus.trait_sub_tree > 0 );
      set_bonus_spec_data[ bonus.enum_id ][ composite_idx( bonus ) ][ bonus.bonus - 1 ].bonus = &bonus;
    }
  }
}

int set_bonus_t::spec_idx( specialization_e spec ) const
{
  return dbc::spec_idx( spec, actor->dbc->ptr );
}

int set_bonus_t::hero_idx( hero_tree_e hero ) const
{
  return dbc::hero_idx( hero, actor->dbc->ptr );
}

int set_bonus_t::composite_idx( specialization_e spec, hero_tree_e hero ) const
{
  return dbc::composite_idx( spec, hero, actor->dbc->ptr );
}

int set_bonus_t::composite_idx( const item_set_bonus_t& bonus ) const
{
  // only one spec or hero may be selected, or this does very dangerous things!
  // a proposed rewrite of this subsystem to use the standard data-wrapper-and-cache-on-actor
  // approach solves this :)

  if ( ( bonus.spec != -1 ) != ( bonus.trait_sub_tree != -1 ) )
  {
    // sets with either tst xor spec based selection
    int index = 0;

    if ( bonus.spec == -1 )
      index += actor->dbc->specialization_max_per_class();
    else
      index += spec_idx( static_cast<specialization_e>( bonus.spec ) );

    if ( bonus.trait_sub_tree == -1 )
      index += 0;
    else
      index += hero_idx( static_cast<hero_tree_e>( bonus.trait_sub_tree ) );

    return index;
  }
  else
  {
    // sets without tst or spec based selection
    // every spec/tst index should contain identical data but just to be safe
    return spec_idx( actor->_spec );
  }
}

// Initialize set bonus counts based on the items of the actor
void set_bonus_t::initialize_items()
{
  // Don't allow 2 worn items of the same id to count as 2 slots
  std::vector<unsigned> item_ids;

  for ( auto& item : actor->items )
  {
    if ( item.parsed.data.id == 0 )
      continue;

    if ( item.parsed.data.id_set == 0 )
      continue;

    auto set_bonuses = item_set_bonus_t::data( actor->dbc->ptr );

    for ( const auto& bonus : set_bonuses )
    {
      if ( bonus.set_id != static_cast<unsigned>( item.parsed.data.id_set ) )
        continue;

      bool has_class = bonus.class_id == -1 || bonus.class_id == util::class_id( actor->type );
      bool has_spec = bonus.spec == static_cast<int>( actor->_spec );
      bool has_trait_sub_tree =
        bonus.trait_sub_tree == -1 ? false : actor->player_sub_trees.count( as<unsigned>( bonus.trait_sub_tree ) );

      bool has_no_constraint = bonus.spec == -1 && bonus.trait_sub_tree == -1;

      if ( range::find( item_ids, item.parsed.data.id ) != item_ids.end() )
        continue;

      if ( has_class && ( has_spec || has_trait_sub_tree || has_no_constraint ) )
      {
        set_bonus_spec_count[ bonus.enum_id ][ composite_idx( bonus ) ]++;
        item_ids.push_back( item.parsed.data.id );
        break;
      }
    }
  }
}

std::vector<const item_set_bonus_t*> set_bonus_t::enabled_set_bonus_data() const
{
  std::vector<const item_set_bonus_t*> bonuses;

  // Disable all set bonuses, and set bonus options if challenge mode is set
  if ( actor->sim->challenge_mode == 1 )
    return bonuses;

  if ( actor->sim->disable_set_bonuses == 1 )  // Or if global disable set bonus override is used.
    return bonuses;

  for ( auto& spec_data : set_bonus_spec_data )
  {
    for ( auto& bonus_type : spec_data )
    {
      for ( auto& bonus_data : bonus_type )
      {
        // Most specs have the fourth specialization empty, or only have limited number of roles, so there's no set
        // bonuses for those entries
        if ( !bonus_data.bonus || bonus_data.quiet || !bonus_data.spell->ok() )
          continue;

        bonuses.push_back( bonus_data.bonus );
      }
    }
  }

  return bonuses;
}

// Fast accessor to a set bonus spell, returns the spell, or spell_data_t::not_found()

const spell_data_t* set_bonus_t::set( specialization_e spec, set_bonus_type_e tier, set_bonus_e bonus ) const
{
  if ( spec_idx( spec ) < 0 )
    return spell_data_t::nil();

#ifndef NDEBUG
  assert( set_bonus_spec_data.size() > static_cast<unsigned>( tier ) );
  assert( set_bonus_spec_data[ tier ].size() > as<unsigned>( composite_idx( spec, hero_tree_e::HERO_NONE ) ) );
  assert( set_bonus_spec_data[ tier ][ spec_idx( spec ) ].size() > static_cast<unsigned>( bonus ) );
#endif

  return set_bonus_spec_data[ tier ][ spec_idx( spec ) ][ bonus ].spell;
}

const spell_data_t* set_bonus_t::set( hero_tree_e hero, set_bonus_type_e tier, set_bonus_e bonus ) const
{
  if ( hero_idx( hero ) < 0 )
    return spell_data_t::nil();

#ifndef NDEBUG
  assert( set_bonus_spec_data.size() > static_cast<unsigned>( tier ) );
  assert( set_bonus_spec_data[ tier ].size() >
          as<unsigned>( actor->dbc->specialization_max_per_class() + hero_idx( hero ) ) );
  assert( set_bonus_spec_data[ tier ][ actor->dbc->specialization_max_per_class() + hero_idx( hero ) ].size() >
          static_cast<unsigned>( bonus ) );
#endif

  return set_bonus_spec_data[ tier ][ actor->dbc->specialization_max_per_class() + hero_idx( hero ) ][ bonus ].spell;
}

void set_bonus_t::initialize()
{
  // Disable all set bonuses, and set bonus options if challenge mode is set
  if ( actor->sim->challenge_mode == 1 )
    return;

  if ( actor->sim->disable_set_bonuses == 1 )  // Or if global disable set bonus override is used.
    return;

  if ( actor->sim->enable_all_sets )
    enable_all_sets();
  else if ( !actor->set_bonus_str.empty() )
    parse_set_bonus_string();

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

        // No set bonus for spec, skip
        if ( data.bonus == nullptr )
          continue;

        bool is_overridden = data.overridden > 0;
        bool is_in_range = set_bonus_spec_count[ idx ][ spec_idx ] >= data.bonus->bonus && data.overridden == -1;
        bool is_allowed_spec = data.bonus->has_spec( actor->_spec );
        bool is_allowed_trait_sub_tree = data.bonus->trait_sub_tree != -1
                                           ? actor->player_sub_trees.count( as<unsigned>( data.bonus->trait_sub_tree ) )
                                           : false;
        bool is_equippable = is_allowed_spec || is_allowed_trait_sub_tree;

        bool enable_2_set = util::str_compare_ci( actor->sim->enable_2_set, data.bonus->tier );
        bool disable_2_set = util::str_compare_ci( actor->sim->disable_2_set, data.bonus->tier );
        bool is_enabled_2p = data.bonus->bonus == 2 && enable_2_set;
        bool is_disabled_2p = data.bonus->bonus == 2 && disable_2_set;

        bool enable_4_set = util::str_compare_ci( actor->sim->enable_4_set, data.bonus->tier );
        bool disable_4_set = util::str_compare_ci( actor->sim->disable_4_set, data.bonus->tier );
        bool is_enabled_4p = data.bonus->bonus == 4 && enable_4_set;
        bool is_disabled_4p = data.bonus->bonus == 4 && disable_4_set;

        bool is_enabled = is_enabled_2p || is_enabled_4p;

        if ( is_equippable && ( is_overridden || is_in_range || is_enabled ) )
        {
          if ( is_disabled_2p || is_disabled_4p )
            data.enabled = false;
          else
          {
            data.spell = actor->find_spell( data.bonus->spell_id );
            data.enabled = true;
          }
        }
      }
    }
  }

  actor->sim->print_debug( "Initialized set bonus: {}", *this );
}

void set_bonus_t::parse_set_bonus_string()
{
  auto do_error = [ this ]( std::string_view value ) {
    throw sc_invalid_player_argument(
      fmt::format( "Invalid 'set_bonus' option '{}', available options: {}", value, generate_set_bonus_options() ) );
  };

  auto split = util::string_split<std::string_view>( actor->set_bonus_str, "/" );

  for ( auto value : split )
  {
    set_bonus_type_e tier = SET_BONUS_NONE;
    set_bonus_e bonus = B_NONE;
    bool enabled = false;
    specialization_e spec = SPEC_NONE;
    hero_tree_e hero = HERO_NONE;

    if ( parse_set_bonus_option_verbose( value, tier, bonus, enabled, spec, hero ) )
    {
      set_bonus_spec_data[ tier ][ dbc::composite_idx( spec, hero, actor->dbc->ptr ) ][ bonus ].overridden = enabled;
      continue;
    }

    auto set_bonus_split = util::string_split<std::string_view>( value, "=" );

    if ( set_bonus_split.size() != 2 )
    {
      do_error( value );
      return;
    }

    int opt_val = util::to_int( set_bonus_split[ 1 ] );
    if ( opt_val != 0 && opt_val != 1 )
    {
      do_error( value );
      return;
    }

    if ( !parse_set_bonus_option( set_bonus_split[ 0 ], tier, bonus, hero ) )
    {
      do_error( value );
      return;
    }

    int _idx = 0;

    if ( hero != HERO_NONE )
      _idx = dbc::composite_idx( spec, hero, actor->dbc->ptr );
    else
      _idx = dbc::spec_idx( actor->specialization(), actor->dbc->ptr );

    set_bonus_spec_data[ tier ][ _idx ][ bonus ].overridden = opt_val;
  }
}

void set_bonus_t::enable_all_sets()
{
  auto set_bonuses = item_set_bonus_t::data( actor->dbc->ptr );
  auto hero_tree_ids = trait_data_t::get_valid_hero_tree_ids( actor->_spec, actor->dbc->ptr );

  // assume class & spec matching bonuses are tier or hero tree is available to spec
  for ( const auto& bonus : set_bonuses )
  {
    bool has_class = bonus.class_id == -1 || bonus.class_id == util::class_id( actor->type );
    bool has_spec  = bonus.spec == -1 || bonus.spec == static_cast<int>( actor->_spec );
    bool has_trait_sub_tree =
      bonus.trait_sub_tree == -1 || range::contains( hero_tree_ids, as<unsigned>( bonus.trait_sub_tree ) );

    if ( has_class && has_spec && has_trait_sub_tree )
    {
      set_bonus_spec_data[ bonus.enum_id ][ composite_idx( bonus ) ][ bonus.bonus - 1 ].overridden = 1;
    }
  }
}

void set_bonus_t::enable_set_bonus( specialization_e spec, set_bonus_type_e tier, set_bonus_e bonus, bool quiet )
{
  if ( spec_idx( spec ) < 0 )
    return;

  auto& entry = set_bonus_spec_data[ tier ][ spec_idx( spec ) ][ bonus ];

  entry.enabled = true;
  entry.spell = actor->find_spell( entry.bonus->spell_id );
  entry.quiet = quiet;
}

void set_bonus_t::enable_set_bonus( hero_tree_e hero, set_bonus_type_e tier, set_bonus_e bonus, bool quiet )
{
  if ( hero_idx( hero ) < 0 )
    return;

  auto& entry = set_bonus_spec_data[ tier ][ actor->dbc->specialization_max_per_class() + hero_idx( hero ) ][ bonus ];

  entry.enabled = true;
  entry.spell = actor->find_spell( entry.bonus->spell_id );
  entry.quiet = quiet;
}

bool set_bonus_t::has_set_bonus( specialization_e spec, set_bonus_type_e tier, set_bonus_e bonus ) const
{
  if ( spec_idx( spec ) < 0 )
    return false;

  return set_bonus_spec_data[ tier ][ spec_idx( spec ) ][ bonus ].enabled;
}

bool set_bonus_t::has_set_bonus( hero_tree_e hero, set_bonus_type_e tier, set_bonus_e bonus ) const
{
  if ( hero_idx( hero ) < 0 )
    return false;

  return set_bonus_spec_data[ tier ][ actor->dbc->specialization_max_per_class() + hero_idx( hero ) ][ bonus ].enabled;
}

std::string set_bonus_t::to_string() const
{
  return fmt::format( "{}", *this );
}

void sc_format_to( const set_bonus_t& sb, fmt::format_context::iterator out )
{
  int i = 0;
  for ( size_t idx = 0; idx < sb.set_bonus_spec_data.size(); idx++ )
  {
    for ( size_t spec_idx = 0; spec_idx < sb.set_bonus_spec_data[ idx ].size(); spec_idx++ )
    {
      for ( size_t bonus_idx = 0; bonus_idx < sb.set_bonus_spec_data[ idx ][ spec_idx ].size(); bonus_idx++ )
      {
        auto& data = sb.set_bonus_spec_data[ idx ][ spec_idx ][ bonus_idx ];
        if ( data.bonus == nullptr )
          continue;

        // sanely reporting the active subtree is nonfeasible without a lot
        // of additional code. we'll just omit this for now.

        if ( data.overridden >= 1 ||
             ( data.overridden == -1 && sb.set_bonus_spec_count[ idx ][ spec_idx ] >= data.bonus->bonus ) )
        {
          fmt::format_to( out, "{}{{ {}, {}, {}, {} piece bonus {} }}", i > 0 ? ", " : "", data.bonus->set_name,
                          data.bonus->set_opt_name, util::specialization_string( sb.actor->specialization() ),
                          data.bonus->bonus, ( data.overridden >= 1 ) ? " (overridden)" : "" );
          ++i;
        }
      }
    }
  }
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
        if ( data.bonus == nullptr )
          continue;

        unsigned spec_role_idx = static_cast<int>( spec_idx );

        if ( data.overridden == -1 && set_bonus_spec_count[ idx ][ spec_role_idx ] < data.bonus->bonus )
          continue;

        if ( data.bonus->trait_sub_tree == -1 )
          s += fmt::format( "# set_bonus={}_{}pc=1{}", data.bonus->set_opt_name, data.bonus->bonus, newline );
        else
        {
          std::string ht =
            util::tokenize_fn( trait_data_t::get_hero_tree_name( data.bonus->trait_sub_tree, actor->dbc->ptr ) );

          s += fmt::format( "# set_bonus=name={},pc={},hero_tree={},enable=1{}", data.bonus->set_opt_name,
                            data.bonus->bonus, ht, newline );
        }
      }
    }
  }

  return s;
}

std::unique_ptr<expr_t> set_bonus_t::create_expression( const player_t*, util::string_view type )
{
  int spec_id = spec_idx( actor->specialization() );
  if ( spec_id < 0 )
    return expr_t::create_constant( type, 0.0 );

  set_bonus_type_e tier = SET_BONUS_NONE;
  set_bonus_e bonus = B_NONE;
  hero_tree_e hero = HERO_NONE;

  if ( !parse_set_bonus_option( type, tier, bonus, hero ) )
  {
    throw std::invalid_argument( fmt::format( "Cannot parse set bonus '{}'.", type ) );
  }

  bool state = false;

  if ( tier != SET_BONUS_NONE )
  {
    for ( const auto& bonus_type : set_bonus_spec_data[ tier ] )
    {
      if ( bonus_type[ bonus ].spell->id() > 0 &&
           ( hero == HERO_NONE || bonus_type[ bonus ].bonus->trait_sub_tree == static_cast<int>( hero ) ) )
      {
        state = true;
        break;
      }
    }
  }

  return expr_t::create_constant( type, static_cast<double>( state ) );
}

bool set_bonus_t::parse_set_bonus_option( util::string_view opt_str, set_bonus_type_e& tier, set_bonus_e& bonus,
                                          hero_tree_e& hero )
{
  tier = SET_BONUS_NONE;
  bonus = B_NONE;
  hero = HERO_NONE;

  auto split = util::string_split<util::string_view>( opt_str, "_" );
  if ( split.size() < 2 )
    return false;

  auto bonus_offset = split.back().find( "pc" );
  if ( bonus_offset == std::string::npos )
    return false;

  auto bonus_value = util::to_unsigned( split.back().substr( 0, bonus_offset ) );
  if ( bonus_value > B_MAX )
    return false;

  bonus = static_cast<set_bonus_e>( bonus_value - 1 );

  auto set_name_long = opt_str.substr( 0, opt_str.size() - split.back().size() - 1 );
  auto set_name_short = std::string( split.front() );
  set_bonus_type_e last_tier_set = SET_BONUS_NONE;

  auto set_bonuses = item_set_bonus_t::data( actor->dbc->ptr );

  // special handling for 'latest_#pc'
  if ( util::str_compare_ci( set_name_short, "latest" ) )
  {
    // assume __set_bonus_data is in chronological order
    for ( auto it = set_bonuses.rbegin(); it != set_bonuses.rend(); it++ )
    {
      // assume sets with 4pc bonus & at least 5 items are tier sets
      if ( it->bonus == 4 )
      {
        size_t i = 0;
        while ( it->item_ids[ i ] )
          i++;

        if ( i >= 5 )
        {
          set_name_short = it->tier;
          break;
        }
      }
    }
  }

  for ( const auto& bonus : set_bonuses )
  {
    if ( bonus.class_id != -1 && bonus.class_id != util::class_id( actor->type ) )
      continue;

    // process hero sets
    if ( bonus.trait_sub_tree != -1 && util::str_compare_ci( set_name_short, bonus.tier ) )
    {
      // we at least have the correct tier name, so it is a valid set_bonus expresssion
      tier = static_cast<set_bonus_type_e>( bonus.enum_id );

      // check standard syntax `<tier>_#pc` against player's hero tree
      if ( split.size() == 2 && actor->player_sub_trees.count( bonus.trait_sub_tree ) )
      {
        hero = static_cast<hero_tree_e>( bonus.trait_sub_tree );
        return true;
      }
      // check hero set syntax `<tier>_<tokenized hero tree>_#pc`
      else if ( split.size() >= 3 &&
                trait_data_t::is_hero_tree_valid( static_cast<hero_tree_e>( bonus.trait_sub_tree ), actor->_spec,
                                                  actor->dbc->ptr ) )
      {
        auto hero_name =
          opt_str.substr( set_name_short.size() + 1, opt_str.size() - split.back().size() - set_name_short.size() - 2 );
        int hero_tree_id = trait_data_t::get_hero_tree_id( hero_name, actor->dbc->ptr );

        if ( bonus.trait_sub_tree == hero_tree_id )
        {
          hero = static_cast<hero_tree_e>( hero_tree_id );
          return true;
        }
      }
    }
    // process normal sets
    else if ( util::str_compare_ci( set_name_long, bonus.set_opt_name ) ||
              util::str_compare_ci( set_name_long, bonus.tier ) )
    {
      tier = static_cast<set_bonus_type_e>( bonus.enum_id );
      return true;
    }
  }

  // Exception for if you have a valid tier, the tier requires hero talents, but you have no hero trees at all. This
  // will handle set_bonus.<Tier>_#pc apl expressions when the actor has no hero tree.
  if ( split.size() == 2 && tier != SET_BONUS_NONE && bonus != B_NONE && hero == HERO_NONE &&
       actor->player_sub_trees.empty() )
  {
    // return true as it is a valid set, but unset tier
    tier = SET_BONUS_NONE;
    return true;
  }

  tier = SET_BONUS_NONE;
  return false;
}

bool set_bonus_t::parse_set_bonus_option_verbose( util::string_view opt_str, set_bonus_type_e& tier, set_bonus_e& bonus,
                                                  bool& enabled, specialization_e& spec, hero_tree_e& hero )
{
  tier = SET_BONUS_NONE;
  bonus     = B_NONE;
  enabled   = false;
  spec      = SPEC_NONE;
  hero      = HERO_NONE;

  auto params = util::string_split<util::string_view>( opt_str, "," );

  util::string_view set_name = "";
  for ( util::string_view param : params )
  {
    auto value_pair = util::string_split<util::string_view>( param, "=" );
    if ( util::str_compare_ci( value_pair[ 0 ], "name" ) )
    {
      set_name = value_pair[ 1 ];
      continue;
    }
    if ( util::str_compare_ci( value_pair[ 0 ], "pc" ) )
    {
      unsigned bonus_index = util::to_unsigned( value_pair[ 1 ] );
      if ( bonus_index > B_MAX )
        return false;
      bonus = static_cast<set_bonus_e>( bonus_index - 1 );
      continue;
    }
    if ( util::str_compare_ci( value_pair[ 0 ], "spec" ) )
    {
      spec = util::parse_specialization_type( value_pair[ 1 ] );
      if ( spec == SPEC_NONE )
        return false;
      continue;
    }
    if ( util::str_compare_ci( value_pair[ 0 ], "hero_tree" ) )
    {
      hero = static_cast<hero_tree_e>( trait_data_t::get_hero_tree_id( value_pair[ 1 ], actor->dbc->ptr ) );
      bool is_valid = false;

      if ( hero != HERO_NONE )
        is_valid = trait_data_t::is_hero_tree_valid( hero, actor->_spec, actor->dbc->ptr );

      if ( !is_valid && hero != HERO_NONE )
        actor->sim->error( "Invalid hero tree set bonus '{}' for '{}'.", value_pair[ 1 ],
                           util::specialization_string( actor->_spec ) );

      if ( hero == HERO_NONE || !is_valid )
        return false;

      continue;
    }
    if ( util::str_compare_ci( value_pair[ 0 ], "0" ) || util::str_compare_ci( value_pair[ 0 ], "1" ) )
      if ( int val = util::to_int( value_pair[ 0 ] ); val == 1 || val == 0 )
      {
        enabled = val;
        continue;
      }
    if ( util::str_compare_ci( value_pair[ 0 ], "enable" ) &&
         ( util::str_compare_ci( value_pair[ 1 ], "0" ) || util::str_compare_ci( value_pair[ 1 ], "1" ) ) )
      if ( int val = util::to_int( value_pair[ 1 ] ); val == 1 || val == 0 )
      {
        enabled = val;
        continue;
      }
    return false;
  }

  auto set_bonuses = item_set_bonus_t::data( actor->dbc->ptr );
  for ( const auto& bonus : set_bonuses )
    if ( util::str_compare_ci( set_name, bonus.set_opt_name ) || util::str_compare_ci( set_name, bonus.tier ) )
      tier = static_cast<set_bonus_type_e>( bonus.enum_id );

  return tier != SET_BONUS_NONE && bonus != B_NONE;
}

std::string set_bonus_t::generate_set_bonus_options() const
{
  std::vector<std::string> opts;

  auto set_bonuses = item_set_bonus_t::data( actor->dbc->ptr );

  for ( const auto& bonus : set_bonuses )
  {
    std::string opt;

    if ( bonus.trait_sub_tree == -1 )
    {
      opt = fmt::format( "{}_{}pc", bonus.set_opt_name, bonus.bonus );
    }
    else if ( trait_data_t::is_hero_tree_valid( static_cast<hero_tree_e>( bonus.trait_sub_tree ), actor->_spec,
                                                actor->dbc->ptr ) )
    {
      opt = util::tokenize_fn( fmt::format( "{}_{}_{}pc", bonus.tier,
                                            trait_data_t::get_hero_tree_name( bonus.trait_sub_tree, actor->dbc->ptr ),
                                            bonus.bonus ) );
    }
    else
    {
      continue;
    }

    if ( std::find( opts.begin(), opts.end(), opt ) == opts.end() )
    {
      opts.push_back( opt );
    }
  }

  return util::string_join( opts, ", " );
}
