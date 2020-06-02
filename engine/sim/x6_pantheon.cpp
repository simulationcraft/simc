#include "x6_pantheon.hpp"

#include "simulationcraft.hpp"
#include <memory>
#include <iomanip>

namespace
{
// Ticker that continuously attempts to proc proxy pantheon buffs (and potentially the empowered
// version if there are all pantheon base buffs up at the same time)
struct pantheon_ticker_t : public event_t
{
  unique_gear::pantheon_state_t& obj;

  pantheon_ticker_t( unique_gear::pantheon_state_t& o, timespan_t delay ) :
    event_t( *o.player ), obj( o )
  {
    // TODO: Configurable
    schedule( delay );
  }

  void execute() override
  { obj.attempt(); }

  const char* name() const override
  { return "proxy_pantheon_event"; }
};

static const std::vector<std::string> mark_strs {
  "Am", "Go", "Kh", "Eo", "No", "Ag"
};
} // Anonymous namespace ends

namespace unique_gear
{
void initialize_pantheon( player_t* proxy_player )
{
  if ( ! proxy_player -> sim -> legion_data.pantheon_proxy )
  {
    proxy_player -> sim -> legion_data.pantheon_proxy =
      std::make_unique<pantheon_state_t>( proxy_player );
  }
}

pantheon_state_t::pantheon_state_t( player_t* player ) :
  player( player ), attempt_event( nullptr )
{
  rppm_objs.resize( drivers.size() );
  actor_buffs.resize( drivers.size() );
  pantheon_state.resize( drivers.size() );

  // Initialize buff durations list from the mark auras so we don't have to access spell data
  // needlessly
  range::for_each( marks, [ player, this ]( unsigned aura_id ) {
    auto spell = player -> find_spell( aura_id );
    buff_durations.push_back( spell -> duration() );
  } );

  parse_options();

  auto obj_idx = 0;
  // Make some RPPM object to proxy off of, based on parsed user options. Note that this is "hasted
  // RPPM" aware, and will not allow the user input to apply haste to trinket procs that are not
  // haste scaled (according to client data).
  range::for_each( pantheon_opts, [ & ]( const pantheon_opt_t& opt ) {
    auto driver = player -> find_spell( drivers[ opt.first ] );
    auto rppm_scale = player -> dbc->real_ppm_scale( driver -> id() );

    std::string name = driver -> name_cstr();
    util::tokenize( name );
    name += util::to_string( obj_idx++ );

    auto rppm_obj = player -> get_rppm( name, driver -> real_ppm(), rppm_scale == RPPM_HASTE ? 1.0 + opt.second : 1.0 );

    rppm_objs[ opt.first ].push_back( rppm_obj );
  } );
}

void pantheon_state_t::parse_options()
{
  auto splits = util::string_split( player -> sim -> legion_opts.pantheon_trinket_users, "/" );
  range::for_each( splits, [ this ]( const std::string& str ) {
    auto split = util::string_split( str, ":" );
    const auto& pantheon = split[ 0 ];
    int pantheon_index = -1;
    double haste = 0.0;
    if ( pantheon.size() < 2 )
    {
      return;
    }

    for ( size_t i = 0; i < mark_strs.size(); ++i )
    {
      if ( util::str_compare_ci( pantheon.substr( 0, 2 ), mark_strs[ i ] ) )
      {
        pantheon_index = as<int>( i );
        break;
      }
    }

    if ( pantheon_index == -1 )
    {
      player -> sim -> errorf( "Unknown pantheon type %s", pantheon.c_str() );
      return;
    }

    if ( split.size() >= 2 )
    {
      haste = std::stod( split[ 1 ] );
      if ( haste <= 0 )
      {
        player -> sim -> errorf( "Invalid haste value %s", split[ 1 ].c_str() );
        return;
      }
    }

    pantheon_opts.emplace_back( as<size_t>( pantheon_index ), haste );
  } );
}

// Figure out if the proxy pantheon system must be started. Returns true, if the combination of
// actor base trinket buffs and proxy buffs is equal or exceeds the required number of simultaneous
// active pantheon base trinket buffs.
bool pantheon_state_t::has_pantheon_capability() const
{
  size_t n_wildcard = rppm_objs[ O_WILDCARD_TRINKET ].size();
  if ( player -> sim -> single_actor_batch )
  {
    auto it = range::find_if( actor_buffs[ O_WILDCARD_TRINKET ], []( const buff_t* b ) {
      return b -> source == b -> sim -> player_no_pet_list[ b -> sim -> current_index ];
    } );

    n_wildcard += as<size_t>( it != actor_buffs[ O_WILDCARD_TRINKET ].end() );
  }
  else
  {
    n_wildcard += actor_buffs[ O_WILDCARD_TRINKET ].size();
  }

  if ( n_wildcard >= N_UNIQUE_BUFFS )
  {
    return true;
  }

  size_t n_unique = 0;
  for ( size_t slot = O_UNIQUE_TRINKET, end = drivers.size(); slot < end; ++slot )
  {
    if ( rppm_objs[ slot ].size() )
    {
      n_unique++;
    }
    else
    {
      if ( player -> sim -> single_actor_batch )
      {
        // Currently active player has a pantheon base trinket buff of the type
        auto it = range::find_if( actor_buffs[ slot ], []( const buff_t* b ) {
          return b -> source == b -> sim -> player_no_pet_list[ b -> sim -> current_index ];
        } );

        n_unique += as<size_t>( it != actor_buffs[ slot ].end() );
      }
      else
      {
        n_unique += as<size_t>( actor_buffs[ slot ].size() > 0 );
      }
    }

    if ( n_wildcard + n_unique >= N_UNIQUE_BUFFS )
    {
      return true;
    }
  }

  return n_wildcard + n_unique >= N_UNIQUE_BUFFS;
}

void pantheon_state_t::start()
{
  if ( has_pantheon_capability() )
  {
    attempt_event = make_event<pantheon_ticker_t>( *player -> sim, *this, pantheon_ticker_delay() );
  }
}

void pantheon_state_t::attempt()
{
  cleanup_state();

  for ( size_t i = 0; i < rppm_objs.size(); ++i )
  {
    range::for_each( rppm_objs[ i ], [ this, i ]( real_ppm_t* rppm ) {
      if ( rppm -> trigger() )
      {
        auto now = player -> sim -> current_time();
        auto state = buff_state( i, rppm );

        // New proxy state, add to the slot's state vector
        if ( state == nullptr )
        {
          pantheon_state[ i ].push_back( { nullptr, rppm, now + buff_durations[ i ] } );
        }
        // Existing proxy state, extend the buff duration
        else
        {
          state -> proxy_end_time = now + buff_durations[ i ];
        }

        if ( empowerment_state() )
        {
          trigger_empowerment();
        }
      }
    } );
  }

  debug();

  attempt_event = make_event<pantheon_ticker_t>( *player -> sim, *this, pantheon_ticker_delay() );
}

// Trigger pantheon base trinket buff from a real actor. Check empowerment state, and potentially
// trigger all queued actor empowerment buffs.
void pantheon_state_t::trigger_pantheon_buff( buff_t* actor_buff )
{
  if ( ! actor_buff )
  {
    return;
  }

  auto slot = buff_type( actor_buff );
  if ( slot >= marks.size() )
  {
    return;
  }

  auto state = buff_state( slot, actor_buff );
  if ( state == nullptr )
  {
    pantheon_state[ slot ].push_back( { actor_buff, nullptr, timespan_t::min() } );
  }

  // Clean up state before trying to figure out if there's empowerment to trigger
  cleanup_state();

  if ( empowerment_state() )
  {
    trigger_empowerment();
  }
}

// Empowerment state check, current rules (as of 2017-11-14)
//
// 1) Require a combination of at least 4 active base trinket buffs
// 2) Treat Aman'Thul's Vision as a "wildcard buff", allowing each to count for 1)
// 3) Treat any number of Aggramar's Conviction, Golganneth's Vitality, Eonar's Compassion,
//    Khazgoroth's Courage, and Norgannon's Prowess as a single buff; one buff of the given type
//    will satisfy (a single) buff for the empowerment state check
// 4) Once at least 4 base trinket buffs are up, trigger empowerment on all real actors who have
//    buffs up. Note that this can result in more than 4 empowerment buffs to trigger (if multiples
//    of the trinkets in 3) are up currently)
// 5) Clear the empowerment state
//
// Note that empowerment_state() must always be called after state has been clean up (for that
// timestamp); no expiration checks are done here.
//
bool pantheon_state_t::empowerment_state() const
{
  size_t n_wildcard = pantheon_state[ O_WILDCARD_TRINKET ].size();
  if ( n_wildcard >= N_UNIQUE_BUFFS )
  {
    return true;
  }

  size_t n_unique = 0;
  for ( size_t slot = O_UNIQUE_TRINKET, end = pantheon_state.size(); slot < end; ++slot )
  {
    n_unique += as<size_t>( pantheon_state[ slot ].size() > 0 );
  }

  return n_wildcard + n_unique >= N_UNIQUE_BUFFS;
}

// Trigger the empowerment buffs for all real actors and clear the empowerment state. Note that if
// there are no real actors with base trinket buffs up, nothing is triggered.
void pantheon_state_t::trigger_empowerment()
{
  if ( player -> sim -> debug )
  {
    player -> sim -> out_debug.printf( "Pantheon state: Triggering empowerment on actors and clearing state" );
    debug();
  }

  range::for_each( pantheon_state[ O_WILDCARD_TRINKET ], [ this ]( const pantheon_buff_state_t& state ) {
    if ( state.actor_buff )
    {
      auto it = cbmap.find( state.actor_buff );
      if ( it != cbmap.end() )
      {
        it -> second();
      }
    }
  } );

  pantheon_state[ O_WILDCARD_TRINKET ].clear();

  range::for_each( pantheon_state, [ this ]( std::vector<pantheon_buff_state_t>& states ) {
    if ( states.size() == 0 )
      return;

    auto random_it = states.begin() + static_cast<int>( player -> rng().range( 0, as<double>( states.size() ) ) );
    if ( random_it -> actor_buff )
    {
      auto it = cbmap.find( random_it -> actor_buff );
      if ( it != cbmap.end() )
      {
        it -> second();
      }
    }

    states.erase( random_it );
  } );

  debug();
}

void pantheon_state_t::debug() const
{
  if ( ! player -> sim -> debug )
  {
    return;
  }

  std::stringstream proxy_s;

  proxy_s << std::fixed;
  proxy_s << std::setprecision( 3 );
  proxy_s << "Pantheon state: ";

  for ( size_t slot = 0, end = pantheon_state.size(); slot < end; ++slot )
  {
    const auto& states = pantheon_state[ slot ];

    proxy_s << mark_strs[ slot ] << ": ";
    proxy_s << "(" << states.size() << ")";

    if ( states.size() > 0 )
    {
      timespan_t max_end = timespan_t::min();
      bool is_user = false;
      for ( const auto& state : states )
      {
        if ( state.actor_buff && state.actor_buff -> remains() > max_end )
        {
          is_user = true;
          max_end = player -> sim -> current_time() + state.actor_buff -> remains();
        }
        else if ( state.proxy_end_time > max_end )
        {
          is_user = false;
          max_end = state.proxy_end_time;
        }
      }

      proxy_s << " " << ( is_user ? "*" : "" ) <<
        ( max_end - player -> sim -> current_time() ).total_seconds();
    }

    if ( slot < end - 1 )
    {
      proxy_s << ", ";
    }
  }

  player -> sim -> out_debug << proxy_s.str();
}

timespan_t pantheon_state_t::pantheon_ticker_delay() const
{
  if ( player -> sim -> legion_opts.pantheon_trinket_interval_stddev == 0 )
  {
    return player -> sim -> legion_opts.pantheon_trinket_interval;
  }

  auto dev = player -> sim -> legion_opts.pantheon_trinket_interval *
             player -> sim -> legion_opts.pantheon_trinket_interval_stddev;

  return player -> rng().gauss( player -> sim -> legion_opts.pantheon_trinket_interval, dev );
}

pantheon_state_t::pantheon_buff_state_t* pantheon_state_t::buff_state( size_t type, const real_ppm_t* rppm )
{
  assert( type < pantheon_state.size() );

  auto it = range::find_if( pantheon_state[ type ], [ rppm ]( pantheon_buff_state_t& state ) {
    return state.rppm_object == rppm;
  } );

  if ( it != pantheon_state[ type ].end() )
  {
    return &( *it );
  }

  return nullptr;
}

pantheon_state_t::pantheon_buff_state_t* pantheon_state_t::buff_state( size_t type, const buff_t* buff )
{
  assert( type < pantheon_state.size() );

  auto it = range::find_if( pantheon_state[ type ], [ buff ]( pantheon_buff_state_t& state ) {
    return state.actor_buff == buff;
  } );

  if ( it != pantheon_state[ type ].end() )
  {
    return &( *it );
  }

  return nullptr;
}

// Remove all state from the system that has expired. In practice, either
// 1) Current simulation time is past a given proxy buff end time
// 2) An actor base trinket buff is no longer up
void pantheon_state_t::cleanup_state()
{
  range::for_each( pantheon_state, [ this ]( std::vector<pantheon_buff_state_t>& states ) {
    auto it = states.begin();
    while ( it != states.end() )
    {
      auto& state = *it;
      if ( ! state.actor_buff && state.proxy_end_time < player -> sim -> current_time() )
      {
        it = states.erase( it );
      }
      else if ( state.actor_buff && ! state.actor_buff -> check() )
      {
        it = states.erase( it );
      }
      else
      {
        it++;
      }
    }
  } );
}

size_t pantheon_state_t::buff_type( const buff_t* actor_buff ) const
{
  auto it = range::find_if( marks, [ actor_buff ]( unsigned id ) {
    return id == actor_buff -> data().id();
  } );

  if ( it != marks.end() )
  {
    return std::distance( marks.begin(), it );
  }
  else
  {
    return marks.size();
  }
}

void pantheon_state_t::reset()
{
  range::for_each( pantheon_state, []( std::vector<pantheon_buff_state_t>& states ) {
    states.clear();
  } );

  // We need to reset the RPPM objects manually with single_actor_batch=1, because the first defined
  // actor is only active on the first actor simulation.
  if ( player -> sim -> single_actor_batch )
  {
    range::for_each( rppm_objs, []( const std::vector<real_ppm_t*>& rppms ) {
      range::for_each( rppms, []( real_ppm_t* obj ) { obj -> reset(); } );
    } );
  }

  attempt_event = nullptr;
}


const std::vector<unsigned> pantheon_state_t::drivers {
  256817, // Mark of Aman'thul
  256819, // Mark of Golganneth
  256825, // Mark of Khaz'goroth
  256822, // Mark of Eonar
  256827, // Mark of Norgannon
  256815, // Mark of Aggramar
};

const std::vector<unsigned> pantheon_state_t::marks {
  256818, // Mark of Aman'thul
  256821, // Mark of Golganneth
  256826, // Mark of Khaz'goroth
  256824, // Mark of Eonar
  256828, // Mark of Norgannon
  256816, // Mark of Aggramar
};

} // Namespace unique_gear ends
