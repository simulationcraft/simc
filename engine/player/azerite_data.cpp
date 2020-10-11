#include "azerite_data.hpp"

#include "simulationcraft.hpp"
#include "util/static_map.hpp"

namespace
{
struct azerite_essence_major_t : public spell_t
{
  azerite_essence_t essence;

  azerite_essence_major_t( player_t* p, const std::string& name, const spell_data_t* spell ) :
    spell_t( name, p, spell ), essence( p->find_azerite_essence( spell->essence_id() ) )
  {
    background = !essence.enabled() || !essence.is_major();
    item = &( player->items[ SLOT_NECK ] );
  }
};
} // Namespace anonymous ends

azerite_power_t::azerite_power_t() :
  m_player( nullptr ), m_spell( spell_data_t::not_found() ), m_data( nullptr )
{ }

azerite_power_t::azerite_power_t( const player_t* p, const azerite_power_entry_t* data,
    const std::vector<const item_t*>& items ) :
  m_player( p ), m_spell( p->find_spell( data->spell_id ) ), m_data( data )
{
  range::for_each( items, [ this ]( const item_t* item ) {
    if ( item->item_level() > 0 && item->item_level() <= MAX_ILEVEL )
    {
      m_ilevels.push_back( item -> item_level() );
    }
  } );
}

azerite_power_t::azerite_power_t( const player_t* p, const azerite_power_entry_t* data,
    const std::vector<unsigned>& ilevels ) :
  m_player( p ), m_spell( p->find_spell( data->spell_id ) ), m_ilevels( ilevels ), m_data( data )
{ }

azerite_power_t::operator const spell_data_t*() const
{ return m_spell; }

const spell_data_t* azerite_power_t::spell() const
{ return m_spell; }

const spell_data_t& azerite_power_t::spell_ref() const
{ return *m_spell; }

bool azerite_power_t::ok() const
{ return m_spell -> ok(); }

bool azerite_power_t::enabled() const
{ return ok(); }

const azerite_power_entry_t* azerite_power_t::data() const
{ return m_data; }

double azerite_power_t::value( size_t index ) const
{
  if ( index > m_spell -> effect_count() || index == 0 )
  {
    return 0.0;
  }

  if ( m_value.size() >= index &&
       m_value[ index - 1 ] != std::numeric_limits<double>::lowest() )
  {
    return m_value[ index - 1 ];
  }

  double sum = 0.0;
  auto budgets = budget();
  for ( size_t budget_index = 0, end = budgets.size(); budget_index < end; ++budget_index )
  {
    auto budget = budgets[ budget_index ];
    auto value = floor( m_spell -> effectN( index ).m_coefficient() * budget + 0.5 );

    unsigned actual_level = m_ilevels[ budget_index ];
    if ( m_spell->max_scaling_level() > 0 && m_spell->max_scaling_level() < actual_level )
    {
      actual_level = m_spell -> max_scaling_level();
    }

    // Apply combat rating penalties consistently
    if ( m_spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 ||
         check_combat_rating_penalty( index ) )
    {
      value = item_database::apply_combat_rating_multiplier( m_player,
          CR_MULTIPLIER_ARMOR, actual_level, value ) + 0.5;
    }

    // TODO: Is this floored, or allowed to accumulate with fractions (and floored at the end?)
    sum += floor( value );
  }

  if ( m_value.size() < index )
  {
    m_value.resize( index, std::numeric_limits<double>::lowest() );
  }

  m_value[ index - 1 ] = sum;

  return sum;
}

double azerite_power_t::value( const azerite_value_fn_t& fn ) const
{
  return fn( *this );
}

double azerite_power_t::percent( size_t index ) const
{ return value( index ) * .01; }

timespan_t azerite_power_t::time_value( size_t index, time_type tt ) const
{
  switch ( tt )
  {
    case time_type::S:
      return timespan_t::from_seconds( value( index ) );
    case time_type::MS:
      return timespan_t::from_millis( value( index ) );
    // Should be never reached
    default:
      return timespan_t::zero();
  }
}

std::vector<double> azerite_power_t::budget() const
{ return budget( m_spell ); }

std::vector<double> azerite_power_t::budget( const spell_data_t* scaling_spell ) const
{
  std::vector<double> b;

  if ( m_player == nullptr )
  {
    return {};
  }

  range::for_each( m_ilevels, [ & ]( unsigned ilevel ) {
    if ( ilevel < 1 || ilevel > MAX_ILEVEL )
    {
      return;
    }

    unsigned min_ilevel = ilevel;
    if ( scaling_spell->max_scaling_level() > 0 && scaling_spell->max_scaling_level() < min_ilevel )
    {
      min_ilevel = scaling_spell->max_scaling_level();
    }

    auto budget = item_database::item_budget( m_player, min_ilevel );
    if ( scaling_spell->scaling_class() == PLAYER_SPECIAL_SCALE8 )
    {
      const auto& props = m_player->dbc->random_property( min_ilevel );
      budget = props.damage_replace_stat;
    }
    b.push_back( budget );
  } );

  return b;
}

const std::vector<unsigned> azerite_power_t::ilevels() const
{ return m_ilevels; }

unsigned azerite_power_t::n_items() const
{ return as<unsigned>( m_ilevels.size() ); }

// Currently understood rules for the detection, since we don't know how to see it from spell data
// 1) Spell must use scaling type -1
// 2) Effect must be of type "apply combat rating"
// 3) Effect must be a dynamically scaling one (average coefficient > 0)
bool azerite_power_t::check_combat_rating_penalty( size_t index ) const
{
  if ( index == 0 || index > m_spell->effect_count() )
  {
    return false;
  }

  if ( m_spell->scaling_class() != PLAYER_SPECIAL_SCALE )
  {
    return false;
  }

  return m_spell->effectN( index ).subtype() == A_MOD_RATING &&
         m_spell->effectN( index ).m_coefficient() != 0;
}

azerite_essence_t::azerite_essence_t() :
  m_player( nullptr ), m_essence( nullptr ), m_rank( 0 ), m_type( essence_type::INVALID )
{ }

azerite_essence_t::azerite_essence_t( const player_t* p ) :
  m_player( p ), m_essence( nullptr ), m_rank( 0 ), m_type( essence_type::INVALID )
{ }

azerite_essence_t::azerite_essence_t( const player_t* player, essence_type type, unsigned rank,
    const azerite_essence_entry_t* essence ) :
  m_player( player ), m_essence( essence ), m_rank( rank ), m_type( type )
{
  auto entries = azerite_essence_power_entry_t::data_by_essence_id( essence->id, player->dbc->ptr );
  range::for_each( entries, [ this ]( const azerite_essence_power_entry_t& entry ) {
    m_base_major.push_back( m_player->find_spell( entry.spell_id_base[ 0 ] ) );
    m_base_minor.push_back( m_player->find_spell( entry.spell_id_base[ 1 ] ) );
    m_upgrade_major.push_back( m_player->find_spell( entry.spell_id_upgrade[ 0 ] ) );
    m_upgrade_minor.push_back( m_player->find_spell( entry.spell_id_upgrade[ 1 ] ) );
  } );
}

azerite_essence_t::azerite_essence_t( const player_t* player, const spell_data_t* passive ) :
  m_player( player ), m_essence( nullptr ), m_rank( 1u ), m_type( essence_type::PASSIVE )
{
  // Store the passive into first slot of major spells
  m_base_major.push_back( passive );
}

const item_t* azerite_essence_t::item() const
{
  if ( m_player->items.size() > static_cast<size_t>( SLOT_NECK ) )
  {
    return &( m_player->items[ SLOT_NECK ] );
  }
  else
  {
    return nullptr;
  }
}

bool azerite_essence_t::enabled() const
{ return item() && item()->parsed.data.id == 158075 && m_type != essence_type::INVALID; }

const spell_data_t* azerite_essence_t::spell( unsigned rank, essence_type type ) const
{
  return spell( rank, essence_spell::BASE, type );
}

const spell_data_t* azerite_essence_t::spell( unsigned rank, essence_spell spell, essence_type type ) const
{
  // Passive essence. Note, returns nullptr if someone explicitly asks for a passive spell on a
  // minor/major essence
  if ( ( type == essence_type::INVALID && m_type == essence_type::PASSIVE ) ||
       type == essence_type::PASSIVE )
  {
    return m_base_major.size() ? m_base_major.front(): spell_data_t::not_found();
  }
  // Major or Minor essence
  else
  {
    if ( m_type == essence_type::INVALID )
    {
      return spell_data_t::not_found();
    }

    if ( rank == 0 )
    {
      return spell_data_t::not_found();
    }

    if ( rank - 1 >= m_base_major.size() )
    {
      return spell_data_t::not_found();
    }

    if ( rank > m_rank )
    {
      return spell_data_t::not_found();
    }

    // Figure out spell type requested
    auto t = type == essence_type::INVALID ? m_type : type;

    if ( t == essence_type::MAJOR )
    {
      return spell == essence_spell::BASE ? m_base_major[ rank - 1 ] : m_upgrade_major[ rank - 1 ];
    }
    else
    {
      return spell == essence_spell::BASE ? m_base_minor[ rank - 1 ] : m_upgrade_minor[ rank - 1 ];
    }
  }
}

const spell_data_t& azerite_essence_t::spell_ref( unsigned rank, essence_type type ) const
{
  return spell_ref( rank, essence_spell::BASE, type );
}

const spell_data_t& azerite_essence_t::spell_ref( unsigned rank, essence_spell spell_, essence_type type ) const
{
  if ( const auto ptr = spell( rank, spell_, type ) )
  {
    return *ptr;
  }

  return *spell_data_t::nil();
}

std::vector<const spell_data_t*> azerite_essence_t::spells( essence_spell spell, essence_type type ) const
{
  std::vector<const spell_data_t*> spells;

  // Passive essence. Note, returns an empty vector if someone explicitly requests a passive essence
  // type on a non-passive essence.
  if ( ( type == essence_type::INVALID && m_type == essence_type::PASSIVE ) ||
       type == essence_type::PASSIVE )
  {
    return m_base_major;
  }
  else
  {
    // Figure out spell type requested
    auto t = type == essence_type::INVALID ? m_type : type;

    if ( t == essence_type::MAJOR )
    {
      return spell == essence_spell::BASE ? m_base_major : m_upgrade_major;
    }
    else
    {
      return spell == essence_spell::BASE ? m_base_minor : m_upgrade_minor;
    }
  }

  return spells;
}

