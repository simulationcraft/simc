// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "set_bonus.hpp"
#include "dbc/dbc.hpp"
#include "dbc/item_set_bonus.hpp"
#include "item/item.hpp"
#include "player/sc_player.hpp"
#include "sim/sc_expressions.hpp"
#include "sim/sc_sim.hpp"

set_bonus_t::set_bonus_t( player_t* player ) :
  actor( player ),
  set_bonus_spec_data( SET_BONUS_MAX ),
  set_bonus_spec_count( SET_BONUS_MAX )
{
  for ( size_t i = 0; i < set_bonus_spec_data.size(); i++ )
  {
    set_bonus_spec_data[ i ].resize( actor->dbc->specialization_max_per_class() );
    set_bonus_spec_count[ i ].resize( actor->dbc->specialization_max_per_class() );
    // For now only 2, and 4 set bonuses
    for ( size_t j = 0; j < set_bonus_spec_data[ i ].size(); j++ )
    {
      set_bonus_spec_data[ i ][ j ].resize( N_BONUSES, set_bonus_data_t(spell_data_t::not_found()) );
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
    // All specs share the same set bonus
    if ( bonus.spec == -1 )
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
    // Note, Spec specific set bonuses are allowed to override "generic" bonuses (bonus.spec == -1)
    else
    {
      assert( bonus.spec > 0 );
      specialization_e spec = static_cast< specialization_e >( bonus.spec );

      set_bonus_spec_data[ bonus.enum_id ][ dbc::spec_idx( spec ) ][ bonus.bonus - 1 ].bonus = &bonus;
    }
  }
}

// Initialize set bonus counts based on the items of the actor
void set_bonus_t::initialize_items()
{
  // Don't allow 2 worn items of the same id to count as 2 slots
  std::vector<unsigned> item_ids;

  for ( auto& item : actor->items)
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

      if ( bonus.class_id != -1 && ! bonus.has_spec( static_cast<int>( actor->_spec ) ) )
        continue;

      if ( range::find( item_ids, item.parsed.data.id ) != item_ids.end() )
      {
        continue;
      }

      // T17+ and PVP is spec specific, T16 and lower is "role specific"
      set_bonus_spec_count[ bonus.enum_id ][ dbc::spec_idx( actor->_spec ) ]++;
      item_ids.push_back( item.parsed.data.id );
      break;
    }
  }
}

std::vector<const item_set_bonus_t*> set_bonus_t::enabled_set_bonus_data() const
{
  std::vector<const item_set_bonus_t*> bonuses;

  // Disable all set bonuses, and set bonus options if challenge mode is set
  if ( actor->sim->challenge_mode == 1 )
    return bonuses;

  if ( actor->sim->disable_set_bonuses == 1 ) // Or if global disable set bonus override is used.
    return bonuses;

  for ( auto& spec_data : set_bonus_spec_data )
  {
    for ( auto& bonus_type : spec_data )
    {
      for ( auto& bonus_data : bonus_type )
      {
        // Most specs have the fourth specialization empty, or only have
        // limited number of roles, so there's no set bonuses for those entries
        if ( bonus_data.bonus == nullptr )
          continue;

        if ( bonus_data.spell->id() == 0 )
        {
          continue;
        }

        bonuses.push_back( bonus_data.bonus );
      }
    }
  }

  return bonuses;
}

// Fast accessor to a set bonus spell, returns the spell, or spell_data_t::not_found()

const spell_data_t* set_bonus_t::set(specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus) const
{
  if ( dbc::spec_idx(spec) < 0 )
  {
    return spell_data_t::nil();
  }
#ifndef NDEBUG
  assert(set_bonus_spec_data.size() > (unsigned)set_bonus);
  assert(set_bonus_spec_data[set_bonus].size() > (unsigned)dbc::spec_idx(spec));
  assert(set_bonus_spec_data[set_bonus][dbc::spec_idx(spec)].size() > (unsigned)bonus);
#endif
  return set_bonus_spec_data[set_bonus][dbc::spec_idx(spec)][bonus].spell;
}

