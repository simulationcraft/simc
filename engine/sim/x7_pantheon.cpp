#include "x7_pantheon.hpp"
#include "simulationcraft.hpp"

namespace
{
// Ticker that continuously attempts to proc proxy pantheon buffs (and potentially the empowered
// version if there are all pantheon base buffs up at the same time)
struct pantheon_ticker_t : public event_t
{
  unique_gear::pantheon_state_t& obj;

  pantheon_ticker_t( unique_gear::pantheon_state_t& o ) :
    event_t( *o.player ), obj( o )
  {
    // TODO: Configurable
    schedule( timespan_t::from_seconds( 1 ) );
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
  if ( ! proxy_player -> sim -> expansion_data.pantheon_proxy )
  {
    proxy_player -> sim -> expansion_data.pantheon_proxy =
      std::unique_ptr<pantheon_state_t>( new pantheon_state_t( proxy_player ) );
  }
}

pantheon_state_t::pantheon_state_t( player_t* player ) : player( player ), attempt_event( nullptr )
{
  rppm_objs.resize( drivers.size() );
  start_time.resize( drivers.size(), timespan_t::min() );
  actor_buffs.resize( drivers.size() );

  // Initialize buff durations list from the mark auras so we don't have to access spell data
  // needlessly
  range::for_each( marks, [ player, this ]( unsigned aura_id ) {
    auto spell = player -> find_spell( aura_id );
    buff_durations.push_back( spell -> duration() );
  } );

  parse_options();

  auto obj_idx = 0;
  // Make some RPPM object to proxy off of, based on parsed user options
  range::for_each( pantheon_opts, [ & ]( const pantheon_opt_t& opt ) {
    auto driver = player -> find_spell( drivers[ opt.first ] );
    auto rppm_scale = player -> dbc.real_ppm_scale( driver -> id() );

    std::string name = driver -> name_cstr();
    util::tokenize( name );
    name += util::to_string( obj_idx++ );

    auto rppm_obj = player -> get_rppm( name, driver -> real_ppm(), rppm_scale == RPPM_HASTE ? 1.0 + opt.second : 1.0 );

    rppm_objs[ opt.first ].push_back( rppm_obj );
  } );
}

void pantheon_state_t::parse_options()
{
  auto splits = util::string_split( player -> sim -> expansion_opts.pantheon_trinket_users, "/" );
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
      haste = std::atof( split[ 1 ].c_str() );
      if ( haste <= 0 )
      {
        player -> sim -> errorf( "Invalid haste value %s", split[ 1 ].c_str() );
        return;
      }
    }

    pantheon_opts.push_back( std::make_pair( as<size_t>( pantheon_index ), haste ) );
  } );
}

bool pantheon_state_t::has_pantheon_capability() const
{
  for ( size_t i = 0; i < drivers.size(); ++i )
  {
    if ( rppm_objs[ i ].size() )
    {
      continue;
    }

    auto user_state = false;
    if ( player -> sim -> single_actor_batch )
    {
      // Currently active player has a pantheob buff of the type?
      auto it = range::find_if( actor_buffs[ i ], []( const buff_t* b ) {
        return b -> source == b -> sim -> player_no_pet_list[ b -> sim -> current_index ];
      } );

      user_state = it != actor_buffs[ i ].end();
    }
    else
    {
      user_state = actor_buffs[ i ].size() > 0;
    }

    if ( ! user_state )
    {
      return false;
    }
  }

  return true;
}

void pantheon_state_t::start()
{
  if ( has_pantheon_capability() )
  {
    attempt_event = make_event<pantheon_ticker_t>( *player -> sim, *this );
  }
}

void pantheon_state_t::attempt()
{
  for ( size_t i = 0; i < rppm_objs.size(); ++i )
  {
    range::for_each( rppm_objs[ i ], [ this, i ]( real_ppm_t* rppm ) {
      if ( rppm -> trigger() )
      {
        start_time[ i ] = player -> sim -> current_time();

        trigger_pantheon_buff();
      }
    } );
  }

  debug();

  attempt_event = make_event<pantheon_ticker_t>( *player -> sim, *this );
}

void pantheon_state_t::trigger_pantheon_buff() const
{
  for ( size_t i = 0, end = buff_durations.size(); i < end; ++i )
  {
    auto has_proxy = start_time[ i ] != timespan_t::min() &&
                     player -> sim -> current_time() - start_time[ i ] <= buff_durations[ i ];

    auto has_real  = range::find_if( actor_buffs[ i ], []( const buff_t* b ) {
      return b -> check();
    } ) != actor_buffs[ i ].end();

    if ( ! has_proxy && ! has_real )
    {
      return;
    }
  }

  range::for_each( pantheon_effects, []( const pantheon_effect_cb_t& effect ) { effect(); } );
}

void pantheon_state_t::register_pantheon_buff( buff_t* b )
{
  auto it = range::find( marks, b -> data().id() );
  if ( it != marks.end() )
  {
    auto index = std::distance( marks.begin(), it );
    actor_buffs[ index ].push_back( b );
  }
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
  proxy_s << "Pantheon state [proxy]: ";
  for ( size_t i = 0; i < start_time.size(); ++i )
  {
    proxy_s << mark_strs[ i ];
    proxy_s << ": ";

    if ( start_time[ i ] >= timespan_t::zero() )
    {
      auto time_left = buff_durations[ i ] - ( player -> sim -> current_time() - start_time[ i ] );
      if ( time_left >= timespan_t::zero() )
      {
        proxy_s << time_left.total_seconds();
      }
      else
      {
        proxy_s << "N/A";
      }
    }
    else
    {
      proxy_s << "N/A";
    }

    if ( i < start_time.size() - 1 )
    {
      proxy_s << " | ";
    }
  }

  player -> sim -> out_debug << proxy_s.str();

  std::stringstream user_s;

  user_s << std::fixed;
  user_s << std::setprecision( 3 );
  user_s << "Pantheon state [user ]: ";

  for ( size_t i = 0; i < actor_buffs.size(); ++i )
  {
    user_s << mark_strs[ i ];
    user_s << ": ";

    if ( actor_buffs[ i ].size() == 0 )
    {
      user_s << "N/A";
    }
    else
    {
      const buff_t* longest_remaining = nullptr;
      range::for_each( actor_buffs[ i ], [ &longest_remaining ]( const buff_t* buff ) {
        if ( buff -> check() )
        {
          if ( ! longest_remaining || longest_remaining -> remains() < buff -> remains() )
          {
            longest_remaining = buff;
          }
        }
      } );

      if ( ! longest_remaining )
      {
        user_s << "N/A";
      }
      else
      {
        user_s << longest_remaining -> remains().total_seconds();
      }
    }

    if ( i < actor_buffs.size() - 1 )
    {
      user_s << " | ";
    }
  }

  player -> sim -> out_debug << user_s.str();
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