namespace azerite
{
std::unique_ptr<azerite_state_t> create_state( player_t* p )
{
  return std::make_unique<azerite_state_t>( p );
}

void initialize_azerite_powers( player_t* actor )
{
  if ( ! actor -> azerite )
  {
    return;
  }

  for ( auto azerite_spell : actor -> azerite -> enabled_spells() )
  {
    const auto spell = actor -> find_spell( azerite_spell );
    if ( ! spell -> ok() )
    {
      continue;
    }

    special_effect_t effect { actor };
    effect.source = SPECIAL_EFFECT_SOURCE_AZERITE;

    unique_gear::initialize_special_effect( effect, azerite_spell );
    // Note, only apply custom special effects for azerite for an abundance of safety
    if ( ! effect.is_custom() )
    {
      continue;
    }

    actor -> special_effects.push_back( new special_effect_t( effect ) );
  }

  // Update the traversal nodes if they were not defined, based on the azerite level of the neck
  // item
  actor->azerite_essence->update_traversal_nodes();

  for ( auto azerite_essence : actor->azerite_essence->enabled_essences() )
  {
    const auto spell = actor->find_spell( azerite_essence );
    if ( !spell->ok() )
    {
      continue;
    }

    special_effect_t effect { actor };
    effect.source = SPECIAL_EFFECT_SOURCE_AZERITE_ESSENCE;

    unique_gear::initialize_special_effect( effect, azerite_essence );
    // Note, only apply custom special effects for azerite essences for an abundance of safety
    if ( !effect.is_custom() )
    {
      continue;
    }

    actor->special_effects.push_back( new special_effect_t( effect ) );
  }
}

azerite_state_t::azerite_state_t( player_t* p ) : m_player( p )
{ }

void azerite_state_t::initialize()
{
  range::for_each( m_player -> items, [ this ]( const item_t& item ) {
    range::for_each( item.parsed.azerite_ids, [ & ]( unsigned id ) {
      const auto& power = m_player->dbc->azerite_power( id );
      if ( power.id )
      {
        m_items[ id ].push_back( &( item ) );
        m_spell_items[ power.spell_id ].push_back( &( item ) );
      }
    } );
  });
}

azerite_power_t azerite_state_t::get_power( unsigned id )
{
  // All azerite disabled
  if ( m_player -> sim -> azerite_status == azerite_control::DISABLED_ALL )
  {
    return {};
  }

  const auto& power = m_player -> dbc->azerite_power( id );

  auto override_it = m_overrides.find( id );
  // Found an override, use the overridden ilevels instead
  if ( override_it != m_overrides.end() )
  {
    // Explicitly disabled, in this case there's only going to be a single ilevel entry in the
    // overrides, and it will alwyas be 0
    if ( override_it -> second[ 0 ] == 0 )
    {
      if ( m_player -> sim -> debug )
      {
        m_player -> sim -> out_debug.printf( "%s disabling overridden azerite power %s",
            m_player -> name(), power.name );
      }

      return {};
    }
    else
    {
      if ( m_player -> sim -> debug )
      {
        std::stringstream s;
        for ( size_t i = 0; i < override_it -> second.size(); ++i )
        {
          s << override_it -> second[ i ];

          if ( i < override_it -> second.size() - 1 )
          {
            s << ", ";
          }
        }

        m_player -> sim -> out_debug.printf( "%s initializing overridden azerite power %s: "
                                             "ilevels=%s",
            m_player -> name(), power.name, s.str().c_str() );
      }

      return { m_player, &power, override_it -> second };
    }
  }
  else
  {
    // Item-related azerite effects are only enabled when "all" is defined
    if ( m_spell_items[ power.spell_id ].size() > 0 &&
         m_player -> sim -> azerite_status == azerite_control::ENABLED )
    {
      if ( m_player -> sim -> debug )
      {
        std::stringstream s;
        const auto& items = m_spell_items[ power.spell_id ];

        for ( size_t i = 0; i < items.size(); ++i )
        {
          s << items[ i ] -> full_name()
            << " (id=" << items[ i ] -> parsed.data.id
            << ", ilevel=" << items[ i ] -> item_level() << ")";

          if ( i < items.size() - 1 )
          {
            s << ", ";
          }
        }

        m_player -> sim -> out_debug.printf( "%s initializing azerite power %s: %s",
            m_player -> name(), power.name, s.str().c_str() );
      }

      return { m_player, &power, m_spell_items[ power.spell_id ] };
    }
    else
    {
      return {};
    }
  }
}

azerite_power_t azerite_state_t::get_power( util::string_view name, bool tokenized )
{
  const auto& power = m_player -> dbc->azerite_power( name, tokenized );

  if ( power.id == 0 )
  {
    return {};
  }

  return get_power( power.id );
}

void azerite_state_t::copy_overrides( const std::unique_ptr<azerite_state_t>& other )
{
  if ( other.get() == this )
  {
    return;
  }

  m_overrides = other -> m_overrides;
}

std::string azerite_state_t::overrides_str() const
{
  std::vector<std::string> override_strings;

  // Always normalize the user-given azerite_override string into a specific (power id) order
  std::vector<unsigned> powers;
  for ( const auto& e : m_overrides )
  {
    powers.push_back( e.first );
  }

  range::sort( powers );

  // Iterate over the powers in order, grab all overrides into a vector
  range::for_each( powers, [ & ]( unsigned power_id ) {
      const auto& power = m_player -> dbc->azerite_power( power_id );
      auto power_name = util::tokenize_fn( power.name );

      const auto& overrides = m_overrides.at( power_id );
      range::for_each( overrides, [ & ]( unsigned ilevel ) {
        override_strings.push_back( power_name + ":" + util::to_string( ilevel ) );
      } );
  } );

  if ( override_strings.size() == 0 )
  {
    return {};
  }

  std::stringstream s;

  // Construct an overall azerite override string
  for ( size_t i = 0; i < override_strings.size(); ++i )
  {
    s << override_strings[ i ];
    if ( i < override_strings.size() - 1 )
    {
      s << "/";
    }
  }

  return s.str();
}

bool azerite_state_t::parse_override( sim_t* sim, util::string_view, util::string_view value )
{
  m_overrides.clear();

  auto override_split = util::string_split<util::string_view>( value, "/" );
  for ( const util::string_view opt : override_split )
  {
    if ( opt.empty() )
    {
      continue;
    }

    auto opt_split = util::string_split<util::string_view>( opt, ":" );
    if ( opt_split.size() != 2 )
    {
      sim -> error( "{} unknown azerite override string \"{}\"", sim -> active_player -> name(),
          opt);
      return false;
    }

    auto power_str = opt_split[ 0 ];
    unsigned ilevel = util::to_unsigned_ignore_error( opt_split[ 1 ], 0 );

    if ( ilevel > MAX_ILEVEL )
    {
      sim -> errorf( "%s too high ilevel %u for azerite override of \"%s\"",
          sim -> active_player -> name(), ilevel, power_str );
      return false;
    }

    const auto* power = &( m_player->dbc->azerite_power( power_str, true ) );
    // Additionally try with an azerite power id if the name-based lookup fails
    if ( power->id == 0 )
    {
      unsigned power_id = util::to_unsigned_ignore_error( opt_split[ 0 ], 0 );
      if ( power_id > 0 )
      {
        power = &( m_player->dbc->azerite_power( power_id ) );
      }
    }

    if ( power->id == 0 )
    {
      sim -> errorf( "%s unknown azerite power \"%s\" for override",
          sim -> active_player -> name(), power_str );
      return false;
    }

    // Ilevel 0 will disable the azerite power fully, no matter its position in the override
    // string
    if ( ilevel == 0 )
    {
      m_overrides[ power->id ] = { 0 };
    }
    else
    {
      // Make sure there's no existing zero ilevel value for the overrides
      if ( m_overrides[ power->id ].size() == 1 && m_overrides[ power->id ][ 0 ] == 0 )
      {
        continue;
      }

      m_overrides[ power->id ].push_back( ilevel );
    }
  }

  return true;
}

size_t azerite_state_t::rank( unsigned id ) const
{
  // All azerite-related effects disabled
  if ( m_player -> sim -> azerite_status == azerite_control::DISABLED_ALL )
  {
    return 0u;
  }

  auto it = m_overrides.find( id );
  // Override found, figure enable status from it
  if ( it != m_overrides.end() )
  {
    if ( it -> second.size() == 1 && it -> second[ 0 ] == 0 )
    {
      return 0u;
    }

    return it -> second.size();
  }

  // Only look at item-based azerite, if all azerite is enabled
  if ( m_player -> sim -> azerite_status == azerite_control::ENABLED )
  {
    auto it = m_items.find( id );
    if ( it != m_items.end() )
    {
      return it -> second.size();
    }
  }

  // Out of options, it can't be enabled
  return 0u;
}

size_t azerite_state_t::rank( util::string_view name, bool tokenized ) const
{
  const auto& power = m_player -> dbc->azerite_power( name, tokenized );
  if ( power.id == 0 )
  {
    return 0u;
  }

  return rank( power.id );
}

bool azerite_state_t::is_enabled( unsigned id ) const
{
  return rank( id ) > 0;
}

bool azerite_state_t::is_enabled( util::string_view name, bool tokenized ) const
{
  const auto& power = m_player -> dbc->azerite_power( name, tokenized );
  if ( power.id == 0 )
  {
    return false;
  }

  return is_enabled( power.id );
}

std::unique_ptr<expr_t> azerite_state_t::create_expression( util::span<const util::string_view> expr_str ) const
{
  if ( expr_str.size() == 1 )
  {
    return nullptr;
  }

  const auto& power = m_player -> dbc->azerite_power( expr_str[ 1 ], true );
  if ( power.id == 0 )
  {
    throw std::invalid_argument( fmt::format( "Unknown azerite power '{}'.", expr_str[ 1 ] ) );
  }

  if ( util::str_compare_ci( expr_str[ 2 ], "enabled" ) )
  {
    return expr_t::create_constant( "azerite_enabled", as<double>( is_enabled( power.id ) ) );
  }
  else if ( util::str_compare_ci( expr_str[ 2 ], "rank" ) )
  {
    return expr_t::create_constant( "azerite_rank", as<double>( rank( power.id ) ) );
  }
  else if ( util::str_compare_ci( expr_str[ 2 ], "trait_value" ) )
  {
    unsigned idx = 1;

    if ( expr_str.size() > 3 )
    {
      idx = util::to_unsigned( expr_str[ 3 ] );
    }

    return expr_t::create_constant( "azerite_trait_value",
                                    as<double>( m_player->azerite->get_power( power.id ).value( idx) ) );
  }
  else
  {
    throw std::invalid_argument( fmt::format( "Unsupported azerite expression '{}'.", expr_str[ 2 ] ) );
  }

  return nullptr;
}

std::vector<unsigned> azerite_state_t::enabled_spells() const
{
  std::vector<unsigned> spells;

  for ( const auto& entry : m_items )
  {
    if ( ! is_enabled( entry.first ) )
    {
      continue;
    }

    const auto& power = m_player -> dbc->azerite_power( entry.first );
    if ( power.id == 0 )
    {
      continue;
    }

    spells.push_back( power.spell_id );
  }

  for ( const auto& entry : m_overrides )
  {
    if ( ! is_enabled( entry.first ) )
    {
      continue;
    }

    const auto& power = m_player -> dbc->azerite_power( entry.first );
    if ( power.id == 0 )
    {
      continue;
    }

    spells.push_back( power.spell_id );
  }

  range::sort( spells );
  auto it = range::unique( spells );

  spells.erase( it, spells.end() );

  return spells;
}

report::sc_html_stream& azerite_state_t::generate_report( report::sc_html_stream& root ) const
{
  // Heart of Azeroth
  const item_t& hoa = m_player->items[ SLOT_NECK ];

  if ( !hoa.active() || hoa.parsed.data.id != 158075 || m_player->sim->azerite_status == azerite_control::DISABLED_ALL )
    return root;

  size_t n_traits = m_overrides.size();

  for ( auto item : m_items )
    n_traits += rank( item.first );

  if ( n_traits == 0 )
    return root;

  for ( auto slot : { SLOT_HEAD, SLOT_SHOULDERS, SLOT_CHEST } )
  {
    const item_t& item = m_player->items[ slot ];

    if ( item.active() )
    {
      root << "<tr class=\"left\">\n"
           << "<th></th>\n"
           << "<td><ul class=\"float\">\n"
           << "<li>" << report_decorators::decorated_item(item) << "&#160;(" << item.item_level() << ")</li>\n";

      if ( item.parsed.azerite_ids.size() )
      {
        for ( auto id : item.parsed.azerite_ids )
        {
          if ( !is_enabled( id ) )
          {
            root << "<li>Disabled</li>\n";
            continue;
          }

          if ( m_overrides.find( id ) != m_overrides.end() )
          {
            root << "<li>Overridden</li>\n";
            continue;
          }

          auto decorator = report_decorators::decorated_spell_data_item(
            *m_player->sim, m_player->find_spell( m_player->dbc->azerite_power( id ).spell_id ), item );

          root << "<li>" << decorator << "</li>\n";
        }
      }

      root << "</ul></td>\n"
           << "</tr>";
    }
  }

  if ( m_overrides.size() )
  {
    root << "<tr class=\"left\">\n"
         << "<th></th>\n"
         << "<td><ul class=\"float\">\n"
         << "<li>Azerite Overrides:</li>\n";

    for ( auto override : m_overrides )
    {
      for ( auto ilevel : override.second )
      {
        auto decorator = report_decorators::decorated_spell_data(
          *m_player->sim, m_player->find_spell( m_player->dbc->azerite_power( override.first ).spell_id ) );

        if ( !is_enabled( override.first ) )
        {
          root << "<li><del>" << decorator << "</del>&#160;(Disable)</li>\n";
        }
        else
        {
          root << "<li>" << decorator << "&#160;(" << ilevel << ")</li>\n";
        }
      }
    }

    root << "</ul></td>\n"
         << "</tr>";
  }

  return root;
}

report::sc_html_stream& azerite_essence_state_t::generate_report( report::sc_html_stream& root ) const
{
  // Heart of Azeroth
  const item_t& hoa = m_player->items[ SLOT_NECK ];

  if ( !hoa.active() || hoa.parsed.data.id != 158075 )
    return root;

  root << "<tr class=\"left\">\n"
       << "<th>Azerite</th>\n"
       << "<td><ul class=\"float\">\n"
       << "<li>Level:&#160;<strong>" << hoa.parsed.azerite_level << "</strong>&#160;"
       << report_decorators::decorated_item(hoa) << "&#160;(" << hoa.item_level() << ")</li>\n";

  for ( const auto& slot : m_state )
  {
    if ( slot.type() == essence_type::PASSIVE || slot.type() == essence_type::INVALID )
      continue;

    auto essence = get_essence( slot.id() );
    auto decorated = report_decorators::decorated_spell_data_item( *m_player->sim, essence.spell( slot.rank(), slot.type() ), hoa );

    root << "<li>Rank:&#160;<strong>" << slot.rank() << "</strong>&#160;" << decorated;

    if ( essence.is_major() )
    {
      auto decorator2 = report_decorators::decorated_spell_data_item( *m_player->sim, essence.spell( slot.rank(), essence_type::MINOR ), hoa );

      root << " (Major)</li>\n"
           << "<li>Rank:&#160;<strong>" << slot.rank() << "</strong>&#160;" << decorator2;
    }

    root << "</li>\n";
  }

  root << "</ul></td>\n"
       << "</tr>\n";

  return root;
}

std::unique_ptr<azerite_essence_state_t> create_essence_state( player_t* p )
{
  return std::make_unique<azerite_essence_state_t>( p );
}

// These are hardcoded, based on AzeriteItemMilestonePower.db2
void azerite_essence_state_t::update_traversal_nodes()
{
  std::vector<unsigned> passives;
  const auto& neck = m_player->items[ SLOT_NECK ];

  if ( neck.parsed.azerite_level >= 52 )
  {
    passives.push_back( 300573u );
  }

  if ( neck.parsed.azerite_level >= 57 )
  {
    passives.push_back( 300575u );
  }

  if ( neck.parsed.azerite_level >= 62 )
  {
    passives.push_back( 300576u );
  }

  if ( neck.parsed.azerite_level >= 67 )
  {
    passives.push_back( 300577u );
  }

  if ( neck.parsed.azerite_level >= 71 )
  {
    passives.push_back( 312927u );
  }

  if ( neck.parsed.azerite_level >= 80 )
  {
    passives.push_back( 312928u );
  }

  range::for_each( passives, [ this ]( unsigned id ) {
    auto it = range::find_if( m_state, [id]( const slot_state_t& slot ) {
      return id == slot.id() && slot.type() == essence_type::PASSIVE;
    } );

    if ( it == m_state.end() )
    {
      add_essence( essence_type::PASSIVE, id, 1u );
    }
  } );
}

azerite_essence_state_t::azerite_essence_state_t( const player_t* player ) : m_player( player )
{ }

// Get an azerite essence object by name
azerite_essence_t azerite_essence_state_t::get_essence( util::string_view name, bool tokenized ) const
{
  const auto& essence = azerite_essence_entry_t::find( name, tokenized, m_player->dbc->ptr );
  // Could also be a passive spell, so check if the passives logged for the player match this
  for ( size_t i = 0; essence.id == 0 && i < m_state.size(); ++i )
  {
    if ( m_state[ i ].type() != essence_type::PASSIVE )
    {
      continue;
    }
    const auto spell = m_player->find_spell( m_state[ i ].id() );
    if ( tokenized )
    {
      auto name_str = util::tokenize_fn( spell->name_cstr() );
      if ( util::str_compare_ci( name, name_str ) )
      {
        return { m_player, spell };
      }
    }
    else if ( util::str_compare_ci( name, spell->name_cstr() ) )
    {
      return { m_player, spell };
    }
  }

  // No essence found with the name, and no passive essence either, bail out
  if ( essence.id == 0 )
  {
    return { m_player };
  }

  return get_essence( essence.id );
}

azerite_essence_t azerite_essence_state_t::get_essence( unsigned id ) const
{
  const auto& essence = azerite_essence_entry_t::find( id, m_player->dbc->ptr );
  // Could also be a passive spell, so check if the passives for the actor match the id
  for ( size_t i = 0; essence.id == 0 && i < m_state.size(); ++i )
  {
    if ( m_state[ i ].type() != essence_type::PASSIVE )
    {
      continue;
    }

    if ( id == m_state[ i ].id() )
    {
      return { m_player, m_player->find_spell( id ) };
    }
  }

  // No essence found with the name, and no passive essence either, bail out
  if ( essence.id == 0 )
  {
    return { m_player };
  }

  // Find state slot based on id now
  auto it = range::find_if( m_state, [ &essence ]( const slot_state_t& slot ) {
      return slot.id() == essence.id;
  } );

  // Actor does not have the minor or major essence, bail out
  if ( it == m_state.end() )
  {
    return { m_player };
  }

  // Return the essence based on the slot state
  return { m_player, it->type(), it->rank(), &essence };
}

// TODO: Validation
bool azerite_essence_state_t::add_essence( essence_type type, unsigned id, unsigned rank )
{
  m_state.emplace_back( type, id, rank );
  return true;
}


void azerite_essence_state_t::copy_state( const std::unique_ptr<azerite_essence_state_t>& other )
{
  if ( other.get() == this )
  {
    return;
  }

  m_state = other->m_state;
}

// TODO: Once armory import works with essences, adjust
std::string azerite_essence_state_t::option_str() const
{
  if ( !m_option_str.empty() )
  {
    return m_option_str;
  }

  // Return a three-state option set for the essences grabbed from armory
  std::vector<std::string> options;
  range::for_each( m_state, [ &options ]( const slot_state_t& slot ) {
    if ( slot.type() != essence_type::PASSIVE )
    {
      options.push_back( slot.str() );
    }
  } );

  return util::string_join( options, "/" );
}

// Formats:
// token/token/token/...
// Where token:
// essence_id:rank OR essence_id:rank:type OR spell_id
// Where essence_id: AzeriteEssence.db2 id OR tokenized form of the essence name
//       rank: 1..N
//       type: 0 (Minor) OR 1 (Major)
//       spell_id: Passive milestone spell id
//
// Only 2 or 3 element tokens for major/minor powers allowed. If 2 element tokens are used, the
// first element is the major essence, and the two subsequent tokens are the minor essences. In 3
// element tokens, ordering does not matter. Placement of passive milestone spells is irrelevant.
bool azerite_essence_state_t::parse_azerite_essence( sim_t* sim,
                                                     util::string_view /* name */,
                                                     util::string_view value )
{
  // Three or two digit options used?
  bool explicit_type = false;
  int n_parsed_powers = 0;
  bool major_parsed = false;

  auto splits = util::string_split<util::string_view>( value, "/" );

  m_state.clear();

  for ( size_t i = 0u; i < splits.size(); ++i )
  {
    // Split by :
    auto token_split = util::string_split<util::string_view>( splits[ i ], ":" );

    unsigned id = util::to_unsigned_ignore_error( token_split[ 0 ], 0 );
    unsigned rank = 0;
    essence_type type = essence_type::INVALID;

    // Check if the id is "0", and insert a placeholder into the slot state
    if ( token_split[ 0 ].front() == '0' )
    {
      m_state.emplace_back( );
      n_parsed_powers++;
      continue;
    }

    // Parse/Sanitize rank
    if ( token_split.size() > 1 )
    {
      rank = util::to_unsigned( token_split[ 1 ] );
      if ( rank > MAX_AZERITE_ESSENCE_RANK )
      {
        sim->error( "Invalid Azerite Essence rank, expected max of {}, got {}",
          MAX_AZERITE_ESSENCE_RANK, token_split[ 1 ] );
        return false;
      }
    }

    // Parse/Sanitize essence type in 3-element tokens
    if ( token_split.size() > 2 )
    {
      type = token_split[ 2 ].front() == '0'
                          ? essence_type::MINOR
                          : token_split[ 2 ].front() == '1'
                            ? essence_type::MAJOR
                            : essence_type::INVALID;

      if ( type == essence_type::INVALID )
      {
        sim->error( "Invalid Azerite Essence type, expected '0' or '1', got '{}'",
            token_split[ 2 ] );
        return false;
      }
    }

    // For 2 and 3 element tokens, try to find the essence with a tokenized name
    if ( id == 0 && token_split.size() > 1 )
    {
      const auto& essence = azerite_essence_entry_t::find( token_split[ 0 ], true, m_player->dbc->ptr );
      if ( essence.id == 0 )
      {
        sim->error( "Unable to find Azerite Essence with name '{}'", splits[ i ] );
        return false;
      }

      id = essence.id;
    }
    // Verify the essence exists for 2 and 3 element tokens
    else if ( id > 0 && token_split.size() > 1 )
    {
      const auto& essence = azerite_essence_entry_t::find( id, m_player->dbc->ptr );
      if ( essence.id != id )
      {
        sim->error( "Unable to find Azerite Essence with id {}", splits[ i ] );
        return false;
      }
    }

    switch ( token_split.size() )
    {
      // Passive essence power, spell id
      case 1u:
      {
        if ( id == 0 )
        {
          sim->error( "Unable to parse passive Azerite Essence '{}' into a spell id",
            token_split[ 0 ] );
          return false;
        }

        auto spell = m_player->find_spell( id );
        if ( !spell->ok() )
        {
          sim->error( "Unable to find passive Azerite Essence '{}'",
            token_split[ 0 ] );
          return false;
        }

        m_state.emplace_back( essence_type::PASSIVE, id, 1u );
        break;
      }
      // Essence id, rank
      case 2u:
      {
        if ( explicit_type && n_parsed_powers > 0 )
        {
          sim->errorf( "Unable to mix two- and three-element Azerite Essence options together" );
          return false;
        }

        m_state.emplace_back( major_parsed ? essence_type::MINOR : essence_type::MAJOR,
            id, rank );

        if ( !major_parsed )
        {
          major_parsed = true;
        }

        break;
      }
      // Essence id, rank, type
      case 3u:
      {
        if ( major_parsed )
        {
          sim->errorf( "Unable to mix two- and three-element Azerite Essence options together" );
          return false;
        }

        explicit_type = true;

        m_state.emplace_back( type, id, rank );
        break;
      }
      default:
        sim->error( "Invalid token format for '{}'", splits[ i ] );
        return false;
    }

    n_parsed_powers++;
  }

  m_option_str = std::string( value );

  return true;
}

std::unique_ptr<expr_t> azerite_essence_state_t::create_expression( util::span<const util::string_view> expr_str ) const
{
  if ( expr_str.size() <= 2 )
    return nullptr;

  const auto& entry = azerite_essence_entry_t::find( expr_str[ 1 ], true, m_player->dbc->ptr );
  if ( entry.id == 0 )
    throw std::invalid_argument( fmt::format( "Unknown azerite essence '{}'.", expr_str[ 1 ] ) );

  auto essence = get_essence( entry.id );
  if ( util::str_compare_ci( expr_str[ 2 ], "enabled" ) )
    return expr_t::create_constant( "essence_enabled", as<double>( essence.enabled() ) );
  else if ( util::str_compare_ci( expr_str[ 2 ], "rank" ) )
    return expr_t::create_constant( "essence_rank", as<double>( essence.rank() ) );
  else if ( util::str_compare_ci( expr_str[ 2 ], "major" ) )
    return expr_t::create_constant( "essence_major", as<double>( essence.is_major() ) );
  else if ( util::str_compare_ci( expr_str[ 2 ], "minor" ) )
    return expr_t::create_constant( "essence_minor", as<double>( essence.is_minor() ) );
  else
    throw std::invalid_argument( fmt::format( "Unknown azerite essence expression: '{}'.", expr_str[ 2 ] ) );

  return {};
}

std::vector<unsigned> azerite_essence_state_t::enabled_essences() const
{
  std::vector<unsigned> spells;

  // Iterate over slot state, collecting spell information
  range::for_each( m_state, [this, &spells]( const slot_state_t& slot ) {
    auto essence = get_essence( slot.id() );
    // Note, enabled essences will always report the "base" (rank 1) spell
    spells.push_back( essence.spell_ref( 1u, slot.type() ).id() );
    if ( slot.type() == essence_type::MAJOR )
    {
      spells.push_back( essence.spell_ref( 1u, essence_type::MINOR ).id() );
    }
  } );

  return spells;
}

void register_azerite_powers()
{
  unique_gear::register_special_effect( 263962, special_effects::resounding_protection );
  unique_gear::register_special_effect( 263984, special_effects::elemental_whirl       );
  unique_gear::register_special_effect( 264108, special_effects::blood_siphon          );
  unique_gear::register_special_effect( 267665, special_effects::lifespeed             );
  unique_gear::register_special_effect( 267879, special_effects::on_my_way             );
  unique_gear::register_special_effect( 280710, special_effects::champion_of_azeroth   );
  unique_gear::register_special_effect( 279899, special_effects::unstable_flames       );
  unique_gear::register_special_effect( 280380, special_effects::thunderous_blast      );
  unique_gear::register_special_effect( 273834, special_effects::filthy_transfusion    );
  unique_gear::register_special_effect( 280407, special_effects::blood_rite            );
  unique_gear::register_special_effect( 280402, special_effects::tidal_surge           );
  unique_gear::register_special_effect( 263987, special_effects::heed_my_call          );
  unique_gear::register_special_effect( 266936, special_effects::azerite_globules      );
  unique_gear::register_special_effect( 266180, special_effects::overwhelming_power    );
  unique_gear::register_special_effect( 279926, special_effects::earthlink             );
  unique_gear::register_special_effect( 273823, special_effects::wandering_soul        ); // Blightborne Infusion
  unique_gear::register_special_effect( 273150, special_effects::wandering_soul        ); // Ruinous Bolt
  unique_gear::register_special_effect( 280429, special_effects::swirling_sands        );
  unique_gear::register_special_effect( 280579, special_effects::retaliatory_fury      ); // Retaliatory Fury
  unique_gear::register_special_effect( 280624, special_effects::retaliatory_fury      ); // Last Gift
  unique_gear::register_special_effect( 280577, special_effects::glory_in_battle       ); // Glory In Battle
  unique_gear::register_special_effect( 280623, special_effects::glory_in_battle       ); // Liberator's Might
  unique_gear::register_special_effect( 280598, special_effects::sylvanas_resolve      ); // Sylvanas' Resolve
  unique_gear::register_special_effect( 280628, special_effects::sylvanas_resolve      ); // Anduin's Determination
  unique_gear::register_special_effect( 281841, special_effects::tradewinds            );
  unique_gear::register_special_effect( 281514, special_effects::unstable_catalyst     );
  unique_gear::register_special_effect( 280626, special_effects::stand_as_one          );  // Stand as One
  unique_gear::register_special_effect( 280581, special_effects::stand_as_one          );  // CollectiveWill
  unique_gear::register_special_effect( 280555, special_effects::archive_of_the_titans );
  unique_gear::register_special_effect( 280559, special_effects::laser_matrix          );
  unique_gear::register_special_effect( 280410, special_effects::incite_the_pack       );
  unique_gear::register_special_effect( 280284, special_effects::dagger_in_the_back    );
  unique_gear::register_special_effect( 273790, special_effects::rezans_fury           );
  unique_gear::register_special_effect( 273829, special_effects::secrets_of_the_deep   );
  unique_gear::register_special_effect( 280580, special_effects::combined_might        ); // Combined Might
  unique_gear::register_special_effect( 280625, special_effects::combined_might        ); // Stronger Together
  unique_gear::register_special_effect( 280178, special_effects::relational_normalization_gizmo );
  unique_gear::register_special_effect( 280163, special_effects::barrage_of_many_bombs );
  unique_gear::register_special_effect( 273682, special_effects::meticulous_scheming   );
  unique_gear::register_special_effect( 280174, special_effects::synaptic_spark_capacitor );
  unique_gear::register_special_effect( 280168, special_effects::ricocheting_inflatable_pyrosaw );
  unique_gear::register_special_effect( 266937, special_effects::gutripper             );
  unique_gear::register_special_effect( 280582, special_effects::battlefield_focus_precision );
  unique_gear::register_special_effect( 280627, special_effects::battlefield_focus_precision );
  unique_gear::register_special_effect( 287662, special_effects::endless_hunger        ); // Endless Hunger
  unique_gear::register_special_effect( 287604, special_effects::endless_hunger        ); // Ancient's Bulwark
  unique_gear::register_special_effect( 287631, special_effects::apothecarys_concoctions );
  unique_gear::register_special_effect( 287467, special_effects::shadow_of_elune       );
  unique_gear::register_special_effect( 288953, special_effects::treacherous_covenant  );
  unique_gear::register_special_effect( 288749, special_effects::seductive_power       );
  unique_gear::register_special_effect( 288802, special_effects::bonded_souls          );
  unique_gear::register_special_effect( 287818, special_effects::fight_or_flight       );
  unique_gear::register_special_effect( 303008, special_effects::undulating_tides      );
  unique_gear::register_special_effect( 303007, special_effects::loyal_to_the_end      );
  unique_gear::register_special_effect( 303006, special_effects::arcane_heart          );
  unique_gear::register_special_effect( 300170, special_effects::clockwork_heart       );
  unique_gear::register_special_effect( 300168, special_effects::personcomputer_interface );
  unique_gear::register_special_effect( 317137, special_effects::heart_of_darkness );
  unique_gear::register_special_effect( 268437, special_effects::impassive_visage );
  unique_gear::register_special_effect( 268596, special_effects::gemhide );
  unique_gear::register_special_effect( 271536, special_effects::crystalline_carapace );

  // Generic Azerite Essences
  //
  // NOTE: The On-Use effects are built as separate actions to allow maximum flexibility
  unique_gear::register_special_effect( 300573, azerite_essences::stamina_milestone );
  unique_gear::register_special_effect( 300575, azerite_essences::stamina_milestone );
  unique_gear::register_special_effect( 300576, azerite_essences::stamina_milestone );
  unique_gear::register_special_effect( 300577, azerite_essences::stamina_milestone );
  unique_gear::register_special_effect( 312927, azerite_essences::stamina_milestone );
  unique_gear::register_special_effect( 312928, azerite_essences::stamina_milestone );

  // Generic minor Azerite Essences
  unique_gear::register_special_effect( 295365, azerite_essences::the_crucible_of_flame );
  unique_gear::register_special_effect( 295246, azerite_essences::essence_of_the_focusing_iris );
  unique_gear::register_special_effect( 297147, azerite_essences::blood_of_the_enemy );
  unique_gear::register_special_effect( 295834, azerite_essences::condensed_life_force );
  unique_gear::register_special_effect( 304081, azerite_essences::conflict_and_strife );
  unique_gear::register_special_effect( 295293, azerite_essences::purification_protocol );
  unique_gear::register_special_effect( 302916, azerite_essences::ripple_in_space );
  unique_gear::register_special_effect( 298407, azerite_essences::the_unbound_force );
  unique_gear::register_special_effect( 295078, azerite_essences::worldvein_resonance );
  unique_gear::register_special_effect( 296320, azerite_essences::strive_for_perfection );
  unique_gear::register_special_effect( 294964, azerite_essences::anima_of_life_and_death );
  unique_gear::register_special_effect( 295750, azerite_essences::nullification_dynamo );
  unique_gear::register_special_effect( 298193, azerite_essences::aegis_of_the_deep );
  unique_gear::register_special_effect( 294910, azerite_essences::sphere_of_suppression );
  unique_gear::register_special_effect( 310712, azerite_essences::breath_of_the_dying ); // lethal strikes
  unique_gear::register_special_effect( 311210, azerite_essences::spark_of_inspiration ); // unified strength
  unique_gear::register_special_effect( 312771, azerite_essences::formless_void ); // symbiotic prensence
  unique_gear::register_special_effect( 310603, azerite_essences::strength_of_the_warden ); // endurance
  unique_gear::register_special_effect( 293030, azerite_essences::unwavering_ward ); // unwavering ward
  unique_gear::register_special_effect( 297411, azerite_essences::spirit_of_preservation ); // devout spirit
  unique_gear::register_special_effect( 295164, azerite_essences::touch_of_the_everlasting ); // will to survive

  // Vision of Perfection major Azerite Essence
  unique_gear::register_special_effect( 296325, azerite_essences::vision_of_perfection );
}

void register_azerite_target_data_initializers( sim_t* sim )
{
  // Azerite Globules
  sim -> register_target_data_initializer( [] ( actor_target_data_t* td ) {
    auto&& azerite = td -> source -> azerite;
    if ( azerite && azerite -> is_enabled( "Azerite Globules" ) )
    {
      assert( !td -> debuff.azerite_globules );

      td -> debuff.azerite_globules = make_buff( *td, "azerite_globules", td -> source -> find_spell( 279956 ) );
      td -> debuff.azerite_globules -> reset();
    }
    else
    {
      td -> debuff.azerite_globules = make_buff( *td, "azerite_globules" );
      td -> debuff.azerite_globules -> set_quiet( true );
    }
  } );

  // Battlefield Focus / Precision
  sim -> register_target_data_initializer( [] ( actor_target_data_t* td ) {
    auto&& azerite = td -> source -> azerite;
    if ( ! azerite )
    {
      return;
    }

    if ( azerite->is_enabled( "Battlefield Focus" ) )
    {
      td->debuff.battlefield_debuff = make_buff( *td, "battlefield_focus",
          td->source->find_spell( 280817 ) );
      td->debuff.battlefield_debuff->reset();
    }
    else if ( azerite->is_enabled( "Battlefield Precision" ) )
    {
      td->debuff.battlefield_debuff = make_buff( *td, "battlefield_precision",
          td->source->find_spell( 280855 ) );
      td->debuff.battlefield_debuff->reset();
    }
    else
    {
      td -> debuff.battlefield_debuff = make_buff( *td, "battlefield_debuff" );
      td -> debuff.battlefield_debuff->set_quiet( true );
    }
  } );

  // Blood of the Enemy
  sim->register_target_data_initializer( []( actor_target_data_t* td ) {
    auto essence = td->source->find_azerite_essence( "Blood of the Enemy" );
    if ( essence.enabled() )
    {
      td->debuff.blood_of_the_enemy = make_buff( *td, "blood_of_the_enemy", td->source->find_spell( 297108 ) )
        ->set_default_value( td->source->find_spell( 297108 )->effectN( 2 ).percent() )
        ->set_cooldown( 0_ms );
      td->debuff.blood_of_the_enemy->reset();
    }
    else
    {
      td->debuff.blood_of_the_enemy = make_buff( *td, "blood_of_the_enemy" )->set_quiet( true );
    }
  } );

  // Condensed Life-Force
  sim->register_target_data_initializer( [] ( actor_target_data_t* td ) {
    auto essence = td->source->find_azerite_essence( "Condensed Life-Force" );
    if ( essence.enabled() )
    {
      td->debuff.condensed_lifeforce = make_buff( *td, "condensed_lifeforce", td->source->find_spell( 295838 ) )
        ->set_default_value( td->source->find_spell( 295838 )->effectN( 1 ).percent() );
      td->debuff.condensed_lifeforce->reset();
    }
    else
    {
      td->debuff.condensed_lifeforce = make_buff( *td, "condensed_lifeforce" )
        ->set_quiet( true );
    }
  } );

  sim->register_target_data_initializer( [] ( actor_target_data_t * td ) {
    auto essence = td->source->find_azerite_essence( "Breath of the Dying" );
    if ( essence.enabled() )
    {
      td->debuff.reaping_flames_tracker = make_buff( *td, "reaping_flames_tracker", td->source->find_spell( 311947 ) )
        ->set_quiet( true );
      td->debuff.reaping_flames_tracker->reset();
    }
    else
    {
      td->debuff.reaping_flames_tracker = make_buff( *td, "reaping_flames_tracker" )
        ->set_quiet( true );
    }
  } );
}

std::tuple<int, int, int> compute_value( const azerite_power_t& power, const spelleffect_data_t& effect )
{
  int min_ = 0, max_ = 0, avg_ = 0;
  if ( !power.enabled() || effect.m_coefficient() == 0 )
  {
    return std::make_tuple( 0, 0, 0 );
  }

  auto budgets = power.budget( effect.spell() );
  range::for_each( budgets, [&]( double budget ) {
    avg_ += static_cast<int>( budget * effect.m_coefficient() + 0.5 );
    min_ += static_cast<int>( budget * effect.m_coefficient() *
        ( 1.0 - effect.m_delta() / 2 ) + 0.5 );
    max_ += static_cast<int>( budget * effect.m_coefficient() *
        ( 1.0 + effect.m_delta() / 2 ) + 0.5 );
  } );

  return std::make_tuple( min_, avg_, max_ );
}

void parse_blizzard_azerite_information( item_t& item, const rapidjson::Value& data )
{
  if ( !data.HasMember( "azerite_details" ) )
  {
    return;
  }

  if ( data[ "azerite_details" ].HasMember( "selected_powers" ) )
  {
    for ( auto idx = 0u, end = data[ "azerite_details" ][ "selected_powers" ].Size(); idx < end; ++idx )
    {
      const auto& power_data = data[ "azerite_details" ][ "selected_powers" ][ idx ];

      if ( !power_data.HasMember( "id" ) )
      {
        throw std::runtime_error( "Unable to parse Azerite power" );
      }

      item.parsed.azerite_ids.push_back( power_data[ "id" ].GetUint() );
    }
  }

  if ( data[ "azerite_details" ].HasMember( "selected_essences" ) )
  {
    for ( auto idx = 0u, end = data[ "azerite_details" ][ "selected_essences" ].Size(); idx < end; ++idx )
    {
      const auto& essence_data = data[ "azerite_details" ][ "selected_essences" ][ idx ];

      if ( !essence_data.HasMember( "slot" ) )
      {
        throw std::runtime_error( "Unable to parse Azerite essence basic information" );
      }

      if ( !essence_data.HasMember( "essence" ) )
      {
        continue;
      }

      if ( !essence_data[ "essence" ].HasMember( "id" ) || !essence_data.HasMember( "rank" ) )
      {
        throw std::runtime_error( "Unable to parse Azerite essence" );
      }

      item.player->azerite_essence->add_essence(
        essence_data[ "slot" ].GetInt() == 0 ? essence_type::MAJOR : essence_type::MINOR,
        essence_data[ "essence" ][ "id" ].GetUint(),
        essence_data[ "rank" ].GetUint() );
    }
  }

  if ( data[ "azerite_details" ].HasMember( "level" ) &&
       data[ "azerite_details" ][ "level" ].HasMember( "value" ) )
  {
    item.parsed.azerite_level = data[ "azerite_details" ][ "level" ][ "value" ].GetUint();
  }
}

} // Namespace azerite ends