void set_bonus_t::initialize()
{
  // Disable all set bonuses, and set bonus options if challenge mode is set
  if ( actor->sim->challenge_mode == 1 )
    return;

  if ( actor->sim->disable_set_bonuses == 1 ) // Or if global disable set bonus override is used.
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
        if ( data.bonus == nullptr )
          continue;

        unsigned spec_role_idx = static_cast<int>( spec_idx );

        // Set bonus is overridden, or we have sufficient number of items to enable the bonus
        if ( data.overridden >= 1 ||
          ( set_bonus_spec_count[idx][spec_role_idx] >= data.bonus->bonus && data.overridden == -1 ) ||
          ( data.bonus->has_spec( actor->_spec ) &&
          ( ! util::str_in_str_ci( data.bonus->set_opt_name, "lfr" ) &&
          ( ( actor->sim->enable_2_set == data.bonus->tier && data.bonus->bonus == 2 ) ||
          ( actor->sim->enable_4_set == data.bonus->tier && data.bonus->bonus == 4 ) ) ) ) )
        {
          if ( data.bonus->bonus == 2 )
          {
            if ( actor->sim->disable_2_set != data.bonus->tier )
            {
              data.spell = actor->find_spell( data.bonus->spell_id );
              data.enabled = true;
            }
            else
            {
              data.enabled = false;
            }
          }
          else if ( data.bonus->bonus == 4 )
          {
            if ( actor->sim->disable_4_set != data.bonus->tier )
            {
              data.spell = actor->find_spell( data.bonus->spell_id );
              data.enabled = true;
            }
            else
            {
              data.enabled = false;
            }
          }
          else
          {
            data.spell = actor->find_spell( data.bonus->spell_id );
            data.enabled = true;
          }
        }
      }
    }
  }

  actor->sim->print_debug("Initialized set bonus: {}", *this);
}

bool set_bonus_t::has_set_bonus(specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus) const
{
  if ( dbc::spec_idx(spec) < 0 )
  {
    return false;
  }

  return set_bonus_spec_data[set_bonus][dbc::spec_idx(spec)][bonus].enabled;
}

std::string set_bonus_t::to_string() const
{
  return fmt::format( "{}", *this );
}


void format_to( const set_bonus_t& sb, fmt::format_context::iterator out )
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

        unsigned spec_role_idx = static_cast<int>( spec_idx );

        if ( data.overridden >= 1 ||
           ( data.overridden == -1 && sb.set_bonus_spec_count[ idx ][ spec_role_idx ] >= data.bonus->bonus ) )
        {
          fmt::format_to( out, "{}{{ {}, {}, {}, {} piece bonus {} }}",
              i > 0 ? ", " : "",
              data.bonus->set_name,
              data.bonus->set_opt_name,
              util::specialization_string( sb.actor->specialization() ),
              data.bonus->bonus,
              ( data.overridden >= 1 ) ? " (overridden)" : "");
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

        if ( data.overridden >= 1 ||
           ( data.overridden == -1 && set_bonus_spec_count[ idx ][ spec_role_idx ] >= data.bonus->bonus ) )
        {
          s += fmt::format("# set_bonus={}_{}pc=1{}",
              data.bonus->set_opt_name,
              data.bonus->bonus,
              newline);
        }
      }
    }
  }

  return s;
}

std::unique_ptr<expr_t> set_bonus_t::create_expression( const player_t* , util::string_view type )
{
  set_bonus_type_e set_bonus = SET_BONUS_NONE;
  set_bonus_e bonus = B_NONE;

  if ( ! parse_set_bonus_option( type, set_bonus, bonus ) )
  {
    throw std::invalid_argument(fmt::format("Cannot parse set bonus '{}'.", type));
  }

  bool state = set_bonus_spec_data[ set_bonus ][ dbc::spec_idx( actor->specialization() ) ][ bonus ].spell->id() > 0;

  return expr_t::create_constant( type, static_cast<double>(state) );
}

bool set_bonus_t::parse_set_bonus_option( util::string_view opt_str,
                                          set_bonus_type_e& set_bonus,
                                          set_bonus_e& bonus )
{
  set_bonus = SET_BONUS_NONE;
  bonus = B_NONE;

  auto split = util::string_split<util::string_view>( opt_str, "_" );
  if ( split.size() < 2 )
  {
    return false;
  }

  auto bonus_offset = split.back().find( "pc" );
  if ( bonus_offset == std::string::npos )
  {
    return false;
  }

  auto b = util::to_unsigned( split.back().substr( 0, bonus_offset ) );
  if ( b > B_MAX )
  {
    return false;
  }
  bonus = static_cast<set_bonus_e>( b - 1 );

  auto set_name = opt_str.substr( 0, opt_str.size() - split.back().size() - 1 );

  auto set_bonuses = item_set_bonus_t::data( SC_USE_PTR );

  for ( const auto& bonus : set_bonuses )
  {
    if ( bonus.class_id != -1 && bonus.class_id != util::class_id( actor->type ) )
      continue;

    if ( set_bonus == SET_BONUS_NONE && util::str_compare_ci( set_name, bonus.set_opt_name ) )
    {
      set_bonus = static_cast< set_bonus_type_e >( bonus.enum_id );
      break;
    }
  }

  return set_bonus != SET_BONUS_NONE && bonus != B_NONE;
}

std::string set_bonus_t::generate_set_bonus_options() const
{
  std::vector<std::string> opts;

  auto set_bonuses = item_set_bonus_t::data( SC_USE_PTR );

  for ( const auto& bonus : set_bonuses )
  {
    std::string opt = fmt::format( "{}_{}pc", bonus.set_opt_name, bonus.bonus );

    if ( std::find( opts.begin(), opts.end(), opt ) == opts.end() )
    {
      opts.push_back(opt);
    }
  }

  return util::string_join( opts, ", " );
}

