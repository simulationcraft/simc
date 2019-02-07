// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "pet_spawner.hpp"

namespace
{
template <typename T>
bool less( const T* l, const T* r )
{
  sim_t* sim = l -> sim;

  auto l_remains = l -> expiration
                   ? l -> expiration -> remains()
                   : sim -> expected_iteration_time - sim -> current_time();
  auto r_remains = r -> expiration
                   ? r -> expiration -> remains()
                   : sim -> expected_iteration_time - sim -> current_time();

  return l_remains < r_remains;
}
} // Anonymous namespace ends

namespace spawner
{
// Constructors

template<typename T, typename O>
pet_spawner_t<T, O>::pet_spawner_t( const std::string& id, O* p, pet_spawn_type st ) :
  base_actor_spawner_t( id, p ), m_max_pets( st == PET_SPAWN_DYNAMIC ? 0 : 1 ),
  m_creator( []( O* p ) { return new T( p ); } ),
  m_duration( timespan_t::zero() ), m_type( st ),
  m_cumulative_uptime( timespan_t::zero() ), m_spawn_time( timespan_t::min() ),
  m_dirty( false ), m_active( 0u ), m_initial_pet( nullptr )
{ }

template<typename T, typename O>
pet_spawner_t<T, O>::pet_spawner_t( const std::string& id, O* p, unsigned max_pets,
                                        pet_spawn_type st ) :
  base_actor_spawner_t( id, p ), m_max_pets( max_pets ),
  m_creator( []( O* p ) { return new T( p ); } ),
  m_duration( timespan_t::zero() ), m_type( st ),
  m_cumulative_uptime( timespan_t::zero() ), m_spawn_time( timespan_t::min() ),
  m_dirty( false ), m_active( 0u ), m_initial_pet( nullptr )
{ }

template<typename T, typename O>
pet_spawner_t<T, O>::pet_spawner_t( const std::string& id, O* p, unsigned max_pets,
                                     const create_fn_t& creator, pet_spawn_type st ) :
  base_actor_spawner_t( id, p ), m_max_pets( max_pets ), m_creator( creator ),
  m_duration( timespan_t::zero() ), m_type( st ),
  m_cumulative_uptime( timespan_t::zero() ), m_spawn_time( timespan_t::min() ),
  m_dirty( false ), m_active( 0u ), m_initial_pet( nullptr )
{ }

template<typename T, typename O>
pet_spawner_t<T, O>::pet_spawner_t( const std::string& id, O* p, const create_fn_t& creator,
                                        pet_spawn_type st ) :
  base_actor_spawner_t( id, p ), m_max_pets( st == PET_SPAWN_DYNAMIC ? 0 : 1 ), m_creator( creator ),
  m_duration( timespan_t::zero() ), m_type( st ),
  m_cumulative_uptime( timespan_t::zero() ), m_spawn_time( timespan_t::min() ),
  m_dirty( false ), m_active( 0u ), m_initial_pet( nullptr )
{ }

// Accessors

template <typename T, typename O>
size_t pet_spawner_t<T, O>::n_active_pets() const
{
  return m_active;
}

template <typename T, typename O>
size_t pet_spawner_t<T, O>::n_active_pets( const check_arg_fn_t& fn )
{
  update_state();

  if ( fn )
  {
    auto n = std::count_if( m_active_pets.begin(), m_active_pets.end(), fn );
    return as<size_t>( n );
  }

  return m_active_pets.size();
}

template <typename T, typename O>
size_t pet_spawner_t<T, O>::n_inactive_pets() const
{ return m_pets.size() - m_active; }

template <typename T, typename O>
size_t pet_spawner_t<T, O>::n_pets() const
{ return m_pets.size(); }

template <typename T, typename O>
size_t pet_spawner_t<T, O>::max_pets() const
{ return m_max_pets; }

template <typename T, typename O>
const timespan_t& pet_spawner_t<T, O>::duration() const
{ return m_duration; }

template <typename T, typename O>
typename std::vector<T*>::iterator pet_spawner_t<T, O>::begin()
{
  update_state();
  return m_active_pets.begin();
}

template <typename T, typename O>
typename std::vector<T*>::iterator pet_spawner_t<T, O>::end()
{
  return m_active_pets.end();
}

template <typename T, typename O>
typename std::vector<T*>::const_iterator pet_spawner_t<T, O>::cbegin()
{
  update_state();
  return m_active_pets.cbegin();
}

template <typename T, typename O>
typename std::vector<T*>::const_iterator pet_spawner_t<T, O>::cend()
{
  return m_active_pets.cend();
}

template <typename T, typename O>
pet_spawner_t<T, O>& pet_spawner_t<T, O>::set_max_pets( unsigned v )
{ m_max_pets = v; return *this; }

template <typename T, typename O>
pet_spawner_t<T, O>& pet_spawner_t<T, O>::set_creation_callback( const create_fn_t& fn )
{ m_creator = fn; return *this; }

template <typename T, typename O>
pet_spawner_t<T, O>& pet_spawner_t<T, O>::set_creation_check_callback( const check_fn_t& fn )
{ m_check_create = fn; return *this; }

template <typename T, typename O>
pet_spawner_t<T, O>& pet_spawner_t<T, O>::set_creation_event_callback( const apply_fn_t& fn )
{ m_event_create.push_back( fn ); return *this; }

template <typename T, typename O>
pet_spawner_t<T, O>& pet_spawner_t<T, O>::set_default_duration( const spell_data_t* spell )
{ m_duration = spell -> duration(); return *this; }

template <typename T, typename O>
pet_spawner_t<T, O>& pet_spawner_t<T, O>::set_default_duration( const spell_data_t& spell )
{ m_duration = spell.duration(); return *this; }

template <typename T, typename O>
pet_spawner_t<T, O>& pet_spawner_t<T, O>::set_default_duration( const timespan_t& duration )
{ m_duration = duration; return *this; }

template <typename T, typename O>
pet_spawner_t<T, O>& pet_spawner_t<T, O>::add_expression( const std::string& name, const expr_fn_t& fn )
{ m_expressions[ name ] = fn; return *this; }

// Despawners

template <typename T, typename O>
size_t pet_spawner_t<T, O>::despawn()
{
  update_state();
  size_t despawned = m_active_pets.size();
  while ( m_active_pets.size() )
  {
    m_active_pets.front() -> dismiss();
  }

  return despawned;
}

template <typename T, typename O>
bool pet_spawner_t<T, O>::despawn( const T* obj )
{
  bool despawned = false;
  auto it = m_active_pets.find( obj );

  if ( ( despawned = it != m_active_pets.end() ) == true )
  {
    it -> dismiss();
  }

  return despawned;
}

template <typename T, typename O>
size_t pet_spawner_t<T, O>::despawn( const std::vector<const T*>& obj )
{
  size_t despawned = 0;

  update_state();

  range::for_each( obj, [ this, &despawned ]( const T* pet ) {
    auto it = range::find( m_active_pets.find( pet ) );
    if ( it != m_active_pets.end() )
    {
      ++despawned;
      it -> dismiss();
    }
  } );

  return despawned;
}

template <typename T, typename O>
size_t pet_spawner_t<T, O>::despawn( const check_arg_fn_t& fn )
{
  size_t despawned = 0;

  update_state();

  std::vector<T*> tmp = { m_active_pets };

  range::for_each( tmp, [ &despawned, &fn ]( const T* pet ) {
    if ( fn( pet ) )
    {
      ++despawned;
      pet -> dismiss();
    }
  } );

  return despawned;
}

// Time adjustments

template <typename T, typename O>
void pet_spawner_t<T, O>::extend_expiration( const timespan_t& adjustment )
{
  if ( adjustment <= timespan_t::zero() )
  {
    return;
  }

  for ( auto pet : m_active_pets )
  {
    if ( ! pet->expiration )
    {
      continue;
    }

    timespan_t new_duration = pet -> remains();
    new_duration += adjustment;

    pet -> expiration -> reschedule( new_duration );
  }
}

// Spawning

template <typename T, typename O>
T* pet_spawner_t<T, O>::create_pet( create_phase phase )
{
  T* pet = m_creator( static_cast<O*>( m_owner ) );
  if ( pet == nullptr )
  {
    return nullptr;
  }

  if ( m_initial_pet == nullptr )
  {
    m_initial_pet = pet;
  }

  pet -> spawner = this;

  // Add callbacks to the newly created pet so we can auto-track it's active state
  pet -> callbacks_on_arise.push_back( [ this ]() {
    m_dirty = true;
    if ( ++m_active == 1u )
    {
      assert( m_spawn_time == timespan_t::min() );

      m_spawn_time = m_owner -> sim -> current_time();
    }
  } );

  pet -> callbacks_on_demise.push_back( [ this, pet ]( player_t* /*pet*/ ) {
    m_dirty = true;

    if ( --m_active == 0u )
    {
      assert( m_spawn_time > timespan_t::min() );
      m_cumulative_uptime += m_owner -> sim -> current_time() - m_spawn_time;
      m_spawn_time = timespan_t::min();
    }

    m_inactive_pets.push_back( pet );
  } );

  // Note, only dynamic spawns go through the init process when they are created. Persistent pet
  // spawns are created early on in the owner's init process, and are readily available for the
  // simulator's global initialization system.
  if ( m_type == PET_SPAWN_DYNAMIC )
  {
    // Pets get set their "dynamic" flag to automatically do data collection optimization
    pet -> dynamic = true;

    // Initialize the pet fully, and start its combat
    m_owner -> sim -> init_actor( pet );
    pet -> init_finished();

    range::for_each( m_event_create, [ pet ]( const apply_fn_t& fn ) { fn( pet ); } );

    if ( m_owner -> sim -> single_actor_batch )
    {
      pet -> activate();
    }

    pet -> reset();

    if ( phase == PHASE_COMBAT )
    {
      pet -> combat_begin();
    }
  }

  return pet;
}

template <typename T, typename O>
std::vector<T*> pet_spawner_t<T, O>::spawn( unsigned n )
{
  return spawn( timespan_t::min(), n );
}

template <typename T, typename O>
std::vector<T*> pet_spawner_t<T, O>::spawn( const timespan_t& duration, unsigned n )
{
  std::vector<T*> pets;

  if ( m_owner -> sim -> debug )
  {
    m_owner -> sim -> out_debug.printf( "%s pet_spawner %s, n_pets=%u, n_active_pets=%u",
      m_owner -> name(), m_name.c_str(), n_pets(), n_active_pets() );
  }

  unsigned actual = n;
  // Default behavior, dynamic = 1, persistent = m_max_pets
  if ( actual == 0 )
  {
    actual = m_type == PET_SPAWN_DYNAMIC ? 1 : m_max_pets;
  }

  if ( m_max_pets > 0 )
  {
    actual = std::min( actual, m_max_pets - as<unsigned>( n_active_pets() ) );
  }

  if ( actual == 0 )
  {
    return {};
  }

  timespan_t actual_duration = duration;
  if ( actual_duration == timespan_t::min() )
  {
    actual_duration = m_duration;
  }

  // Recycle inactive pets
  while ( m_inactive_pets.size() && actual > 0 )
  {
    pets.push_back( m_inactive_pets.back() );
    m_inactive_pets.pop_back();
    --actual;
  }

  // Create new ones if needed
  for ( size_t i = 0; i < actual; ++i )
  {
    T* pet = create_pet( PHASE_COMBAT );
    // Something went wrong in the creation of a new pet object, cancel the simulator
    if ( pet == nullptr )
    {
      m_owner -> sim -> cancel();
      return {};
    }

    m_pets.push_back( pet );
    pets.push_back( pet );
  }

  // Summon all selected pets
  range::for_each( pets, [ &actual_duration ]( T* pet ) {
    assert( pet -> is_sleeping() );
    pet -> summon( actual_duration );
  } );

  return pets;
}

// Access

template <typename T, typename O>
std::vector<T*> pet_spawner_t<T, O>::active_pets()
{
  update_state();

  return { m_active_pets };
}

template <typename T, typename O>
std::vector<T*> pet_spawner_t<T, O>::pets()
{
  return { m_pets };
}

template <typename T, typename O>
std::vector<const T*> pet_spawner_t<T, O>::pets() const
{
  return { m_pets };
}

template <typename T, typename O>
T* pet_spawner_t<T, O>::active_pet( size_t index )
{
  if ( index >= m_active )
  {
    return nullptr;
  }

  update_state();

  return m_active_pets[ index ];
}

template <typename T, typename O>
T* pet_spawner_t<T, O>::active_pet( const check_arg_fn_t& fn )
{
  update_state();

  auto it = range::find_if( m_active_pets, fn );
  if ( it == m_active_pets.end() )
  {
    return nullptr;
  }

  return *it;
}

template <typename T, typename O>
T* pet_spawner_t<T, O>::active_pet_min_remains( const check_arg_fn_t& fn )
{
  update_state();

  auto it = std::min_element( m_active_pets.cbegin(), m_active_pets.cend(),
      [ &fn ]( const T* l, const T* r ) {
        if ( fn && ( ! fn( l ) || ! fn( r ) ) )
        {
          return false;
        }

        return less( l, r );
      } );

  if ( it == m_active_pets.end() )
  {
    return nullptr;
  }

  return *it;
}

template <typename T, typename O>
T* pet_spawner_t<T, O>::active_pet_max_remains( const check_arg_fn_t& fn )
{
  update_state();

  auto it = std::max_element( m_active_pets.cbegin(), m_active_pets.cend(),
    [ &fn ]( const T* l, const T* r ) {
      if ( fn && ( ! fn( l ) || ! fn( r ) ) )
      {
        return false;
      }

      return less( l, r );
    } );

  if ( it == m_active_pets.end() )
  {
    return nullptr;
  }

  return *it;
}

template <typename T, typename O>
T* pet_spawner_t<T, O>::pet( size_t index ) const
{
  if ( index >= n_pets() )
  {
    return nullptr;
  }

  return m_pets[ index ];
}

template <typename T, typename O>
void pet_spawner_t<T, O>::merge( base_actor_spawner_t* other )
{
  // This method should not be invoked from non-main threads
  if ( m_owner -> sim -> parent )
  {
    return;
  }

  // Persistent pet spawns get merged through normal sim merge mechanism
  if ( m_type == PET_SPAWN_PERSISTENT )
  {
    return;
  }

  const auto o = debug_cast<pet_spawner_t<T, O>*>( other );
  if ( o == this )
  {
    return;
  }

  auto n_shared = std::min( n_pets(), o -> n_pets() );
  int n_extra = as<int>( o -> n_pets() ) - as<int>( n_pets() );

  // Merge shared pets .. note that the pet indeices may not be the same, but the dynamic pets
  // don't really care about that. We can presume that between the threads, the "natural order" of
  // the dynamic pets in terms of the simulation model behavior is the order in which they have
  // been created by this class
  for ( size_t idx = 0; idx < n_shared; ++idx )
  {
    m_pets[ idx ] -> merge( *o -> m_pets[ idx ] );
  }

  // Other sim (i.e., other thread) has more pets spawned than main thread, main thread needs to
  // create placeholders and merge the others in
  for ( int new_idx = 0; new_idx < n_extra; ++new_idx )
  {
    T* pet = create_pet( PHASE_MERGE );
    if ( pet == nullptr )
    {
      continue;
    }

    // Merge other thread pet into this newly created pet
    pet -> merge( *o -> m_pets[ n_shared + new_idx ] );
  }
}

template <typename T, typename O>
void pet_spawner_t<T, O>::create_persistent_actors()
{
  if ( m_type == PET_SPAWN_DYNAMIC )
  {
    return;
  }

  if ( m_check_create && m_check_create() == false )
  {
    return;
  }

  for ( size_t i = 0; i < m_max_pets; ++i )
  {
    auto pet = create_pet( PHASE_INIT );
    m_inactive_pets.push_back( pet );
  }
}

template <typename T, typename O>
expr_t* pet_spawner_t<T, O>::create_expression( const arv::array_view<std::string>& expr )
{
  if ( expr.size() == 0 )
  {
    return nullptr;
  }

  sim_t* sim = m_owner -> sim;

  if ( util::str_compare_ci( expr[ 0 ], "active" ) )
  {
    return make_fn_expr( "active", [ this ]() { return as<double>( n_active_pets() ); } );
  }
  else if ( util::str_compare_ci( expr[ 0 ], "min_remains" ) ||
            util::str_compare_ci( expr[ 0 ], "remains" ) )
  {
    return make_fn_expr( "remains", [ this, sim ]() {
      auto pet = active_pet_min_remains();
      if ( pet == nullptr )
      {
        return 0.0;
      }

      return pet -> expiration
             ? pet -> expiration -> remains().total_seconds()
             : ( sim -> expected_iteration_time - sim -> current_time() ).total_seconds();
    } );
  }
  else if ( util::str_compare_ci( expr[ 0 ], "max_remains" ) )
  {
    return make_fn_expr( "max_remains", [ this, sim ]() {
      auto pet = active_pet_max_remains();
      if ( pet == nullptr )
      {
        return 0.0;
      }

      return pet -> expiration
             ? pet -> expiration -> remains().total_seconds()
             : ( sim -> expected_iteration_time - sim -> current_time() ).total_seconds();
    } );
  }

  auto it = m_expressions.find( expr[ 0 ] );
  if ( it != m_expressions.end() )
  {
    return it -> second( expr );
  }

  return nullptr;
}

template <typename T, typename O>
timespan_t pet_spawner_t<T, O>::iteration_uptime() const
{
  return m_cumulative_uptime;
}

template <typename T, typename O>
void pet_spawner_t<T, O>::reset()
{
  m_cumulative_uptime = timespan_t::zero();
  m_spawn_time = timespan_t::min();
}

template <typename T, typename O>
void pet_spawner_t<T, O>::datacollection_end()
{
  // If the initial created pet was not active throughout the iteration, we need to still perform
  // datacollection end/begin operations on it. This is because in many cases the "dynamic pets"
  // aggregate all action data collection to the first pet's action (stats object). That data has to
  // be collected as long as any of the dynamic pets have been active during iteration, otherwise
  // massively inflated stats for an iteration will occur
  if ( m_initial_pet && ! m_initial_pet -> active_during_iteration && m_cumulative_uptime > timespan_t::zero() )
  {
    m_initial_pet -> active_during_iteration = true;
    m_initial_pet -> datacollection_end();
  }
}

template <typename T, typename O>
void pet_spawner_t<T, O>::update_state()
{
  if ( ! m_dirty )
  {
    return;
  }

  m_active_pets.clear();
  auto max_active = m_pets.size() - m_inactive_pets.size();
  for ( size_t i = 0, end = m_pets.size(); i < end && m_active_pets.size() < max_active; ++i )
  {
    auto pet = m_pets[ i ];

    if ( ! pet -> is_sleeping() )
    {
      m_active_pets.push_back( pet );
    }
  }

  m_dirty = false;
}
} // Namespace spawner