namespace azerite
{
namespace special_effects
{

static std::string tokenized_name( const spell_data_t* spell )
{
  return ::util::tokenize_fn( spell -> name_cstr() );
}

void resounding_protection( special_effect_t& effect )
{
  class rp_event_t : public event_t
  {
    buff_t*    buff;
    timespan_t period;

public:
    rp_event_t( buff_t* b, timespan_t t ) :
      event_t( *b -> source, t ), buff( b ), period( t )
    { }

    void execute() override
    {
      buff -> trigger();
      make_event<rp_event_t>( sim(), buff, period );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
  {
    return;
  }

  const spell_data_t* driver = effect.player -> find_spell( 270568 );
  const spell_data_t* absorb = effect.player -> find_spell( 269279 );
  double amount = power.value();
  buff_t* buff = make_buff<absorb_buff_t>( effect.player, "resounding_protection", absorb )
                 -> set_default_value( amount );

  effect.player -> register_combat_begin( [ buff, driver ]( player_t* ) {
    buff -> trigger();
    make_event<rp_event_t>( *buff -> sim, buff, driver -> effectN( 1 ).period() );
  } );
}

void elemental_whirl( special_effect_t& effect )
{
  class ew_proc_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    public:
    ew_proc_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    { }

    void execute( action_t* /* a */, action_state_t* /* state */ ) override
    {
      size_t buff_index = rng().range( buffs.size() );
      buffs[ buff_index ] -> trigger();
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
  {
    return;
  }

  double amount = power.value();
  const spell_data_t* driver = effect.player -> find_spell( 270667 );
  const spell_data_t* crit_spell = effect.player -> find_spell( 268953 );
  const spell_data_t* hast_spell = effect.player -> find_spell( 268954 );
  const spell_data_t* mast_spell = effect.player -> find_spell( 268955 );
  const spell_data_t* vers_spell = effect.player -> find_spell( 268956 );

  buff_t* crit = make_buff<stat_buff_t>( effect.player, "elemental_whirl_crit", crit_spell )
    -> add_stat( STAT_CRIT_RATING, amount );
  buff_t* versatility = make_buff<stat_buff_t>( effect.player, "elemental_whirl_versatility", vers_spell )
    -> add_stat( STAT_VERSATILITY_RATING, amount );
  buff_t* mastery = make_buff<stat_buff_t>( effect.player, "elemental_whirl_mastery", mast_spell )
    -> add_stat( STAT_MASTERY_RATING, amount );
  buff_t* haste = make_buff<stat_buff_t>( effect.player, "elemental_whirl_haste", hast_spell )
    -> add_stat( STAT_HASTE_RATING, amount );

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  new ew_proc_cb_t( effect, { crit, haste, mastery, versatility } );
}

void blood_siphon( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
  {
    return;
  }

  effect.player -> passive.mastery_rating += power.value( 1 );
  effect.player -> passive.leech_rating += power.value( 2 );
}

void lifespeed(special_effect_t& effect)
{
  azerite_power_t power = effect.player->find_azerite_spell(effect.driver()->name_cstr());
  if (!power.enabled())
  {
    return;
  }

  effect.player->passive.haste_rating += power.value(1);
  effect.player->passive.avoidance_rating += power.value(2);
}

void on_my_way(special_effect_t& effect)
{
  azerite_power_t power = effect.player->find_azerite_spell(effect.driver()->name_cstr());
  if (!power.enabled())
  {
    return;
  }

  effect.player->passive.versatility_rating += power.value(1);
  effect.player->passive.speed_rating += power.value(2);
}

void champion_of_azeroth( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  const double amount = power.value();
  const spell_data_t* driver = effect.player -> find_spell( 280712 );
  const spell_data_t* spell = effect.player -> find_spell( 280713 );

  effect.custom_buff = buff_t::find( effect.player, "champion_of_azeroth" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, "champion_of_azeroth", spell )
      -> add_stat( STAT_CRIT_RATING, amount )
      -> add_stat( STAT_VERSATILITY_RATING, amount )
      -> add_stat( STAT_MASTERY_RATING, amount )
      -> add_stat( STAT_HASTE_RATING, amount );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void unstable_flames( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  const double amount = power.value();
  const spell_data_t* driver = effect.player -> find_spell( 279900 );
  const spell_data_t* spell = effect.player -> find_spell( 279902 );

  effect.custom_buff = buff_t::find( effect.player, "unstable_flames" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, "unstable_flames", spell )
      -> add_stat( STAT_CRIT_RATING, amount );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void thunderous_blast( special_effect_t& effect )
{
  struct thunderous_blast_t : public unique_gear::proc_spell_t
  {
    buff_t* building_pressure = nullptr;
    buff_t* rolling_thunder = nullptr;

    thunderous_blast_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "thunderous_blast", e.player, e.player -> find_spell( 280384 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );

      building_pressure = buff_t::find( e.player, "building_pressure" );
      if ( !building_pressure )
      {
        building_pressure =
          make_buff( e.player, "building_pressure", data().effectN( 3 ).trigger() )
            -> set_default_value( data().effectN( 3 ).trigger() -> effectN( 1 ).percent() );
      }

      rolling_thunder = buff_t::find( e.player, "rolling_thunder" );
      if ( !rolling_thunder )
      {
        rolling_thunder =
          make_buff( e.player, "rolling_thunder", e.player -> find_spell( 280400 ) )
            -> set_default_value( e.player -> find_spell( 280400 ) -> effectN( 1 ).percent() );
      }
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( building_pressure -> check() == building_pressure -> max_stack() )
      {
        building_pressure -> expire();
        rolling_thunder -> trigger();
      }
      else if ( rolling_thunder -> check() )
      {
        rolling_thunder -> expire();
      }
      else
      {
        building_pressure -> trigger();
      }
    }

    double action_multiplier() const override
    {
      double am = proc_spell_t::action_multiplier();

      am *= 1.0 + building_pressure -> check_stack_value();
      am *= 1.0 + rolling_thunder -> check_value();

      return am;
    }

    double composite_crit_chance() const override
    {
      double cc = proc_spell_t::composite_crit_chance();

      if ( rolling_thunder -> check() )
        cc += 1.0;

      return cc;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<thunderous_blast_t>( "thunderous_blast", effect, power );
  effect.spell_id = 280383;

  new dbc_proc_callback_t( effect.player, effect );
}

void filthy_transfusion( special_effect_t& effect )
{
  struct filthy_transfusion_t : public unique_gear::proc_spell_t
  {
    filthy_transfusion_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "filthy_transfusion", e.player, e.player -> find_spell( 273836 ) )
    {
      base_td = power.value( 1 );
      tick_may_crit = false;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( ! power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<filthy_transfusion_t>( "filthy_transfusion", effect, power );
  effect.spell_id = 273835;

  new dbc_proc_callback_t( effect.player, effect );
}

void retaliatory_fury( special_effect_t& effect )
{
  class retaliatory_fury_proc_cb_t : public dbc_proc_callback_t
  {
    std::array<buff_t*, 2> buffs;
  public:
    retaliatory_fury_proc_cb_t( const special_effect_t& effect, std::array<buff_t*, 2> b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    {}

    void execute( action_t*, action_state_t* ) override
    {
      for ( auto b : buffs )
        b -> trigger();
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 2 ).trigger();
  const spell_data_t* mastery_spell = effect.player -> find_spell( power.spell_ref().id() == 280624 ? 280861 : 280787 );
  const spell_data_t* absorb_spell = effect.player -> find_spell( power.spell_ref().id() == 280624 ? 280862 : 280788 );

  buff_t* mastery = buff_t::find( effect.player, tokenized_name( mastery_spell ) );
  if ( !mastery )
  {
    mastery = make_buff<stat_buff_t>( effect.player, tokenized_name( mastery_spell ), mastery_spell )
      -> add_stat( STAT_MASTERY_RATING, power.value( 1 ) );
  }

  buff_t* absorb = buff_t::find( effect.player, tokenized_name( absorb_spell ) + "_absorb" );
  if ( !absorb )
  {
    absorb = make_buff<absorb_buff_t>( effect.player, tokenized_name( absorb_spell ) + "_absorb", absorb_spell )
      -> set_absorb_source( effect.player -> get_stats( tokenized_name( absorb_spell ) ) )
      -> set_default_value( power.value( 2 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  // 07-08-2018: this seems to have 2 rppm in pve
  if ( ! effect.player -> sim -> pvp_crit )
    effect.ppm_ = -2.0;

  new retaliatory_fury_proc_cb_t( effect, { { mastery, absorb } } );
}

void blood_rite( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 280408 );
  const spell_data_t* spell = effect.player -> find_spell( 280409 );

  effect.custom_buff = buff_t::find( effect.player, "blood_rite" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, "blood_rite", spell )
      -> add_stat( STAT_HASTE_RATING, power.value( 1 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();
  auto buff = effect.custom_buff;

  //Kills refresh buff
  range::for_each( effect.player -> sim -> actor_list, [buff]( player_t* target ) {
    if ( !target -> is_enemy() )
    {
      return;
    }

    target->register_on_demise_callback( buff->player, [buff]( player_t* ) {
      if ( buff->up( ) )
      {
        buff->refresh();
      }
    } );
  } );

  new dbc_proc_callback_t( effect.player, effect );
}

void glory_in_battle( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
      -> add_stat( STAT_CRIT_RATING, power.value( 1 ) )
      -> add_stat( STAT_HASTE_RATING, power.value( 2 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  // 07-08-2018: this seems to have 2 rppm in pve
  if ( ! effect.player -> sim -> pvp_crit )
    effect.ppm_ = -2.0;

  new dbc_proc_callback_t( effect.player, effect );
}

void tradewinds( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = driver->effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
                             ->add_stat( STAT_MASTERY_RATING, power.value( 1 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  // TODO add the "jump to other player" effect
  // TODO check RPPM

  new dbc_proc_callback_t( effect.player, effect );
}

void unstable_catalyst( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = driver->effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
                             ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) );
  }

  // TODO assumes the player always stands in the pool - might be unrealistic

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void stand_as_one( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = driver->effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
                             ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) )
                             ->add_stat( STAT_MAX_HEALTH, power.value( 2 ) );
  }

  // TODO give to 4 other actors?

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

struct reorigination_array_buff_t : public buff_t
{
  // Comes from server side, hardcoded somewhere there
  double stat_value = 75.0;
  stat_e current_stat = STAT_NONE;

  proc_t* proc_crit, *proc_haste, *proc_mastery, *proc_versatility;

  void add_proc( stat_e stat )
  {
    switch ( stat )
    {
      case STAT_CRIT_RATING:
        proc_crit->occur();
        break;
      case STAT_HASTE_RATING:
        proc_haste->occur();
        break;
      case STAT_MASTERY_RATING:
        proc_mastery->occur();
        break;
      case STAT_VERSATILITY_RATING:
        proc_versatility->occur();
        break;
      default:
        break;
    }
  }

  stat_e rating_to_stat( rating_e r ) const
  {
    switch ( r )
    {
      case RATING_MELEE_CRIT: return STAT_CRIT_RATING;
      case RATING_MELEE_HASTE: return STAT_HASTE_RATING;
      case RATING_MASTERY: return STAT_MASTERY_RATING;
      case RATING_DAMAGE_VERSATILITY: return STAT_VERSATILITY_RATING;
      default: return STAT_NONE;
    }
  }

  rating_e stat_to_rating( stat_e s ) const
  {
    switch ( s )
    {
      case STAT_CRIT_RATING: return RATING_MELEE_CRIT;
      case STAT_HASTE_RATING: return RATING_MELEE_HASTE;
      case STAT_MASTERY_RATING: return RATING_MASTERY;
      case STAT_VERSATILITY_RATING: return RATING_DAMAGE_VERSATILITY;
      default: return RATING_MAX;
    }
  }

  stat_e find_highest_stat() const
  {
    // Simc bunches ratings together so it does not matter which we pick here
    static const std::vector<rating_e> ratings {
      RATING_MELEE_CRIT, RATING_MELEE_HASTE, RATING_MASTERY, RATING_DAMAGE_VERSATILITY
    };

    double high = std::numeric_limits<double>::lowest();
    rating_e rating = RATING_MAX;

    for ( auto r : ratings )
    {
      auto stat = rating_to_stat( r );
      auto composite = source->composite_rating( r );
      // Calculate highest stat without reorigination array included
      if ( stat == current_stat )
      {
        composite -= sim->bfa_opts.reorigination_array_stacks * stat_value *
          source->composite_rating_multiplier( r );

        if ( sim->scaling && sim->bfa_opts.reorigination_array_ignore_scale_factors &&
             stat == sim->scaling->scale_stat )
        {
          composite -= sim->scaling->scale_value * source->composite_rating_multiplier( r );
        }
      }

      if ( sim->debug )
      {
        sim->out_debug.print( "{} reorigination_array check_stat current_stat={} stat={} value={}",
            source->name(), util::stat_type_string( current_stat ),
            util::stat_type_string( rating_to_stat( r ) ), composite );
      }

      if ( composite > high )
      {
        high = composite;
        rating = r;
      }
    }

    return rating_to_stat( rating );
  }

  void trigger_dynamic_stat_change()
  {
    auto highest_stat = find_highest_stat();
    if ( highest_stat == current_stat )
    {
      return;
    }

    if ( current_stat != STAT_NONE )
    {
      rating_e current_rating = stat_to_rating( current_stat );

      double current_amount = stat_value * sim->bfa_opts.reorigination_array_stacks;

      source->stat_loss( current_stat, current_amount );

      if ( sim->debug )
      {
        sim->out_debug.print( "{} reorigination_array stat change, current_stat={}, value={}, total={}",
            source->name(), util::stat_type_string( current_stat ), -current_amount,
            source->composite_rating( current_rating ) );
      }
    }

    if ( highest_stat != STAT_NONE )
    {
      rating_e new_rating = stat_to_rating( highest_stat );

      double new_amount = stat_value * sim->bfa_opts.reorigination_array_stacks;

      source->stat_gain( highest_stat, new_amount );

      if ( sim->debug )
      {
        sim->out_debug.print( "{} reorigination_array stat change, new_stat={}, value={}, total={}",
            source->name(), util::stat_type_string( highest_stat ), new_amount,
            source->composite_rating( new_rating ) );
      }

      add_proc( highest_stat );
    }

    current_stat = highest_stat;
  }

  reorigination_array_buff_t( player_t* p, util::string_view name, const special_effect_t& effect ) :
    buff_t( p, name, effect.player->find_spell( 280573 ), effect.item ),
    proc_crit( p->get_proc( "Reorigination Array: Critical Strike" ) ),
    proc_haste( p->get_proc( "Reorigination Array: Haste" ) ),
    proc_mastery( p->get_proc( "Reorigination Array: Mastery" ) ),
    proc_versatility( p->get_proc( "Reorigination Array: Versatility" ) )
  { }

  void reset() override
  {
    buff_t::reset();

    current_stat = STAT_NONE;
  }

  void execute( int stacks = 1, double value = DEFAULT_VALUE(),
                timespan_t duration = timespan_t::min() ) override
  {
    // Ensure buff only executes once
    if ( current_stack == 0 )
    {
      buff_t::execute( stacks, value, duration );

      trigger_dynamic_stat_change();

      make_repeating_event( *sim, timespan_t::from_seconds( 5.0 ), [ this ]() {
          trigger_dynamic_stat_change();
      } );
    }
  }

  void expire_override( int stacks, timespan_t duration ) override
  {
    buff_t::expire_override( stacks, duration );

    if ( current_stat != STAT_NONE )
    {
      source->stat_loss( current_stat, stat_value * sim->bfa_opts.reorigination_array_stacks );
      current_stat = STAT_NONE;
    }
  }
};

void archive_of_the_titans( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = effect.player -> find_spell( 280709 );

  buff_t* buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
      ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) );
  }

  buff_t* reorg = buff_t::find( effect.player, "reorigination_array" );
  // Reorigination array has to be implemented uniquely, so only Archive of the Titans, or Laser
  // Matrix will properly initialize the system.
  if ( ! reorg && effect.player->sim->bfa_opts.reorigination_array_stacks > 0 )
  {
    reorg = unique_gear::create_buff<reorigination_array_buff_t>( effect.player,
        "reorigination_array", effect );
  }

  effect.player->register_combat_begin( [ buff, driver, reorg ]( player_t* ) {
    if ( buff->sim->bfa_opts.initial_archive_of_the_titans_stacks )
    {
      buff->trigger( buff->sim->bfa_opts.initial_archive_of_the_titans_stacks );
    }

    if ( reorg )
    {
      reorg->trigger( buff->sim->bfa_opts.reorigination_array_stacks );
    }

    make_repeating_event( *buff -> sim, driver -> effectN( 1 ).period(), [ buff ]() {
      buff -> trigger();
    } );
    // TODO Proc Reorigination Array
  } );
}

void laser_matrix( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();

  struct laser_matrix_t : public unique_gear::proc_spell_t
  {
    laser_matrix_t( const special_effect_t& e, const azerite_power_t& power )
      : proc_spell_t( "laser_matrix", e.player, e.player->find_spell( 280705 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
      aoe = -1;
      split_aoe_damage = true;
      radius = e.player->find_spell( 280703 )->effectN( 1 ).radius();
    }
    // TODO: travel_time?
  };

  effect.execute_action = unique_gear::create_proc_action<laser_matrix_t>( "laser_matrix", effect, power );
  effect.spell_id       = driver->id();
  effect.proc_flags_    = PF_ALL_DAMAGE;

  new dbc_proc_callback_t( effect.player, effect );

  buff_t* reorg = buff_t::find( effect.player, "reorigination_array" );
  // Reorigination array has to be implemented uniquely, so only Archive of the Titans, or Laser
  // Matrix will properly initialize the system.
  if ( ! reorg && effect.player->sim->bfa_opts.reorigination_array_stacks > 0 )
  {
    reorg = unique_gear::create_buff<reorigination_array_buff_t>( effect.player,
        "reorigination_array", effect );

    effect.player->register_combat_begin( [ reorg ]( player_t* ) {
      reorg->trigger( reorg->sim->bfa_opts.reorigination_array_stacks );
    } );
  }
}

void incite_the_pack( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell  = driver->effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
                             ->add_stat( STAT_MASTERY_RATING, power.value( 1 ) );
  }

  // TODO buff 2 allies with power.value( 2 )?

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void sylvanas_resolve( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = power.spell_ref().effectN( 1 ).trigger();
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  effect.custom_buff = buff_t::find( effect.player, tokenized_name( spell ) );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<stat_buff_t>( effect.player, tokenized_name( spell ), spell )
      -> add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) );
  }

  // Replace the driver spell, the azerite power does not hold the RPPM value
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void tidal_surge( special_effect_t& effect )
{
  struct tidal_surge_t : public unique_gear::proc_spell_t
  {
    tidal_surge_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "tidal_surge", e.player, e.player -> find_spell( 280404 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
    }
    // TODO: verify travel_time
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<tidal_surge_t>( "tidal_surge", effect, power );
  effect.spell_id = 280403;

  new dbc_proc_callback_t( effect.player, effect );
}

void heed_my_call( special_effect_t& effect )
{
  struct heed_my_call_aoe_t : public unique_gear::proc_spell_t
  {
    heed_my_call_aoe_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "heed_my_call_aoe", e.player, e.player -> find_spell( 271686 ) )
    {
      base_dd_min = base_dd_max = power.value( 2 );
    }
  };
  struct heed_my_call_t : public unique_gear::proc_spell_t
  {
    heed_my_call_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "heed_my_call", e.player, e.player -> find_spell( 271685 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
      execute_action = new heed_my_call_aoe_t( e, power );
      add_child( execute_action );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<heed_my_call_t>( "heed_my_call", effect, power );
  effect.spell_id = 271681;

  new dbc_proc_callback_t( effect.player, effect );
}

void azerite_globules( special_effect_t& effect )
{
  struct azerite_globules_t : public unique_gear::proc_spell_t
  {
    azerite_globules_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "azerite_globules", e.player, e.player -> find_spell( 279958 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
    }
  };

  class azerite_globules_proc_cb_t : public dbc_proc_callback_t
  {
  public:
    azerite_globules_proc_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    {}

    void execute( action_t* a, action_state_t* s ) override
    {
      auto td = a -> player -> get_target_data( s -> target );
      assert( td );
      auto debuff = td -> debuff.azerite_globules;
      assert( debuff );

      debuff -> trigger();
      if ( debuff -> stack() == debuff -> max_stack() )
      {
        debuff -> expire();
        proc_action -> set_target( s -> target );
        proc_action -> execute();
      }
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<azerite_globules_t>( "azerite_globules", effect, power );
  effect.spell_id = 279955;

  new azerite_globules_proc_cb_t( effect );
}

void overwhelming_power( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  buff_t* buff = buff_t::find( effect.player, "overwhelming_power" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "overwhelming_power", effect.player->find_spell( 271711 ) )
      ->add_stat( STAT_HASTE_RATING, power.value( 1 ) )
      ->set_reverse( true );
  }

  effect.custom_buff = buff;
  effect.spell_id = effect.trigger()->id();

  new dbc_proc_callback_t( effect.player, effect );

  // TODO: add on damage taken mechanic
}

void earthlink( special_effect_t& effect )
{
  struct earthlink_t : public stat_buff_t
  {
    earthlink_t( player_t* p ) :
      stat_buff_t( p, "earthlink", p -> find_spell( 279928 ) )
    {}

    void bump( int stacks, double value ) override
    {
      if ( check() == max_stack() )
        reverse = true;
      else
        stat_buff_t::bump( stacks, value );
    }

    void decrement( int stacks, double value ) override
    {
      if ( check() == 1 )
        reverse = false;
      else
        stat_buff_t::decrement( stacks, value );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 279927 );

  buff_t* buff = buff_t::find( effect.player, "earthlink" );
  if ( !buff )
  {
    buff = make_buff<earthlink_t>( effect.player )
      -> add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) )
      -> set_duration( effect.player -> sim -> max_time * 3 )
      -> set_period( driver -> effectN( 1 ).period() );
  }

  effect.player -> register_combat_begin( [ buff ]( player_t* ) {
    buff -> trigger();
  } );
}

void wandering_soul( special_effect_t& effect )
{
  struct ruinous_bolt_t : unique_gear::proc_spell_t
  {
    ruinous_bolt_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "ruinous_bolt", e.player, e.player -> find_spell( 280206 ) )
    {
      aoe = 0;
      range = data().effectN( 1 ).radius_max();
      base_dd_min = base_dd_max = power.value( 1 );
    }
  };

  const spell_data_t* driver = effect.player -> find_spell( 273825 );

  // Check if we already registered the proc callback
  const bool exists = effect.player -> callbacks.has_callback( [ driver ]( const action_callback_t* cb ) {
    auto dbc_cb = dynamic_cast<const dbc_proc_callback_t*>( cb );
    return dbc_cb && dbc_cb -> effect.driver() == driver;
  } );
  if ( exists )
    return;

  azerite_power_t blightborne_infusion = effect.player -> find_azerite_spell( "Blightborne Infusion" );
  azerite_power_t ruinous_bolt = effect.player -> find_azerite_spell( "Ruinous Bolt" );
  if ( !( blightborne_infusion.enabled() || ruinous_bolt.enabled() ) )
    return;

  const spell_data_t* spell = effect.player -> find_spell( 280204 );
  const std::string spell_name = tokenized_name( spell );

  buff_t* buff = buff_t::find( effect.player, spell_name );
  if ( !buff )
  {
    if ( blightborne_infusion.enabled() )
    {
      buff = make_buff<stat_buff_t>( effect.player, spell_name, spell )
        -> add_stat( STAT_CRIT_RATING, blightborne_infusion.value( 1 ) );
    }
    else
    {
      buff = make_buff<buff_t>( effect.player, spell_name, spell );
    }

    if ( ruinous_bolt.enabled() )
    {
      auto action = unique_gear::create_proc_action<ruinous_bolt_t>( "ruinous_bolt", effect, ruinous_bolt );
      buff -> set_tick_callback( [ action ]( buff_t*, int, timespan_t ) {
        const auto& tl = action -> target_list();
        if ( tl.empty() )
          return;
        action -> set_target( tl[ action -> rng().range( tl.size() ) ] );
        action -> execute();
      } );
    }
    else
    {
      buff -> set_period( timespan_t::zero() ); // disable ticking
    }
  }

  effect.custom_buff = buff;
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void swirling_sands( special_effect_t& effect )
{
  struct swirling_sands_buff_t : public stat_buff_t
  {
    dbc_proc_callback_t* extender;
    timespan_t max_extension;
    timespan_t current_extension = timespan_t::zero();

    swirling_sands_buff_t( player_t* p, const azerite_power_t& power, dbc_proc_callback_t* e ):
      stat_buff_t( p, "swirling_sands", p -> find_spell( 280433 ) ),
      extender( e ), max_extension( power.spell_ref().effectN( 3 ).time_value() )
    {
      add_stat( STAT_CRIT_RATING, power.value( 1 ) );
    }

    void extend_duration( player_t* p, timespan_t extra_seconds ) override
    {
      if ( !check() || cooldown -> down() )
        return;

      extra_seconds = std::min( extra_seconds, max_extension - current_extension );
      if ( extra_seconds == timespan_t::zero() )
        return;

      stat_buff_t::extend_duration( p, extra_seconds );
      current_extension += extra_seconds;
      cooldown -> start();
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      stat_buff_t::start( stacks, value, duration );
      extender -> activate();
      current_extension = timespan_t::zero();
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      stat_buff_t::expire_override( expiration_stacks, remaining_duration );
      extender -> deactivate();
    }

    void reset() override
    {
      stat_buff_t::reset();
      extender -> deactivate();
    }
  };

  struct swirling_sands_extender_t : public dbc_proc_callback_t
  {
    timespan_t extension;

    swirling_sands_extender_t( const special_effect_t& effect, const azerite_power_t& power ) :
      dbc_proc_callback_t( effect.player, effect ),
      extension( power.spell_ref().effectN( 2 ).time_value() )
    {}

    void execute( action_t* a, action_state_t* ) override
    {
      proc_buff -> extend_duration( a -> player, extension );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 280432 );

  auto secondary = new special_effect_t( effect.player );
  secondary -> name_str = "swirling_sands_extender";
  secondary -> type = effect.type;
  secondary -> source = effect.source;
  secondary -> proc_flags_ = driver -> proc_flags();
  secondary -> proc_flags2_ = PF2_CRIT;
  secondary -> proc_chance_ = 1.0;
  secondary -> cooldown_ = timespan_t::zero();
  effect.player -> special_effects.push_back( secondary );

  auto extender = new swirling_sands_extender_t( *secondary, power );

  buff_t* buff = buff_t::find( effect.player, "swirling_sands" );
  if ( !buff )
    buff = make_buff<swirling_sands_buff_t>( effect.player, power, extender );

  secondary -> custom_buff = buff;
  extender -> deactivate();

  effect.custom_buff = buff;
  effect.spell_id = driver -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void dagger_in_the_back( special_effect_t& effect )
{
  struct dagger_in_the_back_t : public unique_gear::proc_spell_t
  {
    dagger_in_the_back_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "dagger_in_the_back", e.player, e.player -> find_spell( 280286 ) )
    {
      base_td = power.value( 1 );
      tick_may_crit = true;
    }

    timespan_t calculate_dot_refresh_duration(const dot_t*  /* dot */, timespan_t triggered_duration ) const override
    {
      return triggered_duration;
    }

    // XXX: simply apply twice here for the "from the back" case
    // in-game this happens with a slight delay of about ~250ms (beta realms from eu)
    // and may be better done through custom proc::execute scheduling an action execute event
    void trigger_dot( action_state_t* s ) override
    {
      proc_spell_t::trigger_dot( s );
      if ( player -> position() == POSITION_BACK || player -> position() == POSITION_RANGED_BACK )
        proc_spell_t::trigger_dot( s );
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<dagger_in_the_back_t>( "dagger_in_the_back", effect, power );
  effect.spell_id = power.spell_ref().effectN( 1 ).trigger() -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void rezans_fury( special_effect_t& effect )
{
  struct rezans_fury_t : public unique_gear::proc_spell_t
  {
    rezans_fury_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "rezans_fury", e.player, e.player -> find_spell( 273794 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
      base_td = power.value( 2 );
      tick_may_crit = true;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<rezans_fury_t>( "rezans_fury", effect, power );
  effect.spell_id = power.spell_ref().effectN( 1 ).trigger() -> id();

  new dbc_proc_callback_t( effect.player, effect );
}

void secrets_of_the_deep( special_effect_t& effect )
{
  struct sotd_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    sotd_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      size_t index = as<size_t>( rng().roll( listener->sim->bfa_opts.secrets_of_the_deep_chance ) );
      buffs[ index ]->trigger();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  auto normal_buff = unique_gear::create_buff<stat_buff_t>( effect.player, "secrets_of_the_deep",
    effect.player->find_spell( 273843 ) )
    ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) )
    ->set_chance( effect.player->sim->bfa_opts.secrets_of_the_deep_collect_chance )
    ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  auto rare_buff = unique_gear::create_buff<stat_buff_t>( effect.player, "secrets_of_the_deep_rare",
    effect.player->find_spell( 273843 ) )
    ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 2 ) )
    ->set_chance( effect.player->sim->bfa_opts.secrets_of_the_deep_collect_chance )
    ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  effect.spell_id = effect.driver()->effectN( 1 ).trigger()->id();

  new sotd_cb_t( effect, { normal_buff, rare_buff } );
}

void combined_might( special_effect_t& effect )
{
  struct combined_might_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    combined_might_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      size_t index =  rng().range( buffs.size() );
      buffs[ index ]->trigger();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  std::vector<buff_t*> buffs;
  if ( util::is_alliance( effect.player->race ) )
  {
    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_humans",
        effect.player->find_spell( 280866 ) )
      ->add_stat( STAT_ATTACK_POWER, power.value( 1 ) )
      ->add_stat( STAT_INTELLECT, power.value( 1 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_night_elves",
        effect.player->find_spell( 280867 ) )
      ->add_stat( STAT_HASTE_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_dwarves",
        effect.player->find_spell( 280868 ) )
      ->add_stat( STAT_VERSATILITY_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_draenei",
        effect.player->find_spell( 280869 ) )
      ->add_stat( STAT_CRIT_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "strength_of_the_gnomes",
        effect.player->find_spell( 280870 ) )
      ->add_stat( STAT_MASTERY_RATING, power.value( 3 ) ) );
  }
  else
  {
    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_orcs",
        effect.player->find_spell( 280841 ) )
      ->add_stat( STAT_ATTACK_POWER, power.value( 1 ) )
      ->add_stat( STAT_INTELLECT, power.value( 1 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_trolls",
        effect.player->find_spell( 280842 ) )
      ->add_stat( STAT_HASTE_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_tauren",
        effect.player->find_spell( 280843 ) )
      ->add_stat( STAT_VERSATILITY_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_forsaken",
        effect.player->find_spell( 280844 ) )
      ->add_stat( STAT_CRIT_RATING, power.value( 3 ) ) );

    buffs.push_back( unique_gear::create_buff<stat_buff_t>( effect.player, "might_of_the_sindorei",
        effect.player->find_spell( 280845 ) )
      ->add_stat( STAT_MASTERY_RATING, power.value( 3 ) ) );
  }

  effect.spell_id = 280848;

  new combined_might_cb_t( effect, buffs );
}

void relational_normalization_gizmo( special_effect_t& effect )
{
  struct gizmo_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    gizmo_cb_t( const special_effect_t& effect, const std::vector<buff_t*>& b ) :
      dbc_proc_callback_t( effect.player, effect ), buffs( b )
    { }

    // TODO: Probability distribution?
    void execute( action_t*, action_state_t* ) override
    {
      size_t index = as<size_t>( rng().roll( 0.5 ) ); // Coin flip
      buffs[ index ]->trigger();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  // Need to manually calculate the stat amounts for the buff, as they are not grabbed from the
  // azerite power spell, like normal
  auto ilevels = power.ilevels();
  auto budgets = power.budget();

  const spell_data_t* increase_spell = effect.player->find_spell( 280653 );
  const spell_data_t* decrease_spell = effect.player->find_spell( 280654 );

  double haste_amount = 0, stat_amount = 0, health_amount = 0;
  for ( size_t i = 0, end = ilevels.size(); i < end; ++i )
  {
    double value = floor( increase_spell->effectN( 1 ).m_coefficient() * budgets[ i ] + 0.5 );
    haste_amount += floor( item_database::apply_combat_rating_multiplier( effect.player,
        CR_MULTIPLIER_ARMOR, ilevels[ i ], value ) );

    stat_amount += floor( decrease_spell->effectN( 4 ).m_coefficient() * budgets[ i ] + 0.5 );
    health_amount += floor( decrease_spell->effectN( 6 ).m_coefficient() * budgets[ i ] + 0.5 );
  }

  auto increase = unique_gear::create_buff<stat_buff_t>( effect.player, "normalization_increase",
      effect.player->find_spell( 280653 ) )
    ->add_stat( STAT_HASTE_RATING, haste_amount )
    ->add_invalidate( CACHE_RUN_SPEED );

  auto decrease = unique_gear::create_buff<stat_buff_t>( effect.player, "normalization_decrease",
      effect.player->find_spell( 280654 ) )
    ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), stat_amount )
    ->add_stat( STAT_MAX_HEALTH, health_amount );

  effect.player->buffs.normalization_increase = increase;

  new gizmo_cb_t( effect, { decrease, increase } );
}

void barrage_of_many_bombs( special_effect_t& effect )
{
  struct barrage_damage_t : public unique_gear::proc_spell_t
  {
    barrage_damage_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "barrage_of_many_bombs", effect.player, effect.player->find_spell( 280984 ),
          effect.item )
    {
      aoe = -1;
      auto values = compute_value( power, data().effectN( 1 ) );
      base_dd_min = std::get<0>( values );
      base_dd_max = std::get<2>( values );

      // Grab missile speed from the driveer
      travel_speed = power.spell_ref().effectN( 1 ).trigger()->missile_speed();
    }
  };

  struct bomb_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    int n_bombs;

    bomb_cb_t( const special_effect_t& effect, const azerite_power_t& power ) :
      dbc_proc_callback_t( effect.player, effect ),
      damage( unique_gear::create_proc_action<barrage_damage_t>( "barrage_of_many_bombs", effect,
              power ) ),
      n_bombs( as<int>( power.spell_ref().effectN( 3 ).base_value() ) )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      damage->set_target( state->target );
      for ( int i = 0; i < n_bombs; ++i )
      {
        damage->execute();
      }
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  new bomb_cb_t( effect, power );
}

void meticulous_scheming( special_effect_t& effect )
{
  // Apparently meticulous scheming bugs and does not proc from some foreground abilities. Blacklist
  // specific abilities here
  static constexpr auto __spell_blacklist = util::make_static_set<unsigned>( {
    201427, // Demon Hunter: Annihilation
    210152, // Demon Hunter: Death Sweep
  } );

  struct seize_driver_t : public dbc_proc_callback_t
  {
    std::vector<unsigned> casts;
    buff_t* base;

    seize_driver_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect ), base( nullptr )
    { }

    bool trigger_seize() const
    { return casts.size() == as<size_t>( effect.driver()->effectN( 1 ).base_value() ); }

    void execute( action_t* a, action_state_t* ) override
    {
      // Presume the broken spells not affecting Meticulous Scheming is a bug
      if ( listener->bugs && __spell_blacklist.find( a->id ) != __spell_blacklist.end() )
      {
        return;
      }

      auto it = range::find( casts, a->id );
      if ( it == casts.end() )
      {
        casts.push_back( a->id );
      }

      if ( trigger_seize() )
      {
        base->expire();
      }
    }

    void activate() override
    {
      dbc_proc_callback_t::activate();

      casts.clear();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  auto secondary = new special_effect_t( effect.player );
  secondary->name_str = "meticulous_scheming_driver";
  secondary->spell_id = 273685;
  secondary->type = effect.type;
  secondary->source = effect.source;
  secondary->proc_flags_ = PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY | PF_NONE_HEAL |
                           PF_NONE_SPELL | PF_MAGIC_SPELL | PF_MAGIC_HEAL;
  secondary->proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.player -> special_effects.push_back( secondary );

  auto meticulous_cb = new seize_driver_t( *secondary );

  auto haste_buff = unique_gear::create_buff<stat_buff_t>( effect.player, "seize_the_moment",
      effect.player->find_spell( 273714 ) )
    ->add_stat( STAT_HASTE_RATING, power.value( 4 ) );

  auto base_buff = unique_gear::create_buff<buff_t>( effect.player, "meticulous_scheming",
      effect.player->find_spell( 273685 ) )
    ->set_stack_change_callback( [meticulous_cb, haste_buff]( buff_t*, int, int new_ ) {
        if ( new_ == 0 )
        {
          meticulous_cb->deactivate();
          if ( meticulous_cb->trigger_seize() )
          {
            haste_buff->trigger();
          }
        }
        else
        {
          meticulous_cb->activate();
        }
    } );

  meticulous_cb->deactivate();
  meticulous_cb->base = base_buff;

  effect.spell_id = 273684;
  effect.custom_buff = base_buff;
  // Spell data has no proc flags for the base spell, so make something up that would resemble
  // "spells and abilities"
  effect.proc_flags_ = PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY | PF_NONE_HEAL |
                       PF_NONE_SPELL | PF_MAGIC_SPELL | PF_MAGIC_HEAL | PF_PERIODIC;

  new dbc_proc_callback_t( effect.player, effect );
}

void synaptic_spark_capacitor( special_effect_t& effect )
{
  struct spark_coil_t : public unique_gear::proc_spell_t
  {
    spark_coil_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "spark_coil", effect.player, effect.player->find_spell( 280847 ),
          effect.item )
    {
      aoe = -1;
      radius = power.spell_ref().effectN( 1 ).base_value();

      auto values = compute_value( power, power.spell_ref().effectN( 3 ) );
      base_dd_min = std::get<0>( values );
      base_dd_max = std::get<2>( values );

      // The DoT is handled in the driver spell.
      dot_duration = timespan_t::zero();
    }
  };

  // Driver for the spark coil that triggers damage
  // TODO Last partial tick does minimal damage?
  struct spark_coil_driver_t : public unique_gear::proc_spell_t
  {
    action_t* damage;

    spark_coil_driver_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "spark_coil_driver", effect.player, effect.player->find_spell( 280655 ),
          effect.item )
    {
      quiet = true;
      damage = unique_gear::create_proc_action<spark_coil_t>( "spark_coil_damage", effect, power );
      dot_duration = data().duration();
      base_tick_time = damage->data().effectN( 2 ).period();
    }

    void tick( dot_t* d ) override
    {
      proc_spell_t::tick( d );

      // TODO: The 6th (partial) tick never happens in game.
      if ( d->get_tick_factor() < 1.0 )
        return;

      damage->set_target( d->target );
      damage->execute();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<spark_coil_driver_t>( "spark_coil",
      effect, power );

  new dbc_proc_callback_t( effect.player, effect );
}

void ricocheting_inflatable_pyrosaw( special_effect_t& effect )
{
  struct rip_t : public unique_gear::proc_spell_t
  {
    rip_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "r.i.p.", effect.player, effect.player->find_spell( 280656 ) )
    {
      auto value = compute_value( power, data().effectN( 1 ) );
      base_dd_min = std::get<0>( value );
      base_dd_max = std::get<2>( value );
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<rip_t>( "r.i.p.",
      effect, power );
  // Just the DPS effect for now
  effect.proc_flags_ = PF_MELEE_ABILITY | PF_RANGED_ABILITY | PF_NONE_SPELL | PF_MAGIC_SPELL |
    PF_PERIODIC;

  new dbc_proc_callback_t( effect.player, effect );
}

void gutripper( special_effect_t& effect )
{
  struct gutripper_t : public unique_gear::proc_spell_t
  {
    gutripper_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( "gutripper", effect.player, effect.player->find_spell( 269031 ) )
    {
      auto value = compute_value( power, power.spell_ref().effectN( 1 ) );
      base_dd_min = std::get<0>( value );
      base_dd_max = std::get<2>( value );
    }
  };

  struct gutripper_cb_t : public dbc_proc_callback_t
  {
    double threshold;

    gutripper_cb_t( const special_effect_t& effect, const azerite_power_t& power ) :
      dbc_proc_callback_t( effect.player, effect ),
      threshold( power.spell_ref().effectN( 2 ).base_value() )
    { }

    void trigger( action_t* a, action_state_t* state ) override
    {
      if ( state->target->health_percentage() < threshold )
      {
        rppm->set_frequency( effect.driver()->real_ppm() );
      }
      else
      {
        rppm->set_frequency( listener->sim->bfa_opts.gutripper_default_rppm );
      }

      dbc_proc_callback_t::trigger( a, state );
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  effect.spell_id = 270668;
  effect.execute_action = unique_gear::create_proc_action<gutripper_t>( "gutripper", effect, power );

  new gutripper_cb_t( effect, power );
}

void battlefield_focus_precision( special_effect_t& effect )
{
  struct bf_damage_cb_t : public dbc_proc_callback_t
  {
    bf_damage_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      auto td = listener->get_target_data( state->target );
      td->debuff.battlefield_debuff->decrement();
      proc_action->set_target( state->target );
      proc_action->schedule_execute();

      // Self-deactivate when the debuff runs out
      if ( td->debuff.battlefield_debuff->check() == 0 )
      {
        deactivate();
      }
    }
  };

  struct bf_trigger_cb_t : public dbc_proc_callback_t
  {
    action_callback_t* damage_cb;
    unsigned stacks;

    bf_trigger_cb_t( const special_effect_t& effect, action_callback_t* cb, unsigned stacks ) :
      dbc_proc_callback_t( effect.player, effect ), damage_cb( cb ), stacks( stacks )
    { }

    void execute( action_t*, action_state_t* state ) override
    {
      auto td = listener->get_target_data( state->target );
      td->debuff.battlefield_debuff->trigger( stacks );
      damage_cb->activate();
    }
  };

  struct bf_damage_t : public unique_gear::proc_spell_t
  {
    bf_damage_t( const special_effect_t& effect, const azerite_power_t& power ) :
      proc_spell_t( effect.name(), effect.player,
          effect.player->find_spell( effect.spell_id == 280627 ? 282720 : 282724 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  // Since we are individualizing the Battlefield Debuff proc, we need to create a slightly custom
  // secondary special effect that procs on this actor's damage
  auto secondary = new special_effect_t( effect.player );
  secondary->name_str = "battlefield_debuff_driver";
  secondary->spell_id = effect.spell_id == 280627 ? 280855 : 280817;
  secondary->type = effect.type;
  secondary->source = effect.source;
  // Note, we override the proc flags to be sourced from the actor, instead of the target's "taken"
  secondary->proc_flags_ = PF_ALL_DAMAGE | PF_PERIODIC;
  secondary->execute_action = unique_gear::create_proc_action<bf_damage_t>( effect.name(), effect,
      power );

  effect.player -> special_effects.push_back( secondary );

  auto trigger_cb = new bf_damage_cb_t( *secondary );
  trigger_cb->deactivate();

  // Fix the driver to point to the RPPM base spell
  effect.spell_id = effect.spell_id == 280627 ? 280854 : 280816;

  new bf_trigger_cb_t( effect, trigger_cb, as<int>( power.spell_ref().effectN( 2 ).base_value() ) );
}

void endless_hunger( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
  {
    return;
  }

  effect.player->passive.versatility_rating += power.value( 2 );
}

void apothecarys_concoctions( special_effect_t& effect )
{
  struct apothecarys_blight_t : public unique_gear::proc_spell_t
  {
    apothecarys_blight_t( const special_effect_t& e, const azerite_power_t& power ):
      proc_spell_t( "apothecarys_blight", e.player, e.player -> find_spell( 287638 ) )
    {
      base_td = power.value( 1 );
      tick_may_crit = true;
    }

    double action_multiplier() const override
    {
      double am = proc_spell_t::action_multiplier();

      am *= 2.0 - this->target->health_percentage() / 100.0;

      return am;
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<apothecarys_blight_t>( "apothecarys_concoctions", effect, power );
  effect.spell_id = 287633;

  new dbc_proc_callback_t( effect.player, effect );
}

void shadow_of_elune( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.driver()->effectN( 1 ).trigger();
  const spell_data_t* buff_data = driver->effectN( 1 ).trigger();

  buff_t* buff = buff_t::find( effect.player, "shadow_of_elune" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "shadow_of_elune", buff_data )
      ->add_stat( STAT_HASTE_RATING, power.value( 1 ) );
  }

  effect.custom_buff = buff;
  effect.spell_id = driver->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void treacherous_covenant( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  // This spell selects which buff to trigger depending on player health %.
  const spell_data_t* driver = effect.driver()->effectN( 1 ).trigger();
  const spell_data_t* buff_data = driver->effectN( 1 ).trigger();

  buff_t* buff = buff_t::find( effect.player, "treacherous_covenant" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "treacherous_covenant", buff_data )
      ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) );
  }

  // TODO: Model losing the buff when droping below 50%.
  effect.player->register_combat_begin( [ buff ] ( player_t* )
  {
    buff->trigger();

    // Simple system to allow for some manipulation of the buff uptime.
    if ( buff->sim->bfa_opts.covenant_chance < 1.0 )
    {
      make_repeating_event( buff->sim, buff->sim->bfa_opts.covenant_period, [ buff ]
      {
        if ( buff->rng().roll( buff->sim->bfa_opts.covenant_chance ) )
          buff->trigger();
        else
          buff->expire();
      } );
    }
  } );
}

void seductive_power( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.driver()->effectN( 1 ).trigger();

  buff_t* buff = buff_t::find( effect.player, "seductive_power" );
  if ( !buff )
  {
    // Doesn't look like we can get from the driver to this buff, hardcode the spell id.
    buff = make_buff<stat_buff_t>( effect.player, "seductive_power", effect.player->find_spell( 288777 ) )
      ->add_stat( STAT_ALL, power.value( 1 ) );
  }

  struct seductive_power_cb_t : public dbc_proc_callback_t
  {
    seductive_power_cb_t( special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      if ( rng().roll( listener->sim->bfa_opts.seductive_power_pickup_chance ) )
        proc_buff->trigger();
      else
        proc_buff->decrement();
    }
  };

  // Spell data is missing proc flags, just pick something that's gonna be close to what the
  // tooltip says.
  effect.proc_flags_ = PF_ALL_DAMAGE | PF_ALL_HEAL;
  effect.spell_id = driver->id();
  effect.custom_buff = buff;

  new seductive_power_cb_t( effect );

  int initial_stacks = effect.player->sim->bfa_opts.initial_seductive_power_stacks;
  if ( initial_stacks > 0 )
  {
    effect.player->register_combat_begin( [ = ] ( player_t* ) { buff->trigger( initial_stacks ); } );
  }
}

void bonded_souls( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.driver()->effectN( 2 ).trigger();
  const spell_data_t* driver_buff_data = driver->effectN( 1 ).trigger();
  const spell_data_t* haste_buff_data = driver_buff_data->effectN( 1 ).trigger();

  buff_t* haste_buff = buff_t::find( effect.player, "bonded_souls" );
  if ( !haste_buff )
  {
    haste_buff = make_buff<stat_buff_t>( effect.player, "bonded_souls", haste_buff_data )
      ->add_stat( STAT_HASTE_RATING, power.value( 1 ) )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  buff_t* driver_buff = buff_t::find( effect.player, "bonded_souls_driver" );
  if ( !driver_buff )
  {
    // TODO: Healing portion of the trait?
    driver_buff = make_buff( effect.player, "bonded_souls_driver", driver_buff_data )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_tick_zero( true )
      ->set_tick_callback( [ haste_buff ] ( buff_t*, int, timespan_t )
        { haste_buff->trigger(); } );
  }

  // Spell data is missing proc flags, just pick something that's gonna be close to what the
  // tooltip says.
  effect.proc_flags_ = PF_ALL_DAMAGE | PF_ALL_HEAL;
  effect.spell_id = driver->id();
  effect.custom_buff = driver_buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void fight_or_flight( special_effect_t& effect )
{
  // Callback that attempts to trigger buff if target below 35%
  struct fof_cb_t : public dbc_proc_callback_t
  {
    buff_t* fof_buff;
    double threshold;

    fof_cb_t( const special_effect_t& effect, buff_t* buff, const azerite_power_t& power ) :
      dbc_proc_callback_t( effect.player, effect ),
      fof_buff( buff ),
      threshold( power.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() )
    {}

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      if ( state -> target -> health_percentage() < threshold )
      {
        fof_buff -> trigger();
      }
    }
  };

  effect.proc_flags_ = PF_ALL_DAMAGE;

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  buff_t* fof_buff = buff_t::find( effect.player, "fight_or_flight" );
  if ( !fof_buff )
  {
    fof_buff = make_buff<stat_buff_t>( effect.player, "fight_or_flight", effect.player -> find_spell( 287827 ) )
      -> add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT ), power.value( 1 ) )
      // The buff's icd is handled by a debuff on the player ingame, we skip that and just use the debuff's duration as cooldown for the buff
      -> set_cooldown( effect.player -> find_spell( 287825 ) -> duration() );
  }

  // TODO: Model gaining the buff when droping below 35%.
  effect.player -> register_combat_begin( [ fof_buff ] ( player_t* )
  {
    // Simple system to allow for some manipulation of the buff uptime.
    if ( fof_buff -> sim -> bfa_opts.fight_or_flight_chance > 0.0 )
    {
      make_repeating_event( fof_buff -> sim, fof_buff -> sim -> bfa_opts.fight_or_flight_period, [ fof_buff ]
      {
        if ( fof_buff -> rng().roll( fof_buff -> sim -> bfa_opts.fight_or_flight_chance ) )
          fof_buff -> trigger();
      } );
    }
  } );

  effect.spell_id = power.spell() -> effectN( 1 ).trigger() -> id();

  new fof_cb_t( effect, fof_buff, power );
}

void undulating_tides( special_effect_t& effect )
{
  struct undulating_tides_t : public unique_gear::proc_spell_t
  {
    undulating_tides_t( const special_effect_t& e, const azerite_power_t& power ) :
      proc_spell_t( "undulating_tides", e.player, e.player->find_spell( 303389 ) )
    {
      base_dd_min = base_dd_max = power.value( 1 );
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  effect.execute_action = unique_gear::create_proc_action<undulating_tides_t>( "undulating_tides", effect, power );
  effect.spell_id = 303388;

  auto proc = new dbc_proc_callback_t( effect.player, effect );

  auto lockout = buff_t::find( effect.player, "undulating_tides_lockout" );
  if ( !lockout )
  {
    lockout = make_buff( effect.player, "undulating_tides_lockout", effect.player->find_spell( 303438 ) );
    lockout->set_refresh_behavior( buff_refresh_behavior::DISABLED );
    lockout->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
      if ( new_ == 1 )
        proc->deactivate();
      else
        proc->activate();
    } );
  }

  timespan_t timer = effect.player->sim->bfa_opts.undulating_tides_lockout_timer;
  double chance = effect.player->sim->bfa_opts.undulating_tides_lockout_chance;

  if ( chance )
  {
    effect.player->register_combat_begin( [lockout, timer, chance]( player_t* ) {
      make_repeating_event( *lockout->sim, timer, [lockout, chance]() {
        if ( lockout->rng().roll( chance ) )
          lockout->trigger();
      } );
    } );
  }
}

void loyal_to_the_end( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  double value = std::min( power.value( 3 ),
    power.value( 1 ) + power.value( 2 ) * effect.player->sim->bfa_opts.loyal_to_the_end_allies );
  effect.player->passive.mastery_rating += value;

  // Crude event based triggering of ally death bonus.
  // TODO: Develop something that can be better controlled via APL
  timespan_t timer = effect.player->sim->bfa_opts.loyal_to_the_end_ally_death_timer;
  double chance    = effect.player->sim->bfa_opts.loyal_to_the_end_ally_death_chance;

  auto buff = buff_t::find( effect.player, "loyal_to_the_end" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "loyal_to_the_end", effect.trigger()->effectN( 1 ).trigger() )
      ->add_stat( STAT_CRIT_RATING, value )
      ->add_stat( STAT_HASTE_RATING, value )
      ->add_stat( STAT_VERSATILITY_RATING, value );
    buff->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  }

  if ( chance )
  {
    effect.player->register_combat_begin( [buff, timer, chance]( player_t* ) {
      make_repeating_event( *buff->sim, timer, [buff, chance]() {
        if ( buff->rng().roll( chance ) )
          buff->trigger();
      } );
    } );
  }
}

void arcane_heart( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  auto buff = buff_t::find( effect.player, "arcane_heart" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "arcane_heart", effect.trigger() )
      // Damage/heal threshold (currently at 1.5million) stored in default_value of the "arcane_heart" buff
      ->set_default_value( power.spell_ref().effectN( 1 ).base_value() );
  }

  auto omni = static_cast<stat_buff_t*>( buff_t::find( effect.player, "omnipotence" ) );
  if ( !omni )
  {
    omni = make_buff<stat_buff_t>( effect.player, "omnipotence", effect.trigger()->effectN( 1 ).trigger() );
    omni->add_stat( STAT_NONE, power.value( 2 ) );
    omni->set_stack_change_callback( [omni]( buff_t*, int, int new_ ) {
      if ( new_ )
      {
        static constexpr stat_e ratings[] = {
          STAT_CRIT_RATING, STAT_HASTE_RATING, STAT_MASTERY_RATING, STAT_VERSATILITY_RATING
        };
        omni->stats.front().stat = util::highest_stat( omni->player, ratings );
      }
      omni->sim->print_debug( "arcane_heart omnipotence stack change: highest stat {} {} by {}",
        util::stat_type_string( omni->stats.front().stat ), new_ ? "increased" : "decreased",
        omni->stats.front().amount );
    } );
  }

  // TODO?: Implement healing contribution to Arcane Heart threshold. Not all healing counts:
  // * All direct-cast healing seems to count
  // * Proc heals from trait/essences/etc. seem to count for the most part. Some exceptions may
  //    exist, one being Seeds of Eonar per-stack healing from Lifebinder's Invocation Esssence
  // * Some passive healing may not count, such as Ysera's Gift from Druid Restoration Affinity
  // * Passive absorb effects & 'absorb-like' effects such has the Guardian Druid talent Earthwarden
  //    do not seem to count. Unknown if active shields do.
  // * Leech does not seem to count.
  effect.player->assessor_out_damage.add( assessor::TARGET_DAMAGE + 1, [buff, omni]( result_amount_type, action_state_t* state ) {
    double amount = state->result_amount;

    if ( amount <= 0 || omni->check() )  // doesn't count damage while omnipotence is up
      return assessor::CONTINUE;

    // spell attribute bit 416 being set seems to be a necessary condition for damage to count
    // TODO: determine if this bit is also a sufficient condition
    if ( state->action->s_data != spell_data_t::nil() &&
      !state->action->s_data->flags( static_cast<spell_attribute>( 416u ) ) )
    {
      return assessor::CONTINUE;
    }

    if ( !buff->check() )  // special handling for damage from precombat actions
    {
      make_event( *buff->sim, [buff, amount] {
        buff->current_value -= amount;
        buff->sim->print_debug(
          "arcane_heart_counter ability:precombat damage:{} counter now at:{}", amount, buff->current_value );
      } );

      return assessor::CONTINUE;
    }

    buff->current_value -= amount;

    buff->sim->print_debug( "arcane_heart_counter ability:{} damage:{} counter now at:{}", state->action->name(),
      amount, buff->current_value );

    if ( buff->current_value <= 0 )
    {
      make_event( *buff->sim, [omni] { omni->trigger(); } );
      buff->current_value = buff->default_value;  // damage doesn't spill over? TODO: confirm
    }

    return assessor::CONTINUE;
  } );

  effect.player->register_combat_begin( [buff]( player_t* ) {
    buff->trigger();
    make_repeating_event( buff->sim, 1_s, [buff] {
      buff->current_value -= buff->sim->bfa_opts.arcane_heart_hps;
      buff->sim->print_debug( "arcane_heart_counter healing:{} counter now at:{}", buff->sim->bfa_opts.arcane_heart_hps,
                              buff->current_value );
    } );
  } );
}

void clockwork_heart( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  buff_t* clockwork = buff_t::find( effect.player, "clockwork_heart" );
  if ( !clockwork )
    clockwork = make_buff<stat_buff_t>( effect.player, "clockwork_heart", effect.trigger()->effectN( 1 ).trigger() )
      ->add_stat( STAT_CRIT_RATING, power.value( 1 ) )
      ->add_stat( STAT_HASTE_RATING, power.value( 1 ) )
      ->add_stat( STAT_MASTERY_RATING, power.value( 1 ) )
      ->add_stat( STAT_VERSATILITY_RATING, power.value( 1 ) );

  buff_t* ticker = buff_t::find( effect.player, "clockwork_heart_ticker" );
  if ( !ticker )
  {
    ticker = make_buff( effect.player, "clockwork_heart_ticker", effect.trigger() )
      ->set_quiet( true )
      ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
      ->set_tick_on_application( true )
      ->set_tick_callback( [clockwork]( buff_t*, int, timespan_t ) {
        clockwork->trigger();
      } );
  }

  effect.player->register_combat_begin( [ticker]( player_t* ) {
    ticker->trigger();
  } );
}

void personcomputer_interface( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  auto trinket = effect.player->find_item_by_id( 167555 );  // Pocket-sized Computation Device
  if ( !trinket )
  {
    effect.player->sim->print_debug( "Person-Computer Interface equipped with no Pocket-Sized Computation Device." );
    return;
  }

  auto it = range::find_if( trinket->parsed.special_effects, []( const special_effect_t* e ) {
    // Yellow Punchcards have type == -1
    return e->type == -1 && e->source == SPECIAL_EFFECT_SOURCE_GEM && e->is_stat_buff();
  } );

  if ( it == trinket->parsed.special_effects.end() )
  {
    effect.player->sim->print_debug( "Person-Computer Interface equipped with no Yellow Punchcards." );
    return;
  }

  auto stat = ( *it )->stat_type();

  if ( !stat )
  {
    effect.player->sim->print_debug( "Person-Computer Interface could not find a stat from {}.", ( *it )->name() );
    return;
  }

  effect.player->sim->print_debug( "Person-Computer Interface found {}, adding {} {}.", ( *it )->name(),
    power.value( 1 ), util::stat_type_string( stat ) );

  switch( stat )
  {
    case STAT_HASTE_RATING:       effect.player->passive.haste_rating += power.value(1);   break;
    case STAT_CRIT_RATING:        effect.player->passive.crit_rating += power.value(1);    break;
    case STAT_MASTERY_RATING:     effect.player->passive.mastery_rating += power.value(1); break;
    case STAT_VERSATILITY_RATING: effect.player->passive.haste_rating += power.value(1);   break;
    default: break;
  }
}

//Heart of Darkness
void heart_of_darkness( special_effect_t& effect )
{
  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
    return;

  buff_t* heart_of_darkness = buff_t::find( effect.player, "heart_of_darkness" );
  if ( !heart_of_darkness )
  {
    double value = power.value( 1 );
    heart_of_darkness =
        make_buff<stat_buff_t>( effect.player, "heart_of_darkness", power.spell_ref().effectN( 1 ).trigger() )
            ->add_stat( STAT_CRIT_RATING, value )
            ->add_stat( STAT_HASTE_RATING, value )
            ->add_stat( STAT_MASTERY_RATING, value )
            ->add_stat( STAT_VERSATILITY_RATING, value );
    effect.player->register_combat_begin( [heart_of_darkness]( player_t* ) {
      if ( heart_of_darkness->player->composite_total_corruption() >= 25 )  // This number is not found in spell data
      {
        heart_of_darkness->trigger();
      }
    } );
  }
}

void impassive_visage(special_effect_t& effect)
{
  class impassive_visage_heal_t : public heal_t
  {
  public:
    impassive_visage_heal_t(player_t* p, const spell_data_t* s, double amount) :
      heal_t("impassive_visage", p, s)
    {
      target = p;
      background = true;
      base_dd_min = base_dd_max = amount;
    }
  };

  class impassive_visage_cb_t : public dbc_proc_callback_t
  {
    impassive_visage_heal_t* heal_action;
  public:
    impassive_visage_cb_t(const special_effect_t& effect, impassive_visage_heal_t* heal) :
      dbc_proc_callback_t(effect.player, effect),
      heal_action(heal)
    {
    }

    void execute(action_t* /* a */, action_state_t* /* state */) override
    {
      heal_action->execute();
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell(effect.driver()->name_cstr());
  if (!power.enabled())
  {
    return;
  }
  const spell_data_t* driver = power.spell();
  const spell_data_t* heal = driver->effectN(1).trigger();

  auto heal_action = new impassive_visage_heal_t(effect.player, heal, power.value());
  new impassive_visage_cb_t(effect, heal_action);
}

void gemhide(special_effect_t& effect)
{
  struct gemhide_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;
    double damage_threshold;

    gemhide_cb_t( const special_effect_t& effect, buff_t* buff, double dt ) :
      dbc_proc_callback_t( effect.player, effect ), buff( buff ), damage_threshold( dt )
    {
    }

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      // Gemhide takes the amount of damage before absorbs
      if ( state -> result_mitigated > listener -> max_health() * damage_threshold )
      {
        buff -> trigger();
      }
    }
  };

  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
  {
    return;
  }

  // The callback data isn't on the azerite's main spell ID, but on 270579
  effect.spell_id = power.spell() -> effectN( 2 ).trigger() -> id();

  buff_t* buff = buff_t::find( effect.player, "gemhide" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "gemhide", power.spell()
                                   -> effectN( 2 ).trigger() -> effectN( 1 ).trigger() )
          -> add_stat( STAT_AVOIDANCE_RATING, power.value( 1 ) )
          -> add_stat( STAT_ARMOR, power.value( 2 ) );
  }
  new gemhide_cb_t( effect, buff, power.spell() -> effectN( 3 ).percent() );
}

void crystalline_carapace( special_effect_t& effect )
{
  class crystalline_carapace_buff_t : public stat_buff_t
  {
    action_callback_t* callback;

  public:
    crystalline_carapace_buff_t( player_t* p, const spell_data_t* s, double armor, action_callback_t* callback )
      : stat_buff_t( p, "crystalline_carapace", s ), callback( callback )
    {
      add_stat( STAT_ARMOR, armor );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      stat_buff_t::expire_override( expiration_stacks, remaining_duration );
      callback->deactivate();
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      stat_buff_t::start( stacks, value, duration );
      callback->activate();
    }
  };

  class crystalline_carapace_damage_t : public spell_t
  {
  public:
    crystalline_carapace_damage_t( player_t* p, const spell_data_t* s, double amount )
      : spell_t( "crystalline_carapace_damage", p, s )
    {
      background  = true;
      base_dd_min = base_dd_max = amount;
    }
  };

  class crystalline_carapace_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;

  public:
    crystalline_carapace_cb_t( const special_effect_t& effect, buff_t* buff )
      : dbc_proc_callback_t( effect.player, effect ), buff( buff )
    {
    }

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      if ( state->result_amount * 10.0 > listener->max_health() )
      {
        buff->trigger();
      }
    }
  };

  class crystalline_carapace_melee_attack_cb_t : public dbc_proc_callback_t
  {
    action_t* enemy_damage;

  public:
    crystalline_carapace_melee_attack_cb_t( const special_effect_t& effect, action_t* enemy_damage )
      : dbc_proc_callback_t( effect.player, effect ), enemy_damage( enemy_damage )
    {
    }

    void execute( action_t* /* a */, action_state_t* state ) override
    {
      if ( state->action )
      {
        enemy_damage->target = state->action->player;
        enemy_damage->schedule_execute();
      }
    }
  };

  azerite_power_t power = effect.player->find_azerite_spell( effect.driver()->name_cstr() );
  if ( !power.enabled() )
  {
    return;
  }

  effect.spell_id                = 271537;
  const spell_data_t* buff_spell = effect.player->find_spell( 271538 );
  action_t* damage_action =
      new crystalline_carapace_damage_t( effect.player, effect.player->find_spell( 271539 ), power.value( 1 ) );

  // TODO: get this to proc.
  auto secondary      = new special_effect_t( effect.player );
  secondary->name_str = "crystalline_carapace_melee_taken";
  secondary->spell_id = 271538;
  secondary->type     = effect.type;
  secondary->source   = effect.source;

  effect.player->special_effects.push_back( secondary );

  auto trigger_cb = new crystalline_carapace_melee_attack_cb_t( *secondary, damage_action );
  trigger_cb->deactivate();

  buff_t* buff = buff_t::find( effect.player, "crystalline_carapace" );
  if ( !buff )
  {
    buff = make_buff<crystalline_carapace_buff_t>( effect.player, buff_spell, power.value( 2 ), trigger_cb );
  }

  new crystalline_carapace_cb_t( effect, buff );
}

} // Namespace special effects ends

namespace azerite_essences
{
void stamina_milestone( special_effect_t& effect )
{
  const auto spell = effect.driver();

  if ( effect.player->sim->debug )
  {
    effect.player->sim->out_debug.print( "{} increasing stamina by {}% ({})",
        effect.player->name(), spell->effectN( 1 ).base_value(), spell->name_cstr() );
  }

  effect.player->base.attribute_multiplier[ ATTR_STAMINA ] *= 1.0 + spell->effectN( 1 ).percent();
}

//The Crucible of Flame
//Major Power: Concentrated Flame
// TODO: Open questions:
// 1) Rank 2 tooltip refers to base * y% damage, but the effect data has an actual base value to
//    scale off of. Which is used in game?
// 2) Refresh behavior of both dots
// 3) Is rank 3 proc allowed to stack if you have rank 3 essence?
void the_crucible_of_flame( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
  {
    return;
  }

  struct ancient_flame_t : public unique_gear::proc_spell_t
  {
    ancient_flame_t( const special_effect_t& effect, const std::string& name, const azerite_essence_t& essence ) :
      proc_spell_t( name, effect.player, effect.player->find_spell( 295367 ), essence.item() )
    {
      base_td = item_database::apply_combat_rating_multiplier( effect.player,
        combat_rating_multiplier_type::CR_MULTIPLIER_JEWLERY,
        essence.item()->item_level(),
        essence.spell_ref( 1u, essence_type::MINOR ).effectN( 3 ).average( essence.item() ) );

      base_td_multiplier *= 1 + essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent();
    }

    // Refresh to 10 seconds
    timespan_t calculate_dot_refresh_duration( const dot_t* /* dot */, timespan_t triggered_duration ) const override
    { return triggered_duration; }
  };

  auto action = unique_gear::create_proc_action<ancient_flame_t>( "ancient_flame", effect,
      "ancient_flame", essence );
  // Add rank 3 upgrade that increases stack count of the dot by some value
  action->dot_max_stack +=
    as<int>(essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).base_value());

  effect.proc_flags_ = PF_MELEE_ABILITY | PF_RANGED_ABILITY | PF_NONE_SPELL | PF_MAGIC_SPELL | PF_PERIODIC;
  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.execute_action = action;

  new dbc_proc_callback_t( effect.player, effect );
}

struct concentrated_flame_t : public azerite_essence_major_t
{
  struct missile_t : public unique_gear::proc_spell_t
  {
    unsigned cast_number;
    double multiplier;

    missile_t( player_t* p, const azerite_essence_t& essence ) :
      proc_spell_t( "concentrated_flame_missile", p, p->find_spell( 295374 ), essence.item() ),
      cast_number( 1u ), multiplier( essence.spell_ref( 1u ).effectN( 3 ).percent() )
    {
      // Base damage is defined in the minor spell for some reason
      base_dd_min = base_dd_max =
        essence.spell_ref( 1u, essence_spell::BASE, essence_type::MINOR ).effectN( 2 ).average( essence.item() );
    }

    double action_multiplier() const override
    { return proc_spell_t::action_multiplier() * cast_number * multiplier; }
  };

  struct burn_t : public unique_gear::proc_spell_t
  {
    double multiplier;

    burn_t( player_t* p, const azerite_essence_t& essence ) :
      proc_spell_t( "concentrated_flame_burn", p, p->find_spell( 295368 ), essence.item() ),
      multiplier( essence.spell_ref( 2u, essence_spell::UPGRADE ).effectN( 1 ).percent() )
    {
      callbacks = false;
    }

    void init() override
    {
      proc_spell_t::init();

      snapshot_flags = update_flags = STATE_TARGET;
    }

    void set_damage( double value )
    { base_td = value * multiplier / ( dot_duration / base_tick_time ); }

    // Max duration + ongoing tick
    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
    { return triggered_duration + dot->tick_event->remains(); }
  };

  unsigned stack;
  missile_t* missile;
  burn_t* burn;

  concentrated_flame_t( player_t* p, util::string_view options_str ) :
    azerite_essence_major_t( p, "concentrated_flame", p->find_spell( 295373 ) ),
    stack( 1u ), burn( nullptr )
  {
    parse_options( options_str );

    missile = new missile_t( p, essence );
    add_child( missile );

    cooldown->charges += as<int>(essence.spell_ref( 3u, essence_spell::UPGRADE ).effectN( 1 ).base_value());

    if ( essence.rank() >= 2 )
    {
      burn = new burn_t( p, essence );
      add_child( burn );
    }
  }

  void execute() override
  {
    azerite_essence_major_t::execute();

    missile->set_target( target );
    missile->cast_number = stack;
    missile->execute();

    if ( burn )
    {
      burn->set_target( target );
      burn->set_damage( missile->execute_state->result_amount );
      burn->execute();
    }

    if ( ++stack == 4u )
    {
      stack = 1u;
    }
  }

  void reset() override
  {
    azerite_essence_major_t::reset();

    stack = 1u;
  }
}; //End of The Crucible of Flame

//Memory of Lucid Dreams
//Major Power: Memory of Lucid Dreams
struct memory_of_lucid_dreams_t : public azerite_essence_major_t
{
  double precombat_time;

  memory_of_lucid_dreams_t( player_t* p, util::string_view options_str ) :
    azerite_essence_major_t( p, "memory_of_lucid_dreams", p->find_spell( 298357 ) )
  {
    // Default the precombat timer to the player's base, unhasted gcd
    precombat_time = p -> base_gcd.total_seconds();

    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );

    harmful = false;

    // Adjust the buff based on essence data
    player->buffs.memory_of_lucid_dreams->set_cooldown( timespan_t::zero() );
    player->buffs.memory_of_lucid_dreams->set_duration(
      essence.spell_ref( 1u ).duration() + essence.spell_ref( 2u, essence_spell::UPGRADE ).effectN( 1 ).time_value()
    );

    if ( essence.rank() >= 3 )
    {
      player->buffs.memory_of_lucid_dreams->add_stat( STAT_LEECH_RATING,
        essence.spell_ref( 1u ).effectN( 6 ).average( essence.item() ) );
    }
  }

  void init_finished() override
  {
    azerite_essence_major_t::init_finished();

    if ( action_list -> name_str == "precombat" && !background )
    {
      double MIN_TIME = player -> base_gcd.total_seconds(); // the player's base unhasted gcd: 1.5s
      double MAX_TIME = player -> buffs.memory_of_lucid_dreams -> buff_duration().total_seconds() - 1;

      // Ensure that we're using a positive value
      if ( precombat_time < 0 )
        precombat_time = -precombat_time;

      if ( precombat_time > MAX_TIME )
      {
        precombat_time = MAX_TIME;
        sim -> error( "{} tried to use Memory of Lucid Dreams in precombat more than {} seconds before combat begins, setting to {}",
                       player -> name(), MAX_TIME, MAX_TIME );
      }
      else if ( precombat_time < MIN_TIME )
      {
        precombat_time = MIN_TIME;
        sim -> error( "{} tried to use Memory of Lucid Dreams in precombat less than {} before combat begins, setting to {} (has to be >= base gcd)",
                       player -> name(), MIN_TIME, MIN_TIME );
      }
    }
    else precombat_time = 0;
  }

  void execute() override
  {
    azerite_essence_major_t::execute();

    player->buffs.memory_of_lucid_dreams->trigger();

    if ( ! player -> in_combat && precombat_time > 0 )
    {
      player -> buffs.memory_of_lucid_dreams -> extend_duration( player, timespan_t::from_seconds( -precombat_time ) );
      cooldown -> adjust( timespan_t::from_seconds( -precombat_time ), false );

      sim -> print_debug( "{} used Memory of Lucid Dreams in precombat with precombat time = {}, adjusting duration and remaining cooldown.",
                          player -> name(), precombat_time );
    }
  }
}; //End of Memory of Lucid Dreams

// Blood of the Enemy
void blood_of_the_enemy( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
    return;

  struct bloodsoaked_callback_t : public dbc_proc_callback_t
  {
    buff_t* haste_buff;
    double chance;
    int decrement_stacks;

    bloodsoaked_callback_t( player_t* p, special_effect_t& e, buff_t* b, double c, int decrement_stacks) :
      dbc_proc_callback_t( p, e ), haste_buff( b ), chance( c ), decrement_stacks(decrement_stacks)
    {}

    void execute( action_t*, action_state_t* ) override
    {
      // Does not proc when haste buff is up
      if ( haste_buff->check() )
        return;

      if ( proc_buff && proc_buff->trigger() && proc_buff->check() == proc_buff->max_stack() )
      {
        haste_buff->trigger();
        if ( rng().roll( chance ) )
          proc_buff->decrement(decrement_stacks);
        else
          proc_buff->expire();
      }
    }
  };

  effect.proc_flags2_ = PF2_CRIT;

  // buff id=297162, not referenced in spell data
  auto counter = static_cast<stat_buff_t*>( buff_t::find( effect.player, "bloodsoaked_counter" ) );
  if ( !counter )
    counter = make_buff<stat_buff_t>( effect.player, "bloodsoaked_counter", effect.player->find_spell( 297162 ) );

  // Crit per stack from R2 upgrade stored in R1 MINOR BASE effect#3
  if ( essence.rank() >= 2 )
    counter->add_stat(
      STAT_CRIT_RATING, essence.spell_ref( 1u, essence_type::MINOR ).effectN( 3 ).average( essence.item() ) );

  effect.custom_buff = counter;

  // buff id=297168, not referenced in spell data
  // Haste from end proc stored in R1 MINOR BASE effect#2
  auto haste_buff = static_cast<stat_buff_t*>( buff_t::find( effect.player, "bloodsoaked" ) );
  if ( !haste_buff )
  {
    haste_buff = make_buff<stat_buff_t>( effect.player, "bloodsoaked", effect.player->find_spell( 297168 ) );
    haste_buff->add_stat(
      STAT_HASTE_RATING, essence.spell_ref( 1u, essence_type::MINOR ).effectN( 2 ).average( essence.item() ) );
  }

  double chance = 0;
  int decrement_stacks = 0;
  if ( essence.rank() >= 3 )
  {
    // 25% chance to...
    chance = essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent();
    // only lose 30 stacks
	decrement_stacks = as<int>(essence.spell_ref(3u, essence_spell::UPGRADE, essence_type::MINOR).effectN(2).base_value());
  }

  new bloodsoaked_callback_t( effect.player, effect, haste_buff, chance, decrement_stacks );
}

// Major Power: Blood of the Enemy
struct blood_of_the_enemy_t : public azerite_essence_major_t
{
  blood_of_the_enemy_t( player_t* p, util::string_view options_str ) :
    azerite_essence_major_t( p, "blood_of_the_enemy", p->find_spell( 297108 ) )
  {
    parse_options( options_str );
    aoe         = -1;
    may_crit    = true;
    base_dd_min = base_dd_max = essence.spell_ref( 1u ).effectN( 1 ).average( essence.item() );

    if ( essence.rank() >= 2 )
      cooldown->duration *=
        1.0 + essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MAJOR ).effectN( 1 ).percent();
  }

  result_e calculate_result( action_state_t* s ) const override
  {
    if ( player->rng().roll( 1.0 - sim->bfa_opts.blood_of_the_enemy_in_range ) )
      return RESULT_MISS;

    return azerite_essence_major_t::calculate_result( s );
  }

  void impact( action_state_t* s ) override
  {
    azerite_essence_major_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // 25% increased chance to be hit debuff
      auto td = player->get_target_data( s->target );
      td->debuff.blood_of_the_enemy->trigger();
    }
  }

  void execute() override
  {
    // R3 25% critical hit damage buff
    if ( essence.rank() >= 3 )
    {
      player->buffs.seething_rage->trigger();  // First buff the player
      range::for_each( player->pet_list, []( pet_t* pet ) {
        if ( !pet->is_sleeping() )
          pet->buffs.seething_rage->trigger();
      } );
    }

    azerite_essence_major_t::execute();
  }
};  // End of Blood of the Enemy

//Essence of the Focusing Iris
//Major Power: Focused Azerite Beam
void essence_of_the_focusing_iris( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
  {
    return;
  }

  struct focused_energy_driver_t : dbc_proc_callback_t
  {
    player_t* trigger_target;
    int init_stacks;

    focused_energy_driver_t( const special_effect_t& effect, int init_stacks ) :
      dbc_proc_callback_t( effect.player, effect ),
      trigger_target(),
      init_stacks( init_stacks )
    { }

    void execute( action_t*, action_state_t* s ) override
    {
      // The effect remembers which target was hit while no buff was active and then only triggers when
      // that target is hit again.
      // TODO: Some aoe effects may not proc this and should be checked more rigorously. Proc flags include ticks etc

      if ( !proc_buff->check() )
      {
        trigger_target = s->target;
        proc_buff->trigger( init_stacks );
      }
      else if ( trigger_target == s->target )
      {
        proc_buff->trigger();
      }
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      trigger_target = nullptr;
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT;

  int init_stacks = 1;
  if ( essence.rank() >= 3 )
    init_stacks = as<int>(essence.spell_ref( 3u, essence_type::MINOR ).effectN( 1 ).base_value());

  double haste = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 2 ).average( essence.item() );
  if ( essence.rank() >= 2 )
    haste *= 1.0 + essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent();

  effect.custom_buff = unique_gear::create_buff<stat_buff_t>( effect.player, "focused_energy",
      effect.player->find_spell( 295248 ) )
    ->add_stat( STAT_HASTE_RATING, haste );

  new focused_energy_driver_t( effect, init_stacks );
}

struct focused_azerite_beam_tick_t : public spell_t
{
  focused_azerite_beam_tick_t(player_t* p, const std::string& n, double td) :
    spell_t( n, p, p->find_spell( 295261 ) )
  {
    aoe = -1;
    background = true;
    may_crit = true;
    base_execute_time = 0_ms;
    base_dd_min = base_dd_max = td;
  }

  result_amount_type amount_type( const action_state_t* /* s */, bool ) const override
  {
    return result_amount_type::DMG_OVER_TIME;
  }
};

struct focused_azerite_beam_t : public azerite_essence_major_t
{
  focused_azerite_beam_t( player_t* p, util::string_view options_str ) :
    azerite_essence_major_t( p, "focused_azerite_beam", p->find_spell(295258) )
  {
    parse_options(options_str);

    harmful = true;
    channeled = true;
    tick_zero = true;
    interrupt_auto_attack = true;

    double tick = essence.spell_ref( 1u, essence_type::MAJOR ).effectN( 1 ).average( essence.item() );
    double mult = 1 + essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MAJOR ).effectN( 1 ).percent();

    base_execute_time *= mult;

    dot_duration = base_tick_time * round( dot_duration/base_tick_time );

    tick_action = new focused_azerite_beam_tick_t( p, "focused_azerite_beam_tick", tick );
  }

  bool usable_moving() const override
  {
    if(essence.rank() >= 3)
      return true;

    return azerite_essence_major_t::usable_moving();
  }
}; //End of Essence of the Focusing Iris

/**Condensed Life Force
 * Minor Power: Condensed Life Force
 * Launches missile id=295835, debuffs targets with id=295838 on missile proc,
 * not impact, to take 5% more damage
 */
void condensed_life_force(special_effect_t& effect)
{
  auto essence = effect.player->find_azerite_essence(effect.driver()->essence_id());
  if (!essence.enabled())
    return;

  struct condensed_lifeforce_t : public unique_gear::proc_spell_t
  {
    azerite_essence_t essence;

    condensed_lifeforce_t(const special_effect_t& e, const std::string& n, const azerite_essence_t& ess) :
      proc_spell_t(n, e.player, e.player->find_spell( 295835 )), essence(ess)
    {
      double dam = ess.spell_ref(1u, essence_type::MINOR).effectN(1).average(ess.item()); // R1
      dam *= 1.0 + ess.spell_ref(2u, essence_spell::UPGRADE, essence_type::MINOR).effectN(1).percent(); // R2
      base_dd_min = base_dd_max = dam;
    }

    void execute() override
    {
      unique_gear::proc_spell_t::execute();

      if (essence.rank() >= 3)
      {
        // Debuff applies on execute BEFORE the missile impacts
        auto td = player->get_target_data(target);
        td->debuff.condensed_lifeforce->trigger();
      }
    }
  };

  auto action = unique_gear::create_proc_action<condensed_lifeforce_t>( "azerite_spike", effect, "azerite_spike", essence );

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.execute_action = action;

  new dbc_proc_callback_t( effect.player, effect );
}

/**Condensed Life-Force
 * Major Power: Guardian of Azeroth
 * Summoning spell id=300091
 * Summon buff on player id=295840
 * Stacking haste buff from R3 id=295855
 * Guardian standard cast id=295856
 *  standard damage is id=302555
 *  standard amount is r1 minor base (id=295834)
 * Guardian volley cast is id=303347
 *  volley damage is id=303351
 *  volley amount is r2 major upgrade (id=295841)
 * Shard debuff on target id=295838
 */

struct guardian_of_azeroth_t : public azerite_essence_major_t
{
  struct guardian_of_azeroth_pet_t : public pet_t
  {
    struct azerite_spike_t : public spell_t
    {
      azerite_essence_t essence;
      player_t* owner;

      azerite_spike_t(util::string_view n, pet_t* p, util::string_view options, const azerite_essence_t& ess) :
        spell_t(n, p, p->find_spell(295856)), essence(ess), owner(p->owner)
      {
        parse_options(options);
        may_crit = true;
        double dam = ess.spell_ref(1u, essence_type::MINOR).effectN(1).average(ess.item()); // R1
        dam *= 1.0 + ess.spell_ref(2u, essence_spell::UPGRADE, essence_type::MINOR).effectN(1).percent(); // R2
        // Hotfix per https://us.forums.blizzard.com/en/wow/t/essences-feedback-damage-essences/181165/41
        dam *= 0.65;
        base_dd_min = base_dd_max = dam;
        gcd_type = gcd_haste_type::NONE;
      }

      void execute() override
      {
        spell_t::execute();

        if (essence.rank() >= 3)
          owner->buffs.guardian_of_azeroth->trigger();
      }

      void impact(action_state_t* s) override
      {
        spell_t::impact(s);

        if (essence.rank() >= 3)
        {
          actor_target_data_t* td = nullptr;
          if (s->target)
            td = owner->get_target_data(s->target);
          if (td)
            td->debuff.condensed_lifeforce->trigger();
        }
      }
    };

    struct azerite_volley_tick_t : public unique_gear::proc_spell_t
    {
      azerite_volley_tick_t(player_t* p, const azerite_essence_t& ess) :
        proc_spell_t("azerite_volley", p, p->find_spell(303351), ess.item())
      {
        aoe = -1;
        base_dd_min = base_dd_max = ess.spell_ref(2u, essence_spell::UPGRADE, essence_type::MAJOR).effectN(1).average(ess.item());
      }
    };

    azerite_essence_t essence;
    buff_t* azerite_volley;

    // TODO: Does pet inherit player's stats? Some, all, or none?
    guardian_of_azeroth_pet_t(player_t* p, const azerite_essence_t& ess) :
      pet_t(p->sim, p, "guardian_of_azeroth", true, true), essence(ess)
    {}

    action_t* create_action(util::string_view name, const std::string& options) override
    {
      if (name == "azerite_spike")
        return new azerite_spike_t(name, this, options, essence);

      return pet_t::create_action(name, options);
    }

    void init_action_list() override
    {
      pet_t::init_action_list();

      if (action_list_str.empty())
        get_action_priority_list("default")->add_action("azerite_spike");
    }

    void create_buffs() override
    {
      pet_t::create_buffs();

      auto volley = new azerite_volley_tick_t(this, essence);
      azerite_volley = make_buff(this, "azerite_volley", find_spell(303347))
        ->set_tick_time_behavior(buff_tick_time_behavior::UNHASTED)
        ->set_quiet(true)
        ->set_tick_callback([volley, this](buff_t*, int, timespan_t) {
          volley->set_target(target);
          volley->execute();
        });
    }

    void arise() override
    {
      pet_t::arise();

      if (essence.rank() >= 2)
        azerite_volley->trigger();
    }


    void demise() override
    {
      pet_t::demise();

      azerite_volley->expire();
      owner->buffs.guardian_of_azeroth->expire();
    }
  };

  pet_t* rockboi;
  buff_t* azerite_volley;
  double precombat_time;
  timespan_t summon_duration;

  guardian_of_azeroth_t(player_t* p, util::string_view options_str) :
    azerite_essence_major_t(p, "guardian_of_azeroth", p->find_spell(295840)),
    summon_duration( player -> find_spell( 300091 ) -> duration() )
  {
    // Default the precombat timer to the player's base, unhasted gcd
    precombat_time = p -> base_gcd.total_seconds();

    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );
    harmful = may_crit = false;
    rockboi = p->find_pet( "guardian_of_azeroth" );
    if ( !rockboi )
      rockboi = new guardian_of_azeroth_pet_t(p, essence);
  }

  void init_finished() override
  {
    azerite_essence_major_t::init_finished();

    if ( action_list -> name_str == "precombat" && !background )
    {
      double MIN_TIME = player -> base_gcd.total_seconds(); // the player's base unhasted gcd: 1.5s
      double MAX_TIME = summon_duration.total_seconds() - 1; // Summoning for 0s would spawn the pet permanently

      // Ensure that we're using a positive value
      if ( precombat_time < 0 )
        precombat_time = -precombat_time;

      if ( precombat_time > MAX_TIME )
      {
        precombat_time = MAX_TIME;
        sim -> error( "{} tried to use Guardian of Azeroth in precombat more than {} seconds before combat begins, setting to {}",
                       player -> name(), MAX_TIME, MAX_TIME );
      }
      else if ( precombat_time < MIN_TIME )
      {
        precombat_time = MIN_TIME;
        sim -> error( "{} tried to use Guardian of Azeroth in precombat less than {} before combat begins, setting to {} (has to be >= base gcd)",
                       player -> name(), MIN_TIME, MIN_TIME );
      }
    }
    else precombat_time = 0;
  }

  void execute() override
  {
    azerite_essence_major_t::execute();

    if ( rockboi -> is_sleeping() )
      rockboi -> summon( summon_duration - timespan_t::from_seconds( precombat_time ) );

    if ( ! player -> in_combat && precombat_time > 0 )
    {
      cooldown -> adjust( timespan_t::from_seconds( -precombat_time ), false );

      sim -> print_debug( "{} used Guardian of Azeroth in precombat with precombat time = {}, adjusting pet duration and remaining cooldown.",
                          player -> name(), precombat_time );
    }
  }
}; //End of Condensed Life-Force

/**Conflict and Strike
 * Minor Power: Strife
 * driver id=305148
 * stacking vers buff from driver->effect#1->trigger id=304056
 * R2 duration inc from R2:MINOR:UPGRADE id=304055
 * R3 maxstack inc from R3:MINOR:UPGRADE id=304125
 * R3 major power doubling of vers from R3:MAJOR:UPGRADE->effect#3->trigger id=304720
 */
void conflict_and_strife( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
    return;

  effect.spell_id = 305148;

  auto strife = effect.player->find_spell( effect.spell_id )->effectN( 1 ).trigger();

  auto buff = buff_t::find( effect.player, "strife" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "strife", strife )
      ->add_stat( STAT_VERSATILITY_RATING, strife->effectN( 1 ).average( essence.item() ) )
      ->set_max_stack( strife->max_stacks()
        + as<int>(essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).base_value()) )
      ->set_duration( strife->duration()
        + essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).time_value() );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );

  auto r3_spell = essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MAJOR ).effectN( 3 ).trigger();
  double r3_mul = 1.0 + r3_spell->effectN( 1 ).percent();

  // As this happens here during essence registeration, earlier initialization processes in class modules will not be
  // able to find the buff. Currently can use a workaround by finding the buff in a later process and checking to see it
  // exists before triggering within the various CC/interrupt actions. May be prudent to move to player_t if implemented
  // by more class modules.
  auto r3_buff = buff_t::find( effect.player, "conflict_vers" );
  if ( !r3_buff )
  {
    r3_buff = make_buff( effect.player, "conflict_vers", r3_spell );
    r3_buff->set_stack_change_callback( [buff, r3_mul]( buff_t*, int, int new_ ) {
      if ( new_ )
      {
        static_cast<stat_buff_t*>( buff )->stats.front().amount *= r3_mul;
        buff->trigger();
      }
      else
      {
        static_cast<stat_buff_t*>( buff )->stats.front().amount /= r3_mul;
        buff->bump( 0 );
      }
    } );
  }
}

struct conflict_t : public azerite_essence_major_t
{
  conflict_t(player_t* p, util::string_view options_str) :
    azerite_essence_major_t(p, "conflict", p->find_spell(303823))
  {
    parse_options( options_str );
  }

