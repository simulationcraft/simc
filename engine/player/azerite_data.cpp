#include "azerite_data.hpp"

#include "simulationcraft.hpp"

azerite_power_t::azerite_power_t() :
  m_spell( spell_data_t::not_found() ), m_value( std::numeric_limits<double>::lowest() )
{ }

azerite_power_t::azerite_power_t( const spell_data_t* spell, const std::vector<const item_t*>& items ) :
  m_spell( spell ), m_items( items ), m_value( std::numeric_limits<double>::lowest() )
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

double azerite_power_t::value( size_t index ) const
{
  if ( index > m_spell -> effect_count() || index == 0 )
  {
    return 0.0;
  }

  if ( m_value != std::numeric_limits<double>::lowest() )
  {
    return m_value;
  }

  double sum = 0.0;
  range::for_each( m_items, [ & ]( const item_t* item ) {
      sum += m_spell -> effectN( index ).average( item );
  } );

  m_value = sum;

  return sum;
}

double azerite_power_t::value( const azerite_value_fn_t& fn ) const
{
  if ( m_value != std::numeric_limits<double>::lowest() )
  {
    return m_value;
  }

  double v = fn( *this );

  m_value = v;

  return v;
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
  }
}

std::vector<double> azerite_power_t::budget() const
{
  std::vector<double> b;

  range::for_each( m_items, [ & ]( const item_t* item ) {
    auto budget = item_database::item_budget( item, m_spell -> max_scaling_level() );
    if ( m_spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 )
    {
      budget = item_database::apply_combat_rating_multiplier( *item, budget );
    }
    b.push_back( budget );
  } );

  return b;
}

/// List of items associated with this azerite power
const std::vector<const item_t*> azerite_power_t::items() const
{ return m_items; }

namespace azerite
{
std::unique_ptr<azerite_state_t> create_state( player_t* p )
{
  return std::unique_ptr<azerite_state_t>( new azerite_state_t( p ) );
}

azerite_state_t::azerite_state_t( player_t* p ) : m_player( p )
{ }

void azerite_state_t::initialize()
{
  range::for_each( m_player -> items, [ this ]( const item_t& item ) {
    range::for_each( item.parsed.azerite_ids, [ & ]( unsigned id ) {
      m_state[ id ] = false;
      m_items[ id ].push_back( &( item ) );
    } );
  });
}

bool azerite_state_t::is_initialized( unsigned id ) const
{
  auto it = m_state.find( id );
  if ( it != m_state.end() )
  {
    return it -> second;
  }

  return false;
}

azerite_power_t azerite_state_t::get_power( unsigned id )
{
  auto it = m_state.find( id );
  if ( it == m_state.end() )
  {
    return {};
  }

  // Already initialized (successful invocation of get_power for this actor with the same id), so
  // don't give out the spell data again
  if ( it -> second )
  {
    m_player -> sim -> errorf( "%s attempting to re-initialize azerite power %u",
        m_player -> name(), id );
    return {};
  }

  it -> second = true;

  const auto& power = m_player -> dbc.azerite_power( id );

  if ( m_player -> sim -> debug )
  {
    std::stringstream s;
    const auto& items = m_items[ power.id ];

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

  return { m_player -> find_spell( power.spell_id ), m_items[ id ] };
}

azerite_power_t azerite_state_t::get_power( const std::string& name, bool tokenized )
{
  const azerite_power_entry_t* p = nullptr;

  for ( const auto& power : m_player -> dbc.azerite_powers() )
  {
    if ( tokenized )
    {
      std::string spell_name = power.name;
      util::tokenize( spell_name );
      if ( util::str_compare_ci( name, spell_name ) )
      {
        p = &( power );
        break;
      }
    }
    else if ( util::str_compare_ci( name, power.name ) )
    {
      p = &( power );
      break;
    }
  }

  if ( ! p )
  {
    return {};
  }

  return get_power( p -> id );
}

} // Namespace azerite ends
