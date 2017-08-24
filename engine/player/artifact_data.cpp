#include "simulationcraft.hpp"

#include "artifact_data.hpp"

namespace
{
template <class ...Args>
void debug( const artifact::player_artifact_data_t* obj, Args&& ...args )
{
  if ( obj -> sim() -> debug )
  {
    obj -> sim() -> out_debug.printf( std::forward<Args>( args )... );
  }
}

template <class ...Args>
void error( const artifact::player_artifact_data_t* obj, Args&& ...args )
{
  obj -> sim() -> errorf( std::forward<Args>( args )... );
}
}

namespace artifact
{
artifact_data_ptr_t player_artifact_data_t::create( player_t* player )
{
  artifact_data_ptr_t obj { new player_artifact_data_t( player ) };

  return obj;
}

artifact_data_ptr_t player_artifact_data_t::create( const artifact_data_ptr_t& other )
{
  artifact_data_ptr_t obj { new player_artifact_data_t( *other.get() ) };

  return obj;
}

player_artifact_data_t::player_artifact_data_t( player_t* player ) :
  m_player( player )
{
  reset();
}

void player_artifact_data_t::reset()
{
  m_points.clear();
  m_has_relic_opts = false;
  m_artifact_str.clear();
  m_total_points = 0;
  m_purchased_points = 0;
  m_slot = SLOT_INVALID;
  m_artificial_damage = spell_data_t::not_found();
  m_artificial_stamina = spell_data_t::not_found();
}

void player_artifact_data_t::initialize()
{
  auto p = powers();

  auto damage_it = range::find_if( p , [ this ]( const artifact_power_data_t* power ) {
    auto spell_data = player() -> find_spell( power -> power_spell_id );
    return util::str_compare_ci( spell_data -> name_cstr(), "Artificial Damage" );
  } );

  m_artificial_damage = damage_it != p.end()
                        ? player() -> find_spell( ( *damage_it ) -> power_spell_id )
                        : spell_data_t::not_found();

  auto stamina_it = range::find_if( p, [ this ]( const artifact_power_data_t* power ) {
    auto spell_data = player() -> find_spell( power -> power_spell_id );
    return util::str_compare_ci( spell_data -> name_cstr(), "Artificial Stamina" );
  } );

  m_artificial_stamina = stamina_it != p.end()
                         ? player() -> find_spell( ( *stamina_it ) -> power_spell_id )
                         : spell_data_t::not_found();

  // Set the primary artifact slot for this player. It will be the item that has an artifact
  // identifier on it and it does not have a parent slot as per client data. The parent slot
  // initialization is performed in player_t::init_items.
  range::for_each( player() -> items, [ this ]( const item_t& i ) {
    if ( i.parsed.data.id_artifact > 0 && i.parent_slot == SLOT_INVALID )
    {
      assert( m_slot == SLOT_INVALID );
      debug( this, "Player %s setting artifact slot to '%s'", player() -> name(),
        util::slot_type_string( i.slot ) );
      m_slot = i.slot;
    }
  } );
}

bool player_artifact_data_t::enabled() const
{
  if ( sim() -> disable_artifacts )
  {
    return false;
  }
  else
  {
    return m_slot != SLOT_INVALID;
  }
}

sim_t* player_artifact_data_t::sim() const
{ return m_player -> sim; }

player_t* player_artifact_data_t::player() const
{ return m_player; }

void player_artifact_data_t::set_artifact_str( const std::string& value )
{
  debug( this, "Player %s setting artifact input to '%s'", player() -> name(), value.c_str() );
  m_artifact_str = value;
}

bool player_artifact_data_t::add_power( unsigned power_id, unsigned rank )
{
  if ( power_id == 0 || rank == 0 || rank > MAX_TRAIT_RANK )
  {
    debug( this, "Player %s invalid power, id=%u rank=%u", player() -> name(), power_id, rank );
    return false;
  }

  // Note, must not use overrides here to ensure artifact input parsing is not affected
  if ( purchased_power_rank( power_id, DISALLOW_OVERRIDE ) > 0 )
  {
    debug( this, "Player%s re-adding power %u", player() -> name(), power_id );
    return false;
  }

  if ( ! valid_power( power_id ) )
  {
    debug( this, "Player %s invalid power, id=%u", player() -> name(), power_id );
    return false;
  }

  m_points[ power_id ] = { as<uint8_t>( rank ), 0U };
  m_total_points += rank;
  m_purchased_points += rank;

  if ( sim() -> debug )
  {
    debug( this, "Player %s added artifact trait '%s' (id=%u), rank=%u",
      player() -> name(), power( power_id ) -> name, power_id, rank );
  }

  return true;
}

void player_artifact_data_t::override_power( const std::string& name_str, unsigned rank )
{
  if ( rank > MAX_TRAIT_RANK || name_str.empty() || ! enabled() )
  {
    return;
  }

  auto p = powers();
  auto it = range::find_if( p, [ &name_str ]( const artifact_power_data_t* power ) {
    std::string power_name = power -> name;
    util::tokenize( power_name );
    return util::str_compare_ci( name_str, power_name );
  } );

  if ( it == p.end() )
  {
    error( this, "Player %s unknown artifact power name '%s' to override",
      player() -> name(), name_str.c_str() );
    return;
  }

  if ( rank > ( *it ) -> max_rank )
  {
    error( this, "Player %s too high rank '%u' for '%s', expected %u max",
      player() -> name(), rank, ( *it ) -> name, ( *it ) -> max_rank );
    return;
  }

  // Remove old ranks
  auto purchased_ranks = purchased_power_rank( ( *it ) -> id );
  auto bonus_ranks = power_rank( ( *it ) -> id ) - purchased_ranks;

  m_total_points -= purchased_ranks;
  m_purchased_points -= purchased_ranks;

  if ( bonus_ranks > 0 )
  {
    m_total_points -= bonus_ranks;
  }

  // Inject the overridden rank as purely purchased points
  m_total_points += rank;
  m_purchased_points += rank;

  auto point_it = m_points.find( ( *it ) -> id );
  if ( point_it == m_points.end() )
  {
    m_points[ ( *it ) -> id ] = { 0U, 0U, as<int16_t>( rank ) };
  }
  else
  {
    m_points[ ( *it ) -> id ].overridden = as<int16_t>( rank );
  }
}

void player_artifact_data_t::add_relic( unsigned item_id,
                                        unsigned power_id,
                                        unsigned rank )
{
  if ( item_id == 0 || power_id == 0 || rank == 0 || rank > MAX_TRAIT_RANK )
  {
    debug( this, "Player %s invalid relic id=%u, power=%u, rank=%u",
      player() -> name(), item_id, power_id, rank );
    return;
  }

  if ( ! valid_power( power_id ) )
  {
    debug( this, "Player %s invalid power, id=%u", player() -> name(), power_id );
    return;
  }

  auto it = m_points.find( power_id );
  if ( it == m_points.end() )
  {
    m_points[ power_id ] = { 0U, as<uint8_t>( rank ) };
  }
  else if ( m_points[ power_id ].bonus + rank <= MAX_TRAIT_RANK )
  {
    m_points[ power_id ].bonus += rank;
  }

  m_total_points += rank;

  if ( sim() -> debug )
  {
    debug( this, "Player %s added artifact relic trait '%s' (id=%u, item_id=%u), rank=%u",
      player() -> name(), power( power_id ) -> name, power_id, item_id, rank );
  }
}

void player_artifact_data_t::move_purchased_rank( unsigned power_id, unsigned rank )
{
  if ( power_id == 0 || rank == 0 || rank > MAX_TRAIT_RANK )
  {
    debug( this, "Player %s invalid power to move, id=%u rank=%u", player() -> name(), power_id, rank );
    return;
  }

  if ( ! valid_power( power_id ) )
  {
    debug( this, "Player %s invalid power to move, id=%u", player() -> name(), power_id );
    return;
  }

  auto it = m_points.find( power_id );
  if ( it == m_points.end() )
  {
    debug( this, "Player %s no trait for power id=%u", player() -> name(), power_id );
    return;
  }

  // Ensure we don't underflow
  auto max_removed = std::min( it -> second.purchased, as<uint8_t>( rank ) );

  it -> second.purchased -= max_removed;
  it -> second.bonus += max_removed;
  m_purchased_points -= max_removed;

  // Moving around purchased ranks indicates there are relics in the artifact trait data
  // automatically
  m_has_relic_opts = true;

  if ( sim() -> debug )
  {
    debug( this, "Player %s moved rank for artifact trait '%s' (id=%u), rank=%u",
      player() -> name(), power( power_id ) -> name, power_id, rank );
  }
}

std::string player_artifact_data_t::encode() const
{
  if ( ! m_artifact_str.empty() )
  {
    return m_artifact_str;
  }

  std::stringstream s;

  auto artifact_id = m_player -> dbc.artifact_by_spec( m_player -> specialization() );
  if ( artifact_id == 0 )
  {
    return std::string();
  }

  // Artifact id, then 4 zeros (relics), simc uses the weapon relic data for this
  s << artifact_id << ":0:0:0:0";

  // Order artifact powers statically
  auto order = powers();
  range::for_each( order, [ &s, this ]( const artifact_power_data_t* power ) {
    auto it = m_points.find( power -> id );
    if ( it != m_points.end() && it -> second.purchased > 0 )
    {
      s << ":" << it -> first << ":" << +it -> second.purchased;
    }
  } );

  return s.str();
}

std::string player_artifact_data_t::option_string() const
{
  if ( ! enabled() || m_player -> specialization() == SPEC_NONE )
  {
    return std::string();
  }

  auto artifact_id = m_player -> dbc.artifact_by_spec( m_player -> specialization() );
  if ( artifact_id == 0 )
  {
    return std::string();
  }

  auto encoded_data = encode();
  if ( ! encoded_data.empty() )
  {
    return "artifact=" + encoded_data;
  }
  else
  {
    return encoded_data;
  }
}

std::vector<const artifact_power_data_t*> player_artifact_data_t::powers() const
{
  auto artifact_id = player() -> dbc.artifact_by_spec( player() -> specialization() );
  if ( artifact_id == 0 )
  {
    return { };
  }

  return player() -> dbc.artifact_powers( artifact_id );
}

const artifact_power_data_t* player_artifact_data_t::power( unsigned power_id ) const
{ return player() -> dbc.artifact_power( power_id ); }

bool player_artifact_data_t::valid_power( unsigned power_id ) const
{
  // Ensure artifact power belongs to the specialization
  auto p = powers();
  auto it = range::find_if( p, [ power_id ]( const artifact_power_data_t* power ) {
      return power_id == power -> id;
  } );

  return it != p.end();
}

js::JsonOutput&& player_artifact_data_t::generate_report( js::JsonOutput&& root ) const
{
  if ( ! enabled() || points() == 0 )
  {
    return std::move( root );
  }

  root.make_array();

  auto order = powers();
  range::for_each( order, [ &root, this ]( const artifact_power_data_t* power ) {
    // No point found
    auto it = m_points.find( power -> id );
    if ( it == m_points.end() )
    {
      return;
    }

    // Unknown spell
    auto spell = player() -> dbc.spell( power -> power_spell_id );
    if ( spell == nullptr )
    {
      return;
    }

    auto node = root.add();

    node[ "id" ] = power -> id;
    node[ "spell_id" ] = spell -> id();
    node[ "name" ] = spell -> name_cstr();
    node[ "total_rank" ] = it -> second.purchased + it -> second.bonus;
    node[ "purchased_rank" ] = it -> second.purchased;
    node[ "relic_rank" ] = it -> second.bonus;
    if ( it -> second.overridden >= 0 )
    {
      node[ "override_rank" ] = it -> second.overridden;
    }
  } );

  return std::move( root );
}

report::sc_html_stream& player_artifact_data_t::generate_report( report::sc_html_stream& root ) const
{
  if ( ! enabled() || points() == 0 )
  {
    return root;
  }

  root << "<tr class=\"left\">\n<th>Artifact</th>\n<td><ul class=\"float\">\n";

  root << "<li><a href=\""
       << util::create_wowhead_artifact_url( *player() )
       << "\" target=\"_blank\">Calculator (Wowhead.com)</a></li>";

  root << "<li>Purchased points: " << purchased_points() << ", total " << points() << "</li>";
  root << "</ul></td></tr>";

  root << "<tr class=\"left\">\n<th></th><td><ul class=\"float\">\n";

  auto order = powers();
  range::for_each( order, [ &root, this ]( const artifact_power_data_t* power ) {
    // No point found
    auto it = m_points.find( power -> id );
    if ( it == m_points.end() )
    {
      return;
    }

    auto spell = player() -> dbc.spell( power -> power_spell_id );
    auto purchased_rank = purchased_power_rank( power -> id );
    auto relic_rank = power_rank( power -> id ) - purchased_rank;

    std::string rank_str;
    if ( relic_rank > 0 )
    {
      rank_str = util::to_string( purchased_rank ) + " + " +
                 util::to_string( relic_rank );
    }
    else
    {
      rank_str = util::to_string( purchased_rank );
    }

    root << "<li>" << ( spell ? report::spell_data_decorator_t( player(), spell ).decorate()
                              : power -> name );

    if ( power -> max_rank > 1 && purchased_rank + relic_rank > 0 )
    {
      root << " (Rank " << rank_str << ")";
    }

    if ( it -> second.overridden >= 0 )
    {
      if ( purchased_rank == 0 )
      {
        root << " (<b>Disabled</b>)";
      }
      else
      {
        root << " (<b>Overridden</b>)";
      }
    }

    root << "</li>";
  } );

  root << "</ul></td>";

  root << "</tr>";

  return root;
}
} // Namespace artifact ends