  //Spell id used above is possible driver but this is going to be overridden for every spec probably
  //Minor grants stacking vers proc, driver spell seems to be spellid 304081
  //Minor has an ICD listed, but this is probably for the feature where it procs on loss of control. RPPM is not listed
}; //End of Conflict and Strife

//Purification Protocol
//Major Power: Purifying Blast
void purification_protocol(special_effect_t& effect)
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
  {
    return;
  }

  //Not currently implemented: rank 3 also triggers a healing buff on the player
  //Also, minor proc does 50% more damage to aberrations
  struct purification_protocol_t : public unique_gear::proc_spell_t
  {
    purification_protocol_t(const special_effect_t& effect, const std::string& name, const azerite_essence_t& essence) :
      proc_spell_t(name, effect.player, effect.player->find_spell(295293), essence.item())
    {
      base_dd_min = base_dd_max = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 1 ).average( essence.item() )
        * ( 1 + essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent() );
      aoe=-1;
      school = SCHOOL_FIRE;
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = proc_spell_t::composite_target_multiplier( target );

      if ( target->race == RACE_ABERRATION )
      {
        m *= 1.0 + data().effectN( 2 ).percent();
      }

      return m;
    }
  };

  auto action = unique_gear::create_proc_action<purification_protocol_t>( "purification_protocol", effect,
    "purification_protocol", essence );

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.execute_action = action;

  new dbc_proc_callback_t( effect.player, effect );
}

//Not implemented: Enemies dying in the ground effect trigger a damage buff for the player
struct purifying_blast_t : public azerite_essence_major_t
{
  action_t* blast_tick;

  struct purifying_blast_tick_t : public spell_t
  {
    purifying_blast_tick_t( const std::string& n, player_t* p, double ta = 0 ) :
      spell_t( n, p, p->find_spell( 295338 ) )
    {
      aoe = -1;
      background = ground_aoe = true;
      may_crit = true;
      base_dd_min = base_dd_max = ta;
    }

    result_amount_type amount_type( const action_state_t* /* s */, bool ) const override
    {
      return result_amount_type::DMG_OVER_TIME;
    }
  };

  purifying_blast_t(player_t* p, util::string_view options_str) :
    azerite_essence_major_t(p, "purifying_blast", p->find_spell(295337))
  {
    parse_options(options_str);

    harmful = true;

    double tick_damage = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 3 ).average( essence.item() );

    blast_tick = new purifying_blast_tick_t( "purifying_tick", p, tick_damage );
    add_child( blast_tick );
  }

  void execute() override
  {
    azerite_essence_major_t::execute();

    int pulse_count = as<int>(essence.spell_ref( 1u, essence_type::MAJOR ).effectN( 2 ).base_value());
    timespan_t pulse_interval = essence.spell_ref( 1u, essence_type::MAJOR ).duration() / (pulse_count - 1);

    make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
      .target( target )
      .action( blast_tick )
      .n_pulses( pulse_count )
      .pulse_time( pulse_interval ), true );
  }

}; //End of Purification Protocol

//Ripple in Space
//Major Power: Ripple in Space
//Not implemented: rank 3 power reduces damage taken after activating
void ripple_in_space(special_effect_t& effect)
{
  timespan_t period = 1_s;
  buff_t* rs = buff_t::find( effect.player, "reality_shift" );

  effect.player->register_combat_begin( [ rs, period ] ( player_t* )
  {
    make_repeating_event( rs->sim, period, [ rs ]
    {
      if ( rs->rng().roll( rs->sim->bfa_opts.ripple_in_space_proc_chance ) )
        rs->trigger();
    } );
  } );
}

struct ripple_in_space_t : public azerite_essence_major_t
{
  timespan_t delay;

  ripple_in_space_t(player_t* p, util::string_view options_str) :
    azerite_essence_major_t(p, "ripple_in_space", p->find_spell(302731))
  {
    parse_options( options_str );
    may_crit = true;
    aoe = -1;

    base_dd_min = base_dd_max = essence.spell_ref( 1u, essence_type::MAJOR ).effectN( 2 ).average( essence.item() );
    school = SCHOOL_FIRE;

    delay = essence.spell_ref( 1u, essence_type::MAJOR ).duration()
      + timespan_t::from_seconds( essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MAJOR ).effectN( 1 ).base_value() / 1000 );
  }

  timespan_t travel_time() const override
  {
    return delay;
  }

  void impact(action_state_t* s) override
  {
    azerite_essence_major_t::impact( s );

    s->action->player->buffs.reality_shift->trigger();
  }
}; //End of Ripple in Space

//The Unbound Force
//Minor driver is 298407
void the_unbound_force(special_effect_t& effect)
{
  auto essence = effect.player->find_azerite_essence(effect.driver()->essence_id());
  if (!essence.enabled())
  {
    return;
  }

  struct reckless_force_callback_t : public dbc_proc_callback_t
  {
    buff_t* crit_buff;

    reckless_force_callback_t(player_t *p, special_effect_t& e, buff_t* b) :
      dbc_proc_callback_t(p, e), crit_buff(b)
    {}

    void execute(action_t*, action_state_t* s) override
    {
      // TODO?: Possible that non-damaging/fully absorbs crits may not proc?
      if (s->result == RESULT_CRIT)
        return;

      if (proc_buff && proc_buff->trigger() && proc_buff->check() == proc_buff->max_stack())
      {
        crit_buff->trigger();
        proc_buff->expire();
      }
    }
  };

  // buff id=302917, not referenced in spell data
  effect.custom_buff = effect.player->buffs.reckless_force_counter;

  buff_t* crit_buff = effect.player->buffs.reckless_force; // id=302932

  if (essence.rank() >= 3)
    crit_buff->base_buff_duration += timespan_t::from_millis(essence.spell_ref(3u, essence_spell::UPGRADE, essence_type::MINOR).effectN(1).base_value());

  if (essence.rank() >= 2)
    crit_buff->default_value += essence.spell_ref(2u, essence_spell::UPGRADE, essence_type::MINOR).effectN(1).percent();

  new reckless_force_callback_t(effect.player, effect, crit_buff);
}

// Major Power: The Unbound Force
// Launches 8 shards on-use over 2 seconds.
// 1 shard on 0 tic, 1 shard every 0.33s, 1 shard at end for the partial tic
// Each shard that crits causes an extra shard to be launched, up to a maximum of 5
struct the_unbound_force_t : public azerite_essence_major_t
{
  struct the_unbound_force_tick_t : public unique_gear::proc_spell_t
  {
    unsigned* max_shard;

    the_unbound_force_tick_t(player_t* p, const azerite_essence_t& essence, unsigned *max) :
      proc_spell_t("the_unbound_force_tick", p, p->find_spell(298453), essence.item()), max_shard(max) // missile spell
    {
      background = dual = true;
      // damage stored in effect#3 of MINOR BASE spell
      base_dd_min = base_dd_max = essence.spell_ref(1u, essence_type::MINOR).effectN(3).average(essence.item());
      crit_bonus_multiplier *= 1.0 + essence.spell_ref(1u, essence_type::MAJOR).effectN(2).percent();
    }

    void impact(action_state_t* s) override
    {
      proc_spell_t::impact(s);

      if (*max_shard > 0 && s->result == RESULT_CRIT)
      {
        (*max_shard)--;
        this->execute();
      }
    }
  };

  unsigned max_shard;

  the_unbound_force_t(player_t* p, util::string_view options_str) :
    azerite_essence_major_t(p, "the_unbound_force", p->find_spell(298452)), max_shard(0u)
  {
    parse_options(options_str);

    hasted_ticks = false;
    tick_zero = true;
    tick_action = new the_unbound_force_tick_t(p, essence, &max_shard);

    if (essence.rank() >= 2)
      cooldown->duration *= 1.0 + essence.spell_ref(2u, essence_spell::UPGRADE, essence_type::MAJOR).effectN(1).percent();
  }

  void execute() override
  {
    azerite_essence_major_t::execute();
    if (essence.rank() >= 3)
      max_shard = 5;
  }
  double last_tick_factor( const dot_t*, timespan_t, timespan_t ) const override
  {
    return 1.0;
  }
}; //End of The Unbound Force

// Vision of Perfection
// Minor Power: Strive for Perfection (Passive)
void strive_for_perfection( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
    return;

  effect.player->sim->print_debug(
    "{} reducing cooldown by {}%", effect.name(), vision_of_perfection_cdr( essence ) * -100 );

  if ( essence.rank() >= 3 )
  {
    double avg =
      essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).average( essence.item() );
    effect.player->passive.versatility_rating += avg;
    effect.player->sim->print_debug( "{} increasing versatility by {} rating", effect.name(), avg );
  }
}

// Major Power: Vision of Perfect (Passive)
void vision_of_perfection( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
    return;

  struct vision_of_perfection_callback_t : public dbc_proc_callback_t
  {
    vision_of_perfection_callback_t( player_t* p, special_effect_t& e ) : dbc_proc_callback_t( p, e )
    {}

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );
      make_event( *listener->sim, [this] { listener->vision_of_perfection_proc(); } );
    }
  };

  // There are 4 RPPM spells for Vision of Perfection
  //   1) id=297866 0.85 rppm, 0.1s icd, yellow hits + dot -- tank spec: logs show tank spec procing with 2.4s interval
  //   2) id=297868 0.85 rppm, 0.1s icd, heals only      -- healer spec: hotfix on this spell for holy paladins
  //   3) id=297869 0.85 rppm, 3.5s icd, yellow hits + dot -- dps spec?: by process of elimination?
  //   4) id=296326 1.00 rppm, no icd?, in-combat -- confirmed for resto shaman
  switch ( effect.player->specialization() )
  {
    case DEATH_KNIGHT_BLOOD:
    case DEMON_HUNTER_HAVOC:
    case DRUID_GUARDIAN:
    case MONK_BREWMASTER:
    case PALADIN_PROTECTION:
    case WARRIOR_PROTECTION:
      effect.spell_id = 297866;
      break;
    case DRUID_RESTORATION:
    case MONK_MISTWEAVER:
    case PALADIN_HOLY:
    case PRIEST_DISCIPLINE:
    case PRIEST_HOLY:
      effect.spell_id = 297868;
      break;
    case SHAMAN_RESTORATION:
      effect.spell_id = 296326;
      break;
    default:
      effect.spell_id = 297869;
      break;
  }

  if ( essence.rank() >= 3 )
  {
    // buff id=303344, not referenced in spell data
    // amount from R3 major/upgrade
    effect.custom_buff = buff_t::find( effect.player, "vision_of_perfection" );
    if ( !effect.custom_buff )
    {
      effect.custom_buff =
        make_buff<stat_buff_t>( effect.player, "vision_of_perfection", effect.player->find_spell( 303344 ) )
          ->add_stat( STAT_HASTE_RATING,
            essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MAJOR )
              .effectN( 2 ).average( essence.item() ) );
    }
  }

  new vision_of_perfection_callback_t( effect.player, effect );
}

// Worldvein Resonance
// The shards & resultant buff in-game exhibits various small delays in their timings:
// 1) the buff is not applied immediately upon the shard being summoned (NYI)
// 2) the buff duration varies from ~17.8s to ~19s with possibly discrete intervals (NYI)
// 3) shards summoned within a small timeframe can have their buffs grouped up and applied all at once, as well as have
// the buff expirations grouped up and expiring all at once. the application grouping and expiration grouping can be
// different (NYI)
// 4) only four stacks of the buff is applied at a time. when a shard expires the buff is decremented by a stack
// regardless of how many shards exist. after a short delay the buff is re-adjusted upwards if enough shards exist to
// refill the decremented stack (Implemented, TODO: further investigate range/distribution of delay)

struct lifeblood_shard_t : public buff_t
{
  lifeblood_shard_t( player_t* p, const azerite_essence_t& ess ) :
    buff_t( p, "lifeblood_shard", p->find_spell( 295114 ), ess.item() )
  {
    set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
    set_max_stack( 64 );  // sufficiently large enough to cover major esssence + 10 allies
    set_quiet( true );
    base_buff_duration *= 1.0 + ess.spell_ref( 2, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent();

    set_stack_change_callback( [this]( buff_t*, int old_, int new_ ) {
      if ( new_ > old_ )
      {
        player->buffs.lifeblood->trigger( new_ - old_ );
      }
      else if ( old_ > new_ )
      {
        player->buffs.lifeblood->decrement( old_ - new_ );

        // testing data shows an exponential variable-like distribution of the delay times, with 0.636s mean and highest
        // gap seen was 1.923s. for now implement as an exponential variable with a mean of 0.636 generated by a uniform
        // distribution on the interval [0.048, 1] (0.048 derived from exp(-1.93/0.636)).
        auto delay = timespan_t::from_seconds( std::log( rng().range( 0.048, 1.0 ) ) * -0.636 );

        make_event( *sim, delay, [this]() {
          auto delta = std::min( player->buffs.lifeblood->max_stack(), check() ) - player->buffs.lifeblood->check();
          if ( delta > 0 )
          {
            player->buffs.lifeblood->trigger( delta );
          }
        } );
      }
    } );
  }
};

// Major Power: Worldvein Resonance
void worldvein_resonance( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
    return;

  auto base_spell = essence.spell( 1, essence_type::MINOR );

  int period_min = as<int>( base_spell->effectN( 4 ).base_value() +
    essence.spell( 3, essence_spell::UPGRADE, essence_type::MINOR )->effectN( 1 ).base_value() );
  int period_max = as<int>( base_spell->effectN( 1 ).base_value() );

  auto lifeblood = buff_t::find( effect.player, "lifeblood_shard" );
  if ( !lifeblood )
    lifeblood = make_buff<lifeblood_shard_t>( effect.player, essence );

  struct lifeblood_event_t : public event_t
  {
    int period_min;
    int period_max;
    buff_t* buff;

    timespan_t next_event()
    {
      // For some reason, Lifeblood triggers every K - 0.5 seconds (for integer K). For the last rank, the
      // lowest observed interval is 1.5 seconds, which occurs with double the frequency. Highest observed
      // interval was 24.5 seconds.
      return std::max( 1.5_s, timespan_t::from_seconds( buff->sim->rng().range( period_min, period_max + 1 ) ) - 0.5_s );
    }

    lifeblood_event_t( int period_min_, int period_max_, buff_t* buff_ )
      : event_t( *buff_->source ), period_min( period_min_ ), period_max( period_max_ ), buff( buff_ )
    {
      auto next = next_event();
      buff->sim->print_debug( "Scheduling Lifeblood event, next occurence in: {}", next.total_seconds() );
      schedule( next );
    }

    void execute() override
    {
      buff->trigger();
      make_event<lifeblood_event_t>( *buff->sim, period_min, period_max, buff );
    }
  };

  for ( int i = 0; i < effect.player->sim->bfa_opts.worldvein_allies + 1; i++ )
  {
    effect.player->register_combat_begin( [period_min, period_max, lifeblood]( player_t* /* p */ ) {
      make_event<lifeblood_event_t>( *lifeblood->sim, period_min, period_max, lifeblood );
    } );
  }
}

struct worldvein_resonance_buff_t : public buff_t
{
  stat_buff_t* lifeblood;

  worldvein_resonance_buff_t( player_t* p, const azerite_essence_t& ess ) :
    buff_t( p, "worldvein_resonance", p->find_spell( 313310 ), ess.item() )
  {
    lifeblood = static_cast<stat_buff_t*>( buff_t::find( p, "lifeblood" ) );
  }

  void adjust_stat( bool increase )
  {
    if ( !lifeblood )
    {
      return;
    }

    auto& stat_entry = lifeblood->stats.front();
    if ( increase )
    {
      stat_entry.amount *= 1.0 + data().effectN( 1 ).percent();
    }
    else
    {
      stat_entry.amount /= 1.0 + data().effectN( 1 ).percent();
    }

    if ( lifeblood->check() )
    {
      double delta = stat_entry.amount * lifeblood->current_stack - stat_entry.current_value;
      sim->print_debug( "{} worldvein_resonance {}creases lifeblood stats by {}%,"
                        " stacks={}, old={}, new={} ({}{})",
        player->name(),
        increase ? "in" : "de", data().effectN( 1 ).base_value(), lifeblood->current_stack,
        stat_entry.current_value, stat_entry.current_value + delta,
        increase ? "+" : "", delta );

      if ( delta > 0 )
      {
        player->stat_gain( stat_entry.stat, delta, lifeblood->stat_gain, nullptr, true );
      }
      else
      {
        player->stat_loss( stat_entry.stat, std::fabs( delta ), lifeblood->stat_gain,
          nullptr, true );
      }

      stat_entry.current_value += delta;
    }
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    adjust_stat( true /* Stat increase */ );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    adjust_stat( false /* Stat decrease */ );
  }
};

struct worldvein_resonance_t : public azerite_essence_major_t
{
  double precombat_time;

  int stacks;
  buff_t* lifeblood;
  buff_t* worldvein_resonance;

  worldvein_resonance_t( player_t* p, util::string_view options_str ) :
    azerite_essence_major_t( p, "worldvein_resonance", p->find_spell( 295186 ) ), stacks()
  {
    // Default the precombat timer to the player's base, unhasted gcd
    precombat_time = p -> base_gcd.total_seconds();

    add_option( opt_float( "precombat_time", precombat_time ) );
    parse_options( options_str );
    harmful = false;
    stacks = as<int>( data().effectN( 1 ).base_value() +
      essence.spell( 2, essence_spell::UPGRADE, essence_type::MAJOR )->effectN( 1 ).base_value() );

    lifeblood = buff_t::find( player, "lifeblood_shard" );
    if ( !lifeblood )
      lifeblood = make_buff<lifeblood_shard_t>( p, essence );

    worldvein_resonance = buff_t::find( player, "worldvein_resonance" );
    if ( !worldvein_resonance )
      worldvein_resonance = make_buff<worldvein_resonance_buff_t>( p, essence );
  }

  void init_finished() override
  {
    azerite_essence_major_t::init_finished();

    if ( action_list -> name_str == "precombat" && !background )
    {
      double MIN_TIME = player -> base_gcd.total_seconds(); // the player's base unhasted gcd: 1.5s
      double MAX_TIME = worldvein_resonance -> buff_duration().total_seconds() - 1;

      // Ensure that we're using a positive value
      if ( precombat_time < 0 )
        precombat_time = -precombat_time;

      if ( precombat_time > MAX_TIME )
      {
        precombat_time = MAX_TIME;
        sim -> error( "{} tried to use Worldvein Resonance in precombat more than {} seconds before combat begins, setting to {}",
                       player -> name(), MAX_TIME, MAX_TIME );
      }
      else if ( precombat_time < MIN_TIME )
      {
        precombat_time = MIN_TIME;
        sim -> error( "{} tried to use Worldvein Resonance in precombat less than {} before combat begins, setting to {} (has to be >= base gcd)",
                       player -> name(), MIN_TIME, MIN_TIME );
      }
    }
    else precombat_time = 0;
  }

  void execute() override
  {
    azerite_essence_major_t::execute();

    // Apply the duration penalty directly in trigger() because lifeblood stacks are asynchronous
    lifeblood -> trigger( stacks, lifeblood -> DEFAULT_VALUE(), -1.0,
                          lifeblood -> buff_duration() - timespan_t::from_seconds( precombat_time ) );
    worldvein_resonance->trigger();

    if ( ! player -> in_combat && precombat_time > 0 )
    {
      worldvein_resonance -> extend_duration( player, timespan_t::from_seconds( -precombat_time ) );
      cooldown -> adjust( timespan_t::from_seconds( -precombat_time ), false );

      sim -> print_debug( "{} used Worldvein Resonance in precombat with precombat time = {}, adjusting duration and remaining cooldown.",
                          player -> name(), precombat_time );
    }
  }

  //Minor and major power both summon Lifeblood shards that grant primary stat (max benefit of 4 shards)
  //Lifeblood shards are governed by spellid 295078
  //Major power summons a number of shards based on rank.
}; //End of Worldvein Resonance

//Anima of Life and Death
//Healing effects aren't implemented
//Major Power: Anima of Death

void anima_of_life_and_death( special_effect_t& effect )
{
  auto essence = effect.player -> find_azerite_essence( effect.driver() -> essence_id() );
  if ( ! essence.enabled() )
  {
    return;
  }

  const spell_data_t* base_spell = essence.spell( 1u, essence_type::MINOR );
  const spell_data_t* buff_spell = effect.player -> find_spell( 294966 );

  buff_t* buff = buff_t::find( effect.player, "anima_of_life" ) ;
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "anima_of_life", buff_spell )
      -> add_stat( STAT_MAX_HEALTH, buff_spell -> effectN( 1 ).average( essence.item() ) );
  }

  effect.player -> register_combat_begin( [ buff, base_spell ]( player_t* )
  {
    make_repeating_event( *buff -> sim, base_spell -> effectN( 1 ).period(), [ buff ]() {
      buff -> trigger();
    } );
  } );
}

struct anima_of_death_t : public azerite_essence_major_t
{
  anima_of_death_t( player_t* p, util::string_view options_str ) :
    azerite_essence_major_t( p, "anima_of_death", p -> find_spell( 294926 ) )
  {
    parse_options( options_str );
    may_crit = false;
    aoe = -1;
    // TODO: check that the damage is indeed increased by chaos brand and other target-based modifiers
    // Known: doesn't scale with versatility
    snapshot_flags = update_flags = STATE_TGT_MUL_DA; // The damage is based on the player's maximum health and doesn't seem to scale with anything
    if ( essence.rank() >= 2 )
    {
      cooldown -> duration *= 1.0 + essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MAJOR ).effectN( 1 ).percent();
    }
  }

  double base_da_min( const action_state_t* ) const override
  {
    return player -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent();
  }

  double base_da_max( const action_state_t* ) const override
  {
    return player -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent();
  }
};

//Nullification Dynamo
//Major isn't implemented (absorb+cleanse on-use)
//Minor is implemented assuming full consumption of the absorb
//TODO: add an option to make it more realistic based on actual damage taken

void nullification_dynamo( special_effect_t& effect )
{
  struct null_barrier_damage_t : public unique_gear::proc_spell_t
  {
    null_barrier_damage_t( const special_effect_t& effect, const azerite_essence_t& essence ) :
      proc_spell_t( "null_barrier_damage", effect.player,
                    effect.player -> find_spell( 296061 ),
                    essence.item() )
    {
      may_crit = false;
      split_aoe_damage = true;
      aoe = -1;
      base_dd_min = base_dd_max = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 1 ).average( essence.item() );
    }
  };

  struct null_barrier_absorb_buff_t : absorb_buff_t
  {
    action_t* null_barrier_damage;

    null_barrier_absorb_buff_t( player_t* p, const azerite_essence_t& essence, action_t* damage ) :
      absorb_buff_t( p, "null_barrier", p -> find_spell( 295842 ) ),
      null_barrier_damage( damage )
    {
      default_value = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 1 ).average( essence.item() );
      set_absorb_source(p->get_stats("null_barrier"));
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      absorb_buff_t::expire_override( expiration_stacks, remaining_duration );
      if ( null_barrier_damage )
      {
        null_barrier_damage -> execute();
      }
    }
  };

  auto essence = effect.player -> find_azerite_essence( effect.driver() -> essence_id() );
  if ( ! essence.enabled() )
  {
    return;
  }

  // The damage only procs at rank 3
  action_t* nbd = nullptr;
  if ( essence.rank() >= 3 )
  {
    nbd = unique_gear::create_proc_action<null_barrier_damage_t>( "null_barrier_damage", effect, essence );
  }

  buff_t* buff = buff_t::find( effect.player, "null_barrier" ) ;
  if ( !buff )
  {
    buff = make_buff<null_barrier_absorb_buff_t>( effect.player, essence, nbd );
  }

  timespan_t interval = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 1 ).period();
  if ( essence.rank() >= 2 )
  {
    interval *= 1.0 + essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent();
  }

  effect.player -> register_combat_begin( [ buff, interval ]( player_t* )
  {
    make_repeating_event( *buff -> sim, interval, [ buff ]() {
      buff -> trigger();
    } );
  } );
}

//Aegis of the Deep
//Only the passive versatility buff from the Minor effect is implemented
void aegis_of_the_deep( special_effect_t& effect )
{
  auto essence = effect.player -> find_azerite_essence( effect.driver() -> essence_id() );
  if ( ! essence.enabled() )
  {
    return;
  }

  buff_t* aegis_buff = buff_t::find( effect.player, "stand_your_ground" );
  if ( !aegis_buff )
  {
    double amount = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 2 ).average( essence.item() );

    aegis_buff = make_buff<stat_buff_t>( effect.player, "stand_your_ground", effect.player -> find_spell( 298197 ) )
      -> add_stat( STAT_VERSATILITY_RATING, amount );
  }

  //Spelldata shows a buff trigger every 1s, but in-game observation show an immediate change
  //Add a callback on arise and demise to each enemy in the simulation
  range::for_each( effect.player -> sim -> actor_list, [ p = effect.player, aegis_buff ]( player_t* target )
  {
    // Don't do anything on players
    if ( !target -> is_enemy() )
    {
      return;
    }

    target -> register_on_arise_callback( p, [ p, aegis_buff ] ()
    {
      if ( aegis_buff )
      {
        p -> sim -> print_debug( "An enemy arises! Stand your Ground on player {} is increased by one stack",
                                 p -> name_str );
        aegis_buff -> trigger();
      }
    } );


    target -> register_on_demise_callback( p, [ p, aegis_buff ] ( player_t* target )
    {
      // Don't do anything if the sim is ending
      if ( target -> sim -> event_mgr.canceled )
      {
        return;
      }

      if ( aegis_buff )
      {
        target -> sim -> print_debug( "Enemy {} demises! Stand your Ground on player {} is reduced by one stack",
                                      target -> name_str, p -> name_str );
        aegis_buff -> decrement();
      }
    } );
  } );

  // Required because players spawn after the enemies
  effect.player -> register_combat_begin( [ aegis_buff, effect ]( player_t* )
  {
    aegis_buff -> trigger( effect.player -> sim -> desired_targets );
  } );
}

//Sphere of Suppresion
//Minor: Proc callback on melee damage taken, triggers a haste buff
//The other effects are strictly defensive and NYI
void sphere_of_suppression( special_effect_t& effect )
{
  auto essence = effect.player -> find_azerite_essence( effect.driver() -> essence_id() );
  if ( ! essence.enabled() )
  {
    return;
  }

  buff_t* sphere_buff = buff_t::find( effect.player, "sphere_of_suppresion" );
  if ( !sphere_buff )
  {
    double amount = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 2 ).average( essence.item() );

    sphere_buff = make_buff<stat_buff_t>( effect.player, "sphere_of_suppresion", effect.player -> find_spell( 294912 ) )
      -> add_stat( STAT_HASTE_RATING, amount );
  }

  effect.custom_buff = sphere_buff;

  new dbc_proc_callback_t( effect.player, effect );
}


struct resolute_courage_t : public stat_buff_t
{
  resolute_courage_t( player_t* p ) :
    stat_buff_t( p, "resolute_courage" )
  {
    add_stat( STAT_CORRUPTION_RESISTANCE, 10 );
  }
};

void register_essence_corruption_resistance( special_effect_t& effect )
{
  // Corruption resistance from essence
  buff_t* buff = buff_t::find( effect.player, "resolute_courage" );
  if ( ! buff )
  {
    buff = make_buff<resolute_courage_t>( effect.player );
    // this needs to happen on arise which is before snapshot stats
    effect.player->register_on_arise_callback( effect.player, [ buff ] {
      buff -> trigger();
    });
  }
}

// Breath of the Dying
// Minor: Lethal Strikes
// id=310712 R1 minor base, driver
// id=311192 minor damage spell
// id=311201 R2 minor heal spell
// R1: proc for damage
// R2: heal for 50% (not implemented)
// R3: 400% more proc when target below 20%
void breath_of_the_dying( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
    return;

  struct lethal_strikes_t : public unique_gear::proc_spell_t
  {
    lethal_strikes_t( const special_effect_t& e, const std::string& n, const azerite_essence_t& ess ) :
      proc_spell_t( n, e.player, e.player->find_spell( 311192 ), ess.item() )
    {
      base_dd_min = base_dd_max = ess.spell_ref( 1u, essence_type::MINOR ).effectN( 1 ).average( ess.item() );
    }
  };

  struct lethal_strikes_cb_t : public dbc_proc_callback_t
  {
    double r3_lo_hp; // note this will be base_value, NOT percent
    double r3_mul;

    lethal_strikes_cb_t( const special_effect_t& e, const azerite_essence_t& ess ) :
      dbc_proc_callback_t( e.player, e )
    {
      r3_lo_hp = ess.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 2 ).base_value();
      r3_mul   = ess.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent();
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      double mod = 1.0;

      // TODO: confirm '400% more' means 5x multiplier
      if ( s->target->health_percentage() < r3_lo_hp )
          mod += r3_mul;

      rppm->set_modifier( mod );

      dbc_proc_callback_t::trigger( a, s );
    }
  };

  auto action =
      unique_gear::create_proc_action<lethal_strikes_t>( "lethal_strikes", effect, "lethal_strikes", essence );

  effect.type           = SPECIAL_EFFECT_EQUIP;
  effect.execute_action = action;
  effect.ppm_           = -2.0; // RPPM of 10 hasted in spell data seems to be the buffed rppm, assuming base is 2

  register_essence_corruption_resistance( effect );

  new lethal_strikes_cb_t( effect, essence );
}

// Major: Reaping Flames
// id=310690 R1 major base, damage spell
// id=311202 R3 buff after kill
// R1: damage, -30s cd if target < 20%
// R2: -30s cd if target > 80%
// R3: reset cd if target dies + 100% more damage on next (NYI)
// TODO: Implement R3
struct reaping_flames_t : public azerite_essence_major_t
{
  double lo_hp; // note these are base_value and NOT percent
  double hi_hp;
  double cd_mod;
  double cd_reset;
  buff_t* damage_buff;

  reaping_flames_t( player_t* p, util::string_view options_str ) :
    azerite_essence_major_t( p, "reaping_flames", p->find_spell( 310690 ) ),
    damage_buff()
  {
    parse_options( options_str );
    // Damage stored in R1 MINOR
    base_dd_min = base_dd_max = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 3 ).average( essence.item() );

    may_crit = true;

    lo_hp    = essence.spell_ref( 1u ).effectN( 2 ).base_value();
    hi_hp    = essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MAJOR ).effectN( 1 ).base_value();
    cd_mod   = -essence.spell_ref( 1u ).effectN( 3 ).base_value();
    cd_reset = essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MAJOR ).effectN( 2 ).base_value();
  }

  void init() override
  {
    azerite_essence_major_t::init();

    if ( essence.rank() < 3 )
      return;

    damage_buff = buff_t::find( player, "reaping_flames" );
    if ( !damage_buff )
    {
      damage_buff = make_buff( player, "reaping_flames", player->find_spell( 311202 ) )
        ->set_default_value( essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MAJOR ).effectN( 1 ).percent() );

      range::for_each( sim->actor_list, [ this ] ( player_t* target )
      {
        if ( !target->is_enemy() )
          return;

        target->register_on_demise_callback( player, [ this ] ( player_t* enemy )
        {
          if ( player->get_target_data( enemy )->debuff.reaping_flames_tracker->check() )
          {
            cooldown->adjust( timespan_t::from_seconds( cd_reset ) - cooldown->duration );
            damage_buff->trigger();
          }
        } );
      } );
    }
  }

  double action_multiplier() const override
  {
    double am = azerite_essence_major_t::action_multiplier();

    if ( damage_buff )
    {
      am *= 1.0 + damage_buff->value();
    }

    return am;
  }

  void execute() override
  {
    bool reduce = false;
    double tar_hp = target->health_percentage();

    if ( tar_hp < lo_hp || ( hi_hp && tar_hp > hi_hp ) )
      reduce = true;

    azerite_essence_major_t::execute();

    if ( reduce )
      cooldown->adjust( timespan_t::from_seconds ( cd_mod ) );

    if ( damage_buff )
    {
      damage_buff->expire();
      player->get_target_data( target )->debuff.reaping_flames_tracker->trigger();
    }
  }
};
// End Breath of the Dying

// Spark of Inspiration
// Minor: Unified Strength
// id=311210 R1 minor base, cdr on effect 3
// id=313643 minor haste buff
// id=311304 R2 minor base, rppm increase
void spark_of_inspiration( special_effect_t& effect )
{
  struct unified_strength_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;
    timespan_t cdr;
    azerite_essence_major_t* major;

    unified_strength_cb_t( special_effect_t& effect, azerite_essence_t& essence, buff_t* b ) :
      dbc_proc_callback_t( effect.player, effect ),
      buff(b)
    {
      cdr = timespan_t::from_seconds( essence.spell_ref( 1u, essence_type::MINOR ).effectN( 3 ).base_value() / 10.0 );
    }

    void execute( action_t*, action_state_t* ) override
    {
      buff->trigger();

      if ( !major || !major->essence.enabled() )
      {
        major = nullptr;

        for ( action_t* a : listener->action_list )
        {
          azerite_essence_major_t* candidate = dynamic_cast<azerite_essence_major_t*>( a );
          if ( candidate && candidate->essence.enabled() )
          {
            major = candidate;
            break;
          }
        }

        if ( !major )
          return;
      }

      major->cooldown->adjust( -cdr );
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      major = nullptr;
    }
  };

  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
    return;

  buff_t* buff = buff_t::find( effect.player, "unified_strength" );
  if ( !buff )
  {
    double haste = essence.spell_ref( 1u, essence_type::MINOR ).effectN( 1 ).average( essence.item() );

    buff = make_buff<stat_buff_t>( effect.player, "unified_strength", effect.player->find_spell( 313643 ) )
      -> add_stat( STAT_HASTE_RATING, haste );
  }

  if ( essence.rank() >= 2 )
  {
    effect.ppm_ = -essence.spell_ref( 2u, essence_type::MINOR ).real_ppm();
    effect.rppm_scale_ = effect.player->dbc->real_ppm_scale( essence.spell_ref( 2u, essence_type::MINOR ).id() );
  }

  register_essence_corruption_resistance( effect );

  new unified_strength_cb_t( effect, essence, buff );
}

// Formless Void
// Minor: Symbiotic Presence
// id=312771 R1 minor base, primary stat on effect 2
// id=312915 minor agi/haste buff (duration)
// id=312773 R2 upgrade, increase R1 buff by effect 1 %
// id=312774 R3 upgrade, haste on effect 1
void formless_void( special_effect_t& effect )
{
  auto essence = effect.player->find_azerite_essence( effect.driver()->essence_id() );
  if ( !essence.enabled() )
    return;

  buff_t* buff = buff_t::find( effect.player, "symbiotic_presence" );
  if ( !buff )
  {
    auto primary = essence.spell_ref( 1u, essence_spell::BASE, essence_type::MINOR ).effectN( 2 ).average( essence.item() );
    primary *= 1 + essence.spell_ref( 2u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent();

    stat_buff_t* stat_buff = make_buff<stat_buff_t>( effect.player, "symbiotic_presence", effect.player->find_spell( 312915 ) )
      ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), primary );

    if ( essence.rank() >= 3 )
    {
      stat_buff -> add_stat( STAT_HASTE_RATING, essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).average( essence.item() ) );
    }

    buff = stat_buff;
  }

  timespan_t interval = buff->sim->bfa_opts.symbiotic_presence_interval;
  effect.player -> register_combat_begin( [ buff, interval ]( player_t* )
  {
    buff -> trigger();
    make_repeating_event( *buff -> sim, interval, [ buff ]()
    {
      buff -> trigger();
    } );
  } );

  register_essence_corruption_resistance( effect );
}

// Strength of the Warden
// Ignore Rank 1 major (taunt)
// TODO: Implement R2 major (dodge on use), R3 major (damage increase against affected enemies)
// Ignore R1/R2 minor ? (stores healing taken and triggers on parry/dodge)
// TODO: add a user-input option for R3 minor? (tanks get 3% max health for every player with secondary r3 in the raid)
// Technically a player without the essence can get buffed by others too, as long as they're in tank spec
void strength_of_the_warden( special_effect_t& effect )
{
  auto essence = effect.player -> find_azerite_essence( effect.driver() -> essence_id() );
  if ( !essence.enabled() )
    return;

  // R3 Minor, +3% max health to you (implemented) and other tank specializations characters in your raid (NYI)
  // There doesn't seem to be any buff associated with the health gain, similarly to Warriors' Indomitable talent
  double health_gain = essence.spell_ref( 3u, essence_spell::UPGRADE, essence_type::MINOR ).effectN( 1 ).percent();
  effect.player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_gain;
  effect.player -> recalculate_resource_max( RESOURCE_HEALTH );

  register_essence_corruption_resistance( effect );
}

// Unwavering Ward
// Just register the corruption resistance
void unwavering_ward( special_effect_t& effect )
{
  register_essence_corruption_resistance( effect );
}

// Spirit of Preservation
// Just register the corruption resistance
void spirit_of_preservation( special_effect_t& effect )
{
  register_essence_corruption_resistance( effect );
}

// Touch of the Everlasting
// Just register the corruption resistance
void touch_of_the_everlasting( special_effect_t& effect )
{
  register_essence_corruption_resistance( effect );
}

} // Namespace azerite essences ends

action_t* create_action( player_t* player, util::string_view name, const std::string& options )
{
  if ( util::str_compare_ci( name, "focused_azerite_beam" ) )
  {
    return new azerite_essences::focused_azerite_beam_t( player, options );
  }
  else if ( util::str_compare_ci( name, "memory_of_lucid_dreams" ) )
  {
    return new azerite_essences::memory_of_lucid_dreams_t( player, options );
  }
  else if ( util::str_compare_ci( name, "blood_of_the_enemy" ) )
  {
    return new azerite_essences::blood_of_the_enemy_t( player, options );
  }
  else if ( util::str_compare_ci( name, "guardian_of_azeroth" ) )
  {
    return new azerite_essences::guardian_of_azeroth_t( player, options );
  }
  else if ( util::str_compare_ci( name, "purifying_blast" ) )
  {
    return new azerite_essences::purifying_blast_t( player, options );
  }
  else if ( util::str_compare_ci( name, "ripple_in_space" ) )
  {
    return new azerite_essences::ripple_in_space_t( player, options );
  }
  else if ( util::str_compare_ci( name, "concentrated_flame" ) )
  {
    return new azerite_essences::concentrated_flame_t( player, options );
  }
  else if ( util::str_compare_ci( name, "the_unbound_force" ) )
  {
    return new azerite_essences::the_unbound_force_t( player, options );
  }
  else if ( util::str_compare_ci( name, "worldvein_resonance" ) )
  {
    return new azerite_essences::worldvein_resonance_t( player, options );
  }
  else if ( util::str_compare_ci( name, "anima_of_death" ) )
  {
    return new azerite_essences::anima_of_death_t( player, options );
  }
  else if ( util::str_compare_ci( name, "reaping_flames" ) )
  {
    return new azerite_essences::reaping_flames_t( player, options );
  }
  // "Heart Essence" is a proxy action, generating whatever the user has for the major action
  // selected
  else if ( util::str_compare_ci( name, "heart_essence" ) )
  {
    azerite_essence_t focused_azerite_beam   = player->find_azerite_essence( "Essence of the Focusing Iris" );
    azerite_essence_t memory_of_lucid_dreams = player->find_azerite_essence( "Memory of Lucid Dreams" );
    azerite_essence_t blood_of_the_enemy     = player->find_azerite_essence( "Blood of the Enemy" );
    azerite_essence_t guardian_of_azeroth    = player->find_azerite_essence( "Condensed Life-Force" );
    azerite_essence_t purifying_blast        = player->find_azerite_essence( "Purification Protocol" );
    azerite_essence_t ripple_in_space        = player->find_azerite_essence( "Ripple in Space" );
    azerite_essence_t concentrated_flame     = player->find_azerite_essence( "The Crucible of Flame" );
    azerite_essence_t the_unbound_force      = player->find_azerite_essence( "The Unbound Force" );
    azerite_essence_t worldvein_resonance    = player->find_azerite_essence( "Worldvein Resonance" );
    azerite_essence_t anima_of_death         = player->find_azerite_essence( "Anima of Life and Death" );
    azerite_essence_t reaping_flames         = player->find_azerite_essence( "Breath of the Dying" );

    if ( focused_azerite_beam.is_major() )
    {
      return new azerite_essences::focused_azerite_beam_t( player, options );
    }
    else if ( memory_of_lucid_dreams.is_major() )
    {
      return new  azerite_essences::memory_of_lucid_dreams_t( player, options );
    }
    else if ( blood_of_the_enemy.is_major() )
    {
      return new azerite_essences::blood_of_the_enemy_t( player, options );
    }
    else if ( guardian_of_azeroth.is_major() )
    {
      return new azerite_essences::guardian_of_azeroth_t( player, options );
    }
    else if ( purifying_blast.is_major() )
    {
      return new azerite_essences::purifying_blast_t( player, options );
    }
    else if ( ripple_in_space.is_major() )
    {
      return new azerite_essences::ripple_in_space_t( player, options );
    }
    else if ( concentrated_flame.is_major() )
    {
      return new azerite_essences::concentrated_flame_t( player, options );
    }
    else if ( the_unbound_force.is_major() )
    {
      return new azerite_essences::the_unbound_force_t( player, options );
    }
    else if ( worldvein_resonance.is_major() )
    {
      return new azerite_essences::worldvein_resonance_t( player, options );
    }
    else if ( anima_of_death.is_major() )
    {
      return new azerite_essences::anima_of_death_t( player, options );
    }
    else if ( reaping_flames.is_major() )
    {
      return new azerite_essences::reaping_flames_t( player, options );
    }
    // Construct a dummy action so that "heart_essence" still works in the APL even without a major
    // essence
    else
    {
      struct heart_essence_dummy_t : public action_t
      {
        heart_essence_dummy_t( player_t* p ) :
          action_t( ACTION_OTHER, "heart_essence", p, spell_data_t::nil() )
        {
          background = quiet = true;
          callbacks = false;
        }
      };

      return new heart_essence_dummy_t( player );
    }
  }

  return nullptr;
}

/**
 * 8.2 Vision of Perfection minor essence CDR function
 *
 * Tooltip formula reads ((<effect#1> + 2896) / 100)% reduction, minimum 10%, maximum 25%,
 * where <effect#1> is a large negative value.
 */
double vision_of_perfection_cdr( const azerite_essence_t& essence )
{
  if ( essence.enabled() )
  {
    // Formula from tooltip
    double cdr = ( essence.spell( 1u, essence_type::MINOR )->effectN( 1 ).average( essence.item() ) + 2896 ) / -100.0;
    // Clamped to 10 .. 25
    cdr = fmax( 10.0, fmin( 25.0, cdr ) );
    // return the negative percent
    return cdr / -100;
  }

  return 0.0;
}

} // Namespace azerite ends
